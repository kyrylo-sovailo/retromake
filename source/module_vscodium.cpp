#include "../include/module_vscodium.h"
#include "../include/retromake.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <stdexcept>

bool rm::VSCodiumModule::_read_document(JSONDocument &document) const
{
    //Read file
    const std::string tasks_file = _system->source_directory + ".vscode/tasks.json";
    if (!path_exists(tasks_file, false, nullptr)) return true;
    std::string content;
    {
        std::ifstream file(tasks_file, std::ios::binary);
        if (!file.is_open()) return true;
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        if (!file.read(&content[0], content.size())) throw std::runtime_error("Failed to read file " + tasks_file);
    }

    //Parse file
    try { document.Parse(content.c_str()); }
    catch(...) { throw std::runtime_error("Failed to parse JSON file " + tasks_file); }
    return false;
}

bool rm::VSCodiumModule::_checkout_string(JSONValue &string, JSONAllocator &allocator, const char *value) const
{
    if (!string.IsString() || std::strcmp(string.GetString(), value) != 0)
    {
        string.SetString(value, allocator);
        return true;
    }
    return false;
}

bool rm::VSCodiumModule::_checkout_args(JSONValue &args, JSONAllocator &allocator) const
{
    bool change = false;
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
        change |= _checkout_string(*arg, allocator, "-c");
        arg++;
        change |= _checkout_string(*arg, allocator, "cmake --build .");
    }
    return change;
}

bool rm::VSCodiumModule::_checkout_options(JSONValue &options, JSONAllocator &allocator) const
{
    bool change = false;
    if (!options.IsObject() || options.MemberCount() != 1 || std::strcmp(options.MemberBegin()->name.GetString(), "cwd") != 0)
    {
        options.SetObject();
        options.AddMember("cwd", JSONValue(_system->binary_directory.c_str(), allocator), allocator);
        change = true;
    }
    else
    {
        auto option = options.MemberBegin();
        change |= _checkout_string(option->value, allocator, _system->binary_directory.c_str());
    }
    return change;
}

bool rm::VSCodiumModule::_checkout_task(JSONValue &task, JSONAllocator &allocator) const
{
    bool change = false;
    //label
    if (!task.HasMember("label")) { task.AddMember("label", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(task["label"], allocator, "retromake-build-task");
    //type
    if (!task.HasMember("type")) { task.AddMember("type", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(task["type"], allocator, "shell");
    //command
    if (!task.HasMember("command")) { task.AddMember("command", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(task["command"], allocator, "/bin/sh");
    //args
    if (!task.HasMember("args")) { task.AddMember("args", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_args(task["args"], allocator);
    //options
    if (!task.HasMember("options")) { task.AddMember("options", JSONValue(rapidjson::kObjectType), allocator); change = true; }
    change |= _checkout_options(task["options"], allocator);
    return change;
}

bool rm::VSCodiumModule::_checkout_tasks(JSONValue &tasks, JSONAllocator &allocator) const
{
    bool change = false;
    if (!tasks.IsArray()) { tasks.SetArray(); change = true; }
    for (auto task = tasks.Begin(); task != tasks.End(); task++)
    {
        if (!task->IsObject()) continue;
        if (!task->HasMember("label")) continue;
        JSONValue &label = (*task)["label"];
        if (!label.IsString() || std::strcmp(label.GetString(), "retromake-build-task") != 0) continue;
        change |= _checkout_task((*task), allocator);
        return change;
    }
    JSONValue task = JSONValue(rapidjson::kObjectType);
    _checkout_task(task, allocator);
    return true;
}

bool rm::VSCodiumModule::_checkout_document(JSONValue &document, JSONAllocator &allocator) const
{
    bool change = false;
    if (!document.IsObject()) { document.SetObject(); change = true; }
    if (!document.HasMember("tasks")) { document.AddMember("tasks", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_tasks(document["tasks"], allocator);
    return change;
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
    JSONDocument document;
    bool change = _read_document(document);
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