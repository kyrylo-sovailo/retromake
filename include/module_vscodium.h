#pragma once
#include "module.h"

namespace rapidjson
{
    class Value;
    class CrtAllocator;
    template <typename BaseAllocator = CrtAllocator> class MemoryPoolAllocator;
}

namespace rm
{
    class VSCodiumModule : public Module
    {
    private:
        bool _checkout_string(rapidjson::Value &string, rapidjson::MemoryPoolAllocator<> &allocator, const char *value);
        bool _checkout_args(rapidjson::Value &args, rapidjson::MemoryPoolAllocator<> &allocator);
    
    public:
        static Module *create_module(const std::string &requested_module);

        VSCodiumModule();
        std::string id() const override;
        std::string name() const override;
        std::vector<std::string> slots() const override;
        void check(const std::vector<Module*> &modules) const override;
        void pre_work(RetroMake *system) override;
        void post_work(RetroMake *system) override;
        ~VSCodiumModule() override;
    };
}