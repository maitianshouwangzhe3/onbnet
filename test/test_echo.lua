local onbnet = require "onbnet"
local socket = require "onbnet.socket"
local echo = require "echo"


local function on_message(fd)
    local data, len = echo.recv(fd)
    echo.print(fd)
    echo.send(fd, data, len)
end

local function on_accept(fd, new_fd)
    print("accept")
    socket.start(new_fd, on_message)
end

onbnet.start(function()
    onbnet.on_hotfix()
    print("start echo server")
	local fd = socket.listen("", 8001)
    socket.start(fd, on_accept)
end)