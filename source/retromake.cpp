#include "../include/retromake.h"
#include "../include/util.h"
#include "../include/module_clang.h"
#include "../include/module_gcc.h"
#include "../include/module_make.h"
#include "../include/module_native_debug.h"
#include "../include/module_vscode.h"
#include "../include/module_vscodium.h"

#include <dirent.h>
#include <dlfcn.h>
#include <limits.h>
#include <pwd.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <fstream>
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
            if (!good || c == '\0' || c == '\n' || c == '\r') return result;
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
    //4. Set _requested_modules and remove -G from arguments

    //Detect help or passthrough
    if (argc == 0) throw std::runtime_error("argc is zero");
    arguments = std::vector<std::string>(argv + 1, argv + argc);
    for (auto argument = arguments.cbegin(); argument != arguments.cend(); argument++)
    {
        if (*argument == "--open" || *argument == "--build" || *argument == "--install"
        || *argument == "-E" || *argument == "-P" || *argument == "-N"
        || (*argument == "--help" && argument + 1 != arguments.cend())
        || (argument->find("--help-") == 0)) return Mode::passthrough;
        
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
            std::vector<std::string> requested_modules = parse(*argument, true);
            if (requested_modules.empty()) throw std::runtime_error("Expected list of requested modules after -G");
            for (auto requested_module = requested_modules.cbegin(); requested_module != requested_modules.cend(); requested_module++)
                _requested_modules.push_back(trim(*requested_module));
            argument = arguments.erase(argument);
        }
        else if (*argument == "-B")
        {
            argument = arguments.erase(argument);
            if (argument == arguments.cend()) throw std::runtime_error("Expected binary directory after -B");
            if (!binary_directory.empty()) throw std::runtime_error("Multiply binary directories provided");
            if (!directory_exists(*argument, &binary_directory)) throw std::runtime_error("Directory " + *argument + " does not exist");
            argument = arguments.erase(argument);
        }
        else if (*argument == "-S")
        {
            argument = arguments.erase(argument);
            if (argument == arguments.cend()) throw std::runtime_error("Expected source directory after -S");
            if (!source_directory.empty()) throw std::runtime_error("Multiply source directories provided");
            if (!directory_exists(*argument, &source_directory)) throw std::runtime_error("Directory " + *argument + " does not exist");
            argument = arguments.erase(argument);
        }
        else if (argument->at(0) != '-')
        {
            std::string argument_directory;
            if (!directory_exists(*argument, &argument_directory)) throw std::runtime_error("Directory " + *argument + " does not exist");
            const std::string cmakelists_directory = argument_directory + "/CMakeLists.txt";
            const std::string cmakecache_directory = argument_directory + "/CMakeCache.txt";
            const bool cmakelists_exists = file_exists(cmakelists_directory, nullptr);
            const bool cmakecache_exists = file_exists(cmakecache_directory, nullptr);
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
                source_directory = _find_source_directory(cmakecache_directory);
                if (source_directory.empty()) throw std::runtime_error(*argument + "/CMakeCache.txt does not contain source directory");
            }
            argument = arguments.erase(argument);
        }
        else argument++;
    }

    //Did not get binary directory, assume CWD
    if (!source_directory.empty() && binary_directory.empty())
    {
        binary_directory.resize(256);
        while (true)
        {
            char *cwd = getcwd(&binary_directory[0], binary_directory.size());
            if (cwd == nullptr) binary_directory.resize(binary_directory.size() * 2);
            else { binary_directory.resize(strlen(binary_directory.c_str())); break; }
        }
    }
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
    if (!_requested_modules.empty()) return;
    
    //Check environment
    auto find = environment.find("RETROMAKE_REQUESTED_MODULES");
    if (find != environment.cend()) { _requested_modules = parse(trim(find->second), true); return; }

    //Check home directory
    find = environment.find("HOME");
    std::string home;
    if (find != environment.cend()) home = find->second;
    else
    {
        struct passwd *user = getpwuid(getuid());
        if (user == nullptr) throw std::runtime_error("getpwuid() failed");
        home = user->pw_dir;
    }
    if (home.empty()) throw std::runtime_error("Invalid home directory");
    else if (home.back() != '/') home += '/';
    std::string config_path = home + ".retromake.conf";
    auto config = parse_config(config_path);
    find = config.find("RETROMAKE_REQUESTED_MODULES");
    if (find != config.cend()) { _requested_modules = parse(trim(find->second), true); return; }

    //Check configuration directory
    config = parse_config("/etc/retromake.conf");
    if (find != config.cend()) { _requested_modules = parse(trim(find->second), true); return; }

    throw std::runtime_error("List of requested modules not provided");
}

void rm::RetroMake::_call_passthrough()
{
    std::vector<char*> argv; argv.reserve(arguments.size());
    for (auto argument = arguments.begin(); argument != arguments.end(); argument++)
        argv.push_back(&argument->at(0));
    execv("cmake", argv.data());
    throw std::runtime_error("execv() failed"); //Never happens
}

void rm::RetroMake::_print_help()
{
    std::cout << "RetroMake help" << std::endl;
}

void rm::RetroMake::_print_version()
{
    std::cout << "RetroMake version -1" << std::endl;
}

void rm::RetroMake::_load_modules()
{
    typedef Module* (CreateModule)(const std::string &requested_module);
    
    //Built-in modules
    std::vector<CreateModule*> module_functions;
    module_functions.push_back(ClangModule::create_module);
    module_functions.push_back(GCCModule::create_module);
    module_functions.push_back(MakeModule::create_module);
    module_functions.push_back(NativeDebugModule::create_module);
    module_functions.push_back(VSCodeModule::create_module);
    module_functions.push_back(VSCodiumModule::create_module);

    //External modules
    std::string executable_directory(256, '\0');
    while (true)
    {
        int read = readlink("/proc/self/exe", &executable_directory[0], executable_directory.size());
        if (read < 0) throw std::runtime_error("readlink(\"/proc/self/exe\") failed");
        else if (read == executable_directory.size()) executable_directory.resize(executable_directory.size() * 2);
        else { executable_directory.resize(read); break; }
    }
    auto slash = executable_directory.rfind('/');
    if (slash == std::string::npos) throw std::runtime_error("readlink(\"/proc/self/exe\") returned unexpected path");
    executable_directory.resize(slash + 1);
    DIR *directory = opendir(executable_directory.c_str());
    if (directory == nullptr) throw std::runtime_error("opendir() failed");
    while (true)
    {
        struct dirent *entry = readdir(directory);
        if (entry == nullptr) break;
        if (entry->d_type != DT_REG) continue;
        if (strlen(entry->d_name) < 4 || std::memcmp(entry->d_name + strlen(entry->d_name) - 3, ".so", 3) != 0) continue;
        executable_directory.resize(slash + 1);
        executable_directory += entry->d_name;
        void *handle = dlopen(executable_directory.c_str(), RTLD_NOW);
        if (handle == nullptr) { std::cerr << "RetroMake Warning: failed to open module " << entry->d_name << std::endl; continue; }
        void *create_module = dlsym(handle, "create_module");
        if (create_module == nullptr) { std::cerr << "RetroMake Warning: failed to find \"create_module\" in module " << entry->d_name << std::endl; continue; }
        std::cerr << "RetroMake Info: loading external module " << entry->d_name << std::endl;
        module_functions.push_back(reinterpret_cast<CreateModule*>(create_module));
    }
    closedir(directory);

    //Module matching
    for (auto requested_module = _requested_modules.cbegin(); requested_module != _requested_modules.cend(); requested_module++)
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
        _modules.push_back(matched_module.release());
    }
}

void rm::RetroMake::_check() const
{
    std::map<std::string, const Module*> occupied_slots;
    for (auto module = _modules.cbegin(); module != _modules.cend(); module++)
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

    for (auto module = _modules.cbegin(); module != _modules.cend(); module++) (*module)->check(_modules);
}

void rm::RetroMake::_pre_work()
{
    for (auto module = _modules.cbegin(); module != _modules.cend(); module++) (*module)->pre_work(this);
}

int rm::RetroMake::_work()
{
    const pid_t pid = fork();
    if (pid < 0) throw std::runtime_error("fork() failed");
    else if (pid == 0)
    {
        //I am child
        std::vector<std::string> prepand({"cmake", "-B", binary_directory, "-S", source_directory});
        arguments.insert(arguments.begin(), prepand.cbegin(), prepand.cend());
        std::vector<char*> argv; argv.reserve(arguments.size() + 1);
        for (auto argument = arguments.begin(); argument != arguments.end(); argument++)
            argv.push_back(&argument->at(0));
        argv.push_back(nullptr);

        std::vector<std::string> envp_mem; envp_mem.reserve(environment.size() + 1);
        std::vector<char*> envp; envp.reserve(environment.size());
        for (auto entry = environment.cbegin(); entry != environment.cend(); entry++)
        {
            envp_mem.push_back(entry->first + "=" + entry->second);
            envp.push_back(&envp_mem.back().at(0));
        }
        envp.push_back(nullptr);

        execvpe("cmake", argv.data(), envp.data());
        throw std::runtime_error("execvpe() failed"); //Never happens
    }
    else
    {
        //I am parent
        int status;
        if (wait(&status) < 0) throw std::runtime_error("wait() failed");
        if (!WIFEXITED(status)) throw std::runtime_error("wait() did not wait till child termination");
        return WEXITSTATUS(status);
    }
}

void rm::RetroMake::_post_work()
{
    for (auto module = _modules.cbegin(); module != _modules.cend(); module++) (*module)->post_work(this);
}

int rm::RetroMake::run(int argc, char **argv)
{
    Mode mode = _parse_arguments(argc, argv);
    if (mode == Mode::normal)
    {
        _parse_environment();
        _read_configuration();
        _load_modules();
        _check();
        _pre_work();
        int status = _work();
        if (status != 0) return status;
        _post_work();
    }
    else if (mode == Mode::passthrough)
    {
        _call_passthrough();
        return 1; //Never happens
    }
    else if (mode == Mode::help)
    {
        _print_help();
    }
    else
    {
        _print_version();
    }
    return 0;
}

rm::RetroMake::~RetroMake()
{
    for (auto module = _modules.begin(); module != _modules.end(); module++) delete (*module);
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