#pragma once

#include "spdlog/logger.h"

namespace onbnet {

class Logger {
public:
    Logger();
    ~Logger();

    static Logger* inst;
    
    std::shared_ptr<spdlog::logger> mSyncLogger;
    std::shared_ptr<spdlog::logger> mASyncLogger;
    std::shared_ptr<spdlog::logger> mStdoutLogger;
};

}

#define LOGGER onbnet::Logger::inst

#define console_info(msg, ...) onbnet::Logger::inst->mStdoutLogger->info(msg, ##__VA_ARGS__)

#define console_error(msg, ...) onbnet::Logger::inst->mStdoutLogger->error(msg, ##__VA_ARGS__)

#define console_warn(msg, ...) onbnet::Logger::inst->mStdoutLogger->warn(msg, ##__VA_ARGS__)

#define console_debug(msg, ...) onbnet::Logger::inst->mStdoutLogger->debug(msg, ##__VA_ARGS__)

#define log_info(msg, ...) onbnet::Logger::inst->mSyncLogger->info(msg, ##__VA_ARGS__)

#define log_error(msg, ...) onbnet::Logger::inst->mSyncLogger->error(msg, ##__VA_ARGS__)

#define log_debug(msg, ...) onbnet::Logger::inst->mSyncLogger->debug(msg, ##__VA_ARGS__)

#define log_warn(msg, ...) onbnet::Logger::inst->mSyncLogger->warn(msg, ##__VA_ARGS__)


#define alog_info(msg, ...) onbnet::Logger::inst->mASyncLogger->info(msg, ##__VA_ARGS__)

#define alog_error(msg, ...) onbnet::Logger::inst->mASyncLogger->error(msg, ##__VA_ARGS__)

#define alog_debug(msg, ...) onbnet::Logger::inst->mASyncLogger->debug(msg, ##__VA_ARGS__)

#define alog_warn(msg, ...) onbnet::Logger::inst->mASyncLogger->warn(msg, ##__VA_ARGS__)