

local onbnet = require "onbnet"
local socket = require "onbnet.socket"

local function on_message(fd)
    print("on_message fd " .. fd)
    local data, len = socket.recv(fd)
    print("recv " .. data)
    socket.send(fd, data, len)
end

local function on_accept(fd, newfd)
    print("accept " .. fd)
    socket.start(newfd, on_message, 1)
end

onbnet.start(function()
    local fd = socket.listen("", 8001)
    socket.start(fd, on_accept)
end)