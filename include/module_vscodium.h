#pragma once
#include "module.h"
#include "util.h"

namespace rm
{
    class VSCodiumModule : public Module
    {
    private:
        RetroMake *_system = nullptr;
        bool _read_document(JSONDocument &document) const;
        bool _checkout_string(JSONValue &string, JSONAllocator &allocator, const char *value) const;
        bool _checkout_args(JSONValue &args, JSONAllocator &allocator) const;
        bool _checkout_options(JSONValue &options, JSONAllocator &allocator) const;
        bool _checkout_task(JSONValue &task, JSONAllocator &allocator) const;
        bool _checkout_tasks(JSONValue &tasks, JSONAllocator &allocator) const;
        bool _checkout_document(JSONValue &document, JSONAllocator &allocator) const;
    
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