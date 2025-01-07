#include "../include/codemodel.h"
#include "../include/json.h"
#include "../include/util.h"

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <cstring>
#include <fstream>
#include <stdexcept>

void rm::codemodel_request(const std::string &binary_directory)
{
    const std::string codemodel_file = binary_directory + ".cmake/api/v1/query/codemodel-v2";
    path_ensure(codemodel_file, false, 5);
    if (path_exists(codemodel_file, false, nullptr)) return;
    std::ofstream file(codemodel_file, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Failed to open file " + codemodel_file);
}

std::vector<rm::Target> rm::codemodel_parse(const std::string &binary_directory, const std::string &source_directory, Project *project, bool full)
{
    //Find codemodel file
    const std::string reply_directory = binary_directory + ".cmake/api/v1/reply/";
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
    JSONEditor editor;
    editor.document_read(codemodel_file);
    if (editor.change) throw std::runtime_error("Failed to read file " + codemodel_file);
    std::vector<Target> result;

    //Find name
    if (!editor.document->IsObject()) return result;
    const JSONValue *configurations_node = find_array(*editor.document, "configurations");
    if (configurations_node == nullptr) return result;
    for (auto configuration_node = configurations_node->Begin(); configuration_node != configurations_node->End(); configuration_node++)
    {
        if (!configuration_node->IsObject()) continue;
        const JSONValue *name_node = find_string(*configuration_node, "name");
        const JSONValue *targets_node = find_array(*configuration_node, "targets");
        const JSONValue *projects_node = find_array(*configuration_node, "projects");
        if (targets_node == nullptr || projects_node == nullptr) continue;

        //Name
        const std::string name = (name_node != nullptr) ? name_node->GetString() : "";

        //Project
        if (project != nullptr)
        {
            for (auto project_node = projects_node->Begin(); project_node != projects_node->End(); project_node++)
            {
                if (!project_node->IsObject()) continue;
                const JSONValue *name_node = find_string(*project_node, "name");
                if (name_node != nullptr && name_node->GetStringLength() > 0) project->name = name_node->GetString();
            }
        }

        //Targets
        for (auto target_node = targets_node->Begin(); target_node != targets_node->End(); target_node++)
        {
            if (!target_node->IsObject()) continue;
            const JSONValue *json_file_node = find_string(*target_node, "jsonFile");
            if (json_file_node == nullptr) continue;
            const std::string target_file = reply_directory + json_file_node->GetString();
            JSONEditor target_editor;
            target_editor.document_read(target_file);
            if (target_editor.change || !target_editor.document->IsObject())
                throw std::runtime_error("Failed to read file " + target_file);

            const JSONValue *type_node = find_string(*target_editor.document, "type");
            if (type_node == nullptr) continue;
            if (std::strcmp(type_node->GetString(), "EXECUTABLE") != 0
            && std::strcmp(type_node->GetString(), "SHARED_LIBRARY") != 0
            && std::strcmp(type_node->GetString(), "STATIC_LIBRARY") != 0) continue;
            const JSONValue *name_node = find_string(*target_editor.document, "name");
            if (name_node == nullptr) continue;
            const JSONValue *paths_node = find_object(*target_editor.document, "paths");
            if (paths_node == nullptr) continue;
            const JSONValue *paths_build_node = find_string(*paths_node, "build");
            if (paths_build_node == nullptr) continue;
            const JSONValue *artifacts_node = find_array(*target_editor.document, "artifacts");
            if (artifacts_node == nullptr) continue;
            const char *artifact_path = nullptr;
            for (auto artifact_node = artifacts_node->Begin(); artifact_node != artifacts_node->End(); artifact_node++)
            {
                if (!artifact_node->IsObject()) continue;
                const JSONValue *path_node = find_string(*artifact_node, "path");
                if (path_node == nullptr) continue;
                artifact_path = path_node->GetString();
                break;
            }
            if (artifact_path == nullptr) continue;

            result.push_back(Target());
            Target &result_target = result.back();
            result_target.typ = type_node->GetString();
            result_target.name = name_node->GetString();
            result_target.configuration_name = name;
            result_target.path = binary_directory;
            path_append(&result_target.path, paths_build_node->GetString(), true);
            path_append(&result_target.path, artifact_path, false);
            if (!full) continue;

            const JSONValue *dependencies_node = find_array(*target_editor.document, "dependencies");
            if (dependencies_node != nullptr)
            for (auto dependency_node = dependencies_node->Begin(); dependency_node != dependencies_node->End(); dependency_node++)    
            {
                if (!dependency_node->IsObject()) continue;
                const JSONValue *id_node = find_string(*dependency_node, "id");
                if (id_node == nullptr) continue;
                const char *id = id_node->GetString();
                const char *separator = std::strstr(id, "::@");
                if (separator == nullptr) continue;
                result_target.dependencies.insert(std::string(id, (separator - id)));
            };

            const JSONValue *compile_groups_node = find_array(*target_editor.document, "compileGroups");
            if (compile_groups_node != nullptr)
            for (auto group_node = compile_groups_node->Begin(); group_node != compile_groups_node->End(); group_node++)
            {
                if (!group_node->IsObject()) continue;
                const JSONValue *language_node = find_string(*group_node, "language");
                if (language_node != nullptr) result_target.language = language_node->GetString();
                
                const JSONValue *fragments_node = find_array(*group_node, "compileCommandFragments");
                if (fragments_node != nullptr)
                for (auto fragment_node = fragments_node->Begin(); fragment_node != fragments_node->End(); fragment_node++)
                {
                    if (!fragment_node->IsObject()) continue;
                    const JSONValue *frag_node = find_string(*fragment_node, "fragment");
                    if (frag_node == nullptr) continue;
                    const auto frag = parse(frag_node->GetString(), ' ');
                    result_target.options.insert(frag.cbegin(), frag.cend());
                };
                const JSONValue *defines_node = find_array(*group_node, "defines");
                if (defines_node != nullptr)
                for (auto define_node = defines_node->Begin(); define_node != defines_node->End(); define_node++)
                {
                    if (!define_node->IsObject()) continue;
                    const JSONValue *def_node = find_string(*define_node, "define");
                    if (def_node == nullptr) continue;
                    const char *def = def_node->GetString();
                    const char *equal = std::strchr(def, '=');
                    if (equal == nullptr) result_target.defines.insert({def, ""});
                    else result_target.defines.insert({std::string(def, (equal - def)), equal + 1});
                };
                const JSONValue *includes_node = find_array(*group_node, "includes");
                if (includes_node != nullptr)
                for (auto include_node = includes_node->Begin(); include_node != includes_node->End(); include_node++)
                {
                    if (!include_node->IsObject()) continue;
                    const JSONValue *path_node = find_string(*include_node, "path");
                    if (path_node == nullptr) continue;
                    std::string path = source_directory;
                    path_append(&path, path_node->GetString(), false);
                    result_target.directories.insert(path);
                }
            };

            const JSONValue *link_node = find_object(*target_editor.document, "link");
            if (link_node != nullptr)
            {
                const JSONValue *fragments_node = find_array(*link_node, "commandFragments");
                if (fragments_node != nullptr)
                for (auto fragment_node = fragments_node->Begin(); fragment_node != fragments_node->End(); fragment_node++)
                {
                    if (!fragment_node->IsObject()) continue;
                    const JSONValue *role_node = find_string(*fragment_node, "role");
                    const JSONValue *frag_node = find_string(*fragment_node, "fragment");
                    if (role_node == nullptr || frag_node == nullptr) continue;
                    if (std::strcmp(role_node->GetString(), "flags") == 0)
                    {
                        const auto frag = parse(frag_node->GetString(), ' ');
                        result_target.linker_options.insert(frag.cbegin(), frag.cend());
                    }
                    else if (std::strcmp(role_node->GetString(), "libraries") == 0 && std::strncmp(fragment_node->GetString(), "-W", 2) != 0) //TODO: GCC specific
                    {
                        const auto frag = parse(frag_node->GetString(), ' ');
                        result_target.linker_sources.insert(frag.cbegin(), frag.cend());
                    }
                }
            }

            const JSONValue *sources_node = find_array(*target_editor.document, "sources");
            if (sources_node != nullptr)
            for (auto source_node = sources_node->Begin(); source_node != sources_node->End(); source_node++)
            {
                if (!source_node->IsObject() || !source_node->HasMember("compileGroupIndex")) continue;
                const JSONValue *path_node = find_string(*source_node, "path");
                if (path_node == nullptr) continue;
                std::string path = source_directory;
                path_append(&path, path_node->GetString(), false);
                result_target.sources.insert(path);
            }
        }
    }
    return result;
}