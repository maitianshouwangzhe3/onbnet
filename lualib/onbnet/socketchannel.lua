local onbnet = require "onbnet"
local socket = require "onbnet.socket"
local socketdriver = require "onbnet.socketdriver"


local socket_channel = {}
local channel = {}
local channel_socket = {}
local channel_meta = { __index = channel }
local channel_socket_meta = {
	__index = channel_socket,
	__gc = function(cs)
		local fd = cs[1]
		cs[1] = false
		if fd then
			socket.shutdown(fd)
		end
	end
}

local socket_error = setmetatable({}, {__tostring = function() return "[Error: socket]" end })
socket_channel.error = socket_error

function socket_channel.channel(desc)
	local c = {
		__host = assert(desc.host),
		__port = assert(desc.port),
		__backup = desc.backup,
		__auth = desc.auth,
		__response = desc.response,
		__request = {},
		__thread = {},
		__result = {},
		__result_data = {},
		__connecting = {},
		__sock = false,
		__closed = false,
		__authcoroutine = false,
		__nodelay = desc.nodelay,
		__overload_notify = desc.overload,
		__overload = false,
		__socket_meta = channel_socket_meta,
	}
	if desc.socket_read or desc.socket_readline then
		c.__socket_meta = {
			__index = {
				read = desc.socket_read or channel_socket.read,
				readline = desc.socket_readline or channel_socket.readline,
			},
			__gc = channel_socket_meta.__gc
		}
	end

	return setmetatable(c, channel_meta)
end

local function close_channel_socket(self)
	if self.__sock then
		local so = self.__sock
		self.__sock = false
		if self.__wait_response then
			onbnet.wakeup(self.__wait_response)
			self.__wait_response = nil
		end
		-- never raise error
		pcall(socket.close,so[1])
	end
end

local function check_connection(self)
	if self.__sock then
		local authco = self.__authcoroutine
		if socket.disconnected(self.__sock[1]) then
			-- closed by peer
			-- skynet.error("socket: disconnect detected ", self.__host, self.__port)
            
			close_channel_socket(self)
			if authco and authco == coroutine.running() then
				return false
			end
			return
		end
		if not authco then
			return true
		end
		if authco == coroutine.running() then
			return true
		end
	end
	if self.__closed then
		return false
	end
end

local function push_response(self, response, co)
	if self.__response then
		-- response is session
		self.__thread[response] = co
	else
		-- response is a function, push it to __request
		table.insert(self.__request, response)
		table.insert(self.__thread, co)
		if self.__wait_response then
			onbnet.wakeup(self.__wait_response)
			self.__wait_response = nil
		end
	end
end

local function term_dispatch_thread(self)
	if not self.__response and self.__dispatch_thread then
		-- dispatch by order, send close signal to dispatch thread
		push_response(self, true, false)	-- (true, false) is close signal
	end
end

local function connect_once(self)
	if self.__closed then
		return false
	end

	local addr_list = {}
	local addr_set = {}

	local function _add_backup()
		if self.__backup then
			for _, addr in ipairs(self.__backup) do
				local host, port
				if type(addr) == "table" then
					host,port = addr.host, addr.port
				else
					host = addr
					port = self.__port
				end

				-- don't add the same host
				local hostkey = host..":"..port
				if not addr_set[hostkey] then
					addr_set[hostkey] = true
					table.insert(addr_list, { host = host, port = port })
				end
			end
		end
	end

	local function _next_addr()
		local addr =  table.remove(addr_list,1)
		if addr then
			-- skynet.error("socket: connect to backup host", addr.host, addr.port)
		end
		return addr
	end

	local function _connect_once(self, addr)
		local fd,err = socket.open(addr.host, addr.port)
		if not fd then
			-- try next one
			addr = _next_addr()
			if addr == nil then
				return false, err
			end
			return _connect_once(self, addr)
		end

		self.__host = addr.host
		self.__port = addr.port

		assert(not self.__sock and not self.__authcoroutine)
		-- term current dispatch thread (send a signal)
		term_dispatch_thread(self)

		if self.__nodelay then
			socketdriver.nodelay(fd)
		end

		-- register overload warning

		local overload = self.__overload_notify
		if overload then
			local function overload_trigger(id, size)
				if id == self.__sock[1] then
					if size == 0 then
						if self.__overload then
							self.__overload = false
							overload(false)
						end
					else
						if not self.__overload then
							self.__overload = true
							overload(true)
						else
							-- skynet.error(string.format("WARNING: %d K bytes need to send out (fd = %d %s:%s)", size, id, self.__host, self.__port))
						end
					end
				end
			end

			skynet.fork(overload_trigger, fd, 0)
			socket.warning(fd, overload_trigger)
		end

		while self.__dispatch_thread do
			-- wait for dispatch thread exit
			skynet.yield()
		end

		self.__sock = setmetatable( {fd} , self.__socket_meta )
		self.__dispatch_thread = skynet.fork(function()
			if self.__sock then
				-- self.__sock can be false (socket closed) if error during connecting, See #1513
				pcall(dispatch_function(self), self)
			end
			-- clear dispatch_thread
			self.__dispatch_thread = nil
		end)

		if self.__auth then
			self.__authcoroutine = coroutine.running()
			local ok , message = pcall(self.__auth, self)
			if not ok then
				close_channel_socket(self)
				if message ~= socket_error then
					self.__authcoroutine = false
					skynet.error("socket: auth failed", message)
				end
			end
			self.__authcoroutine = false
			if ok then
				if not self.__sock then
					-- auth may change host, so connect again
					return connect_once(self)
				end
				-- auth succ, go through
			else
				-- auth failed, try next addr
				_add_backup()	-- auth may add new backup hosts
				addr = _next_addr()
				if addr == nil then
					return false, "no more backup host"
				end
				return _connect_once(self, addr)
			end
		end

		return true
	end

	_add_backup()
	return _connect_once(self, { host = self.__host, port = self.__port })
end

local function try_connect(self , once)
	local t = 0
	while not self.__closed do
		local ok, err = connect_once(self)
		if ok then
			if not once then
				-- skynet.error("socket: connect to", self.__host, self.__port)
			end
			return
		elseif once then
			return err
		else
			-- skynet.error("socket: connect", err)
		end
		if t > 1000 then
			-- skynet.error("socket: try to reconnect", self.__host, self.__port)
			skynet.sleep(t)
			t = 0
		else
			skynet.sleep(t)
		end
		t = t + 100
	end
end

local function block_connect(self, once)
	local r = check_connection(self)
	if r ~= nil then
		return r
	end
	local err

	if #self.__connecting > 0 then
		-- connecting in other coroutine
		local co = coroutine.running()
		table.insert(self.__connecting, co)
		onbnet.wait(co)
	else
		self.__connecting[1] = true
		err = try_connect(self, once)
		for i=2, #self.__connecting do
			local co = self.__connecting[i]
			self.__connecting[i] = nil
			onbnet.wakeup(co)
		end
		self.__connecting[1] = nil
	end

	r = check_connection(self)
	if r == nil then
		-- skynet.error(string.format("Connect to %s:%d failed (%s)", self.__host, self.__port, err))
		error(socket_error)
	else
		return r
	end
end


return socket_channel