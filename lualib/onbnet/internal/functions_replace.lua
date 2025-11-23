

local M = {}

local protected = {}

local replace_obj = {}

local update_func_map = {}

local replace_functions

local function replace_functions_in_table(table_obj)
    local obj = table_obj
    assert("table" == type(obj))
    assert(not protected[obj])
    assert(obj ~= update_func_map)

    replace_functions(debug.getmetatable(obj))
    local new = {}
    for k, v in pairs(obj) do
        local new_k = update_func_map[k]
        local new_v = update_func_map[v]
        if new_k then
            obj[k] = nil
            new[new_k] = new_v or v
        else
            obj[k] = new_v or v
            replace_functions(k)
        end

        if not new_v then
            replace_functions(v)
        end
    end

    for k, v in pairs(new) do
        obj[k] = v
    end
end

local function replace_functions_in_update_values(func_obj)
    local obj = func_obj
    assert("function" == type(obj))
    assert(not protected[obj])
    assert(obj ~= update_func_map)

    for i = 1, math.huge do
        local name, value = debug.getupvalue(obj, i)
        if not name then
            return
        end

        local new_func = update_func_map[value]
        if new_func then
            assert("function" == type(new_func))
            debug.setupvalue(obj, i, new_func)
        else
            replace_functions(value)
        end
    end

    assert(false, "below expectations error")
end

function replace_functions(obj)
    if protected[obj] then
        return
    end

    local obj_type = type(obj)
    if "function" ~= obj_type and "table" ~= obj_type then
        return
    end

    if replace_obj[obj] then
        return
    end

    replace_obj[obj] = true
    assert(obj ~= update_func_map)

    if "function" == obj_type then 
        replace_functions_in_update_values(obj)
    elseif "table" == obj_type then 
        replace_functions_in_table(obj)
    end
end

function M.update_all(protected_args, updata_func_map_args, new_obj)
    protected = protected_args
    update_func_map = updata_func_map_args
    assert(type(protected) == "table")
    assert(type(update_func_map) == "table")

    if next(update_func_map) == nil then
        return
    end

    replace_obj = {}
    replace_functions(new_obj)
    replace_functions(_G)
    replace_functions(debug.getregistry())
    replace_obj = {}
end

return M