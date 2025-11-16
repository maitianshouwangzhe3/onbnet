#pragma once

#include <map>
#include <string>

namespace onbnet::util {

class config_file_reader {
public:
    config_file_reader(const char *filename);
    ~config_file_reader();

    char *get_config_name(const char *name);
    int set_config_value(const char *name, const char *value);

private:
    void _load_file(const char *filename);
    int _write_fIle(const char *filename = NULL);
    void _parse_line(char *line);
    char *_trim_space(char *name);

    bool load_ok_;
    std::map<std::string, std::string> config_map_;
    std::string config_file_;
};

};