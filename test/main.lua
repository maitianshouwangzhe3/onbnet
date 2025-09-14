

local onbnet = require "onbnet"
local socket = require "onbnet.socket"

local test

local function on_message(fd)
    print("on_message fd " .. fd)
    local data, len = socket.recv(fd)
    if data == "test_call\r\n" then
        print("call")
        local succ, ret = onbnet.call(test, "lua", "call_test", "hello")
        
        print("session = " .. tostring(succ) .. " ret = " .. tostring(ret))
    elseif data == "test_send\r\n" then
        print("send")
        local session, ret = onbnet.send(test, "lua", "send_test", "hello")
        print("session = " .. tostring(session) .. " ret = " .. tostring(ret))
    end
    print("recv = " .. tostring(data) .. " len = " .. tostring(len))
    socket.async_send(fd, data, len)
end

local function on_close(fd)
    print("close " .. fd)
end

local function on_accept(fd, newfd)
    print("accept " .. newfd)
    socket.start(newfd, on_message, 1, on_close)
end

onbnet.start(function()
    test = onbnet.new_service("test_message")
    local fd = socket.listen("", 8001)
    socket.start(fd, on_accept)
end)