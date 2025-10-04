
local cpp_onbnet = require "onbnet.core"
local seri = require "onbnet.seri"
local logger = require "onbnet.logger"
local proto = {}
local onbnet = {
    PTYPE_TEXT = 0,
	PTYPE_RESPONSE = 1,
	PTYPE_MULTICAST = 2,
	PTYPE_CLIENT = 3,
	PTYPE_SYSTEM = 4,
	PTYPE_HARBOR = 5,
	PTYPE_SOCKET = 6,
	PTYPE_ERROR = 7,
	PTYPE_QUEUE = 8,
	PTYPE_DEBUG = 9,
	PTYPE_LUA = 10,
	PTYPE_SNAX = 11,
}

local unresponse = {}
local error_queue = {}
local wakeup_queue = {}
local sleep_session = {}
local watching_session = {}
local session_coroutine_id = {}
local session_id_coroutine = {}
local session_coroutine_address = {}
local running_thread
local fork_queue = { h = 1, t = 0 }

function onbnet.register_protocol(class)
	local name = class.name
	local id = class.id
	assert(proto[name] == nil and proto[id] == nil)
	assert(type(name) == "string" and type(id) == "number" and id >=0 and id <=255)
	proto[name] = class
	proto[id] = class
end

local register = onbnet.register_protocol

do
    local class = {}
    class.name = "lua"
    class.id = onbnet.PTYPE_LUA
	class.pack =seri.pack
	class.unpack = seri.unpack
    register(class)
end

onbnet.pack = seri.pack
onbnet.unpack = seri.unpack

local function coroutine_resume(co, ...)
	running_thread = co
	return coroutine.resume(co, ...)
end

local co_pool = setmetatable({}, {__mode = "kv"})
local function co_create(func)
	local co = table.remove(co_pool)
	if co == nil then
		co = coroutine.create(function(...) 
			func(...)
			while true do
				local session = session_coroutine_id[co]
				if session and session ~= 0 then
					print("error session ", session)
				end

				func = nil
				co_pool[#co_pool + 1] = co
				func = coroutine.yield("SUSPEND")
				func(coroutine.yield())
			end
		end)
	else
		local running = running_thread
		coroutine_resume(co, func)
		running_thread = running
	end
	return co
end

local suspendfunc

local function dispatch_error_queue()
	-- local session = table.remove(error_queue,1)
	-- if session then
	-- 	local co = session_id_coroutine[session]
	-- 	session_id_coroutine[session] = nil
	-- 	return suspendfunc(co, coroutine_resume(co, false, nil, nil, session))
	-- end
end

local function dispatch_wakeup()
	while true do
		local token = table.remove(wakeup_queue, 1)
		if token then
			local session = sleep_session[token]
			if session then
				local co = session_id_coroutine[session]
				session_id_coroutine[session] = "BREAK"
				return suspendfunc(co, coroutine_resume(co, false, "BREAK", nil, session))
			end
		else
			break
		end
	end
	-- return dispatch_error_queue()
	return 
end

local function suspend(co, result, command)
	if not result then
		local session = session_coroutine_id[co]
		if session then
			session_coroutine_id[co] = nil
		end

		coroutine.close(co)
	end

	if command == "SUSPEND" then
		return dispatch_wakeup()
	elseif command == "QUIT" then
		logger.console_info("quit")
		return
	elseif command == nil then
		logger.console_info("command nil")
		return
	else
		logger.console_info("error suspend command")
	end
end

suspendfunc = suspend
local function yield_call(service, session)
	watching_session[session] = service
	session_id_coroutine[session] = running_thread
	local succ, msg = coroutine.yield("SUSPEND")
	watching_session[session] = nil
	if not succ then
		return false, "call faild"
	end

	return true, msg
end

onbnet.service_manager = cpp_onbnet.get_obj_from_cpp()
 
local function raw_dispatch_message(prototype, msg, sz, session, source)
	if prototype == onbnet.PTYPE_RESPONSE then
		local co = session_id_coroutine[session]
		if co == "BREAK" then
			session_id_coroutine[session] = nil
		elseif co == nil then
			-- unknown_response(session, source, msg, sz)
			print("unknown response")
		else
			session_id_coroutine[session] = nil
			suspend(co, coroutine_resume(co, true, msg, sz, session))
		end
	else
		local p = proto[prototype]
		if p then
			local func = p.dispatch
			if func then
				local co = co_create(func)
				session_coroutine_id[co] = session
				session_coroutine_address[co] = source
				return suspend(co, coroutine_resume(co, session, source, p.unpack(msg, sz)))
			end
		else
			print("unknown message type")
		end
	end
end

function onbnet.dispatch_message(...)
	local succ, err = pcall(raw_dispatch_message,...)
	while true do
		if fork_queue.h > fork_queue.t then
			-- queue is empty
			fork_queue.h = 1
			fork_queue.t = 0
			break
		end
		-- pop queue
		local h = fork_queue.h
		local co = fork_queue[h]
		fork_queue[h] = nil
		fork_queue.h = h + 1

		local fork_succ, fork_err = pcall(suspend,co, coroutine_resume(co))
		if not fork_succ then
			if succ then
				succ = false
				err = tostring(fork_err)
			else
				err = tostring(err) .. "\n" .. tostring(fork_err)
			end
		end
	end
	assert(succ, tostring(err))
end

function onbnet.dispatch(typename, func)
	local p = proto[typename]
	if func then
		local ret = p.dispatch
		p.dispatch = func
		return ret
	else
		return p and p.dispatch
	end
end

local init_thread
function onbnet.start(start_func)
	cpp_onbnet.callback(onbnet.dispatch_message, true)
	init_thread = onbnet.timeout(0, function()
		onbnet.init_service(start_func)
		init_thread = nil
	end)
end

function onbnet.new_service(name)
    return onbnet.service_manager.newService(name)
end

function onbnet.send(addr, typename, ...)
    local p = proto[typename]
    return cpp_onbnet.send(addr, p.id, nil, p.pack(...))
end

function onbnet.call(addr, typename, ...)
    local p = proto[typename]
    local session = cpp_onbnet.send(addr, p.id, 0, p.pack(...))
	if session < 1 then
		return false, "call failed"
	end

	local succ, msg = yield_call(addr, session)
    return succ, p.unpack(msg)
end

function onbnet.ret(msg)
    local co_session = session_coroutine_id[running_thread]
	if co_session == nil then
		logger.console_info("No session")
	end

	session_coroutine_id[running_thread] = nil
	if co_session == 0 then
		logger.console_info("session 0")
		return false
	end

	local co_addr = session_coroutine_address[running_thread]
	return cpp_onbnet.send(co_addr, onbnet.PTYPE_RESPONSE, co_session, msg)
end

function onbnet.retpack(...)
	return onbnet.ret(onbnet.pack(...))
end

function onbnet.self()
	return cpp_onbnet.self()
end

function onbnet.response(pack)
	pack = pack or onbnet.pack

	local co_session = assert(session_coroutine_id[running_thread], "no session")
	session_coroutine_id[running_thread] = nil
	local co_address = session_coroutine_address[running_thread]
	if co_session == 0 then
		return function() end
	end
	local function response(ok, ...)
		if ok == "TEST" then
			return unresponse[response] ~= nil
		end
		if not pack then
			error "Can't response more than once"
		end

		local ret
		if unresponse[response] then
			if ok then
				ret = onbnet.send(co_address, onbnet.PTYPE_RESPONSE, co_session, pack(...))
				if ret == false then
					-- If the package is too large, returns false. so we should report error back
					onbnet.send(co_address, onbnet.PTYPE_ERROR, co_session, "")
				end
			else
				ret = onbnet.send(co_address, onbnet.PTYPE_ERROR, co_session, "")
			end
			unresponse[response] = nil
			ret = ret ~= nil
		else
			ret = false
		end
		pack = nil
		return ret
	end
	unresponse[response] = co_address

	return response
end

function onbnet.wakeup(token)
	if sleep_session[token] then
		table.insert(wakeup_queue, token)
		return true
	end
end

local function suspend_sleep(session, token)
	session_id_coroutine[session] = running_thread
	assert(sleep_session[token] == nil, "token duplicative")
	sleep_session[token] = session
	return coroutine.yield("SUSPEND")
end

function onbnet.sleep(timeout, token)
	token = token or coroutine.running()
	local session = cpp_onbnet.sleep(timeout)
	assert(session > 0, "sleep failed")
	local succ, ret = suspend_sleep(session, token)
	sleep_session[token] = nil
	if succ then
		return
	end

	if ret == "BREAK" then
		return "BREAK"
	else
		error "sleep error"
	end
end

function onbnet.wait(token)
	local session = cpp_onbnet.new_session()
	token = token or coroutine.running()
	suspend_sleep(session, token)
	sleep_session[token] = nil
	session_id_coroutine[session] = nil
end

function onbnet.fork(func,...)
	local n = select("#", ...)
	local co
	if n == 0 then
		co = co_create(func)
	else
		local args = { ... }
		co = co_create(function() func(table.unpack(args, 1, n)) end)
	end
	local t = fork_queue.t + 1
	fork_queue.t = t
	fork_queue[t] = co
	return co
end

function onbnet.yield()
	return onbnet.sleep(0)
end

function onbnet.timeout(ti, func)
	local co = co_create(func)
	local session = cpp_onbnet.sleep(ti)
	assert(session > 0)
	assert(session_id_coroutine[session] == nil)
	session_id_coroutine[session] = co
	return co	-- for debug
end

function onbnet.init_service(start)
	local function main()
		start()
	end
	local ok, err = xpcall(main,  debug.traceback)
	if not ok then
		print("service init error", err)
	else
		print("service init ok")
	end
end

return onbnet