#pragma once
#include "module.h"
#include "codemodel.h"
#include "util.h"

namespace rm
{
    class NativeDebugModule : public Module
    {
    private:
        struct Checkout
        {
            const NativeDebugModule &owner;
            JSONAllocator &allocator;
            bool change;
            
            Checkout(const NativeDebugModule &owner, JSONAllocator &allocator);
            void checkout_debugger_args(JSONValue &debugger_args, const Target &target, bool creation);
            void checkout_configuration(JSONValue &configuration, const Target &target, bool creation);
            void checkout_configurations(JSONValue &configurations, const std::vector<Target> &targets);
            void checkout_document(JSONValue &document, const std::vector<Target> &targets);
        };

        bool _lldb;

    public:
        static Module *create_module(const std::string &requested_module);

        NativeDebugModule(bool lldb);
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