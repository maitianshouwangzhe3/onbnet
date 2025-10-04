

local function test(msg)
	-- body
	print("msg = " .. msg)
	coroutine.yield("11111")
	return "hello world"
end

local co = coroutine.create(test)

local succ, ret = coroutine.resume(co, "test")

print(ret)

succ, ret = coroutine.resume(co, "test")

print(ret)