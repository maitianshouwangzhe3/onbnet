
local cpp_onbnet = require "onbnet.core"
local seri = require "onbnet.seri"

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


local error_queue = {}
local wakeup_queue = {}
local sleep_session = {}
local watching_session = {}
local session_coroutine_id = {}
local session_id_coroutine = {}
local session_coroutine_address = {}
local running_thread


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

local suspend

local function dispatch_error_queue()
	local session = table.remove(error_queue,1)
	if session then
		local co = session_id_coroutine[session]
		session_id_coroutine[session] = nil
		return suspend(co, coroutine_resume(co, false, nil, nil, session))
	end
end

local function dispatch_wakeup()
	while true do
		local token = table.remove(wakeup_queue,1)
		if token then
			local session = sleep_session[token]
			if session then
				local co = session_id_coroutine[session]
				session_id_coroutine[session] = "BREAK"
				return suspend(co, coroutine_resume(co, false, "BREAK", nil, session))
			end
		else
			break
		end
	end
	return dispatch_error_queue()
end

local function suspend(co, result, command)
	if not result then
		local session = session_coroutine_id[co]
		if session then
			session_coroutine_id[co] = nil
		end
		print("close co", co)
		coroutine.close(co)
	end

	if command == "SUSPEND" then
		return dispatch_wakeup()
	elseif command == "QUIT" then
		print("quit")
		return
	elseif command == nil then
		print("command nil")
		return
	else
		print("error suspend command")
	end
end

local function yield_call(service, session)
	watching_session[session] = service
	session_id_coroutine[session] = running_thread
	local succ, msg = coroutine.yield "SUSPEND"
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
		if co then
			session_id_coroutine[session] = nil
			suspend(co, coroutine_resume(co, true, msg, sz, session, source))
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
		end
	end
end

function onbnet.dispatch_message(...)
	local succ, err = pcall(raw_dispatch_message,...)
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

function onbnet.start(start_func)
	cpp_onbnet.callback(onbnet.dispatch_message, true)
    start_func()
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
		print("No session")
	end

	session_coroutine_id[running_thread] = nil
	if co_session == 0 then
		print("session 0")
		return false
	end

	local co_addr = session_coroutine_address[running_thread]
	return cpp_onbnet.send(co_addr, onbnet.PTYPE_RESPONSE, co_session, msg)
end

function onbnet.retpack(...)
	return onbnet.ret(onbnet.pack(...))
end

return onbnet