#pragma once
#include "module.h"
#include "codemodel.h"
#include "util.h"
#include <map>
#include <set>

namespace rm
{
    class CodeBlocksModule : public Module
    {
    public:
        static Module *create_module(const std::string &requested_module);

        CodeBlocksModule();
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