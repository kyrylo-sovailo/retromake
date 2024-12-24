#include "../include/module_native_debug.h"
#include "../include/retromake.h"
#include "../include/util.h"

#include <sys/stat.h>

#include <stdexcept>

rm::Module *rm::NativeDebugModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new NativeDebugModule();

    std::vector<std::string> module_parse = parse(requested_module, false);
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "webfreak") return new NativeDebugModule();
    if (module_parse.size() == 2 && module_parse[0] == "web" && module_parse[1] == "freak") return new NativeDebugModule();
    if (module_parse.size() == 2 && module_parse[0] == "native" && module_parse[1] == "debug") return new NativeDebugModule();
    return nullptr;
}

rm::NativeDebugModule::NativeDebugModule()
{}

int rm::NativeDebugModule::order() const
{
    return 80;
}

std::string rm::NativeDebugModule::id() const
{
    return "nativedebug";
}

std::string rm::NativeDebugModule::name() const
{
    return "Native Debug";
}

std::string rm::NativeDebugModule::help() const
{
    return "WebFreak's debugger for VS Code/Codium";
}

std::vector<std::string> rm::NativeDebugModule::slots() const
{
    return { "debug" };
}

void rm::NativeDebugModule::check(const std::vector<Module*> &modules) const
{
    for (auto module = modules.cbegin(); module != modules.cend(); module++)
    {
        if ((*module)->id() == "vscodium" || (*module)->id() == "vscode") return;
    }
    throw std::runtime_error("Module \"Native Debug\" depends on either \"VS Codium\" or \"VS Code\"");
}

void rm::NativeDebugModule::pre_work(RetroMake *system)
{
    //Do nothing
}

void rm::NativeDebugModule::post_work(RetroMake *system)
{
    const std::string vscode_directory = system->source_directory + ".vscode/";
    if (!directory_exists(vscode_directory, nullptr))
    {
        if (mkdir(vscode_directory.c_str(), 0700) != 0) throw std::runtime_error("Failed to create directory " + vscode_directory);
    }

    const std::string tasks_file = vscode_directory + "launch.json";
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
    //update_file(tasks_file, content);
}