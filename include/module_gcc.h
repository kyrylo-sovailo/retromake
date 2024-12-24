#pragma once
#include "module.h"

namespace rm
{
    class GCCModule : public Module
    {
    protected:
        void _delete_compiler_arguments(RetroMake *system);
        void _delete_compiler_environemnt(RetroMake *system);
    
    public:
        static Module *create_module(const std::string &requested_module);
        
        GCCModule();
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