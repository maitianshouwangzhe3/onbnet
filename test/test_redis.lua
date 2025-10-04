local onbnet = require "onbnet"
local redis = require "onbnet.db.redis"
-- local LuaPanda = require "LuaPanda"
-- LuaPanda.start("127.0.0.1",8818)
local conf = {
	host = "127.0.0.1" ,
	port = 6379
	-- db = 0
}

local function watching()
	local w = redis.watch(conf)
	w:subscribe "foo"
	w:psubscribe "hello.*"
	while true do
		print("Watch", w:message())
	end
end

onbnet.start(function()
	-- onbnet.fork(watching)
	local db = redis.connect(conf)
	print("start redis test", coroutine.running())
	db:set("A", "hello")
	print("start del C")
	db:del "C"
	
	db:set("B", "world")
	db:sadd("C", "one")

	print(db:get("A"))
	print(db:get("B"))

	db:del "D"
	for i=1,10 do
		db:hset("D",i,i)
	end
	local r = db:hvals "D"
	for k,v in pairs(r) do
		print(k,v)
	end

	db:multi()
	db:get "A"
	db:get "B"
	local t = db:exec()
	for k,v in ipairs(t) do
		print("Exec", v)
	end

	print(db:exists "A")
	print(db:get "A")
	print(db:set("A","hello world"))
	print(db:get("A"))
	print(db:sismember("C","one"))
	print(db:sismember("C","two"))

	print("===========publish============")

	for i=1,10 do
		db:publish("foo", i)
	end
	for i=11,20 do
		db:publish("hello.foo", i)
	end

	db:disconnect()
--	skynet.exit()
end)

