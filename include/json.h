#pragma once
#include <string>

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
    typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> JSONAllocator;
    typedef rapidjson::GenericValue<rapidjson::UTF8<char>, JSONAllocator> JSONValue;
    typedef rapidjson::GenericDocument<rapidjson::UTF8<char>, JSONAllocator, JSONAllocator> JSONDocument;

    const JSONValue *find_string(const JSONValue &parent, const char *name);
    const JSONValue *find_array(const JSONValue &parent, const char *name);
    const JSONValue *find_object(const JSONValue &parent, const char *name);

    struct JSONEditor
    {
        JSONDocument *document;
        JSONAllocator *allocator;
        bool change;

        JSONEditor();
        void document_read(const std::string &path);
        void document_write(const std::string &path, size_t depth) const;

        JSONValue &create_string(JSONValue &parent, const char *name);
        JSONValue &create_array(JSONValue &parent, const char *name);
        JSONValue &create_object(JSONValue &parent, const char *name);
        void checkout_string(JSONValue &node, const char *value);
        void checkout_string(JSONValue &node, const std::string &value);

        ~JSONEditor();
    };
}