#pragma once
#include <map>
#include <string>
#include <vector>

namespace rapidjson
{
    class CrtAllocator;
    template <typename BaseAllocator> class MemoryPoolAllocator;
    template<typename CharType> struct UTF8;
    template <typename Encoding, typename Allocator> class GenericValue;
    template <typename Encoding, typename Allocator, typename StackAllocator> class GenericDocument;
}

namespace rm
{
    //Parsing
    std::string trim(const std::string &string);
    std::string lower(const std::string &string);
    std::vector<std::string> parse(const std::string &string, bool delimiter_only_comma);
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
    void path_parent(std::string *path);
    void path_append(std::string *path, const std::string &path2, bool is_directory);
    bool path_exists(const std::string &path, bool is_directory, std::string *absolute_path);
    void path_ensure(const std::string &path, bool is_directory, size_t depth);

    //RapidJSON
    typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> JSONAllocator;
    typedef rapidjson::GenericValue<rapidjson::UTF8<char>, JSONAllocator> JSONValue;
    typedef rapidjson::GenericDocument<rapidjson::UTF8<char>, JSONAllocator, JSONAllocator> JSONDocument;

    //Other
    int call_wait(std::vector<std::string> &arguments, std::map<std::string, std::string> *environment);
    void call_no_return(std::vector<std::string> &arguments, std::map<std::string, std::string> *environment);
}