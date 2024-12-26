#include "../include/module_codeblocks.h"
#include "../include/retromake.h"

#include <cstddef>
#include <cstring>
#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

#include <rapidxml/rapidxml.hpp>
#include <rapidxml/rapidxml_print.hpp>
#include <rapidxml/rapidxml_utils.hpp>
#include <vector>

rm::Module *rm::CodeBlocksModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new CodeBlocksModule();

    std::vector<std::string> module_parse = parse(requested_module, false);
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

void rm::CodeBlocksModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::CodeBlocksModule::pre_work(RetroMake *system)
{
    const std::string codemodel_file = system->binary_directory + ".cmake/api/v1/query/codemodel-v2";
    path_ensure(codemodel_file, false, 5);
    if (path_exists(codemodel_file, false, nullptr)) return;
    std::ofstream file(codemodel_file, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open file " + codemodel_file);
}

namespace rm {

class Checkout
{
    XMLDocument &document;
    bool change;

    Checkout(XMLDocument &document) : document(document), change(false) {}

    XMLNode *create_node(XMLNode *parent, const char *name) //Find or create node with name
    {
        auto child = parent->first_node(name);
        if (child == nullptr || child->type() != rapidxml::node_element)
        {
            if (child == nullptr) parent->remove_node(child);
            child = document.allocate_node(rapidxml::node_element, name);
            parent->append_node(child);
        }
        return child;
    }

    XMLNode *create_node(XMLNode *parent, const char *name, const char *attribute_name)
    {
        auto child = parent->first_node(name);
        while (child != nullptr)
        {
            if (child->type() == rapidxml::node_element)
            {
                auto attribute = child->first_attribute(attribute_name);
                if (attribute != nullptr) break;
            }
            child = child->next_sibling(name);
        }
        if (child == nullptr)
        {
            child = document.allocate_node(rapidxml::node_element, name);
            parent->append_node(child);

            change = true;
        }
        return child;
    }

    XMLNode *create_node(XMLNode *parent, const char *name, const char *attribute_name, const char *attribute_value)
    {
        auto child = parent->first_node(name);
        while (child != nullptr)
        {
            if (child->type() == rapidxml::node_element)
            {
                auto attribute = child->first_attribute(attribute_name);
                if (attribute != nullptr && std::strcmp(attribute->value(), attribute_value) == 0) break;
            }
            child = child->next_sibling(name);
        }
        if (child == nullptr)
        {
            child = document.allocate_node(rapidxml::node_element, name);
            parent->append_node(child);

            auto attribute = document.allocate_attribute(attribute_name, attribute_value);
            child->append_attribute(attribute);

            change = true;
        }
        return child;
    }

    void checkout_attribute(XMLNode *node, const char *attribute_name, const char *attribute_value) //Find attribute and set value
    {
        auto attribute = node->first_attribute(attribute_name);
        if (attribute != nullptr && std::strcmp(attribute->value(), attribute_value) == 0) return;
        if (attribute != nullptr) node->remove_attribute(attribute);
        attribute = document.allocate_attribute(attribute_name, attribute_value);
        node->append_attribute(attribute);
        change = true;
    }

    void checkout_target(XMLNode *target_node, const Target &target, size_t release)
    {
        const char *target_directory = release ? "Release/" : "Debug/";

        //<Option output
        auto option_output = create_node(target_node, "Option", "output");
        std::string option_output_string = std::string("bin/") + target_directory + target.name;
        checkout_attribute(option_output, "output", option_output_string.c_str());
        if (target.typ == "SHARED_LIBRARY")
        {
            checkout_attribute(option_output, "imp_lib", "$(TARGET_OUTPUT_DIR)$(TARGET_OUTPUT_BASENAME).a");
            checkout_attribute(option_output, "def_file", "$(TARGET_OUTPUT_DIR)$(TARGET_OUTPUT_BASENAME).def");
        }
        checkout_attribute(option_output, "prefix_auto", "1");
        checkout_attribute(option_output, "extension_auto", "1");

        //Option object_output
        auto option_object_output = create_node(target_node, "Option", "object_output");
        std::string option_object_output_string = std::string("obj/") + target_directory;
        checkout_attribute(option_object_output, "object_output", option_object_output_string.c_str());

        //Option type
        if (target.typ == "EXECUTABLE") checkout_attribute(create_node(target_node, "Option", "type"), "type", "1");
        else if (target.typ == "STATIC_LIBRARY") checkout_attribute(create_node(target_node, "Option", "type"), "type", "2");
        else if (target.typ == "SHARED_LIBRARY") checkout_attribute(create_node(target_node, "Option", "type"), "type", "3");

        //Option compiler
        checkout_attribute(create_node(target_node, "Option", "compiler"), "compiler", "gcc");

        //Option createDefFile & Option createStaticLib
        if (target.typ == "SHARED_LIBRARY")
        {
            checkout_attribute(option_output, "createDefFile", "1");
            checkout_attribute(option_output, "createStaticLib", "1");
        }

        //Compiler
        auto compiler_node = create_node(target_node, "Compiler");
        for (auto option = target.options.cbegin(); option != target.options.cend(); option++)
        {
            create_node(compiler_node, "Add", "option", option->c_str());
        }
        for (auto define = target.defines.cbegin(); define != target.defines.cend(); define++)
        {
            std::string define_string = "-D" + define->first + "=" + define->second;
            create_node(compiler_node, "Add", "option", define_string.c_str());
        }
        for (auto directory = target.directories.cbegin(); directory != target.directories.cend(); directory++)
        {
            create_node(compiler_node, "Add", "directory", directory->c_str());
        }
        create_node(compiler_node, "Add", "option", release ? "-O2" : "-g"); //TODO: 

        //Linker
        auto linker_node = create_node(target_node, "Linker");
        for (auto option = target.linker_options.cbegin(); option != target.linker_options.cend(); option++)
        {
            create_node(linker_node, "Add", "option", option->c_str());
        }
        for (auto library = target.linker_sources.cbegin(); library != target.linker_sources.cend(); library++)
        {
            create_node(linker_node, "Add", "library", library->c_str());
        }

        //TODO: Debug/Release options
    }

    void checkout_build(XMLNode *build_node, const std::vector<Target> &targets, const Project &project)
    {
        for (size_t release = 0; release < 2; release++)
        {
            const char *target_suffix = release ? "_release" : "_debug";
            const char *target_directory = release ? "Release/" : "Debug/";
            for (auto target = targets.cbegin(); target != targets.cend(); target++)
            {
                const std::string target_title_string = project.name + target_suffix;
                auto target_node = create_node(build, "Target", "title", target_title_string.c_str());
                
            }
        }
    }

    void checkout_document(const std::vector<Target> &targets, const Project &project)
    {
        //Declaration
        auto declaration = document.first_node();
        if (declaration == nullptr || declaration->type() != rapidxml::node_declaration)
        {
            document.remove_all_nodes();
            declaration = document.allocate_node(rapidxml::node_declaration);
            change = true;
        }

        //CodeBlocks_project_file
        auto root = create_node(&document, "CodeBlocks_project_file");

        //FileVersion
        auto file_version = create_node(root, "FileVersion");
        checkout_attribute(file_version, "major", "1");
        checkout_attribute(file_version, "minor", "6");

        //Project
        auto project_node = create_node(root, "Project");
        checkout_attribute(create_node(project_node, "Option", "title"), "title", project.name.c_str());
        checkout_attribute(create_node(project_node, "Option", "pch_mode"), "pch_mode", "2");
        checkout_attribute(create_node(project_node, "Option", "compiler"), "compiler", "gcc");
        
        //Build
        auto build = create_node(project_node, "Build");


        //VirtualTargets
        auto virtual_targets = create_node(project_node, "VirtualTargets");
        auto add_alias_all = create_node(virtual_targets, "Add", "alias", "All");
        std::string add_alias_all_string = "";
        for (size_t release = 0; release < 2; release++)
        {
            const char *target_suffix = release ? "_release" : "_debug";
            for (auto target = targets.cbegin(); target != targets.cend(); target++)
                add_alias_all_string += target->name + target_suffix + ';';
        }
        checkout_attribute(add_alias_all, "targets", add_alias_all_string.c_str());

        //Compiler
        //TODO

        //Unit
        std::map<std::string, std::vector<std::string>> map;
        for (size_t release = 0; release < 2; release++)
        {
            const char *target_suffix = release ? "_release" : "_debug";
            for (auto target = targets.cbegin(); target != targets.cend(); target++)
            {
                add_alias_all_string += target->name + target_suffix + ';';
            }
        }

        //Extensions
        create_node(project_node, "Extensions");
    }
};

void checkout_attribute(rapidxml::xml_document<> &document, rapidxml::xml_node<> *node, const char *name, const char *value, bool *change)
{
    auto attribute = node->first_attribute(name);
    if (attribute != nullptr && std::strcmp(attribute->value(), value) == 0) return;
    if (attribute != nullptr) node->remove_attribute(attribute);
    attribute = document.allocate_attribute(name, value);
    node->append_attribute(document.allocate_attribute(name, value));
}

rapidxml::xml_node<>* checkout_node(rapidxml::xml_document<> &document, rapidxml::xml_node<> *parent, const char *name, bool *change)
{
    auto child = parent->first_node(name);
    if (child != nullptr && child->type() == rapidxml::node_element) return child;
    if (child->type() != rapidxml::node_element) parent->remove_node(child);
    child = document.allocate_node(rapidxml::node_element, name);
    parent->append_node(child);
    *change = true;
    return child;
}

bool checkout_document(rapidxml::xml_document<> &document, const Project &project, const std::vector<Target> &targets)
{
    //Declaration
    bool change = false;
    
    checkout_attribute(document, declaration, "version", "1.0", &change);
    checkout_attribute(document, declaration, "encoding", "UTF-8", &change);
    checkout_attribute(document, declaration, "standalone", "yes", &change);

    //Root
    auto root = checkout_node(document, &document, "CodeBlocks_project_file", &change);

    //FileVersion
    auto FileVersion = checkout_node(document, root, "FileVersion", &change);
    checkout_attribute(document, FileVersion, "major", "1", &change);
    checkout_attribute(document, FileVersion, "minor", "6", &change);

    //Project
    auto Project = checkout_node(document, root, "Project", &change);



    rapidxml::xml_node<> *parent, const char *name) document.first_node();
    if (root == nullptr || root->type() != rapidxml::node_element)
    {
        if (root != nullptr) document.remove_node(root);
        root = document.allocate_node(rapidxml::node_element, "CodeBlocks_project_file");
        document.append_node(root);
        change = true;
    }

    //FileVersion
    auto FileVersion = root->first_node("FileVersion");
    if (FileVersion == nullptr || FileVersion->type() != rapidxml::node_element)
    {
        if (root != nullptr) document.remove_node(root);
        root = document.allocate_node(rapidxml::node_element, "CodeBlocks_project_file");
        document.append_node(root);
        change = true;
    }



    xml_node<>* cur_node = doc.first_node("rootnode");


    ;
    if () {  }
    change |= 
    {
        document_node->append_attribute(document.allocate_attribute("version", "1.0"));
        document_node->append_attribute(document.allocate_attribute());
        document_node->append_attribute(document.allocate_attribute());
        document.append_node(document_node);
        change = true;
    }
    auto version = document_node->first_attribute("version");
    if (version == nullptr) { version = document.allocate_attribute("version", "URF-8"); document_node->append_attribute(version); }
    if (std::strcmp(version->value(), "URF-8") != 0) version.

    if ( == nullptr || document_node->first_attribute("version")->value())

}

} //namespace rm

void rm::CodeBlocksModule::post_work(RetroMake *system)
{
    std::vector<Target> targets = codemodel_parse(system->binary_directory, system->source_directory, true);

    JSONDocument document;
    bool change = document_read(document, );
    change |= _checkout_document(document, document.GetAllocator());
    if (!change) return;

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    const std::string tasks_file = _system->source_directory + ".vscode/tasks.json";
    path_ensure(tasks_file, false, 2);
    std::ofstream file(tasks_file, std::ios::binary);
    if (!file.write(buffer.GetString(), buffer.GetSize())) throw std::runtime_error("Failed to write file " + tasks_file);
}