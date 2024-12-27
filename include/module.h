#pragma once
#include <string>
#include <vector>

namespace rm
{
    class RetroMake;
    
    class Module
    {
    public:
        virtual int order() const = 0;
        virtual std::string id() const = 0;
        virtual std::string name() const = 0;
        virtual std::string help() const = 0;
        virtual std::vector<std::string> slots() const = 0;
        virtual void check(const RetroMake *system) const = 0;
        virtual void pre_work(RetroMake *system) = 0;
        virtual void post_work(const RetroMake *system) = 0;
        virtual ~Module() = 0;
    };

    typedef Module* (CreateModule)(const std::string &requested_module);
}