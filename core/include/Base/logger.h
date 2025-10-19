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

#define console_info(msg, ...) LOGGER->mStdoutLogger->info(msg, ##__VA_ARGS__)

#define console_error(msg, ...) LOGGER->mStdoutLogger->error(msg, ##__VA_ARGS__)

#define console_warn(msg, ...) LOGGER->mStdoutLogger->warn(msg, ##__VA_ARGS__)

#define console_debug(msg, ...) LOGGER->mStdoutLogger->debug(msg, ##__VA_ARGS__)

#define log_info(msg, ...) LOGGER->mSyncLogger->info(msg, ##__VA_ARGS__)

#define log_error(msg, ...) LOGGER->mSyncLogger->error(msg, ##__VA_ARGS__)

#define log_debug(msg, ...) LOGGER->mSyncLogger->debug(msg, ##__VA_ARGS__)

#define log_warn(msg, ...) LOGGER->mSyncLogger->warn(msg, ##__VA_ARGS__)


#define alog_info(msg, ...) LOGGER->mASyncLogger->info(msg, ##__VA_ARGS__)

#define alog_error(msg, ...) LOGGER->mASyncLogger->error(msg, ##__VA_ARGS__)

#define alog_debug(msg, ...) LOGGER->mASyncLogger->debug(msg, ##__VA_ARGS__)

#define alog_warn(msg, ...) LOGGER->mASyncLogger->warn(msg, ##__VA_ARGS__)