#pragma once
#include <string>
#include <vector>

namespace rm
{
    std::string trim(const std::string &string);
    std::string lower(const std::string &string);
    std::vector<std::string> parse(const std::string &string, bool delimiter_only_comma);
    bool directory_exists(const std::string &path, std::string *absolute_path);
    bool file_exists(const std::string &path, std::string *absolute_path);
    void update_file(const std::string &path, const std::string &content);
}