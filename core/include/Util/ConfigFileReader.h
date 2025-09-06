#pragma once

#include <map>
#include <string>

class CConfigFileReader {
  public:
    CConfigFileReader(const char *filename);
    ~CConfigFileReader();

    char *GetConfigName(const char *name);
    int SetConfigValue(const char *name, const char *value);

  private:
    void _LoadFile(const char *filename);
    int _WriteFIle(const char *filename = NULL);
    void _ParseLine(char *line);
    char *_TrimSpace(char *name);

    bool load_ok_;
    std::map<std::string, std::string> config_map_;
    std::string config_file_;
};
