#include "../include/module_vscodium.h"
#include "../include/retromake.h"
#include "../include/util.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>
#include <sys/stat.h>

#include <cstring>
#include <fstream>
#include <stdexcept>

bool rm::VSCodiumModule::_checkout_string(JSONValue &string, JSONAllocator &allocator, const char *value)
{
    if (!string.IsString() || std::strcmp(string.GetString(), value) != 0)
    {
        string.SetString(value, allocator);
        return true;
    }
    return false;
}

bool rm::VSCodiumModule::_checkout_args(JSONValue &args, JSONAllocator &allocator)
{
    bool change = false;
    if (!args.IsArray() || args.GetArray().Size() != 2)
    {
        args.SetArray();
        args.PushBack(rapidjson::Value("-c", allocator), allocator);
        args.PushBack(rapidjson::Value("cmake --build .", allocator), allocator);
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

bool rm::VSCodiumModule::_checkout_options(JSONValue &options, JSONAllocator &allocator)
{
    bool change = false;
    if (!options.IsObject() || options.MemberCount() != 1 || std::strcmp(options.MemberBegin()->name.GetString(), "cwd") != 0)
    {
        options.SetObject();
        options.AddMember("cwd", rapidjson::Value(_system->binary_directory.c_str(), allocator), allocator);
        change = true;
    }
    else
    {
        auto option = options.MemberBegin();
        change |= _checkout_string(option->value, allocator, _system->binary_directory.c_str());
    }
    return change;
}

bool rm::VSCodiumModule::_checkout_task(JSONValue &task, JSONAllocator &allocator)
{
    bool change = false;
    if (!task.HasMember("label")) { task.AddMember("label", rapidjson::Value(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(task["label"], allocator, "retromake-build-task");
    change |= _checkout_string(task["type"], allocator, "shell");
    change |= _checkout_string(task["command"], allocator, "/bin/sh");
    if (!task.HasMember("args")) { task.AddMember("args", rapidjson::Value(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_args(task["args"], allocator);
    if (!task.HasMember("options")) { task.AddMember("options", rapidjson::Value(rapidjson::kObjectType), allocator); change = true; }
    change |= _checkout_options(task["options"], allocator);
    return change;
}

bool rm::VSCodiumModule::_checkout_tasks(JSONValue &tasks, JSONAllocator &allocator)
{
    bool change = false;
    if (!tasks.IsArray()) { tasks.SetArray(); change = true; }
    for (auto task = tasks.Begin(); task != tasks.End(); task++)
    {
        if (!task->IsObject()) continue;
        if (!task->HasMember("label")) continue;
        if (!(*task)["label"].IsString()) continue;
        if (std::strcmp((*task)["label"].GetString(), "retromake-build-task") != 0) continue;
        change |= _checkout_task((*task), allocator);
        return change;
    }
    rapidjson::Value task = rapidjson::Value(rapidjson::kObjectType);
    _checkout_task(task, allocator);
    return true;
}

bool rm::VSCodiumModule::_checkout_document(JSONValue &document, JSONAllocator &allocator)
{
    bool change = false;
    if (!document.IsObject()) { document.SetObject(); change = true; }
    if (!document.HasMember("tasks")) { document.AddMember("tasks", rapidjson::Value(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_tasks(document["tasks"], allocator);
    return change;
}

bool rm::VSCodiumModule::_read_document(JSONDocument &document)
{
    //Create directory
    const std::string vscode_directory = _system->source_directory + ".vscode/";
    const std::string tasks_file = vscode_directory + "tasks.json";
    if (!directory_exists(vscode_directory, nullptr))
    {
        if (mkdir(vscode_directory.c_str(), 0700) != 0) throw std::runtime_error("Failed to create directory " + vscode_directory);
        return true;
    }

    //Read file
    std::ifstream file(tasks_file, std::ios::binary);
    if (!file.is_open()) return true;
    file.seekg(0, std::ios::end);
    std::string content; content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    if (!file.read(&content[0], content.size())) throw std::runtime_error("Failed to read file " + tasks_file);

    //Parse file
    try
    {
        document.Parse(content.c_str());
    }
    catch(...)
    {
        throw std::runtime_error("Failed to parse JSON file " + tasks_file);
    }
    return false;
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

    const std::string vscode_directory = _system->source_directory + ".vscode/";
    const std::string tasks_file = vscode_directory + "tasks.json";
    std::ofstream file(tasks_file, std::ios::binary);
    if (!file.write(buffer.GetString(), buffer.GetSize())) throw std::runtime_error("Failed to write file " + tasks_file);
}