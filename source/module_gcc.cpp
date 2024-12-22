#include "../include/module_gcc.h"
#include "../include/retromake.h"
#include "../include/util.h"
#include <stdexcept>

rm::GCCModule::GCCModule()
{}

std::string rm::GCCModule::id() const
{
    return "gcc";
}

std::string rm::GCCModule::name() const
{
    return "GCC";
}

std::vector<std::string> rm::GCCModule::slots() const
{
    return { "compiler" };
}

bool rm::GCCModule::match(const std::string &module) const
{
    const std::vector<std::string> module_parse = parse(module, false);
    if (module_parse.size() == 1 && lower(trim(module_parse[0])) == "gcc") return true;
    if (module_parse.size() == 1 && lower(trim(module_parse[0])) == "g++") return true;
    return false;
}

void rm::GCCModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::GCCModule::pre_work(RetroMake *system)
{
    auto cc = system->environment.find("CC");
    if (cc != system->environment.cend()) system->environment.erase(cc);
    auto cxx = system->environment.find("CXX");
    if (cxx != system->environment.cend()) system->environment.erase(cxx);
    for (size_t i = 0; i < system->arguments.size();)
    {
        if (i + 1 < system->arguments.size() && system->arguments[i] == "-D"
        && (system->arguments[i].find("CMAKE_C_COMPILER=") == 0 || system->arguments[i].find("CMAKE_CXX_COMPILER=") == 0))
        {
            system->arguments.erase(system->arguments.begin() + i);
            system->arguments.erase(system->arguments.begin() + i);
        }
        else if (system->arguments[i].find("-DCMAKE_C_COMPILER=") == 0 && system->arguments[i].find("-DCMAKE_CXX_COMPILER=") == 0)
        {
            system->arguments.erase(system->arguments.begin() + i);
        }
        else i++;
    }
    system->arguments.push_back("-DCMAKE_C_COMPILER=gcc");
    system->arguments.push_back("-DCMAKE_CXX_COMPILER=g++");
}

void rm::GCCModule::post_work(RetroMake *system)
{
    //Do nothing
}

rm::GCCModule::~GCCModule()
{}