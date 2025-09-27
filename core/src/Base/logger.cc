
#include "logger.h"
#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

onbnet::Logger* onbnet::Logger::inst = nullptr;
onbnet::Logger::Logger() {
    if (!inst) {
        inst = this;
        mStdoutLogger = spdlog::stdout_color_mt("console");
        mSyncLogger = spdlog::basic_logger_mt("onbnet_sync_logger", "logs/onbnet-log.txt");
        mASyncLogger = spdlog::basic_logger_mt<spdlog::async_factory>("onbnet_async_logger", "logs/onbnet-log.txt");
    }
}
onbnet::Logger::~Logger() {

}
