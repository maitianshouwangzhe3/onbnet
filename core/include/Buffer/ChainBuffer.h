#pragma once

#include <memory>
#include <cstdint>

namespace onbnet {

#include "CopyAble.h"

struct Buffer;
struct BufChain;

class ChainBuffer: public onbnet::CopyAble {
public:
    ChainBuffer();
    ~ChainBuffer();

    int BufferAdd(const void* DataIn, uint32_t Datlen);

    int BufferRemove(void* data, uint32_t datlen);

    int BufferDrain(uint32_t len);

    void BufferFree();

    uint32_t BufferLen();

    int BufferSearch(const char* sep, const int seplen);

    uint8_t* BufferWriteAtmost();

private:
    BufChain* BufChainInsertNew(uint32_t datlen);

    BufChain* BufChainNew(uint32_t size);

    void BufChainInsert(BufChain *chain);

    BufChain** FreeEmptyChains();

    void BufChainFreeAll(BufChain* chain);

    int BufChainShouldRealign(BufChain* chain, uint32_t datlen);

    void BufChainAlign(BufChain* chain);

    uint32_t BufCopyOut(void* DataOut, uint32_t Datlen);

    void ZeroChain();

    bool CheckSep(BufChain* chain, int from, const char* sep, int seplen);

private:
    Buffer* mBuffer;
};

}