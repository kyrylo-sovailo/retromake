#include "../include/module_vscodium.h"
#include "../include/retromake.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cstring>

rm::VSCodiumModule::Checkout::Checkout(const VSCodiumModule &owner, JSONAllocator &allocator) : owner(owner), allocator(allocator), change(false) {}

void rm::VSCodiumModule::Checkout::checkout_args(JSONValue &args)
{
    if (!args.IsArray() || args.GetArray().Size() != 2)
    {
        args.SetArray();
        args.PushBack(JSONValue("-c", allocator), allocator);
        args.PushBack(JSONValue("cmake --build .", allocator), allocator);
        change = true;
    }
    else
    {
        auto arg = args.Begin();
        change |= checkout_string(*arg, allocator, "-c");
        arg++;
        change |= checkout_string(*arg, allocator, "cmake --build .");
    }
}

void rm::VSCodiumModule::Checkout::checkout_options(JSONValue &options)
{
    if (!options.IsObject() || options.MemberCount() != 1 || std::strcmp(options.MemberBegin()->name.GetString(), "cwd") != 0)
    {
        options.SetObject();
        options.AddMember("cwd", JSONValue(owner._system->binary_directory.c_str(), allocator), allocator);
        change = true;
    }
    else
    {
        auto option = options.MemberBegin();
        change |= checkout_string(option->value, allocator, owner._system->binary_directory.c_str());
    }
}

void rm::VSCodiumModule::Checkout::checkout_task(JSONValue &task)
{
    //label
    if (!task.HasMember("label")) { task.AddMember("label", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= checkout_string(task["label"], allocator, "retromake-build-task");
    //type
    if (!task.HasMember("type")) { task.AddMember("type", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= checkout_string(task["type"], allocator, "shell");
    //command
    if (!task.HasMember("command")) { task.AddMember("command", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= checkout_string(task["command"], allocator, "/bin/sh");
    //args
    if (!task.HasMember("args")) { task.AddMember("args", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    checkout_args(task["args"]);
    //options
    if (!task.HasMember("options")) { task.AddMember("options", JSONValue(rapidjson::kObjectType), allocator); change = true; }
    checkout_options(task["options"]);
}

void rm::VSCodiumModule::Checkout::checkout_tasks(JSONValue &tasks)
{
    if (!tasks.IsArray()) { tasks.SetArray(); change = true; }
    bool task_exists = false;
    for (auto task = tasks.Begin(); task != tasks.End(); task++)
    {
        if (!task->IsObject()) continue;
        if (!task->HasMember("label")) continue;
        JSONValue &label = (*task)["label"];
        if (!label.IsString() || std::strcmp(label.GetString(), "retromake-build-task") != 0) continue;
        task_exists = true;
        checkout_task((*task));
        break;
    }
    if (!task_exists)
    {
        JSONValue task = JSONValue(rapidjson::kObjectType);
        checkout_task(task);
        tasks.PushBack(std::move(task), allocator);
        change = true;
    }
}

void rm::VSCodiumModule::Checkout::checkout_document(JSONValue &document)
{
    if (!document.IsObject()) { document.SetObject(); change = true; }
    if (!document.HasMember("tasks")) { document.AddMember("tasks", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    checkout_tasks(document["tasks"]);
}

rm::Module *rm::VSCodiumModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new VSCodiumModule();

    std::vector<std::string> module_parse = parse(requested_module, false);
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "vscodium") return new VSCodiumModule();
    if (module_parse.size() == 1 && module_parse[0] == "codium") return new VSCodiumModule();
    if (module_parse.size() == 2 && module_parse[0] == "vs" && module_parse[1] == "codium") return new VSCodiumModule();
    if (module_parse.size() == 2 && module_parse[0] == "vscode" && module_parse[1] == "oss") return new VSCodiumModule();
    if (module_parse.size() == 3 && module_parse[0] == "vs" && module_parse[1] == "code" && module_parse[2] == "oss") return new VSCodiumModule();
    return nullptr;
}

rm::VSCodiumModule::VSCodiumModule() : _system(nullptr)
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

void rm::VSCodiumModule::check(const std::vector<Module*> &modules) const
{
    //No additional checks
}

void rm::VSCodiumModule::pre_work(RetroMake *system)
{
    //Do nothing
}

void rm::VSCodiumModule::post_work(RetroMake *system)
{
    _system = system;
    const std::string tasks_file = system->source_directory + ".vscode/tasks.json";
    JSONDocument document;
    bool change = !document_read(document, tasks_file);
    Checkout checkout(*this, document.GetAllocator());
    checkout.checkout_document(document);
    change |= checkout.change;
    if (change) document_write(document, tasks_file, 2);
}