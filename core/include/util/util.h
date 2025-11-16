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

namespace onbnet::util {

#define NOTUSED_ARG(v)                                                         \
    ((void)v) // used this to remove warning C4100, unreferenced parameter

class cstr_explode {
  public:
    cstr_explode(char *str, char seperator);
    virtual ~cstr_explode();

    uint32_t get_item_cnt() { return item_cnt_; }
    char* get_item(uint32_t idx) { return item_list_[idx]; }

  private:
    uint32_t item_cnt_;
    char **item_list_;
};


char* replace_str(char *src, char old_char, char new_char);
std::string int2string(uint32_t user_id);
uint32_t string2int(const std::string &value);
void replace_mark(std::string &str, std::string &new_value, uint32_t &begin_pos);
void replace_mark(std::string &str, uint32_t new_value, uint32_t &begin_pos);

void write_pid();
inline unsigned char to_hex(const unsigned char &x);
inline unsigned char from_hex(const unsigned char &x);
std::string URL_encode(const std::string &sIn);
std::string URL_decode(const std::string &sIn);

int64_t get_file_size(const char *path);
const char *mem_find(const char *src_str, size_t src_len, const char *sub_str, size_t sub_len, bool flag = true);

}
