#pragma once
#include "module.h"
#include "json.h"
#include "retromake.h"
#include "util.h"

namespace rm
{
    class VSCodiumModule : public Module
    {
    private:
        struct Checkout : JSONEditor
        {
            const RetroMake &owner;
            Checkout(const RetroMake &owner);

            void checkout_args(JSONValue &args_node);
            void checkout_options(JSONValue &options_node);
            void checkout_task(JSONValue &task_node);
            void checkout_tasks(JSONValue &tasks_node);
            void checkout_document();
        };
    
    public:
        static Module *create_module(const std::string &requested_module);

        VSCodiumModule();
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