#include "../include/retromake.h"
#include "../include/util.h"
#include "../include/module_clang.h"
#include "../include/module_codeblocks.h"
#include "../include/module_gcc.h"
#include "../include/module_make.h"
#include "../include/module_native_debug.h"
#include "../include/module_vscode.h"
#include "../include/module_vscodium.h"

#include <dlfcn.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

std::string rm::RetroMake::_find_source_directory(const std::string &cmakecache)
{
    const std::string pattern = "CMAKE_HOME_DIRECTORY:INTERNAL=";
    size_t pattern_match = 0;
    std::string result;
    std::ifstream file(cmakecache, std::ios::binary);
    while (true)
    {
        char c;
        const bool good = static_cast<bool>(file.read(&c, 1));
        if (pattern_match == pattern.size())
        {
            if (!good || c == '\0' || c == '\n' || c == '\r')
            {
                if (result.back() != '/') result.push_back('/');
                return result;
            }
            else result.push_back(c);
        }
        else
        {
            if (!good) return "";
            else if (c == pattern[pattern_match]) pattern_match++;
            else pattern_match = 0;
        }
    }
}

std::vector<rm::CreateModule*> rm::RetroMake::_load_module_functions()
{
    //Built-in modules
    std::vector<CreateModule*> module_functions;
    module_functions.push_back(ClangModule::create_module);
    module_functions.push_back(CodeBlocksModule::create_module);
    module_functions.push_back(GCCModule::create_module);
    module_functions.push_back(MakeModule::create_module);
    module_functions.push_back(NativeDebugModule::create_module);
    module_functions.push_back(VSCodeModule::create_module);
    module_functions.push_back(VSCodiumModule::create_module);

    //External modules
    std::string executable_directory = path_executable();
    path_parent(&executable_directory);
    const size_t executable_directory_size = executable_directory.size();
    Search search(executable_directory);
    while (true)
    {
        bool directory;
        const char *entry = search.get(&directory);
        if (entry == nullptr) break;
        if (directory) continue;
        if (std::strlen(entry) < 4 || std::memcmp(entry + strlen(entry) - 3, ".so", 3) != 0) continue;
        executable_directory.resize(executable_directory_size);
        executable_directory += entry;
        void *handle = dlopen(executable_directory.c_str(), RTLD_NOW);
        if (handle == nullptr) { std::cerr << "RetroMake Warning: failed to open module " << entry << std::endl; continue; }
        void *create_module = dlsym(handle, "create_module");
        if (create_module == nullptr) { std::cerr << "RetroMake Warning: failed to find \"create_module\" in module " << entry << std::endl; continue; }
        std::cerr << "RetroMake Info: loading external module " << entry << std::endl;
        module_functions.push_back(reinterpret_cast<CreateModule*>(create_module));
    }

    return module_functions;
}

rm::RetroMake::Mode rm::RetroMake::_parse_arguments(int argc, char **argv)
{
    //Objective:
    //If arguments contain --open, --build, --install, -E, -P, -N, --help-*, --help <arg> then _mode=normal
    //If arguments contain --help then _mode=help
    //If arguments contain --help-cmake then _mode=passthrough and set arguments
    //else normal mode
    //1. Set arguments
    //2. Set source_directory
    //3. Set binary_directory
    //4. Set requested_modules and remove -G from arguments

    //Detect help or passthrough
    if (argc == 0) throw std::runtime_error("argc is zero");
    arguments = std::vector<std::string>(argv + 1, argv + argc);
    for (auto argument = arguments.cbegin(); argument != arguments.cend(); argument++)
    {
        if (*argument == "--open" || *argument == "--build" || *argument == "--install"
        || *argument == "-E" || *argument == "-P" || *argument == "-N"
        || (*argument == "--help" && argument + 1 != arguments.cend() && *(argument + 1) != "cmake")
        || (argument->find("--help-") == 0 && *argument != "--help-cmake")) return Mode::passthrough;

        if ((*argument == "--help" && argument + 1 != arguments.cend() && *(argument + 1) == "cmake")
        || (*argument == "--help-cmake")) return Mode::cmake_help;
        
        if (*argument == "--help" || *argument == "-help" || *argument == "-usage"
        || *argument == "-h" || *argument == "-H" || *argument == "/?") return Mode::help;
        
        if (*argument == "--version" || *argument == "-version"
        || *argument == "/V") return Mode::version;
    }

    //Parse
    for (auto argument = arguments.begin(); argument != arguments.end(); /*argument++*/)
    {
        if (*argument == "-G")
        {
            argument = arguments.erase(argument);
            if (argument == arguments.cend()) throw std::runtime_error("Expected list of requested modules after -G");
            std::vector<std::string> argument_modules = parse(*argument, ',');
            if (argument_modules.empty()) throw std::runtime_error("Expected list of requested modules after -G");
            for (auto argument_module = argument_modules.cbegin(); argument_module != argument_modules.cend(); argument_module++)
                requested_modules.push_back(trim(*argument_module));
            argument = arguments.erase(argument);
        }
        else if (*argument == "-B")
        {
            argument = arguments.erase(argument);
            if (argument == arguments.cend()) throw std::runtime_error("Expected binary directory after -B");
            if (!binary_directory.empty()) throw std::runtime_error("Multiply binary directories provided");
            if (!path_exists(*argument, true, &binary_directory)) throw std::runtime_error("Directory " + *argument + " does not exist");
            argument = arguments.erase(argument);
        }
        else if (*argument == "-S")
        {
            argument = arguments.erase(argument);
            if (argument == arguments.cend()) throw std::runtime_error("Expected source directory after -S");
            if (!source_directory.empty()) throw std::runtime_error("Multiply source directories provided");
            if (!path_exists(*argument, true, &source_directory)) throw std::runtime_error("Directory " + *argument + " does not exist");
            argument = arguments.erase(argument);
        }
        else if (argument->at(0) != '-')
        {
            std::string argument_directory;
            if (!path_exists(*argument, true, &argument_directory)) throw std::runtime_error("Directory " + *argument + " does not exist");
            const std::string cmakelists_file = argument_directory + "CMakeLists.txt";
            const std::string cmakecache_file = argument_directory + "CMakeCache.txt";
            const bool cmakelists_exists = path_exists(cmakelists_file, false, nullptr);
            const bool cmakecache_exists = path_exists(cmakecache_file, false, nullptr);
            if (!cmakelists_exists && !cmakecache_exists)
                throw std::runtime_error("Could not determine whether provided directory " + *argument + " is source or binary"
                " (contains both CMakeLists.txt and CMakeCache.txt)");
            if (cmakelists_exists && cmakecache_exists)
                throw std::runtime_error("Could not determine whether provided directory " + *argument + " is source or binary"
                " (contains neither CMakeLists.txt nor CMakeCache.txt)");
            if (cmakelists_exists)
            {
                if (!source_directory.empty()) throw std::runtime_error("Multiply source directories provided");
                source_directory = argument_directory;
            }
            else
            {
                if (!binary_directory.empty()) throw std::runtime_error("Multiply binary directories provided");
                binary_directory = argument_directory;
                if (!source_directory.empty()) throw std::runtime_error("Multiply source directories provided");
                source_directory = _find_source_directory(cmakecache_file);
                if (source_directory.empty()) throw std::runtime_error(*argument + "/CMakeCache.txt does not contain source directory");
            }
            argument = arguments.erase(argument);
        }
        else argument++;
    }

    //Did not get binary directory, assume CWD
    if (!source_directory.empty() && binary_directory.empty()) binary_directory = path_working_directory();

    //Failure
    if (source_directory.empty() || binary_directory.empty()) throw std::runtime_error("No source or binary directory provided");

    return Mode::normal;
}

extern char **environ;
void rm::RetroMake::_parse_environment()
{
    for (char **pentry = environ; *pentry != nullptr; pentry++)
    {
        const char *entry = *pentry;
        const char *equal = std::strchr(entry, '=');
        if (equal == nullptr) throw std::runtime_error("invalid environ");
        const std::string name = std::string(entry, (equal - entry));
        const std::string value = std::string(equal + 1);
        if (name.empty()) throw std::runtime_error("invalid environ"); //Value can be empty!
        environment[name] = value;
    }
}

void rm::RetroMake::_read_configuration()
{
    //Check if -G argument was given
    if (!requested_modules.empty()) return;
    
    //Check environment
    auto find = environment.find("RETROMAKE_REQUESTED_MODULES");
    if (find != environment.cend()) { requested_modules = parse(trim(find->second), ','); return; }

    //Check home directory
    find = environment.find("HOME");
    std::string home_directory;
    if (find != environment.cend())
    {
        if (!path_exists(find->second, true, &home_directory)) throw std::runtime_error("Home directory " + find->second + " does not exist");
    }
    else
    {
        home_directory = path_user_directory();
    }
    const std::string config_file = home_directory + ".retromake.conf";
    auto config = parse_config(config_file);
    find = config.find("RETROMAKE_REQUESTED_MODULES");
    if (find != config.cend()) { requested_modules = parse(trim(find->second), ','); return; }

    //Check configuration directory
    config = parse_config("/etc/retromake.conf");
    if (find != config.cend()) { requested_modules = parse(trim(find->second), ','); return; }

    throw std::runtime_error("List of requested modules not provided");
}

void rm::RetroMake::_call_passthrough()
{
    std::vector<std::string> argv{"cmake"};
    argv.insert(argv.end(), arguments.cbegin(), arguments.cend());
    call_no_return(argv, nullptr);
}

const char *help1 =
R"(Usage

  retromake [options] <path-to-source> [-G <requested-modules>]
  retromake [options] <path-to-existing-build> [-G <requested-modules>]
  retromake [options] -S <path-to-source> -B <path-to-build> [-G <requested-modules>]

Specify a source directory to (re-)generate a build system for it in the
current working directory. Specify an existing build directory to
re-generate its build system.

Options
  -G <requested-modules>            = Specify a comma-separated case-invariant list of requested modules.
  -h,-H,--help,-help,-usage,/?      = Print usage information and exit.
  --version,-version,/V [<file>]    = Print version number and exit.
  --help cmake                      = Print cmake's help and exit.
  --help-cmake                      = Print cmake's help and exit.
  
  [ all other are passed through to cmake, see 'cmake --help' ]

Modules

The following built-in modules are available:
)";

const char *help2 =
R"(
Further information on the project's page (github.com/kyrylo-sovailo/retromake).
)";

void rm::RetroMake::_print_help()
{
    _load_all_modules();
    std::cout << help1;
    for (auto module = modules.cbegin(); module != modules.cend(); module++)
    {
        std::cout << "  " << std::setw(32) << std::left << (*module)->name() << "= " << (*module)->help() << '\n';
    }
    std::cout << help2;
}

const char *version =
R"(retromake version 0.0.1

RetroMake is developed by Kyrylo Sovailo (github.com/kyrylo-sovailo/retromake).
)";
void rm::RetroMake::_print_version()
{
    std::cout << version << std::endl;
}

void rm::RetroMake::_print_cmake_help()
{
    std::vector<std::string> argv{"cmake", "--help"};
    call_no_return(argv, nullptr);
}

void rm::RetroMake::_load_requested_modules()
{
    auto module_functions = _load_module_functions();

    for (auto requested_module = requested_modules.cbegin(); requested_module != requested_modules.cend(); requested_module++)
    {
        std::unique_ptr<Module> matched_module;
        for (auto module_function = module_functions.cbegin(); module_function != module_functions.cend(); module_function++)
        {
            std::unique_ptr<Module> module = std::unique_ptr<Module>((*module_function)(*requested_module));
            if (module == nullptr) {}
            else if (matched_module == nullptr) matched_module = std::move(module);
            else throw std::runtime_error(
                "Module \"" + matched_module->name() + "\""
                " and module \"" + module->name() + "\""
                " are both matched by string \"" + *requested_module + "\"");
        }
        if (matched_module == nullptr) throw std::runtime_error(
            "String \"" + *requested_module + "\" is not matched by any module");
        modules.push_back(matched_module.release());
    }
    std::sort(modules.begin(), modules.end(), [](Module *a, Module *b){ return a->order() < b->order(); });
}

void rm::RetroMake::_load_all_modules()
{
    auto module_functions = _load_module_functions();
    for (auto module_function = module_functions.cbegin(); module_function != module_functions.cend(); module_function++)
    {
        modules.push_back((*module_function)(""));
    }
    std::sort(modules.begin(), modules.end(), [](Module *a, Module *b){ return a->order() < b->order(); });
}

void rm::RetroMake::_check() const
{
    std::map<std::string, const Module*> occupied_slots;
    for (auto module = modules.cbegin(); module != modules.cend(); module++)
    {
        const std::vector<std::string> slots = (*module)->slots();
        for (auto slot = slots.cbegin(); slot != slots.cend(); slot++)
        {
            auto find = occupied_slots.find(*slot);
            if (find != occupied_slots.cend()) throw std::runtime_error("Module \"" + find->second->name() + "\""
                " conflicts with module \"" + (*module)->name() + "\""
                " because they both occupy slot \"" + *slot + "\"");
            occupied_slots.insert({*slot, *module});
        }
    }

    if (occupied_slots.find("compiler") == occupied_slots.cend()) throw std::runtime_error("No module occupies the \"compiler\" slot");
    if (occupied_slots.find("build") == occupied_slots.cend()) throw std::runtime_error("No module occupies the \"build\" slot");

    for (auto module = modules.cbegin(); module != modules.cend(); module++) (*module)->check(this);
}

void rm::RetroMake::_pre_work()
{
    for (auto module = modules.cbegin(); module != modules.cend(); module++) (*module)->pre_work(this);
}

int rm::RetroMake::_work()
{
    std::vector<std::string> arguments_copy{"cmake", "-B", binary_directory, "-S", source_directory};
    arguments_copy.insert(arguments_copy.end(), arguments.cbegin(), arguments.cend());
    return call_wait(arguments_copy, &environment);
}

void rm::RetroMake::_post_work()
{
    for (auto module = modules.cbegin(); module != modules.cend(); module++) (*module)->post_work(this);
}

int rm::RetroMake::run(int argc, char **argv)
{
    Mode mode = _parse_arguments(argc, argv);
    int status;
    switch (mode)
    {
    case Mode::normal:
        _parse_environment();
        _read_configuration();
        _load_requested_modules();
        _check();
        _pre_work();
        status = _work();
        if (status != 0) return status;
        _post_work();
        break;
    case Mode::passthrough:
        _call_passthrough();
        return 1; //Never happens
    case Mode::help:
        _print_help();
        break;
    case Mode::version:
        _print_version();
        break;
    case Mode::cmake_help:
        _print_cmake_help();
        return 1; //Never happens
    }
    return 0;
}

rm::RetroMake::~RetroMake()
{
    for (auto module = modules.begin(); module != modules.end(); module++) delete (*module);
}

int _main(int argc, char **argv)
{
    try
    {
        rm::RetroMake retromake;
        return retromake.run(argc, argv);
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << "RetroMake Error: " << e.what() << std::endl;
        return 1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "RetroMake Developer Error: " << e.what() << std::endl;
        return 2;
    }
}

int main(int argc, char **argv)
{
    return _main(argc, argv);
}