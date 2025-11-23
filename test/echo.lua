local socket = require "onbnet.socket"
local M = {}

function M.recv(fd, size)
    local data, len = socket.recv(fd, size)
    return data, len
end

function M.send(fd, data, size)
    return socket.send(fd, data, size)
end

function M.print(fd)
    print("print fd: " .. tostring(fd))
end

return M