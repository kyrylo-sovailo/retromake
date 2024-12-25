#pragma once
#include "module.h"
#include "util.h"

namespace rm
{
    class NativeDebugModule : public Module
    {
    private:
        struct Target
        {
            std::string name;
            std::string path;
        };

        RetroMake *_system = nullptr;
        std::string _get_relative_path(const std::string &absolute) const;
        bool _read_document(JSONDocument &document) const;
        std::vector<Target> _get_executable_targets() const;
        bool _checkout_string(JSONValue &string, JSONAllocator &allocator, const char *value) const;
        bool _checkout_debugger_args(JSONValue &debugger_args, JSONAllocator &allocator, const Target &target, bool creation) const;
        bool _checkout_configuration(JSONValue &configuration, JSONAllocator &allocator, const Target &target, bool creation) const;
        bool _checkout_configurations(JSONValue &configurations, JSONAllocator &allocator, const std::vector<Target> &targets) const;
        bool _checkout_document(JSONValue &document, JSONAllocator &allocator, const std::vector<Target> &targets) const;

    public:
        static Module *create_module(const std::string &requested_module);

        NativeDebugModule();
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