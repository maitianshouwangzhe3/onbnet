
local module_update = require "onbnet.internal.module_update"
local functions_replace = require "onbnet.internal.functions_replace"

local M = {}

local protected = {}

local function add_self_to_protect()
    M.add_protect{
        M,
        M.hotfix_module,
        M.add_protect,
        M.remove_protect,
        module_update,
        module_update.update_module,
        functions_replace,
        functions_replace.update_all,
    }
end

local function hotfix_module_with_obj(module_name, module_obj)
    assert("string" == type(module_name))
    add_self_to_protect()

    -- 更新模块, 记录要更新的函数
    local update_function_map = module_update.update_module(module_name, protected, module_obj)
    -- 替换函数
    functions_replace.update_all(protected, update_function_map, module_obj)
end

function M.hotfix_module(module_name)
    assert("string" == type(module_name))
    if not package.loaded[module_name] then
        return package.loaded[module_name]
    end

    local file_path = assert(package.searchpath(module_name, package.path))
    local fp = assert(io.open(file_path))
    local chunk = fp:read("*all")
    fp:close()

    local func = assert(load(chunk, '@' .. file_path))
    local ok, obj = assert(pcall(func))
    if nil == obj then
        obj = true
    end

    hotfix_module_with_obj(module_name, obj)
    return package.loaded[module_name]
end

function M.add_protect(object_array)
    for _, obj in pairs(object_array) do
        protected[obj] = true
    end
end

function M.remove_protect(object_array)
    for _, obj in pairs(object_array) do
        protected[obj] = nil
    end
end

return M