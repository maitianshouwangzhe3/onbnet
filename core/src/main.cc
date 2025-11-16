
#include "onbnet.h"
#include "config_file_reader.h"
#include "logger.h"

#include <signal.h>
#include <exception>
#include <stdexcept>
void signalHandler(int signum, siginfo_t* info, void* context) {
    onbnet_server_context->stop();
}

int main(int argc, char **argv) {
    new onbnet::Logger();
    struct sigaction sa;
    sa.sa_sigaction = signalHandler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL); 
    if (argc < 2) {
        console_info("argc error, use onbnet [config_path]");
        delete LOGGER;
        return -1;
    }

    try {
        std::shared_ptr<onbnet::util::config_file_reader> config = std::make_shared<onbnet::util::config_file_reader>(argv[1]);
        new onbnet_server(config);

        onbnet_server_context->init();
        onbnet_server_context->start();
    } catch (const std::runtime_error& e) {
        log_error("runtime error: {}", e.what());
    } catch (std::exception e) {
        log_error("exception: {}", e.what());
    }
   
    return 0;
}