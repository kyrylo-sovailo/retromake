#include "../include/module_clang.h"
#include "../include/retromake.h"
#include "../include/util.h"

rm::Module *rm::ClangModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new ClangModule();

    std::vector<std::string> module_parse = parse(requested_module, false);
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "clang") return new ClangModule();
    if (module_parse.size() == 1 && module_parse[0] == "llvm") return new ClangModule();
    return nullptr;
}

rm::ClangModule::ClangModule()
{}

int rm::ClangModule::order() const
{
    return 20;
}

std::string rm::ClangModule::id() const
{
    return "clang";
}

std::string rm::ClangModule::name() const
{
    return "Clang";
}

std::string rm::ClangModule::help() const
{
    return "LLVM compiler";
}

void rm::ClangModule::pre_work(RetroMake *system)
{
    _delete_compiler_arguments(system);
    _delete_compiler_environemnt(system);
    system->arguments.push_back("-DCMAKE_C_COMPILER=clang");
    system->arguments.push_back("-DCMAKE_CXX_COMPILER=clang++");
}