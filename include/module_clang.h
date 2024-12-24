#pragma once
#include "module.h"

namespace rm
{
    class ClangModule : public Module
    {
    public:
        static Module *create_module(const std::string &requested_module);

        ClangModule();
        std::string id() const override;
        std::string name() const override;
        std::vector<std::string> slots() const override;
        void check(const std::vector<Module*> &modules) const override;
        void pre_work(RetroMake *system) override;
        void post_work(RetroMake *system) override;
        ~ClangModule() override;
    };
}