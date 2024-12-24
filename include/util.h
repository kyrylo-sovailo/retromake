#pragma once
#include <map>
#include <string>
#include <vector>

namespace rm
{
    std::string trim(const std::string &string);
    std::string lower(const std::string &string);
    std::vector<std::string> parse(const std::string &string, bool delimiter_only_comma);
    std::map<std::string, std::string> parse_config(const std::string &path);
    bool directory_exists(const std::string &path, std::string *absolute_path);
    bool file_exists(const std::string &path, std::string *absolute_path);
}