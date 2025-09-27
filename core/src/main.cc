
#include "Onbnet.h"
#include "ConfigFileReader.h"
#include "logger.h"

#include <signal.h>
#include <exception>
#include <stdexcept>
void signalHandler(int signum, siginfo_t* info, void* context) {
    onbnetServerContext->Stop();
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
        std::shared_ptr<CConfigFileReader> config = std::make_shared<CConfigFileReader>(argv[1]);
        new onbnetServer(config);

        onbnetServerContext->Init();
        onbnetServerContext->Start();
    } catch (const std::runtime_error& e) {
        log_error("runtime error: {}", e.what());
    } catch (std::exception e) {
        log_error("exception: {}", e.what());
    }
   
    return 0;
}