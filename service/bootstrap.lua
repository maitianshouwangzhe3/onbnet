
local onbnet = require "onbnet"

onbnet.start(function()
	print("bootstrap start")
	pcall(onbnet.new_service, "main")
end)
