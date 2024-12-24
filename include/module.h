#pragma once
#include <string>
#include <vector>

namespace rm
{
    class RetroMake;
    
    class Module
    {
    public:
        virtual std::string id() const = 0;
        virtual std::string name() const = 0;
        virtual std::vector<std::string> slots() const = 0;
        virtual void check(const std::vector<Module*> &modules) const = 0;
        virtual void pre_work(RetroMake *system) = 0;
        virtual void post_work(RetroMake *system) = 0;
        virtual ~Module() = 0;
    };
}