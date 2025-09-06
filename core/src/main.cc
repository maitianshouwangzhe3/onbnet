
#include "Onbnet.h"
#include "ConfigFileReader.h"

#include <signal.h>

void signalHandler(int signum, siginfo_t* info, void* context) {
    onbnetServerContext->Stop();
}

int main(int argc, char **argv) {
    struct sigaction sa;
    sa.sa_sigaction = signalHandler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL); 

    if (argc < 2) {
        return -1;
    }
    std::shared_ptr<CConfigFileReader> config = std::make_shared<CConfigFileReader>(argv[1]);
    new onbnetServer(config);

    onbnetServerContext->Init();
    onbnetServerContext->Start();

    
    return 0;
}