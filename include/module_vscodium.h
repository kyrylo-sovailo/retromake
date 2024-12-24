#pragma once
#include "module.h"

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
    class VSCodiumModule : public Module
    {
    private:
        typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> JSONAllocator;
        typedef rapidjson::GenericValue<rapidjson::UTF8<char>, JSONAllocator> JSONValue;
        typedef rapidjson::GenericDocument<rapidjson::UTF8<char>, JSONAllocator, JSONAllocator> JSONDocument;
        RetroMake *_system = nullptr;
        bool _checkout_string(JSONValue &string, JSONAllocator &allocator, const char *value);
        bool _checkout_args(JSONValue &args, JSONAllocator &allocator);
        bool _checkout_options(JSONValue &options, JSONAllocator &allocator);
        bool _checkout_task(JSONValue &task, JSONAllocator &allocator);
        bool _checkout_tasks(JSONValue &tasks, JSONAllocator &allocator);
        bool _checkout_document(JSONValue &document, JSONAllocator &allocator);
        bool _read_document(JSONDocument &document);
    
    public:
        static Module *create_module(const std::string &requested_module);

        VSCodiumModule();
        int order() const override;
        std::string id() const override;
        std::string name() const override;
        std::string help() const override;
        std::vector<std::string> slots() const override;
        void check(const std::vector<Module*> &modules) const override;
        void pre_work(RetroMake *system) override;
        void post_work(RetroMake *system) override;
    };
}