#include "../include/module_codeblocks.h"
#include "../include/retromake.h"
#include "../include/xml.h"

#include <rapidxml/rapidxml.hpp>
namespace rapidxml { namespace internal {
template <class OutIt, class Ch> inline OutIt print_children(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_attributes(OutIt, const xml_node<Ch>*, int);
template <class OutIt, class Ch> inline OutIt print_data_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_cdata_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_element_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_declaration_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_comment_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_doctype_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_pi_node(OutIt, const xml_node<Ch>*, int, int);
} }
#include <rapidxml/rapidxml_print.hpp>

#include <cstring>
#include <iostream>

rm::CodeBlocksModule::Checkout::Checkout(const CodeBlocksModule &owner, const std::string &compiler) : owner(owner), compiler(compiler) {}

void rm::CodeBlocksModule::Checkout::checkout_target(XMLNode &target_node, const Target &target)
{
    //Option output
    std::string directory = target.path; path_parent(&directory);
    std::string basename = path_base(target.path);
    if ((target.typ == "STATIC_LIBRARY" || target.typ == "SHARED_LIBRARY") && begins_with(basename, "lib"))
        basename.erase(0, 3);
    
    XMLNode &option_output_node = create_node(target_node, "Option", "output");
    const std::string option_output = directory + basename;
    checkout_attribute(option_output_node, "output", option_output);
    if (target.typ == "SHARED_LIBRARY")
    {
        checkout_attribute(option_output_node, "imp_lib", "$(TARGET_OUTPUT_DIR)$(TARGET_OUTPUT_BASENAME).a");
        checkout_attribute(option_output_node, "def_file", "$(TARGET_OUTPUT_DIR)$(TARGET_OUTPUT_BASENAME).def");
    }
    checkout_attribute(option_output_node, "prefix_auto", "1");
    checkout_attribute(option_output_node, "extension_auto", "1");

    //Option object_output
    XMLNode &option_object_output_node = create_node(target_node, "Option", "object_output");
    std::string option_object_output = directory;
    size_t bin = option_object_output.rfind("bin/"); if (bin != std::string::npos) option_object_output.replace(bin, 4, "obj/"); //TODO: INCORRECT!
    checkout_attribute(option_object_output_node, "object_output", option_object_output);

    //Option type
    if (target.typ == "EXECUTABLE") checkout_attribute(create_node(target_node, "Option", "type"), "type", "1");
    else if (target.typ == "STATIC_LIBRARY") checkout_attribute(create_node(target_node, "Option", "type"), "type", "2");
    else if (target.typ == "SHARED_LIBRARY") checkout_attribute(create_node(target_node, "Option", "type"), "type", "3");

    //Option compiler
    checkout_attribute(create_node(target_node, "Option", "compiler"), "compiler", compiler);

    //Option createDefFile & Option createStaticLib
    if (target.typ == "SHARED_LIBRARY")
    {
        checkout_attribute(create_node(target_node, "Option", "createDefFile"), "createDefFile", "1");
        checkout_attribute(create_node(target_node, "Option", "createStaticLib"), "createStaticLib", "1");
    }

    //Compiler
    XMLNode &compiler_node = create_node(target_node, "Compiler");
    for (auto option = target.options.cbegin(); option != target.options.cend(); option++)
    {
        create_node(compiler_node, "Add", "option", *option);
    }
    for (auto define = target.defines.cbegin(); define != target.defines.cend(); define++)
    {
        std::string define_string = "-D" + define->first;
        if (!define->second.empty()) define_string += "=" + define->second;
        create_node(compiler_node, "Add", "option", define_string);
    }
    for (auto directory = target.directories.cbegin(); directory != target.directories.cend(); directory++)
    {
        create_node(compiler_node, "Add", "directory", *directory);
    }

    //Linker
    XMLNode &linker_node = create_node(target_node, "Linker");
    for (auto option = target.linker_options.cbegin(); option != target.linker_options.cend(); option++)
    {
        create_node(linker_node, "Add", "option", *option);
    }
    for (auto library = target.linker_sources.cbegin(); library != target.linker_sources.cend(); library++)
    {
        create_node(linker_node, "Add", "library", *library);
    }
}

void rm::CodeBlocksModule::Checkout::checkout_build(XMLNode &build_node, const std::vector<Target> &targets)
{
    for (auto target = targets.cbegin(); target != targets.cend(); target++)
    {
        std::string target_title = target->name;
        if (!target->configuration_name.empty()) target_title += '_' + target->configuration_name;
        XMLNode &target_node = create_node(build_node, "Target", "title", target_title);
        checkout_target(target_node, *target);
    }
}

void rm::CodeBlocksModule::Checkout::checkout_document(const std::vector<Target> &targets, const Project &project)
{
    //Declaration
    create_declaration("1.0", "UTF-8", "yes");

    //CodeBlocks_project_file
    XMLNode &root = create_root_node("CodeBlocks_project_file");

    //FileVersion
    XMLNode &file_version = create_node(root, "FileVersion");
    checkout_attribute(file_version, "major", "1");
    checkout_attribute(file_version, "minor", "6");

    //Project
    XMLNode &project_node = create_node(root, "Project");
    checkout_attribute(create_node(project_node, "Option", "title"), "title", project.name);
    checkout_attribute(create_node(project_node, "Option", "pch_mode"), "pch_mode", "2");
    checkout_attribute(create_node(project_node, "Option", "compiler"), "compiler", compiler);
    
    //Build
    XMLNode &build_node = create_node(project_node, "Build");
    checkout_build(build_node, targets);

    //VirtualTargets
    XMLNode &virtual_targets = create_node(project_node, "VirtualTargets");
    std::set<std::string> configuration_names;
    for (auto target = targets.cbegin(); target != targets.cend(); target++) configuration_names.insert(target->configuration_name);
    for (auto configuration_name = configuration_names.cbegin(); configuration_name != configuration_names.cend(); configuration_name++)
    {
        std::string add_alias_string = "all";
        if (!configuration_name->empty()) add_alias_string += '_' + *configuration_name;
        XMLNode &add_alias = create_node(virtual_targets, "Add", "alias", add_alias_string);

        std::string add_targets_string = "";
        for (auto target = targets.cbegin(); target != targets.cend(); target++)
        {
            if (target->configuration_name == *configuration_name)
            {
                add_targets_string += target->name;
                if (!configuration_name->empty()) add_targets_string += '_' + *configuration_name;
                add_targets_string += ';';
            }
        }
        checkout_attribute(add_alias, "targets", add_targets_string);
    }

    //Compiler
    create_node(project_node, "Compiler");
    //TODO: common options

    //Unit
    std::map<std::string, std::vector<const Target*>> map;
    for (auto target = targets.cbegin(); target != targets.cend(); target++)
    {
        for (auto source = target->sources.cbegin(); source != target->sources.cend(); source++)
        {
            auto find = map.find(*source);
            if (find == map.cend()) map.insert({ *source, { &(*target) }});
            else find->second.push_back(&(*target));
        }
    }
    for (auto source = map.cbegin(); source != map.cend(); source++)
    {
        XMLNode &unit_filename = create_node(project_node, "Unit", "filename", source->first);
        for (auto target = source->second.cbegin(); target != source->second.cend(); target++)
        {
            std::string option_target_string = (*target)->name;
            if (!(*target)->configuration_name.empty()) option_target_string += '_' + (*target)->configuration_name;
            create_node(unit_filename, "Option", "target", option_target_string);
        }
    }

    //Extensions
    create_node(project_node, "Extensions");
}

rm::Module *rm::CodeBlocksModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new CodeBlocksModule();

    std::vector<std::string> module_parse = parse(requested_module, '\0');
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "codeblocks") return new CodeBlocksModule();
    if (module_parse.size() == 2 && module_parse[0] == "code" && module_parse[1] == "blocks") return new CodeBlocksModule();
    return nullptr;
}

rm::CodeBlocksModule::CodeBlocksModule()
{}

int rm::CodeBlocksModule::order() const
{
    return 100;
}

std::string rm::CodeBlocksModule::id() const
{
    return "codeblocks";
}

std::string rm::CodeBlocksModule::name() const
{
    return "Code::Blocks";
}

std::string rm::CodeBlocksModule::help() const
{
    return "Code::Blocks IDE";
}

std::vector<std::string> rm::CodeBlocksModule::slots() const
{
    return { "build", "editor", "intellisense", "debug" };
}

void rm::CodeBlocksModule::check(const RetroMake *system) const
{
    //No additional checks
}

void rm::CodeBlocksModule::pre_work(RetroMake *system)
{
    codemodel_request(system->binary_directory);

    remove_cmake_define(&system->arguments, "CMAKE_ARCHIVE_OUTPUT_DIRECTORY=");
    remove_cmake_define(&system->arguments, "CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY=");
    remove_cmake_define(&system->arguments, "CMAKE_LIBRARY_OUTPUT_DIRECTORY=");
    remove_cmake_define(&system->arguments, "CMAKE_PDB_OUTPUT_DIRECTORY=");
    remove_cmake_define(&system->arguments, "CMAKE_RUNTIME_OUTPUT_DIRECTORY=");
    system->arguments.push_back("-G");
    system->arguments.push_back("Ninja Multi-Config");
    system->arguments.push_back("-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY=obj");
    system->arguments.push_back("-DCMAKE_COMPILE_PDB_OUTPUT_DIRECTORY=obj");
    system->arguments.push_back("-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=bin");
    system->arguments.push_back("-DCMAKE_PDB_OUTPUT_DIRECTORY=obj");
    system->arguments.push_back("-DCMAKE_RUNTIME_OUTPUT_DIRECTORY=bin");
}

void rm::CodeBlocksModule::post_work(const RetroMake *system)
{
    //Parse codemodel
    Project project;
    std::vector<Target> targets = codemodel_parse(system->binary_directory, system->source_directory, &project, true);
    for (auto target = targets.begin(); target != targets.end(); target++)
    {
        target->path = path_relative(target->path, system->binary_directory, nullptr);

        std::set<std::string> sources;
        for (auto source = target->sources.begin(); source != target->sources.end(); source++)
            sources.insert(path_relative(*source, system->binary_directory, nullptr));
        target->sources = sources;

        if (target->configuration_name == "Debug") target->configuration_name = "deb"; //Name should be shorter then "XXXXXXXXX" (in pixels)
        if (target->configuration_name == "Release") target->configuration_name = "rel";
        if (target->configuration_name == "RelWithDebInfo") target->configuration_name = "rdb";
    }

    //Sort
    std::vector<Target> sorted_targets;
    while (true) //TODO: behold, the most inefficient algorithm in the world
    {
        std::vector<Target> zero_depends, nonzero_depends;
        for (auto target = targets.begin(); target != targets.end(); target++)
        {
            if (target->dependencies.empty()) zero_depends.push_back(std::move(*target));
            else nonzero_depends.push_back(std::move(*target));
        }
        const bool zero_depends_empty = zero_depends.empty();
        for (auto target = zero_depends.begin(); target != zero_depends.end(); target++)
        {
            for (auto target2 = nonzero_depends.begin(); target2 != nonzero_depends.end(); target2++)
            {
                if (target->configuration_name != target2->configuration_name) continue;
                auto find = target2->dependencies.find(target->name);
                if (find != target2->dependencies.end()) target2->dependencies.erase(find);
            }
            sorted_targets.push_back(std::move(*target));
        }
        if (nonzero_depends.empty()) break;
        if (zero_depends_empty)
        {
            std::cout << "RetroMake Warning: targets have circular dependencies" << std::endl;
            for (auto target = nonzero_depends.begin(); target != nonzero_depends.end(); target++)
                sorted_targets.push_back(std::move(*target));
            break;
        }
        targets = nonzero_depends;
    }

    //Find compiler
    std::string compiler;
    for (auto module = system->modules.cbegin(); module != system->modules.cend() && compiler.empty(); module++)
    {
        auto slots = (*module)->slots();
        for (auto slot = slots.cbegin(); slot != slots.cend() && compiler.empty(); slot++)
        {
            if (*slot != "compiler") continue;
            if ((*module)->id() == "gcc") compiler = "gcc";
            else if ((*module)->id() == "clang") compiler = "clang";
        }
    }

    //Create project
    const std::string project_file = system->binary_directory + project.name + ".cbp";
    Checkout checkout(*this, compiler);
    checkout.document_read(project_file);
    checkout.checkout_document(sorted_targets, project);
    if (checkout.change) checkout.document_write(project_file, 2);
}