#include "../include/module_native_debug.h"
#include "../include/retromake.h"
#include "../include/util.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

std::string rm::NativeDebugModule::_get_relative_path(const std::string &absolute) const
{
    size_t i = 0;
    while (i < _system->source_directory.size() && i < absolute.size() && _system->source_directory[i] == absolute[i]) i++;
    std::string relative = "${workspaceFolder}/";
    for (size_t i2 = i; i2 < _system->source_directory.size(); i2++)
    {
        if (_system->source_directory[i2] == '/') relative += "../";
    }
    return relative + (absolute.c_str() + i);
}

bool rm::NativeDebugModule::_read_document(JSONDocument &document) const
{
    //Read file
    const std::string launch_file = _system->source_directory + ".vscode/launch.json";
    if (!path_exists(launch_file, false, nullptr)) return true;
    std::string content;
    {
        std::ifstream file(launch_file, std::ios::binary);
        if (!file.is_open()) return true;
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        if (!file.read(&content[0], content.size())) throw std::runtime_error("Failed to read file " + launch_file);
    }

    //Parse file
    try { document.Parse(content.c_str()); }
    catch(...) { throw std::runtime_error("Failed to parse JSON file " + launch_file); }
    return false;
}

std::vector<rm::NativeDebugModule::Target> rm::NativeDebugModule::_get_executable_targets() const
{
    //Find codemodel file
    const std::string reply_directory = _system->binary_directory + ".cmake/api/v1/reply/";
    std::string codemodel_file;
    {
        Search search(reply_directory);
        while (true)
        {
            bool directory;
            const char *entry = search.get(&directory);
            if (entry == nullptr) break;
            if (directory) continue;
            if (std::strlen(entry) < 13) continue;
            if (std::memcmp(entry, "codemodel-v2-", 13) != 0) continue;
            if (std::memcmp(entry + strlen(entry) - 5, ".json", 5) != 0) continue;
            if (!codemodel_file.empty()) throw std::runtime_error("CMake file API returned several codemodels");
            codemodel_file = reply_directory + entry;
        }
    }
    if (codemodel_file.empty()) throw std::runtime_error("CMake file API did not return a codemodel");

    //Read codemodel file
    std::string content;
    {
        std::ifstream file(codemodel_file, std::ios::binary);
        if (!file.is_open()) throw std::runtime_error("Failed to open file " + codemodel_file);
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        if (!file.read(&content[0], content.size())) throw std::runtime_error("Failed to read file " + codemodel_file);
    }

    //Parse codemodel file
    JSONDocument document;
    try { document.Parse(content.c_str()); }
    catch(...) { throw std::runtime_error("Failed to parse JSON file " + codemodel_file); }

    //Find targets
    std::vector<Target> executable_targets;
    if (!document.IsObject() || !document.HasMember("configurations")) return executable_targets;
    const JSONValue &configurations = document["configurations"];
    if (!configurations.IsArray() || document["configurations"].Size() == 0) return executable_targets;
    const JSONValue &configuration = *configurations.Begin(); //Only the first one for now
    if (!configuration.IsObject() || !configuration.HasMember("targets")) return executable_targets;
    const JSONValue &targets = configuration["targets"];
    for (auto target = targets.Begin(); target != targets.End(); target++)
    {
        if (!target->IsObject() || !target->HasMember("jsonFile")) continue;
        const JSONValue &json_file = (*target)["jsonFile"];
        if (!json_file.IsString()) continue;
        const std::string target_file = reply_directory + json_file.GetString();

        {
            std::ifstream file(target_file, std::ios::binary);
            if (!file.is_open()) throw std::runtime_error("Failed to open file " + target_file);
            file.seekg(0, std::ios::end);
            content.resize(file.tellg());
            file.seekg(0, std::ios::beg);
            if (!file.read(&content[0], content.size())) throw std::runtime_error("Failed to read file " + target_file);
        }

        JSONDocument target_document;
        try { target_document.Parse(content.c_str()); }
        catch(...) { throw std::runtime_error("Failed to parse JSON file " + target_file); }

        if (!target_document.IsObject() || !target_document.HasMember("type")) continue;
        const JSONValue &typ = target_document["type"];
        if (!typ.IsString() || std::strcmp(typ.GetString(), "EXECUTABLE") != 0) continue;
        if (!target_document.HasMember("name")) continue;
        const JSONValue &name = target_document["name"];
        if (!name.IsString()) continue;
        if (!target_document.HasMember("nameOnDisk")) continue;
        const JSONValue &name_on_disk = target_document["nameOnDisk"];
        if (!name_on_disk.IsString()) continue;
        if (!target_document.HasMember("paths")) continue;
        const JSONValue &paths = target_document["paths"];
        if (!paths.IsObject() || !paths.HasMember("build")) continue;
        const JSONValue &build = paths["build"];
        if (!build.IsString()) continue;

        std::string path = _system->binary_directory;
        path_append(&path, build.GetString(), true);
        path_append(&path, name_on_disk.GetString(), false);
        executable_targets.push_back({ name.GetString(), _get_relative_path(path) });
    }
    return executable_targets;
}


bool rm::NativeDebugModule::_checkout_string(JSONValue &string, JSONAllocator &allocator, const char *value) const
{
    if (!string.IsString() || std::strcmp(string.GetString(), value) != 0)
    {
        string.SetString(value, allocator);
        return true;
    }
    return false;
}

bool rm::NativeDebugModule::_checkout_debugger_args(JSONValue &debugger_args, JSONAllocator &allocator, const Target &target, bool creation) const
{
    if (creation || !debugger_args.IsArray())
    {
        debugger_args.SetArray();
        debugger_args.PushBack(JSONValue("-iex"), allocator);
        debugger_args.PushBack(JSONValue("set env RETROMAKE_ENV=RETROMAKE_ENV"), allocator);
        debugger_args.PushBack(JSONValue("--args"), allocator);
        debugger_args.PushBack(JSONValue(target.path.c_str(), allocator), allocator);
        debugger_args.PushBack(JSONValue("RETROMAKE_ARG"), allocator);
        return true;
    }
    return false;
}

bool rm::NativeDebugModule::_checkout_configuration(JSONValue &configuration, JSONAllocator &allocator, const Target &target, bool creation) const
{
    bool change = false;
    //retromake_module
    if (!configuration.HasMember("retromake_module")) { configuration.AddMember("retromake_module", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(configuration["retromake_module"], allocator, "Native Debug");
    //name
    const std::string configuration_name = target.name + " (Native Debug, GDB)";
    if (!configuration.HasMember("name")) { configuration.AddMember("name", JSONValue(configuration_name.c_str(), allocator), allocator); change = true; }
    //type
    if (!configuration.HasMember("type")) { configuration.AddMember("type", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(configuration["type"], allocator, "gdb");
    //request
    if (!configuration.HasMember("request")) { configuration.AddMember("request", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(configuration["request"], allocator, "launch");
    //preLaunchTask
    if (creation) { configuration.AddMember("preLaunchTask", JSONValue("retromake-build-task"), allocator); change = true; }
    //target
    if (!configuration.HasMember("target")) { configuration.AddMember("target", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= _checkout_string(configuration["target"], allocator, target.path.c_str());
    //cwd
    std::string path_directory = target.path;
    path_parent(&path_directory);
    if (!configuration.HasMember("cwd")) { configuration.AddMember("cwd", JSONValue(path_directory.c_str(), allocator), allocator); change = true; } //TODO: cwd
    //debugger_args
    if (!configuration.HasMember("debugger_args")) { configuration.AddMember("debugger_args", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_debugger_args(configuration["debugger_args"], allocator, target, creation);
    return change;
}

bool rm::NativeDebugModule::_checkout_configurations(JSONValue &configurations, JSONAllocator &allocator, const std::vector<Target> &targets) const
{
    bool change = false;
    if (!configurations.IsArray()) { configurations.SetArray(); change = true; }
    for (auto target = targets.cbegin(); target != targets.cend(); target++)
    {
        bool target_exists = false;
        for (auto configuration = configurations.Begin(); configuration != configurations.End(); configuration++)
        {
            if (!configuration->IsObject()) continue;
            if (!configuration->HasMember("type")) continue;
            const JSONValue &typ = (*configuration)["type"];
            if (!typ.IsString() || std::strcmp(typ.GetString(), "gdb") != 0) continue;
            if (!configuration->HasMember("request")) continue;
            const JSONValue &request = (*configuration)["request"];
            if (!request.IsString() || std::strcmp(request.GetString(), "launch") != 0) continue;
            if (!configuration->HasMember("target")) continue;
            const JSONValue &target_value = (*configuration)["target"];
            if (!request.IsString() || target_value.GetString() != target->path) continue;
            target_exists = true;
            change |= _checkout_configuration(*configuration, allocator, *target, false);
            break;
        }
        if (!target_exists)
        {
            JSONValue configuration = JSONValue(rapidjson::kObjectType);
            _checkout_configuration(configuration, allocator, *target, true);
            configurations.PushBack(configuration, allocator);
            change = true;
        }

    }
    return change;
}

bool rm::NativeDebugModule::_checkout_document(JSONValue &document, JSONAllocator &allocator, const std::vector<Target> &targets) const
{
    bool change = false;
    if (!document.IsObject()) { document.SetObject(); change = true; }
    if (!document.HasMember("configurations")) { document.AddMember("configurations", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    change |= _checkout_configurations(document["configurations"], allocator, targets);
    return change;
}

rm::Module *rm::NativeDebugModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new NativeDebugModule();

    std::vector<std::string> module_parse = parse(requested_module, false);
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "webfreak") return new NativeDebugModule();
    if (module_parse.size() == 2 && module_parse[0] == "web" && module_parse[1] == "freak") return new NativeDebugModule();
    if (module_parse.size() == 2 && module_parse[0] == "native" && module_parse[1] == "debug") return new NativeDebugModule();
    return nullptr;
}

rm::NativeDebugModule::NativeDebugModule()
{}

int rm::NativeDebugModule::order() const
{
    return 80;
}

std::string rm::NativeDebugModule::id() const
{
    return "nativedebug";
}

std::string rm::NativeDebugModule::name() const
{
    return "Native Debug";
}

std::string rm::NativeDebugModule::help() const
{
    return "WebFreak's debugger for VS Code/Codium";
}

std::vector<std::string> rm::NativeDebugModule::slots() const
{
    return { "debug" };
}

void rm::NativeDebugModule::check(const std::vector<Module*> &modules) const
{
    for (auto module = modules.cbegin(); module != modules.cend(); module++)
    {
        if ((*module)->id() == "vscodium" || (*module)->id() == "vscode") return;
    }
    throw std::runtime_error("Module \"Native Debug\" depends on either \"VS Codium\" or \"VS Code\"");
}

void rm::NativeDebugModule::pre_work(RetroMake *system)
{
    const std::string codemodel_file = system->binary_directory + ".cmake/api/v1/query/codemodel-v2";
    path_ensure(codemodel_file, false, 5);
    if (path_exists(codemodel_file, false, nullptr)) return;
    std::ofstream file(codemodel_file, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open file " + codemodel_file);
}

void rm::NativeDebugModule::post_work(RetroMake *system)
{
    _system = system;
    std::vector<Target> targets = _get_executable_targets();
    JSONDocument document;
    bool change = _read_document(document);
    change |= _checkout_document(document, document.GetAllocator(), targets);
    if (!change) return;

    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document.Accept(writer);

    const std::string launch_file = _system->source_directory + ".vscode/launch.json";
    path_ensure(launch_file, false, 2);
    std::ofstream file(launch_file, std::ios::binary);
    if (!file.write(buffer.GetString(), buffer.GetSize())) throw std::runtime_error("Failed to write file " + launch_file);
}