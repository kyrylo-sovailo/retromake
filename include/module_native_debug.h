#pragma once
#include "module.h"
#include "codemodel.h"
#include "json.h"
#include "util.h"

namespace rm
{
    class NativeDebugModule : public Module
    {
    private:
        struct Checkout : JSONEditor
        {
            const NativeDebugModule &owner;
            Checkout(const NativeDebugModule &owner);
            
            void checkout_debugger_args(JSONValue &debugger_args_node, const Target &target);
            void checkout_configuration(JSONValue &configuration_node, const Target &target, bool creation);
            void checkout_configurations(JSONValue &configurations_node, const std::vector<Target> &targets);
            void checkout_document(const std::vector<Target> &targets);
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
        void check(const RetroMake *system) const override;
        void pre_work(RetroMake *system) override;
        void post_work(const RetroMake *system) override;
    };
}