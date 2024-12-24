#include "../include/util.h"

#include <limits.h>
#include <sys/stat.h>

#include <cctype>
#include <fstream>
#include <stdexcept>

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
        if (c == ',' || (!delimiter_only_comma && !std::isalnum(c)))
        {
            request_new = true;
        }
        else
        {
            if (request_new) { result.push_back(std::string(1, c)); request_new = false; }
            else result.back().push_back(c);
        }
    }
    return result;
}

std::map<std::string, std::string> rm::parse_config(const std::string &path)
{
    enum class State
    {
        wait_name_begin,
        wait_name_end,
        wait_equal,
        wait_value_begin,
        wait_value_end,
        wait_end_line
    };
    std::map<std::string, std::string> result;
    std::string name, value;
    State state = State::wait_name_begin;

    std::ifstream file(path, std::ios::binary);
    while (true)
    {
        char c;
        const bool good = static_cast<bool>(file.read(&c, 1));
        switch (state)
        {
        case State::wait_name_begin:
            if (!good || c == '\0') return result;
            else if (c == '#') state = State::wait_end_line;
            else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {}
            else if (c == '_' || std::isalpha(c)) { name.push_back(c); state = State::wait_name_end; }
            else throw std::runtime_error("Unexpected symbol in entry name in " + path);
            break;
        case State::wait_name_end:
            if (!good || c == '\0' || c == '\n' || c == '\r') throw std::runtime_error("Unexpected end of entry name in " + path);
            else if (c == ' ' || c == '\t') state = State::wait_equal;
            else if (c == '_' || std::isalnum(c)) name.push_back(c);
            else if (c == '=') state = State::wait_value_begin;
            else throw std::runtime_error("Unexpected symbol in entry name in " + path);
            break;
        case State::wait_equal:
            if (!good || c == '\0' || c == '\n' || c == '\r') throw std::runtime_error("Unexpected end of entry name in " + path);
            else if (c == ' ' || c == '\t') {}
            else if (c == '=') state = State::wait_value_begin;
            else throw std::runtime_error("Unexpected symbol in entry name in " + path);
            break;
        case State::wait_value_begin:
            if (!good || c == '\0' || c == '\n' || c == '\r') throw std::runtime_error("Unexpected end of entry value in " + path);
            else if (c == ' ' || c == '\t') {}
            else { value.push_back(c); state = State::wait_value_end; }
            break;
        case State::wait_value_end:
            if (!good || c == '\0' || c == '\n' || c == '\r')
            {
                result[name] = value;
                if (!good || c == '\0') return result;
                name.resize(0);
                value.resize(0);
                state = State::wait_name_begin;
            }
            else value.push_back(c);
            break;
        case State::wait_end_line:
            if (!good || c == '\0') return result;
            else if (c == '\n' || c == '\r') state = State::wait_name_begin;
            break;
        }
    }
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
        if (absolute_path->back() != '/') absolute_path->push_back('/');
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