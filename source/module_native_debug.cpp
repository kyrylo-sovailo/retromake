#include "../include/module_native_debug.h"
#include "../include/retromake.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cstring>
#include <stdexcept>

rm::NativeDebugModule::Checkout::Checkout(const NativeDebugModule &owner) : owner(owner) {}

void rm::NativeDebugModule::Checkout::checkout_debugger_args(JSONValue &debugger_args_node, const Target &target)
{
    debugger_args_node.SetArray();
    debugger_args_node.PushBack(JSONValue("-iex"), *allocator);
    debugger_args_node.PushBack(JSONValue("set env RETROMAKE_ENV=RETROMAKE_ENV"), *allocator);
    debugger_args_node.PushBack(JSONValue("--args"), *allocator);
    debugger_args_node.PushBack(JSONValue(target.path.c_str(), *allocator), *allocator);
    debugger_args_node.PushBack(JSONValue("RETROMAKE_ARG"), *allocator);
    change = true;
}

void rm::NativeDebugModule::Checkout::checkout_configuration(JSONValue &configuration_node, const Target &target, bool creation)
{
    //retromake_module
    checkout_string(create_string(configuration_node, "retromake_module"), "Native Debug");
    //name
    const std::string configuration_name = target.name + " (Native Debug, " + (owner._lldb ? "LLDB-MI" : "GDB") + ")";
    checkout_string(create_string(configuration_node, "name"), configuration_name);
    //type
    const std::string configuration_type = owner._lldb ? "lldb-mi" : "gdb";
    checkout_string(create_string(configuration_node, "type"), configuration_type);
    //request
    checkout_string(create_string(configuration_node, "request"), "launch");
    //preLaunchTask (can be absent)
    if (creation) checkout_string(create_string(configuration_node, "preLaunchTask"), "retromake-build-task");
    else if (configuration_node.HasMember("preLaunchTask")) create_string(configuration_node, "preLaunchTask");
    //target (can differ)
    bool wrong_format = configuration_node.HasMember("target") && !configuration_node["target"].IsString();
    if (creation || wrong_format) checkout_string(create_string(configuration_node, "target"), target.path);
    //cwd (can differ)
    wrong_format = !configuration_node.HasMember("cwd") || !configuration_node["cwd"].IsString();
    if (creation || wrong_format)
    {
        std::string directory = target.path;
        path_parent(&directory);
        checkout_string(create_string(configuration_node, "cwd"), directory);
    }
    //debugger_args
    wrong_format = !configuration_node.HasMember("debugger_args") || !configuration_node["debugger_args"].IsArray();
    if (creation || wrong_format) checkout_debugger_args(create_array(configuration_node, "debugger_args"), target);
}

void rm::NativeDebugModule::Checkout::checkout_configurations(JSONValue &configurations_node, const std::vector<Target> &targets)
{
    for (auto target = targets.cbegin(); target != targets.cend(); target++)
    {
        bool target_exists = false;
        for (auto configuration = configurations_node.Begin(); configuration != configurations_node.End(); configuration++)
        {
            if (!configuration->IsObject()) continue;
            const JSONValue *typ_node = find_string(*configuration, "type");
            if (typ_node == nullptr || std::strcmp(typ_node->GetString(), "gdb") != 0) continue;
            const JSONValue *request_node = find_string(*configuration, "request");
            if (request_node == nullptr || std::strcmp(request_node->GetString(), "launch") != 0) continue;
            const JSONValue *target_node = find_string(*configuration, "target");
            if (target_node == nullptr || target_node->GetString() != target->path) continue;
            
            checkout_configuration(*configuration, *target, false);
            target_exists = true;
            break;
        }
        if (!target_exists)
        {
            JSONValue configuration_node = JSONValue(rapidjson::kObjectType);
            checkout_configuration(configuration_node, *target, true);
            configurations_node.PushBack(std::move(configuration_node), *allocator);
            change = true;
        }
    }
}

void rm::NativeDebugModule::Checkout::checkout_document(const std::vector<Target> &targets)
{
    if (!document->IsObject()) { document->SetObject(); change = true; }
    JSONValue &configurations_node = create_array(*document, "configurations");
    checkout_configurations(configurations_node, targets);
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

void rm::NativeDebugModule::check(const RetroMake *system) const
{
    for (auto module = system->modules.cbegin(); module != system->modules.cend(); module++)
    {
        if ((*module)->id() == "vscodium" || (*module)->id() == "vscode") return;
    }
    throw std::runtime_error("Module \"Native Debug\" depends on either \"VS Codium\" or \"VS Code\"");
}

void rm::NativeDebugModule::pre_work(RetroMake *system)
{
    codemodel_request(system->binary_directory);
}

void rm::NativeDebugModule::post_work(const RetroMake *system)
{
    //Parse
    std::vector<Target> targets = codemodel_parse(system->binary_directory, system->source_directory, nullptr, false);
    for (auto target = targets.begin(); target != targets.end(); /*target++*/)
    {
        if (target->typ != "EXECUTABLE")
        {
            target = targets.erase(target);
        }
        else
        {
            target->path = "${workspaceFolder}/" + path_relative(target->path, system->source_directory, nullptr);
            target++;
        }
    }
    
    //Checkout
    const std::string launch_file = system->source_directory + ".vscode/launch.json";
    Checkout checkout(*this);
    checkout.document_read(launch_file);
    checkout.checkout_document(targets);
    if (checkout.change) checkout.document_write(launch_file, 2);
}