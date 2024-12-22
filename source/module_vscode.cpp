#include "../include/module_vscode.h"
#include "../include/retromake.h"
#include "../include/util.h"

#include <stdexcept>
#include <sys/stat.h>

rm::VSCodeModule::VSCodeModule()
{}

std::string rm::VSCodeModule::id() const
{
    return "vscode";
}

std::string rm::VSCodeModule::name() const
{
    return "VS Code";
}

std::vector<std::string> rm::VSCodeModule::slots() const
{
    return { "editor" };
}

bool rm::VSCodeModule::match(const std::string &module) const
{
    const std::vector<std::string> module_parse = parse(module, false);
    if (module_parse.size() == 1 && lower(module_parse[0]) == "vscode") return true;
    if (module_parse.size() == 1 && lower(module_parse[0]) == "code") return true;
    if (module_parse.size() == 2 && lower(module_parse[0]) == "vs" && lower(module_parse[1]) == "code") return true;
    if (module_parse.size() == 3 && lower(module_parse[0]) == "visual" && lower(module_parse[1]) == "studio" && lower(module_parse[2]) == "code") return true;
    return false;
}

void rm::VSCodeModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::VSCodeModule::pre_work(RetroMake *system)
{
    //Do nothing
}

void rm::VSCodeModule::post_work(RetroMake *system)
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

rm::VSCodeModule::~VSCodeModule()
{}