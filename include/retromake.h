#pragma once
#include "module.h"
#include <string>
#include <vector>
#include <map>
#include <stdexcept>

namespace rm
{
    class RetroMake
    {
    public:
        std::vector<std::string> arguments;
        std::map<std::string, std::string> environment;
        std::string source_directory;
        std::string binary_directory;
        std::vector<std::string> requested_modules;
        std::vector<Module*> modules;
    
    private:
        enum class Mode
        {
            normal,
            passthrough,
            help,
            version,
            cmake_help
        };

        static std::string _find_source_directory(const std::string &cmakecache);
        static std::vector<CreateModule*> _load_module_functions();
        
        Mode _parse_arguments(int argc, char **argv);
        void _parse_environment();
        void _read_configuration();
        void _call_passthrough();
        void _print_help();
        void _print_cmake_help();
        void _print_version();
        void _load_requested_modules();
        void _load_all_modules();
        void _check() const;
        void _pre_work();
        int _work();
        void _post_work();

    public:
        int run(int argc, char **argv);
        ~RetroMake();
    };
}