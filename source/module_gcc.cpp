#include "../include/module_gcc.h"
#include "../include/retromake.h"
#include "../include/util.h"

void rm::GCCModule::_delete_compiler_arguments(RetroMake *system)
{
    remove_cmake_define(&system->arguments, "CMAKE_C_COMPILER=");
    remove_cmake_define(&system->arguments, "CMAKE_CXX_COMPILER=");
}

void rm::GCCModule::_delete_compiler_environemnt(RetroMake *system)
{
    auto cc = system->environment.find("CC");
    if (cc != system->environment.cend()) system->environment.erase(cc);
    auto cxx = system->environment.find("CXX");
    if (cxx != system->environment.cend()) system->environment.erase(cxx);
}

rm::Module *rm::GCCModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new GCCModule();

    std::vector<std::string> module_parse = parse(requested_module, '\0');
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "gcc") return new GCCModule();
    if (module_parse.size() == 1 && module_parse[0] == "g++") return new GCCModule();
    return nullptr;
}

rm::GCCModule::GCCModule()
{}

int rm::GCCModule::order() const
{
    return 10;
}

std::string rm::GCCModule::id() const
{
    return "gcc";
}

std::string rm::GCCModule::name() const
{
    return "GCC";
}

std::string rm::GCCModule::help() const
{
    return "GNU compiler";
}

std::vector<std::string> rm::GCCModule::slots() const
{
    return { "compiler" };
}

void rm::GCCModule::check(const RetroMake *system) const
{
    //No additional checks
}

void rm::GCCModule::pre_work(RetroMake *system)
{
    _delete_compiler_arguments(system);
    _delete_compiler_environemnt(system);
    system->arguments.push_back("-DCMAKE_C_COMPILER=gcc");
    system->arguments.push_back("-DCMAKE_CXX_COMPILER=g++");
}

void rm::GCCModule::post_work(const RetroMake *system)
{
    //Do nothing
}