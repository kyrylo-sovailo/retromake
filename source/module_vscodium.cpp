#include "../include/module_vscodium.h"
#include "../include/retromake.h"
#include "../include/util.h"

#include <sys/stat.h>

#include <stdexcept>

rm::Module *rm::VSCodiumModule::create_module(const std::string &requested_module)
{
    std::vector<std::string> module_parse = parse(requested_module, false);
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "vscodium") return new VSCodiumModule();
    if (module_parse.size() == 1 && module_parse[0] == "codium") return new VSCodiumModule();
    if (module_parse.size() == 2 && module_parse[0] == "vs" && module_parse[1] == "codium") return new VSCodiumModule();
    if (module_parse.size() == 2 && module_parse[0] == "vscode" && module_parse[1] == "oss") return new VSCodiumModule();
    if (module_parse.size() == 3 && module_parse[0] == "vs" && module_parse[1] == "code" && module_parse[2] == "oss") return new VSCodiumModule();
    return nullptr;
}

rm::VSCodiumModule::VSCodiumModule()
{}

std::string rm::VSCodiumModule::id() const
{
    return "vscodium";
}

std::string rm::VSCodiumModule::name() const
{
    return "VS Codium";
}

std::vector<std::string> rm::VSCodiumModule::slots() const
{
    return { "editor" };
}

void rm::VSCodiumModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::VSCodiumModule::pre_work(RetroMake *system)
{
    //Do nothing
}

void rm::VSCodiumModule::post_work(RetroMake *system)
{
    const std::string vscode_directory = system->source_directory + "/.vscode";
    if (!directory_exists(vscode_directory, nullptr))
    {
        if (mkdir(vscode_directory.c_str(), 0700) != 0) throw std::runtime_error("Failed to create directory " + vscode_directory);
    }

    const std::string tasks_file = vscode_directory + "/tasks.json";
    const std::string content = 
    "{\n"
    "    \"tasks\":\n"
    "    [\n"
    "        {\n"
    "            \"label\": \"retromake-build-task\",\n"
    "            \"type\": \"shell\",\n"
    "            \"command\": \"/bin/sh\",\n"
    "            \"args\": [ \"-c\", \"cmake --build .\" ],\n"
    "            \"options\": { \"cwd\": \"" + system->binary_directory + "\" }\n"
    "        }"
    "    ]"
    "}";
    update_file(tasks_file, content);
}

rm::VSCodiumModule::~VSCodiumModule()
{}