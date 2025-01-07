#include "../include/module_vscodium.h"
#include "../include/retromake.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cstring>

rm::VSCodiumModule::Checkout::Checkout(const RetroMake &owner) : owner(owner) {}

void rm::VSCodiumModule::Checkout::checkout_args(JSONValue &args_node)
{
    if (args_node.GetArray().Size() != 2)
    {
        args_node.SetArray();
        args_node.PushBack(JSONValue("-c", *allocator), *allocator);
        args_node.PushBack(JSONValue("cmake --build .", *allocator), *allocator);
        change = true;
    }
    else
    {
        auto arg_node = args_node.Begin();
        checkout_string(*arg_node, "-c");
        arg_node++;
        checkout_string(*arg_node, "cmake --build .");
    }
}

void rm::VSCodiumModule::Checkout::checkout_options(JSONValue &options_node)
{
    const std::string binary_directory = "${workspaceFolder}/" + path_relative(owner.binary_directory, owner.source_directory, nullptr);
    checkout_string(create_string(options_node, "cwd"), binary_directory);
}

void rm::VSCodiumModule::Checkout::checkout_task(JSONValue &task_node)
{
    //label
    checkout_string(create_string(task_node, "label"), "retromake-build-task");
    //type
    checkout_string(create_string(task_node, "type"), "shell");
    //command
    checkout_string(create_string(task_node, "command"), "/bin/sh");
    //args
    checkout_args(create_array(task_node, "args"));
    //options
    checkout_options(create_object(task_node, "options"));
}

void rm::VSCodiumModule::Checkout::checkout_tasks(JSONValue &tasks_node)
{
    bool task_exists = false;
    for (auto task_node = tasks_node.Begin(); task_node != tasks_node.End(); task_node++)
    {
        if (!task_node->IsObject()) continue;
        const JSONValue *label_node = find_string(*task_node, "label");
        if (label_node == nullptr || std::strcmp(label_node->GetString(), "retromake-build-task") != 0) continue;
        
        checkout_task(*task_node);
        task_exists = true;
        break;
    }
    if (!task_exists)
    {
        JSONValue task_node = JSONValue(rapidjson::kObjectType);
        checkout_task(task_node);
        tasks_node.PushBack(std::move(task_node), *allocator);
        change = true;
    }
}

void rm::VSCodiumModule::Checkout::checkout_document()
{
    if (document->IsObject()) { document->SetObject(); change = true; }
    JSONValue &tasks_node = create_array(*document, "tasks");
    checkout_tasks(tasks_node);
}

rm::Module *rm::VSCodiumModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new VSCodiumModule();

    std::vector<std::string> module_parse = parse(requested_module, '\0');
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

int rm::VSCodiumModule::order() const
{
    return 60;
}

std::string rm::VSCodiumModule::id() const
{
    return "vscodium";
}

std::string rm::VSCodiumModule::name() const
{
    return "VS Codium";
}

std::string rm::VSCodiumModule::help() const
{
    return "VS Codium editor";
}

std::vector<std::string> rm::VSCodiumModule::slots() const
{
    return { "editor" };
}

void rm::VSCodiumModule::check(const RetroMake *system) const
{
    //No additional checks
}

void rm::VSCodiumModule::pre_work(RetroMake *system)
{
    //Do nothing
}

void rm::VSCodiumModule::post_work(const RetroMake *system)
{
    const std::string tasks_file = system->source_directory + ".vscode/tasks.json";
    Checkout checkout(*system);
    checkout.document_read(tasks_file);
    checkout.checkout_document();
    if (checkout.change) checkout.document_write(tasks_file, 2);
}