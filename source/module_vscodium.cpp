#include "../include/module_vscodium.h"
#include "../include/retromake.h"
#include "../include/util.h"

#include <cstring>
#include <fstream>
#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <sys/stat.h>

#include <stdexcept>

rm::Module *rm::VSCodiumModule::create_module(const std::string &requested_module)
{
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

std::string rm::VSCodiumModule::id() const
{
    return "vscodium";
}

std::string rm::VSCodiumModule::name() const
{
    return "VS Codium";
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

bool rm::VSCodiumModule::_checkout_string(rapidjson::Value &string, rapidjson::MemoryPoolAllocator<> &allocator, const char *value)
{
    if (!string.IsString() || std::strcmp(string.GetString(), value) != 0)
    {
        string.SetString(value, allocator);
        return true;
    }
    return false;
}

bool rm::VSCodiumModule::_checkout_args(rapidjson::Value &args, rapidjson::MemoryPoolAllocator<> &allocator)
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

bool rm::VSCodiumModule::_checkout_options(rapidjson::Value &options, rapidjson::MemoryPoolAllocator<> &allocator)
{
    bool change = true;
    if (!options.IsObject() || options.MemberCount() != 1 || std::strcmp(options.MemberBegin()->name.GetString(), "cwd") != 0)
    {
        options.SetObject();
        options.AddMember("cwd", rapidjson::Value(system->binary_directory.c_str(), allocator), allocator);
        change = true;
    }
    else
    {
        auto option = options.MemberBegin();
        change |= _checkout_string(option->value, allocator, system->binary_directory.c_str());
    }
    return change;
}

bool rm::VSCodiumModule::_checkout_task(rapidjson::Value &task, rapidjson::MemoryPoolAllocator<> &allocator)
{
    bool change = false;
    if (!task.HasMember("label")) { task.AddMember("label", rapidjson::Value(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(task["label"], allocator, "retromake-build-task");
    change |= _checkout_string(task["type"], allocator, "shell");
    change |= _checkout_string(task["command"], allocator, "/bin/sh");
    if (!task.HasMember("args")) { task.AddMember("args", rapidjson::Value(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_args(task["args"], allocator);
    if (!task.HasMember("options")) { task.AddMember("options", rapidjson::Value(rapidjson::kObjectType), allocator); change = true; }
    change |= _checkout_args(task["options"], allocator);
    return change;
}

bool rm::VSCodiumModule::_checkout_tasks(rapidjson::Value &tasks, rapidjson::MemoryPoolAllocator<> &allocator)
{
    bool change = false;
    if (!tasks.IsArray()) { tasks.SetArray(); change = true; }
    for (auto task = tasks.Begin(); task != tasks.End(); task++)
    {
        if (!task->IsObject()) continue;
        if (!task->HasMember("label")) continue;
        if (!(*task)["label"].IsString()) continue;
        if (std::strcmp((*task)["label"].GetString(), "retromake-build-task") != 0) continue;
        change |= _checkout_task((*task)["label"], allocator);
        return change;
    }
    rapidjson::Value task = rapidjson::Value(rapidjson::kObjectType);
    _checkout_task(task, allocator);
    return true;
}

bool rm::VSCodiumModule::_checkout_document(rapidjson::Value &document, rapidjson::MemoryPoolAllocator<> &allocator)
{
    bool change = false;
    if (!document.IsObject()) { document.SetObject(); change = true; }
    if (!document.HasMember("tasks")) { document.AddMember("tasks", rapidjson::Value(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_tasks(document["tasks"], allocator);
    return change;
}

void rm::VSCodiumModule::post_work(RetroMake *system)
{
    const std::string vscode_directory = system->source_directory + ".vscode/";
    if (!directory_exists(vscode_directory, nullptr))
    {
        if (mkdir(vscode_directory.c_str(), 0700) != 0) throw std::runtime_error("Failed to create directory " + vscode_directory);
    }

    const std::string tasks_file = vscode_directory + "tasks.json";
    bool rewrite = file_exists(tasks_file, nullptr);
    

    std::ifstream file(tasks_file, std::ios::binary);
    file.seekg(0, std::ios::end);
    const size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string content;
    file.read(&content[0], content.size());

    rapidjson::Document document;
    document.Parse(content.c_str());
    document.GetAllocator()
    if (document.HasMember("tasks") && document["tasks"].IsArray())
    {
        auto &tasks = document["tasks"];
        
        printf("%d ", itr->GetInt());

    }

    const std::string content = 
    "{\n"
    "    \"tasks\":\n"
    "    [\n"
    "        {\n"
    "            \"label\": \"retromake-build-task\",\n"
    "            \"type\": \"shell\",\n"
    "            \"command\": \"/bin/sh\",\n"
    "            \"args\": [ \"-c\", \"cmake --build .\" ],\n"
    "            \"options\": { \"cwd\": \"" + system->binary_directory + "\" }\n"
    "        }"
    "    ]"
    "}";
    update_file(tasks_file, content);
}

rm::VSCodiumModule::~VSCodiumModule()
{}