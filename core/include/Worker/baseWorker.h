#pragma once

namespace onbnet {

    class baseWorker {
    public:
        baseWorker();
        virtual ~baseWorker();

        void operator()();

        virtual void start() {};
        virtual void stop() {};

    private:
        
    };

}