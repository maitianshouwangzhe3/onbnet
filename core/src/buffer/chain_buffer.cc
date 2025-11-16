
#include "chain_buffer.h"
#include "spinlock.h"
#include <cstdint>
#include <memory>
#include <cstdlib>
#include <cstring>

#define CHAIN_SPACE_LEN(ch) ((ch)->buffer_len - ((ch)->misalign + (ch)->off))
#define MIN_BUFFER_SIZE 1024
#define MAX_TO_COPY_IN_EXPAND 4096
#define BUFFER_CHAIN_MAX_AUTO_SIZE 4096
#define MAX_TO_REALIGN_IN_EXPAND 2048
#define BUFFER_CHAIN_MAX 16*1024*1024  // 16M
#define BUFFER_CHAIN_EXTRA(t, c) (t *)((buf_chain *)(c) + 1)
#define BUFFER_CHAIN_SIZE sizeof(buf_chain)

namespace onbnet {

struct buf_chain {
    buf_chain* next;
    uint32_t buffer_len;
    uint32_t misalign;
    uint32_t off;
    uint8_t* buffer;
};

struct buffer {
    buf_chain*  first;
    buf_chain* last;
    buf_chain** last_with_datap;
    uint32_t total_len;
    uint32_t last_read_pos;
};


}

onbnet::chain_buffer::chain_buffer() {
    buffer_ = new buffer();
    zero_chain();
    spinlock_init(&lock_);
}

onbnet::chain_buffer::~chain_buffer() {
    if (buffer_) {
        delete buffer_;
        buffer_ = nullptr;
    }
}

int onbnet::chain_buffer::buffer_add(const void* data_in, uint32_t datlen) {
    spinlock_lock(&lock_);
    buf_chain *chain, *tmp;
    const uint8_t* data = static_cast<const uint8_t*>(data_in);
    uint32_t remain, to_alloc;
    int result = -1;
    if (datlen > BUFFER_CHAIN_MAX - buffer_->total_len) {
        goto done;
    }

    if (!(*buffer_->last_with_datap)) {
        chain = buffer_->last;
    } else {
        chain = *buffer_->last_with_datap;
    }

    if (!chain) {
        chain = buf_chain_insert_new(datlen);
        if (!chain)
            goto done;
    }

    remain = chain->buffer_len - chain->misalign - chain->off;
    if (remain >= datlen) {
        memcpy(chain->buffer + chain->misalign + chain->off, data, datlen);
        chain->off += datlen;
        buffer_->total_len += datlen;
        // buf->n_add_for_cb += datlen;
        goto out;
    } else if (buf_chain_should_realign(chain, datlen)) {
        buf_chain_align(chain);

        memcpy(chain->buffer + chain->off, data, datlen);
        chain->off += datlen;
        buffer_->total_len += datlen;
        // buf->n_add_for_cb += datlen;
        goto out;
    }
    to_alloc = chain->buffer_len;
    if (to_alloc <= BUFFER_CHAIN_MAX_AUTO_SIZE/2)
        to_alloc <<= 1;
    if (datlen > to_alloc)
        to_alloc = datlen;
    tmp = buf_chain_new(to_alloc);
    if (tmp == NULL)
        goto done;
    if (remain) {
        memcpy(chain->buffer + chain->misalign + chain->off, data, remain);
        chain->off += remain;
        buffer_->total_len += remain;
        // buf->n_add_for_cb += remain;
    }

    data += remain;
    datlen -= remain;

    memcpy(tmp->buffer, data, datlen);
    tmp->off = datlen;
    buf_chain_insert(tmp);
    // buf->n_add_for_cb += datlen;
out:
    result = 0;
done:
    spinlock_unlock(&lock_);
    return result;
}

int onbnet::chain_buffer::buffer_remove(void* data, uint32_t datlen) {
    spinlock_lock(&lock_);
    uint32_t n = buf_copy_out( data, datlen);
    spinlock_unlock(&lock_);
    if (n > 0) {
        if (buffer_drain(n) < 0)
            n = -1;
    }
    
    return static_cast<int>(n);
}

int onbnet::chain_buffer::buffer_drain(uint32_t len) {
    spinlock_lock(&lock_);
    buf_chain *chain, *next;
    uint32_t remaining, old_len;
    old_len = buffer_->total_len;
    if (old_len == 0) {
        spinlock_unlock(&lock_);
        return 0;
    }

    if (len >= old_len) {
        len = old_len;
        for (chain = buffer_->first; chain != NULL; chain = next) {
            next = chain->next;
            free(chain);
        }

        zero_chain();
    } else {
        buffer_->total_len -= len;
        remaining = len;
        for (chain = buffer_->first; remaining >= chain->off; chain = next) {
            next = chain->next;
            remaining -= chain->off;

            if (chain == *buffer_->last_with_datap) {
                buffer_->last_with_datap = &buffer_->first;
            }
            if (&chain->next == buffer_->last_with_datap)
                buffer_->last_with_datap = &buffer_->first;

            free(chain);
        }

        buffer_->first = chain;
        chain->misalign += remaining;
        chain->off -= remaining;
    }
    
    // buf->n_del_for_cb += len;
    spinlock_unlock(&lock_);
    return len;
}

void onbnet::chain_buffer::buffer_free() {
    spinlock_lock(&lock_);
    if (buffer_) {
        buf_chain_free_all(buffer_->first);
    }
    spinlock_unlock(&lock_);
}

uint32_t onbnet::chain_buffer::buffer_len() {
    return buffer_->total_len;
}

int onbnet::chain_buffer::buffer_search(const char* sep, const int seplen) {
    spinlock_lock(&lock_);
    buf_chain* chain = nullptr;
    int i;
    chain = buffer_->first;
    if (!chain) {
        spinlock_unlock(&lock_);
        return 0;
    }
        
    int bytes = chain->off;
    while (bytes <= buffer_->last_read_pos) {
        chain = chain->next;
        if (!chain) {
            spinlock_unlock(&lock_);
            return 0;
        }
            
        bytes += chain->off;
    }
    bytes -= buffer_->last_read_pos;
    int from = chain->off - bytes;
    for (i = buffer_->last_read_pos; i <= buffer_->total_len - seplen; i++) {
        if (check_sep(chain, from, sep, seplen)) {
            buffer_->last_read_pos = 0;
            spinlock_unlock(&lock_);
            return i+seplen;
        }
        ++from;
        --bytes;
        if (bytes == 0) {
            chain = chain->next;
            from = 0;
            if (!chain)
                break;
            bytes = chain->off;
        }
    }
    buffer_->last_read_pos = i;
    spinlock_unlock(&lock_);
    return 0;
}

uint8_t* onbnet::chain_buffer::buffer_write_atmost() {
    spinlock_lock(&lock_);
    buf_chain *chain = nullptr, *next = nullptr, *tmp = nullptr, *last_with_data = nullptr;
    uint8_t *buffer = nullptr;
    uint32_t remaining = 0;
    int removed_last_with_data = 0;
    int removed_last_with_datap = 0;

    chain = buffer_->first;
    uint32_t size = buffer_->total_len;

    if (chain->off >= size) {
        spinlock_unlock(&lock_);
        return chain->buffer + chain->misalign;
    }

    remaining = size - chain->off;
    for (tmp=chain->next; tmp; tmp=tmp->next) {
        if (tmp->off >= (size_t)remaining)
            break;
        remaining -= tmp->off;
    }

    if (chain->buffer_len - chain->misalign >= (size_t)size) {
        /* already have enough space in the first chain */
        size_t old_off = chain->off;
        buffer = chain->buffer + chain->misalign + chain->off;
        tmp = chain;
        tmp->off = size;
        size -= old_off;
        chain = chain->next;
    } else {
        if ((tmp = buf_chain_new(size)) == NULL) {
            spinlock_unlock(&lock_);
            return NULL;
        }
        buffer = tmp->buffer;
        tmp->off = size;
        buffer_->first = tmp;
    }

    last_with_data = *buffer_->last_with_datap;
    for (; chain != NULL && (size_t)size >= chain->off; chain = next) {
        next = chain->next;

        if (chain->buffer) {
            memcpy(buffer, chain->buffer + chain->misalign, chain->off);
            size -= chain->off;
            buffer += chain->off;
        }
        if (chain == last_with_data)
            removed_last_with_data = 1;
        if (&chain->next == buffer_->last_with_datap)
            removed_last_with_datap = 1;

        free(chain);
    }

    if (chain != NULL) {
        memcpy(buffer, chain->buffer + chain->misalign, size);
        chain->misalign += size;
        chain->off -= size;
    } else {
        buffer_->last = tmp;
    }

    tmp->next = chain;

    if (removed_last_with_data) {
        buffer_->last_with_datap = &buffer_->first;
    } else if (removed_last_with_datap) {
        if (buffer_->first->next && buffer_->first->next->off)
            buffer_->last_with_datap = &buffer_->first->next;
        else
            buffer_->last_with_datap = &buffer_->first;
    }

    spinlock_unlock(&lock_);
    return tmp->buffer + tmp->misalign;
}

onbnet::buf_chain* onbnet::chain_buffer::buf_chain_insert_new(uint32_t datlen) {
    buf_chain *chain = nullptr;
    if ((chain = buf_chain_new(datlen)) == nullptr)
        return nullptr;
    buf_chain_insert(chain);
    return chain;
}

onbnet::buf_chain* onbnet::chain_buffer::buf_chain_new(uint32_t size) {
    buf_chain* chain = nullptr;
    uint32_t toAlloc = 0;
    if (size > BUFFER_CHAIN_MAX - BUFFER_CHAIN_SIZE)
        return (nullptr);
    size += BUFFER_CHAIN_SIZE;
    
    if (size < BUFFER_CHAIN_MAX / 2) {
        toAlloc = MIN_BUFFER_SIZE;
        while (toAlloc < size) {
            toAlloc <<= 1;
        }
    } else {
        toAlloc = size;
    }
    if ((chain = (buf_chain*)malloc(toAlloc)) == nullptr)
        return (nullptr);
    memset(chain, 0, BUFFER_CHAIN_SIZE);
    chain->buffer_len = toAlloc - BUFFER_CHAIN_SIZE;
    chain->buffer = BUFFER_CHAIN_EXTRA(uint8_t, chain);
    return (chain);
}

void onbnet::chain_buffer::buf_chain_insert(buf_chain *chain) {
    if (!(*buffer_->last_with_datap)) {
        buffer_->first = buffer_->last = chain;
    } else {
        buf_chain **chp = nullptr;
        chp = free_empty_chains();
        *chp = chain;
        if (chain->off)
            buffer_->last_with_datap = chp;
        buffer_->last = chain;
    }
    buffer_->total_len += chain->off;
}

onbnet::buf_chain** onbnet::chain_buffer::free_empty_chains() {
    buf_chain** ch = buffer_->last_with_datap;
    while ((*ch) && (*ch)->off != 0)
        ch = &(*ch)->next;
    if (*ch) {
        buf_chain_free_all(*ch);
        *ch = nullptr;
    }
    return ch;
}

void onbnet::chain_buffer::buf_chain_free_all(buf_chain *chain) {
    buf_chain *next = nullptr;
    for (; chain; chain = next) {
        next = chain->next;
        free(chain);
    }
}

int onbnet::chain_buffer::buf_chain_should_realign(buf_chain* chain, uint32_t datlen) {
    return chain->buffer_len - chain->off >= datlen &&
        (chain->off < chain->buffer_len / 2) &&
        (chain->off <= MAX_TO_REALIGN_IN_EXPAND);
}

void onbnet::chain_buffer::buf_chain_align(buf_chain* chain) {
    memmove(chain->buffer, chain->buffer + chain->misalign, chain->off);
    chain->misalign = 0;
}

uint32_t onbnet::chain_buffer::buf_copy_out(void* DataOut, uint32_t Datlen) {
    buf_chain *chain;
    char *data = static_cast<char*>(DataOut);
    uint32_t nread;
    chain = buffer_->first;
    if (Datlen > buffer_->total_len)
        Datlen = buffer_->total_len;
    if (Datlen == 0)
        return 0;
    nread = Datlen;

    while (Datlen && Datlen >= chain->off) {
        uint32_t copylen = chain->off;
        memcpy(data,chain->buffer + chain->misalign, copylen);
        data += copylen;
        Datlen -= copylen;

        chain = chain->next;
    }
    if (Datlen) {
        memcpy(data, chain->buffer + chain->misalign, Datlen);
    }

    return nread;
}

void onbnet::chain_buffer::zero_chain() {
    buffer_->first = nullptr;
    buffer_->last = nullptr;
    buffer_->last_with_datap = &(buffer_)->first;
    buffer_->total_len = 0;
}

bool onbnet::chain_buffer::check_sep(buf_chain* chain, int from, const char* sep, int seplen) {
    for (;;) {
        int sz = chain->off - from;
        if (sz >= seplen) {
            return memcmp(chain->buffer + chain->misalign + from, sep, seplen) == 0;
        }
        if (sz > 0) {
            if (memcmp(chain->buffer + chain->misalign + from, sep, sz)) {
                return false;
            }
        }
        chain = chain->next;
        sep += sz;
        seplen -= sz;
        from = 0;
    }
}