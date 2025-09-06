#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <assert.h>
#include <sys/stat.h>

#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

#define NOTUSED_ARG(v)                                                         \
    ((void)v) // used this to remove warning C4100, unreferenced parameter

class CStrExplode {
  public:
    CStrExplode(char *str, char seperator);
    virtual ~CStrExplode();

    uint32_t GetItemCnt() { return item_cnt_; }
    char *GetItem(uint32_t idx) { return item_list_[idx]; }

  private:
    uint32_t item_cnt_;
    char **item_list_;
};


char *ReplaceStr(char *src, char old_char, char new_char);
std::string Int2String(uint32_t user_id);
uint32_t String2Int(const std::string &value);
void ReplaceMark(std::string &str, std::string &new_value, uint32_t &begin_pos);
void ReplaceMark(std::string &str, uint32_t new_value, uint32_t &begin_pos);

void WritePid();
inline unsigned char ToHex(const unsigned char &x);
inline unsigned char FromHex(const unsigned char &x);
std::string URLEncode(const std::string &sIn);
std::string URLDecode(const std::string &sIn);

int64_t GetFileSize(const char *path);
const char *MemFind(const char *src_str, size_t src_len, const char *sub_str, size_t sub_len, bool flag = true);
