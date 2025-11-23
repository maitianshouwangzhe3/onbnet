local onbnet = require "onbnet"
-- local socket = require "onbnet.socket"
-- local logger = require "onbnet.logger"
-- local redis = require "onbnet.db.redis"
-- local crypt = require "onbnet.crypt"
-- local mysql = require "onbnet.db.mysql"

-- local LuaPanda = require "LuaPanda"
-- LuaPanda.start("127.0.0.1",8818)
local test

onbnet.start(
    function()
        -- test = onbnet.new_service("test_redis")
        -- test = onbnet.new_service("test_mysql")
        test = onbnet.new_service("test_echo")
        -- local fd = socket.listen("", 8001)
        -- socket.start(fd, on_accept)
        -- test = onbnet.new_service("test_mongodb")
    end
)
