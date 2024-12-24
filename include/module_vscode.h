#pragma once
#include "module.h"

namespace rm
{
    class VSCodeModule : public Module
    {
    public:
        static Module *create_module(const std::string &requested_module);

        VSCodeModule();
        virtual std::string id() const override;
        virtual std::string name() const override;
        virtual std::vector<std::string> slots() const override;
        virtual void check(const std::vector<Module*> &modules) const override;
        virtual void pre_work(RetroMake *system) override;
        virtual void post_work(RetroMake *system) override;
        virtual ~VSCodeModule() override;
    };
}