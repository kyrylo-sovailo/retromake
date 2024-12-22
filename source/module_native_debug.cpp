#include "../include/module_native_debug.h"
#include "../include/retromake.h"
#include "../include/util.h"

#include <stdexcept>
#include <sys/stat.h>

rm::NativeDebugModule::NativeDebugModule()
{}

std::string rm::NativeDebugModule::id() const
{
    return "nativedebug";
}

std::string rm::NativeDebugModule::name() const
{
    return "Native Debug";
}

std::vector<std::string> rm::NativeDebugModule::slots() const
{
    return { "debug" };
}

bool rm::NativeDebugModule::match(const std::string &module) const
{
    const std::vector<std::string> module_parse = parse(module, false);
    if (module_parse.size() == 1 && lower(module_parse[0]) == "webfreak") return true;
    if (module_parse.size() == 2 && lower(module_parse[0]) == "web" && lower(module_parse[1]) == "freak") return true;
    if (module_parse.size() == 2 && lower(module_parse[0]) == "native" && lower(module_parse[1]) == "debug") return true;
    return false;
}

void rm::NativeDebugModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::NativeDebugModule::pre_work(RetroMake *system)
{
    //Do nothing
}

void rm::NativeDebugModule::post_work(RetroMake *system)
{
    const std::string vscode_directory = system->source_directory + "/.vscode";
    if (!directory_exists(vscode_directory, nullptr))
    {
        if (mkdir(vscode_directory.c_str(), 0700) != 0) throw std::runtime_error("Failed to create directory " + vscode_directory);
    }

    const std::string tasks_file = vscode_directory + "/launch.json";
    const std::string content = 
    "{\n"
    "    \"configurations\":\n"
    "    [\n"
    "        {\n"
    "            \"extension_name\":   \"Native Debug\",\n"
    "            \"name\":             \"GDB Debug (Native Debug)\",\n"
    "            \"type\":             \"gdb\",\n"
    "            \"request\":          \"launch\",\n"
    "            \"preLaunchTask\":    \"debugging-sandbox-make\",\n"
    "\n"
    "            \"target\":           \"" + system->binary_directory + "/main_gcc\",\n"
    "            \"cwd\":              \"" + system->binary_directory + "\",\n"
    "            \"debugger_args\" :   [ \"-iex\", \"set env DEBUG_SANDBOX_TEST=TEST\", \"--args\", \"" + system->binary_directory + "/main_gcc\", \"TEST\" ]\n"
    "        }\n"
    "    ]\n"
    "}\n";
    update_file(tasks_file, content);
}

rm::NativeDebugModule::~NativeDebugModule()
{}