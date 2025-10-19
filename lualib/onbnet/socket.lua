
local onbnet = require "onbnet"
local socketdriver = require "onbnet.socketdriver"

local M = {}
local socket_pool = {}
local socket_onclose = {}

local socket_message = setmetatable({}, {__index = function (t, k)
	print("socket message not found k = ".. k)
end})

socket_message[6] = function (fd)
    
end

socket_message[12] = function (fd, newfd)
    local s = {
        id = newfd,
        connected = false,
        listen = false,
    }

    assert(socket_pool[newfd] == nil)
    socket_pool[newfd] = s

    local listen_service = assert(socket_pool[fd])
    listen_service.callback(fd, newfd)
end

socket_message[13] = function (fd, size, data)
	local s = assert(socket_pool[fd])
    if s.callback then
		s.callback(fd)
	end

	local rr = s.read_required
	local rrt = type(rr)
	if rrt == "number" then
		local sz = socketdriver.checkbufferlen(fd)
		if sz >= rr then
			s.read_required = nil
			local co = s.co
			if co then
				s.co = nil
				onbnet.wakeup(co)
			end
		end
	else
		if rrt == "string" then
			if socketdriver.checkline(fd, rr) then
				s.read_required = nil
				local co = s.co
				if co then
					s.co = nil
					onbnet.wakeup(co)
				end
			end
		end
	end
	
end

socket_message[14] = function (fd, ...)
    local s = assert(socket_pool[fd])
    local func = s.on_disconnect
    if func then
        func(fd)
    end

    socket_pool[fd] = nil
end

socket_message[15] = function (fd, addr, ud)
	local s = socket_pool[fd]
	if s == nil then
		return
	end

	if not s.connected then
		s.connected = true
		local co = s.co
		if co then
			s.co = nil
			onbnet.wakeup(co)
		end
	end
end

onbnet.register_protocol({
    name = "socket",
    id = onbnet.PTYPE_SOCKET,
    protocol = "TCP",
    unpack = socketdriver.unpack,
    dispatch = function (_, _, t, ...)
		socket_message[t](...)
	end
})

M.write = socketdriver.send
M.lwrite = socketdriver.async_send

function M.listen(addr, port)
    local fd = socketdriver.listen(port);
    local s = {
		id = fd,
		connected = false,
		listen = true,
	}
    assert(socket_pool[fd] == nil)
    socket_pool[fd] = s
    return fd
end

local function suspend(s)
	assert(not s.co)
	s.co = coroutine.running()
	if s.pause then
		error(string.format("Resume socket (%d)", s.id))
		M.start(s.id, s.callback)
		onbnet.wait(s.co)
		s.pause = nil
	else
		onbnet.wait(s.co)
	end
	-- wakeup closing corouting every time suspend,
	-- because socket.close() will wait last socket buffer operation before clear the buffer.
	if s.closing then
		onbnet.wakeup(s.closing)
	end
end

function M.recv(fd, size)
    local s = socket_pool[fd]
    assert(s)
	if size == nil then
		return socketdriver.recv(fd, size)
	end
    
	local ret, len = socketdriver.recv(fd, size)
	if ret then
		return ret, len
	end
	
	assert(not s.read_required)
	s.read_required = size
	suspend(s)
	ret = socketdriver.recv(fd, size)
	if ret then
		return ret, len
	else
		return false
	end
end

function M.recvall()
	-- body
end

function M.send(fd, msg, len)
    return socketdriver.send(fd, msg, len)
end

function M.async_send(fd, msg, len)
    return socketdriver.async_send(fd, msg, len)
end

function M.start(fd, func, block, close_callback)
    assert(socket_pool[fd])
    local s = socket_pool[fd]
    s.callback = func
    if close_callback then
        s.on_disconnect = close_callback
    end

    if block then
        socketdriver.start(fd, block)
    else
        socketdriver.start(fd, 0)
    end
end

function M.close_callback(fd, func)
    local s = assert(socket_pool[fd])
    s.on_disconnect = func
end

function M.disconnected(id)
	local s = socket_pool[id]
	if s then
		return not(s.connected or s.connecting)
	end
end

function M.recvline(fd, sep)
	sep = sep or "\n"
	local s = socket_pool[fd]
	assert(s)
	local ret = socketdriver.recvline(fd, sep)
	if ret then
		return ret
	end
	if not s.connected then
		return false, socketdriver.recvall(fd)
	end
	assert(not s.read_required)
	s.read_required = sep
	suspend(s)
	if s.connected then
		ret = socketdriver.recvline(fd, sep)
		s.read_required = false
		return ret
	else
		return false, socketdriver.recvall(fd)
	end
end

local function connect(id, func)
	local s = {
		id = id,
		pool = {},
		connected = false,
		connecting = true,
		read_required = false,
		co = false,
		callback = func,
		protocol = "TCP",
	}
	assert(not socket_onclose[id], "socket has onclose callback")
	local s2 = socket_pool[id]
	if s2 and not s2.listen then
		error("socket is not closed")
	end

	socket_pool[id] = s
	suspend(s)
	local err = s.connecting
	s.connecting = nil
	if s.connected then
		return id
	else
		socket_pool[id] = nil
		return nil, err
	end
end

function M.open(host, port)
    local id = socketdriver.connect(host, port)
	assert(id > 0, "connect failed")
    return connect(id)
end

function M.warning(id, callback)
	local obj = socket_pool[id]
	assert(obj)
	obj.on_warning = callback
end

function M.onclose(id, callback)
	socket_onclose[id] = callback
end

return M