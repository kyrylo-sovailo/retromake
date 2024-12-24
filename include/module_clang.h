#pragma once
#include "module_gcc.h"

namespace rm
{
    class ClangModule : public GCCModule
    {
    public:
        static Module *create_module(const std::string &requested_module);

        ClangModule();
        int order() const override;
        std::string id() const override;
        std::string name() const override;
        std::string help() const override;
        void pre_work(RetroMake *system) override;
    };
}