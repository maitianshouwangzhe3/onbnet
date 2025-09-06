
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

onbnet.service_manager = cpp_onbnet.get_obj_from_cpp()
 
local function raw_dispatch_message(prototype, msg, sz, session, source)
	local p = proto[prototype]
	if p then
		local func = p.dispatch
		if func then
			func(session, source, p.unpack(msg, sz))
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
    cpp_onbnet.send(addr, p.id, 0, p.pack(...))
end

return onbnet