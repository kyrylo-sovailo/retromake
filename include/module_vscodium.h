#pragma once
#include "module.h"
#include "util.h"

namespace rm
{
    class VSCodiumModule : public Module
    {
    private:
        struct Checkout
        {
            const VSCodiumModule &owner;
            JSONAllocator &allocator;
            bool change;
            
            Checkout(const VSCodiumModule &owner, JSONAllocator &allocator);
            void checkout_args(JSONValue &args);
            void checkout_options(JSONValue &options);
            void checkout_task(JSONValue &task);
            void checkout_tasks(JSONValue &tasks);
            void checkout_document(JSONValue &document);
        };

        RetroMake *_system;
    
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