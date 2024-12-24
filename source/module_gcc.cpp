#include "../include/module_gcc.h"
#include "../include/retromake.h"
#include "../include/util.h"

rm::Module *rm::GCCModule::create_module(const std::string &requested_module)
{
    std::vector<std::string> module_parse = parse(requested_module, false);
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "gcc") return new GCCModule();
    if (module_parse.size() == 1 && module_parse[0] == "g++") return new GCCModule();
    return nullptr;
}

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