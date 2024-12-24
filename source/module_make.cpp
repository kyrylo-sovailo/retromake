#include "../include/module_make.h"
#include "../include/retromake.h"
#include "../include/util.h"

rm::Module *rm::MakeModule::create_module(const std::string &requested_module)
{
    std::vector<std::string> module_parse = parse(requested_module, false);
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

void rm::MakeModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::MakeModule::pre_work(RetroMake *system)
{
    system->arguments.push_back("-G");
    system->arguments.push_back("Unix Makefiles");
}

void rm::MakeModule::post_work(RetroMake *system)
{
    //Do nothing
}

rm::MakeModule::~MakeModule()
{}