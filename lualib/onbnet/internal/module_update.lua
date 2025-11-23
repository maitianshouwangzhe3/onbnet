

local M = {}

local protected = {}

local update_sig = {}

local update_func_map = {}

local update_table
local update_function

local function check_update(new_obj, old_obj, name, deep)
    local sig = string.format("new(%s):old(%s)", tostring(new_obj), tostring(old_obj))
    if new_obj == old_obj then
        return true
    end

    if update_sig[sig] then
        return true
    end

    update_sig[sig] = true
    return false
end

function update_table(new_table, old_table, name, deep)
    assert("table" == type(new_table))
    assert("table" == type(old_table))
    
    if protected[old_table] then
        return
    end

    if check_update(new_table, old_table, name, deep) then
        return
    end

    deep = deep .. "  "
    for k, v in pairs(new_table) do
        local old_v = old_table[k]
        local type_v = type(v)
        if type_v ~= type(old_v) then
            old_table[k] = value
        elseif type_v == "function" then
            update_function(v, old_v, k, deep)
        elseif type_v == "table" then
            update_table(v, old_v, k, deep)
        end
    end

    local old_meta = debug.getmetatable(old_table)
    local new_meta = debug.getmetatable(new_table)
    if type(old_meta) == "table" and type(new_meta) == "table" then
        update_table(new_meta, old_meta, name .. "'s Meta", deep)
    end
end

function update_function(new_func, old_func, name, deep)
    assert("function" == type(new_func))
    assert("function" == type(old_func))

    if protected[old_func] then
        return
    end

    if check_update(new_func, old_func, name, deep) then
        return
    end

    deep = deep .. "  "
    update_func_map[old_func] = new_func

    -- 获取旧函数的上值
    local old_update_value_map = {}
    for i = 1, math.huge do
        local name, value = debug.getupvalue(old_func, i)
        if not name then
            break
        end

        old_update_value_map[name] = value
    end

    -- 更新
    for i = 1, math.huge do
        local name, value = debug.getupvalue(new_func, i)
        if not name then
            break
        end

        local old_value = old_update_value_map[name]
        if old_value then
            local type_old_value = type(old_value)
            if type_old_value ~= type(value) then
                debug.setupvalue(new_func, i, old_value)
            elseif type_old_value == "function" then
                update_function(value, old_value, name, deep)
            elseif type_old_value == "table" then
                update_table(value, old_value, name, deep)
            else
                debug.setupvalue(new_func, i, old_value)
            end
        end
    end
end

local function update_load_module(module_name, new_obj)
    assert(nil ~= new_obj)
    assert("string" == type(module_name))

    local old_obj = package.loaded[module_name]
    local new_type = type(new_obj)
    local old_type = type(old_obj)
    if new_type == old_type then
        if "table" == new_type then
            update_table(new_obj, old_obj, module_name, "")
            return
        end

        if "function" == new_type then
            update_function(new_obj, old_obj, module_name, "")
            return
        end
    end

    package.loaded[module_name] = new_obj
end

function M.update_module(module_name, protected_obj_args, new_module_obj)
    assert(type(module_name) == "string")
    assert(type(protected_obj_args) == "table")

    protected = protected_obj_args
    update_func_map = {}
    update_sig = {}
    update_load_module(module_name, new_module_obj)
    update_sig = {}
    return update_func_map
end

return M