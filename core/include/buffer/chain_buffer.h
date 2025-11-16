#pragma once

#include <cstdint>
#include "spinlock.h"

namespace onbnet {
#include "copy_able.h"

struct buffer;
struct buf_chain;

class chain_buffer: public onbnet::copy_able {
public:
    chain_buffer();
    ~chain_buffer();

    int buffer_add(const void* data_in, uint32_t datlen);

    int buffer_remove(void* data, uint32_t datlen);

    int buffer_drain(uint32_t len);

    void buffer_free();

    uint32_t buffer_len();

    int buffer_search(const char* sep, const int seplen);

    uint8_t* buffer_write_atmost();

private:
    buf_chain* buf_chain_insert_new(uint32_t datlen);

    buf_chain* buf_chain_new(uint32_t size);

    void buf_chain_insert(buf_chain *chain);

    buf_chain** free_empty_chains();

    void buf_chain_free_all(buf_chain* chain);

    int buf_chain_should_realign(buf_chain* chain, uint32_t datlen);

    void buf_chain_align(buf_chain* chain);

    uint32_t buf_copy_out(void* data_out, uint32_t datlen);

    void zero_chain();

    bool check_sep(buf_chain* chain, int from, const char* sep, int seplen);

private:
    buffer* buffer_;

    spinlock lock_;
};

}