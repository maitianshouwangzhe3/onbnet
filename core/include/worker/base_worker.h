#pragma once

namespace onbnet {

    class base_worker {
    public:
        base_worker();
        virtual ~base_worker();

        void operator()();

        virtual void start() {};
        virtual void stop() {};

    private:
        
    };

}