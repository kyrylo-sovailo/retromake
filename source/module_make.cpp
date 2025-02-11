#include "../include/module_make.h"
#include "../include/retromake.h"
#include "../include/util.h"

rm::Module *rm::MakeModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new MakeModule();

    std::vector<std::string> module_parse = parse(requested_module, '\0');
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "make") return new MakeModule();
    if (module_parse.size() == 1 && module_parse[0] == "makefile") return new MakeModule();
    if (module_parse.size() == 1 && module_parse[0] == "makefiles") return new MakeModule();
    if (module_parse.size() == 2 && module_parse[0] == "unix" && module_parse[1] == "makefile") return new MakeModule();
    if (module_parse.size() == 2 && module_parse[0] == "unix" && module_parse[1] == "makefiles") return new MakeModule();
    return nullptr;
}

rm::MakeModule::MakeModule()
{}

int rm::MakeModule::order() const
{
    return 30;
}

std::string rm::MakeModule::id() const
{
    return "make";
}

std::string rm::MakeModule::name() const
{
    return "Make";
}

std::string rm::MakeModule::help() const
{
    return "Unix Makefile build system";
}

std::vector<std::string> rm::MakeModule::slots() const
{
    return { "build" };
}

void rm::MakeModule::check(const RetroMake *system) const
{
    //No additional checks
}

void rm::MakeModule::pre_work(RetroMake *system)
{
    system->arguments.push_back("-G");
    system->arguments.push_back("Unix Makefiles");
}

void rm::MakeModule::post_work(const RetroMake *system)
{
    //Do nothing
}