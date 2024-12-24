#include "../include/module_vscode.h"
#include "../include/util.h"

#include <sys/stat.h>

rm::Module *rm::VSCodeModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new VSCodeModule();

    std::vector<std::string> module_parse = parse(requested_module, false);
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "vscode") return new VSCodeModule();
    if (module_parse.size() == 1 && module_parse[0] == "code") return new VSCodeModule();
    if (module_parse.size() == 2 && module_parse[0] == "vs" && module_parse[1] == "code") return new VSCodeModule();
    if (module_parse.size() == 3 && module_parse[0] == "visual" && module_parse[1] == "studio" && module_parse[2] == "code") return new VSCodeModule();
    return nullptr;
}

rm::VSCodeModule::VSCodeModule()
{}

int rm::VSCodeModule::order() const
{
    return 50;
}

std::string rm::VSCodeModule::id() const
{
    return "vscode";
}

std::string rm::VSCodeModule::name() const
{
    return "VS Code";
}

std::string rm::VSCodeModule::help() const
{
    return "Visual Studio Code editor";
}