
#include "ChainBuffer.h"
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
#define BUFFER_CHAIN_EXTRA(t, c) (t *)((BufChain *)(c) + 1)
#define BUFFER_CHAIN_SIZE sizeof(BufChain)

namespace onbnet {

struct BufChain {
    BufChain* next;
    uint32_t BufferLen;
    uint32_t misalign;
    uint32_t off;
    uint8_t* Buffer;
};

struct Buffer {
    BufChain*  First;
    BufChain* Last;
    BufChain** LastWithDatap;
    uint32_t TotalLen;
    uint32_t LastReadPos;
};


}

onbnet::ChainBuffer::ChainBuffer() {
    mBuffer = new Buffer();
    ZeroChain();
    spinlock_init(&lock);
}

onbnet::ChainBuffer::~ChainBuffer() {
    if (mBuffer) {
        delete mBuffer;
        mBuffer = nullptr;
    }
}

int onbnet::ChainBuffer::BufferAdd(const void* DataIn, uint32_t Datlen) {
    spinlock_lock(&lock);
    BufChain *chain, *tmp;
    const uint8_t* data = static_cast<const uint8_t*>(DataIn);
    uint32_t remain, to_alloc;
    int result = -1;
    if (Datlen > BUFFER_CHAIN_MAX - mBuffer->TotalLen) {
        goto done;
    }

    if (!(*mBuffer->LastWithDatap)) {
        chain = mBuffer->Last;
    } else {
        chain = *mBuffer->LastWithDatap;
    }

    if (!chain) {
        chain = BufChainInsertNew(Datlen);
        if (!chain)
            goto done;
    }

    remain = chain->BufferLen - chain->misalign - chain->off;
    if (remain >= Datlen) {
        memcpy(chain->Buffer + chain->misalign + chain->off, data, Datlen);
        chain->off += Datlen;
        mBuffer->TotalLen += Datlen;
        // buf->n_add_for_cb += datlen;
        goto out;
    } else if (BufChainShouldRealign(chain, Datlen)) {
        BufChainAlign(chain);

        memcpy(chain->Buffer + chain->off, data, Datlen);
        chain->off += Datlen;
        mBuffer->TotalLen += Datlen;
        // buf->n_add_for_cb += datlen;
        goto out;
    }
    to_alloc = chain->BufferLen;
    if (to_alloc <= BUFFER_CHAIN_MAX_AUTO_SIZE/2)
        to_alloc <<= 1;
    if (Datlen > to_alloc)
        to_alloc = Datlen;
    tmp = BufChainNew(to_alloc);
    if (tmp == NULL)
        goto done;
    if (remain) {
        memcpy(chain->Buffer + chain->misalign + chain->off, data, remain);
        chain->off += remain;
        mBuffer->TotalLen += remain;
        // buf->n_add_for_cb += remain;
    }

    data += remain;
    Datlen -= remain;

    memcpy(tmp->Buffer, data, Datlen);
    tmp->off = Datlen;
    BufChainInsert(tmp);
    // buf->n_add_for_cb += datlen;
out:
    result = 0;
done:
    spinlock_unlock(&lock);
    return result;
}

int onbnet::ChainBuffer::BufferRemove(void* data, uint32_t datlen) {
    spinlock_lock(&lock);
    uint32_t n = BufCopyOut( data, datlen);
    spinlock_unlock(&lock);
    if (n > 0) {
        if (BufferDrain(n) < 0)
            n = -1;
    }
    
    return static_cast<int>(n);
}

int onbnet::ChainBuffer::BufferDrain(uint32_t len) {
    spinlock_lock(&lock);
    BufChain *chain, *next;
    uint32_t remaining, old_len;
    old_len = mBuffer->TotalLen;
    if (old_len == 0) {
        spinlock_unlock(&lock);
        return 0;
    }

    if (len >= old_len) {
        len = old_len;
        for (chain = mBuffer->First; chain != NULL; chain = next) {
            next = chain->next;
            free(chain);
        }

        ZeroChain();
    } else {
        mBuffer->TotalLen -= len;
        remaining = len;
        for (chain = mBuffer->First; remaining >= chain->off; chain = next) {
            next = chain->next;
            remaining -= chain->off;

            if (chain == *mBuffer->LastWithDatap) {
                mBuffer->LastWithDatap = &mBuffer->First;
            }
            if (&chain->next == mBuffer->LastWithDatap)
                mBuffer->LastWithDatap = &mBuffer->First;

            free(chain);
        }

        mBuffer->First = chain;
        chain->misalign += remaining;
        chain->off -= remaining;
    }
    
    // buf->n_del_for_cb += len;
    spinlock_unlock(&lock);
    return len;
}

void onbnet::ChainBuffer::BufferFree() {
    spinlock_lock(&lock);
    if (mBuffer) {
        BufChainFreeAll(mBuffer->First);
    }
    spinlock_unlock(&lock);
}

uint32_t onbnet::ChainBuffer::BufferLen() {
    return mBuffer->TotalLen;
}

int onbnet::ChainBuffer::BufferSearch(const char* sep, const int seplen) {
    spinlock_lock(&lock);
    BufChain* chain = nullptr;
    int i;
    chain = mBuffer->First;
    if (!chain) {
        spinlock_unlock(&lock);
        return 0;
    }
        
    int bytes = chain->off;
    while (bytes <= mBuffer->LastReadPos) {
        chain = chain->next;
        if (!chain) {
            spinlock_unlock(&lock);
            return 0;
        }
            
        bytes += chain->off;
    }
    bytes -= mBuffer->LastReadPos;
    int from = chain->off - bytes;
    for (i = mBuffer->LastReadPos; i <= mBuffer->TotalLen - seplen; i++) {
        if (CheckSep(chain, from, sep, seplen)) {
            mBuffer->LastReadPos = 0;
            spinlock_unlock(&lock);
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
    mBuffer->LastReadPos = i;
    spinlock_unlock(&lock);
    return 0;
}

uint8_t* onbnet::ChainBuffer::BufferWriteAtmost() {
    spinlock_lock(&lock);
    BufChain *chain = nullptr, *next = nullptr, *tmp = nullptr, *last_with_data = nullptr;
    uint8_t *buffer = nullptr;
    uint32_t remaining = 0;
    int RemovedLastWithData = 0;
    int RemovedLastWithDatap = 0;

    chain = mBuffer->First;
    uint32_t size = mBuffer->TotalLen;

    if (chain->off >= size) {
        spinlock_unlock(&lock);
        return chain->Buffer + chain->misalign;
    }

    remaining = size - chain->off;
    for (tmp=chain->next; tmp; tmp=tmp->next) {
        if (tmp->off >= (size_t)remaining)
            break;
        remaining -= tmp->off;
    }

    if (chain->BufferLen - chain->misalign >= (size_t)size) {
        /* already have enough space in the first chain */
        size_t old_off = chain->off;
        buffer = chain->Buffer + chain->misalign + chain->off;
        tmp = chain;
        tmp->off = size;
        size -= old_off;
        chain = chain->next;
    } else {
        if ((tmp = BufChainNew(size)) == NULL) {
            spinlock_unlock(&lock);
            return NULL;
        }
        buffer = tmp->Buffer;
        tmp->off = size;
        mBuffer->First = tmp;
    }

    last_with_data = *mBuffer->LastWithDatap;
    for (; chain != NULL && (size_t)size >= chain->off; chain = next) {
        next = chain->next;

        if (chain->Buffer) {
            memcpy(buffer, chain->Buffer + chain->misalign, chain->off);
            size -= chain->off;
            buffer += chain->off;
        }
        if (chain == last_with_data)
            RemovedLastWithData = 1;
        if (&chain->next == mBuffer->LastWithDatap)
            RemovedLastWithDatap = 1;

        free(chain);
    }

    if (chain != NULL) {
        memcpy(buffer, chain->Buffer + chain->misalign, size);
        chain->misalign += size;
        chain->off -= size;
    } else {
        mBuffer->Last = tmp;
    }

    tmp->next = chain;

    if (RemovedLastWithData) {
        mBuffer->LastWithDatap = &mBuffer->First;
    } else if (RemovedLastWithDatap) {
        if (mBuffer->First->next && mBuffer->First->next->off)
            mBuffer->LastWithDatap = &mBuffer->First->next;
        else
            mBuffer->LastWithDatap = &mBuffer->First;
    }

    spinlock_unlock(&lock);
    return tmp->Buffer + tmp->misalign;
}

onbnet::BufChain* onbnet::ChainBuffer::BufChainInsertNew(uint32_t datlen) {
    BufChain *chain = nullptr;
    if ((chain = BufChainNew(datlen)) == nullptr)
        return nullptr;
    BufChainInsert(chain);
    return chain;
}

onbnet::BufChain* onbnet::ChainBuffer::BufChainNew(uint32_t size) {
    BufChain* chain = nullptr;
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
    if ((chain = (BufChain*)malloc(toAlloc)) == nullptr)
        return (nullptr);
    memset(chain, 0, BUFFER_CHAIN_SIZE);
    chain->BufferLen = toAlloc - BUFFER_CHAIN_SIZE;
    chain->Buffer = BUFFER_CHAIN_EXTRA(uint8_t, chain);
    return (chain);
}

void onbnet::ChainBuffer::BufChainInsert(BufChain *chain) {
    if (!(*mBuffer->LastWithDatap)) {
        mBuffer->First = mBuffer->Last = chain;
    } else {
        BufChain **chp = nullptr;
        chp = FreeEmptyChains();
        *chp = chain;
        if (chain->off)
            mBuffer->LastWithDatap = chp;
        mBuffer->Last = chain;
    }
    mBuffer->TotalLen += chain->off;
}

onbnet::BufChain** onbnet::ChainBuffer::FreeEmptyChains() {
    BufChain** ch = mBuffer->LastWithDatap;
    while ((*ch) && (*ch)->off != 0)
        ch = &(*ch)->next;
    if (*ch) {
        BufChainFreeAll(*ch);
        *ch = nullptr;
    }
    return ch;
}

void onbnet::ChainBuffer::BufChainFreeAll(BufChain *chain) {
    BufChain *next = nullptr;
    for (; chain; chain = next) {
        next = chain->next;
        free(chain);
    }
}

int onbnet::ChainBuffer::BufChainShouldRealign(BufChain* chain, uint32_t datlen) {
    return chain->BufferLen - chain->off >= datlen &&
        (chain->off < chain->BufferLen / 2) &&
        (chain->off <= MAX_TO_REALIGN_IN_EXPAND);
}

void onbnet::ChainBuffer::BufChainAlign(BufChain* chain) {
    memmove(chain->Buffer, chain->Buffer + chain->misalign, chain->off);
    chain->misalign = 0;
}

uint32_t onbnet::ChainBuffer::BufCopyOut(void* DataOut, uint32_t Datlen) {
    BufChain *chain;
    char *data = static_cast<char*>(DataOut);
    uint32_t nread;
    chain = mBuffer->First;
    if (Datlen > mBuffer->TotalLen)
        Datlen = mBuffer->TotalLen;
    if (Datlen == 0)
        return 0;
    nread = Datlen;

    while (Datlen && Datlen >= chain->off) {
        uint32_t copylen = chain->off;
        memcpy(data,chain->Buffer + chain->misalign, copylen);
        data += copylen;
        Datlen -= copylen;

        chain = chain->next;
    }
    if (Datlen) {
        memcpy(data, chain->Buffer + chain->misalign, Datlen);
    }

    return nread;
}

void onbnet::ChainBuffer::ZeroChain() {
    mBuffer->First = nullptr;
    mBuffer->Last = nullptr;
    mBuffer->LastWithDatap = &(mBuffer)->First;
    mBuffer->TotalLen = 0;
}

bool onbnet::ChainBuffer::CheckSep(BufChain* chain, int from, const char* sep, int seplen) {
    for (;;) {
        int sz = chain->off - from;
        if (sz >= seplen) {
            return memcmp(chain->Buffer + chain->misalign + from, sep, seplen) == 0;
        }
        if (sz > 0) {
            if (memcmp(chain->Buffer + chain->misalign + from, sep, sz)) {
                return false;
            }
        }
        chain = chain->next;
        sep += sz;
        seplen -= sz;
        from = 0;
    }
}