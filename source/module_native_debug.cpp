#include "../include/module_native_debug.h"
#include "../include/retromake.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cstring>
#include <stdexcept>

rm::NativeDebugModule::Checkout::Checkout(const NativeDebugModule &owner, JSONAllocator &allocator) : owner(owner), allocator(allocator), change(false) {}

void rm::NativeDebugModule::Checkout::checkout_debugger_args(JSONValue &debugger_args, const Target &target, bool creation)
{
    if (creation || !debugger_args.IsArray())
    {
        debugger_args.SetArray();
        debugger_args.PushBack(JSONValue("-iex"), allocator);
        debugger_args.PushBack(JSONValue("set env RETROMAKE_ENV=RETROMAKE_ENV"), allocator);
        debugger_args.PushBack(JSONValue("--args"), allocator);
        debugger_args.PushBack(JSONValue(target.path.c_str(), allocator), allocator);
        debugger_args.PushBack(JSONValue("RETROMAKE_ARG"), allocator);
        change = true;
    }
}

void rm::NativeDebugModule::Checkout::checkout_configuration(JSONValue &configuration, const Target &target, bool creation)
{
    //retromake_module
    if (!configuration.HasMember("retromake_module")) { configuration.AddMember("retromake_module", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= checkout_string(configuration["retromake_module"], allocator, "Native Debug");
    //name
    const std::string configuration_name = target.name + " (Native Debug, " + (owner._lldb ? "LLDB-MI" : "GDB") + ")";
    if (!configuration.HasMember("name")) { configuration.AddMember("name", JSONValue(configuration_name.c_str(), allocator), allocator); change = true; }
    //type
    if (!configuration.HasMember("type")) { configuration.AddMember("type", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= checkout_string(configuration["type"], allocator, owner._lldb ? "lldb-mi" : "gdb");
    //request
    if (!configuration.HasMember("request")) { configuration.AddMember("request", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= checkout_string(configuration["request"], allocator, "launch");
    //preLaunchTask
    if (creation) { configuration.AddMember("preLaunchTask", JSONValue("retromake-build-task"), allocator); change = true; }
    //target
    if (!configuration.HasMember("target")) { configuration.AddMember("target", JSONValue(rapidjson::kStringType), allocator); change = true; }
    change |= checkout_string(configuration["target"], allocator, target.path.c_str());
    //cwd
    std::string path_directory = target.path;
    path_parent(&path_directory);
    if (!configuration.HasMember("cwd")) { configuration.AddMember("cwd", JSONValue(path_directory.c_str(), allocator), allocator); change = true; } //TODO: cwd
    //debugger_args
    if (!configuration.HasMember("debugger_args")) { configuration.AddMember("debugger_args", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    checkout_debugger_args(configuration["debugger_args"], target, creation);
}

void rm::NativeDebugModule::Checkout::checkout_configurations(JSONValue &configurations, const std::vector<Target> &targets)
{
    if (!configurations.IsArray()) { configurations.SetArray(); change = true; }
    for (auto target = targets.cbegin(); target != targets.cend(); target++)
    {
        bool target_exists = false;
        for (auto configuration = configurations.Begin(); configuration != configurations.End() && !target_exists; configuration++)
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
            checkout_configuration(*configuration, *target, false);
        }
        if (!target_exists)
        {
            JSONValue configuration = JSONValue(rapidjson::kObjectType);
            checkout_configuration(configuration, *target, true);
            configurations.PushBack(std::move(configuration), allocator);
            change = true;
        }
    }
}

void rm::NativeDebugModule::Checkout::checkout_document(JSONValue &document, const std::vector<Target> &targets)
{
    if (!document.IsObject()) { document.SetObject(); change = true; }
    if (!document.HasMember("configurations")) { document.AddMember("configurations", JSONValue(rapidjson::kArrayType), allocator); change = true; }
    checkout_configurations(document["configurations"], targets);
}

rm::Module *rm::NativeDebugModule::create_module(const std::string &requested_module)
{
    if (requested_module.empty()) return new NativeDebugModule(false);

    std::vector<std::string> module_parse = parse(requested_module, '\0');
    for (auto module = module_parse.begin(); module != module_parse.end(); module++) *module = lower(trim(*module));
    if (module_parse.size() == 1 && module_parse[0] == "webfreak") return new NativeDebugModule(false);
    if (module_parse.size() == 2 && module_parse[0] == "web" && module_parse[1] == "freak") return new NativeDebugModule(false);
    if (module_parse.size() == 2 && module_parse[0] == "native" && module_parse[1] == "debug") return new NativeDebugModule(false);

    if (module_parse.size() == 2 && module_parse[0] == "webfreak" && module_parse[1] == "gdb") return new NativeDebugModule(false);
    if (module_parse.size() == 3 && module_parse[0] == "web" && module_parse[1] == "freak" && module_parse[2] == "gdb") return new NativeDebugModule(false);
    if (module_parse.size() == 3 && module_parse[0] == "native" && module_parse[1] == "debug" && module_parse[2] == "gdb") return new NativeDebugModule(false);

    if (module_parse.size() == 2 && module_parse[0] == "webfreak" && module_parse[1] == "lldb") return new NativeDebugModule(true);
    if (module_parse.size() == 3 && module_parse[0] == "web" && module_parse[1] == "freak" && module_parse[2] == "lldb") return new NativeDebugModule(true);
    if (module_parse.size() == 3 && module_parse[0] == "native" && module_parse[1] == "debug" && module_parse[2] == "lldb") return new NativeDebugModule(true);
    return nullptr;
}

rm::NativeDebugModule::NativeDebugModule(bool lldb) : _lldb(lldb)
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
    codemodel_request(system->binary_directory);
}

void rm::NativeDebugModule::post_work(RetroMake *system)
{
    std::vector<Target> targets = codemodel_parse(system->binary_directory, system->source_directory, nullptr, false);
    for (auto target = targets.begin(); target != targets.end(); target++)
    {
        if (target->typ != "EXECUTABLE") target = targets.erase(target);
        else target->path = "${workspaceFolder}/" + path_relative(target->path, system->source_directory, nullptr);
    }
    
    const std::string launch_file = system->source_directory + ".vscode/launch.json";
    JSONDocument document;
    bool change = !document_read(document, launch_file);
    Checkout checkout(*this, document.GetAllocator());
    checkout.checkout_document(document, targets);
    change |= checkout.change;
    if (change) document_write(document, launch_file, 2);
}