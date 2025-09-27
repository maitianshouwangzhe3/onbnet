
local onbnet = require "onbnet"
local socket = require "onbnet.socketdriver"

local M = {}
local socket_pool = {}
local socket_message = {}

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

socket_message[13] = function (fd, ...)
	local s = assert(socket_pool[fd])
    s.callback(fd)
end

socket_message[14] = function (fd, ...)
    local s = assert(socket_pool[fd])
    local func = s.on_disconnect
    if func then
        func(fd)
    end
    socket_pool[fd] = nil
end

onbnet.register_protocol({
    name = "socket",
    id = onbnet.PTYPE_SOCKET,
    protocol = "TCP",
    unpack = socket.unpack,
    dispatch = function (_, _, t, ...)
		socket_message[t](...)
	end
})

function M.listen(addr, port)
    local fd = socket.listen(port);
    local s = {
		id = fd,
		connected = false,
		listen = true,
	}
    assert(socket_pool[fd] == nil)
    socket_pool[fd] = s
    return fd
end

function M.recv(fd)
    local s = socket_pool[fd]
    assert(s)
    return socket.recv(fd)
end

function M.send(fd, msg, len)
    return socket.send(fd, msg, len)
end

function M.async_send(fd, msg, len)
    return socket.async_send(fd, msg, len)
end

function M.start(fd, func, block, close_callback)
    assert(socket_pool[fd])
    local s = socket_pool[fd]
    s.callback = func
    if close_callback then
        s.on_disconnect = close_callback
    end

    if block then
        socket.start(fd, block)
    else
        socket.start(fd, 0)
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

function M.open(host, port)
    return socket.connect(host, port)
end

function M.warning(id, callback)
	local obj = socket_pool[id]
	assert(obj)
	obj.on_warning = callback
end

return M