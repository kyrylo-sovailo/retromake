#include "../include/util.h"
#include <fstream>
#include <limits.h>
#include <stdexcept>
#include <stdlib.h>
#include <sys/stat.h>

std::string rm::trim(const std::string &string)
{
    size_t begin = 0;
    while ((string[begin] == ' ' || string[begin] == '\t' || string[begin] == '\r' || string[begin] == '\n') && begin < string.size()) begin++;
    size_t end = string.size();
    while ((string[end-1] == ' ' || string[end-1] == '\t' || string[end-1] == '\r' || string[end-1] == '\n') && end > begin) end--;
    return string.substr(begin, end - begin);
}

std::string rm::lower(const std::string &string)
{
    std::string lower(string.size(), '\0');
    for (size_t i = 0; i < string.size(); i++) lower[i] = std::tolower(string[i]);
    return lower;
}

std::vector<std::string> rm::parse(const std::string &string, bool delimiter_only_comma)
{
    std::vector<std::string> result;
    bool request_new = true;
    for (auto ci = string.cbegin(); ci != string.cend(); ci++)
    {
        const char c = *ci;
        if (c == ',' || (!delimiter_only_comma && std::isalnum(c)))
        {
            if (request_new) { result.push_back(std::string(1, c)); request_new = false; }
            else result.back().push_back(c);
        }
        else
        {
            request_new = true;
        }
    }
    return result;
}

bool rm::directory_exists(const std::string &path, std::string *absolute_path)
{
    struct stat status;
    if (stat(path.c_str(), &status) != 0) return false;
    if (!S_ISDIR(status.st_mode)) return false;
    if (absolute_path != nullptr)
    {
        char buffer[PATH_MAX+1];
        char *absolute_path_p = ::realpath(path.c_str(), buffer);
        if (absolute_path_p == nullptr) return false;
        *absolute_path = absolute_path_p;
    }
    return true;
}

bool rm::file_exists(const std::string &path, std::string *absolute_path)
{
    struct stat status;
    if (stat(path.c_str(), &status) != 0) return false;
    if (!S_ISREG(status.st_mode)) return false;
    if (absolute_path != nullptr)
    {
        char buffer[PATH_MAX+1];
        char *absolute_path_p = ::realpath(path.c_str(), buffer);
        if (absolute_path_p == nullptr) return false;
        *absolute_path = absolute_path_p;
    }
    return true;
}

void rm::update_file(const std::string &path, const std::string &content)
{
    bool rewrite;
    {
        std::ifstream file(path, std::ios::binary);
        rewrite = !file.good();
        if (!rewrite)
        {
            file.seekg(0, std::ios::end);
            size_t file_size = file.tellg();
            file.seekg(0, std::ios::beg);
            rewrite = !file.good() || file_size != content.size();
        }
        if (!rewrite)
        {
            std::string existing_content;
            file.read(&existing_content[0], content.size());
            rewrite = !file.good() || content != existing_content;
        }
    }

    if (!rewrite) return;
    std::ofstream file(path, std::ios::binary);
    file.write(content.c_str(), content.size());
    if (!file.good()) throw std::runtime_error("Failed to write file " + path);
}