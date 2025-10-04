

local onbnet = require "onbnet"
local socket = require "onbnet.socket"
local logger = require "onbnet.logger"
local redis = require "onbnet.db.redis"
-- local LuaPanda = require "LuaPanda"
-- LuaPanda.start("127.0.0.1",8818)
local conf = {
	host = "127.0.0.1" ,
	port = 6379,
	db = 0
}

local test

local function on_message(fd)
    logger.console_info("on_message fd %d", fd)
    local data, len = socket.recv(fd)
    if data == "test_call\r\n" then
        logger.console_info("call")
        local succ, ret = onbnet.call(test, "lua", "call_test", "hello")
        -- logger.console_info("call succ = %d, ret = %s", succ, ret)
        print(string.format("call succ = %s, ret = %s", succ, ret))
    elseif data == "test_send\r\n" then
        logger.console_info("send")
        local succ, ret = onbnet.send(test, "lua", "send_test", "hello")
        logger.console_info("call succ = %s, ret = %s", succ, ret)
    end
    logger.console_info("sleep 10 start")

    onbnet.sleep(10)

    logger.console_info("sleep 10 end")


    -- logger.console_info("recv = %s, ret = %d", tostring(data), len)
    socket.async_send(fd, data, len)
end

local function on_close(fd)
    logger.console_info("close %d", fd)
end

local function on_accept(fd, newfd)
    logger.console_info("accept %d", newfd)
    socket.start(newfd, on_message, 1, on_close)
end

onbnet.start(function()
    -- test = onbnet.new_service("test_redis")
    -- local fd = socket.listen("", 8001)
    -- socket.start(fd, on_accept)

    local db = redis.connect(conf)
	print("start redis test")
	local ret = db:set("A", "hello")
    print("end redis test", ret)
    db:disconnect()
end)