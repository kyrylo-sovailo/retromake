#include "../include/module_make.h"
#include "../include/retromake.h"
#include "../include/util.h"
#include <stdexcept>

rm::MakeModule::MakeModule()
{}

std::string rm::MakeModule::id() const
{
    return "make";
}

std::string rm::MakeModule::name() const
{
    return "Make";
}

std::vector<std::string> rm::MakeModule::slots() const
{
    return { "build" };
}

bool rm::MakeModule::match(const std::string &module) const
{
    const std::vector<std::string> module_parse = parse(module, false);
    if (module_parse.size() == 1 && lower(module_parse[0]) == "make") return true;
    if (module_parse.size() == 1 && lower(module_parse[0]) == "makefile") return true;
    if (module_parse.size() == 1 && lower(module_parse[0]) == "makefiles") return true;
    if (module_parse.size() == 2 && lower(module_parse[0]) == "unix" && lower(module_parse[1]) == "makefile") return true;
    if (module_parse.size() == 2 && lower(module_parse[0]) == "unix" && lower(module_parse[1]) == "makefiles") return true;
    return false;
}

void rm::MakeModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::MakeModule::pre_work(RetroMake *system)
{
    system->arguments.push_back("-G");
    system->arguments.push_back("'Unix Makefiles'");
}

void rm::MakeModule::post_work(RetroMake *system)
{
    //Do nothing
}

rm::MakeModule::~MakeModule()
{}