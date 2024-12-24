#pragma once
#include "module_vscodium.h"

namespace rm
{
    class VSCodeModule : public VSCodiumModule
    {
    public:
        static Module *create_module(const std::string &requested_module);

        VSCodeModule();
        int order() const override;
        std::string id() const override;
        std::string name() const override;
        std::string help() const override;
    };
}