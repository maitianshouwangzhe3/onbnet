
local onbnet = require "onbnet"

local M = {}

function M.test(msg)
	print("test message ", msg)
end

function M.call_test(msg)
	print("call_test message ", msg)
    onbnet.retpack("ok")
end

function M.send_test(msg)
	print("send_test message ", msg)
end

onbnet.start(function()
	onbnet.dispatch("lua", function(_, _, cmd, ...)
        local f = M[cmd]
        if f then
            f(...)
        else
            print("unknown command", cmd)
        end
    end)
end)