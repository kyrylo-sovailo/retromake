#pragma once
#include "module.h"

namespace rm
{
    class ClangModule : public Module
    {
    public:
        ClangModule();
        virtual std::string id() const;
        virtual std::string name() const;
        virtual std::vector<std::string> slots() const;
        virtual bool match(const std::string &module) const;
        virtual void check(const std::vector<Module*> &modules) const;
        virtual void pre_work(RetroMake *system);
        virtual void post_work(RetroMake *system);
        virtual ~ClangModule();
    };
}