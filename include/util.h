#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

namespace rm
{
    //Parsing
    bool begins_with(const std::string &string, const char *pattern) noexcept;
    bool begins_with(const char *string, const char *pattern) noexcept;
    bool ends_with(const std::string &string, const char *pattern) noexcept;
    bool ends_with(const char *string, const char *pattern) noexcept;
    std::string trim(const std::string &string);
    std::string lower(const std::string &string);
    std::vector<std::string> parse(const std::string &string, char delimiter);
    std::map<std::string, std::string> parse_config(const std::string &path);

    //Filesystem
    class Search
    {
        char _data[32];
    public:
        Search(const std::string &directory);
        const char *get(bool *directory);
        ~Search();
    };
    std::string path_executable();
    std::string path_working_directory();
    std::string path_user_directory();
    std::string path_relative(const std::string &target, const std::string &base, size_t *depth);
    std::string path_base(const std::string &path);
    void path_parent(std::string *path);
    void path_append(std::string *path, const std::string &path2, bool is_directory);
    bool path_exists(const std::string &path, bool is_directory, std::string *absolute_path);
    void path_ensure(const std::string &path, bool is_directory, size_t depth);

    //Other
    int call_wait(std::vector<std::string> &arguments, std::map<std::string, std::string> *environment);
    void call_no_return(std::vector<std::string> &arguments, std::map<std::string, std::string> *environment);
    void remove_cmake_define(std::vector<std::string> *arguments, const char *pattern);
}