
#include "base_worker.h"

onbnet::base_worker::base_worker() {

}

onbnet::base_worker::~base_worker() {

}

void onbnet::base_worker::operator()() {
    start();
}