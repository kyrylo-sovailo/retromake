#pragma once
#include <map>
#include <set>
#include <string>
#include <vector>

namespace rm
{
    struct Project
    {
        std::string name;
    };
    struct Target
    {
        //General
        std::string typ;
        std::string name;
        std::string path;
        std::string language;
        std::set<std::string> dependencies;

        //Compiler
        std::set<std::string> options;
        std::set<std::string> directories;
        std::set<std::string> sources;
        std::map<std::string, std::string> defines;

        //Linker
        std::set<std::string> linker_options;
        std::set<std::string> linker_sources;
    };
    void codemodel_request(const std::string &binary_directory);
    std::vector<Target> codemodel_parse(const std::string &binary_directory, const std::string &source_directory, Project *project, bool full);
}