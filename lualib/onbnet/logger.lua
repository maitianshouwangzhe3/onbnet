
local logger = require "onbnet.loggerdriver"


local M = {}

function M.log_info(msg, ...)
    logger.log_info(string.format(msg, ...))
end

function M.log_error(msg, ...)
    logger.log_error(string.format(msg, ...))
end

function M.log_debug(msg, ...)
    logger.log_debug(string.format(msg, ...))
end

function M.log_warn(msg, ...)
    logger.log_warn(string.format(msg, ...))
end

function M.alog_info(msg, ...)
    logger.alog_info(string.format(msg, ...))
end

function M.alog_error(msg, ...)
    logger.alog_error(string.format(msg, ...))
end

function M.alog_debug(msg, ...)
    logger.alog_debug(string.format(msg, ...))
end

function M.alog_warn(msg, ...)
    logger.alog_warn(string.format(msg, ...))
end

function M.console_info(msg, ...)
    logger.console_info(string.format(msg, ...))
end

function M.console_error(msg, ...)
    logger.console_error(string.format(msg, ...))
end

function M.console_debug(msg, ...)
    logger.console_debug(string.format(msg, ...))
end

function M.console_warn(msg, ...)
    logger.console_warn(string.format(msg, ...))
end

return M