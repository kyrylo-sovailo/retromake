#include "../include/codemodel.h"
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
    JSONDocument document;
    if (!document_read(document, codemodel_file)) throw std::runtime_error("Failed to read file " + codemodel_file);

    //Find name
    std::vector<Target> result;
    if (!document.IsObject() || !document.HasMember("configurations")) return result;
    const JSONValue &configurations = document["configurations"];
    if (!configurations.IsArray() || document["configurations"].Size() == 0) return result;
    for (auto configuration = configurations.Begin(); configuration != configurations.End(); configuration++)
    {
        if (!configuration->IsObject() || !configuration->HasMember("targets")) return result;
        std::string configuration_name;
        if (configuration->HasMember("name"))
        {
            const JSONValue &configuration_name_string = (*configuration)["name"];
            if (configuration_name_string.IsString()) configuration_name = configuration_name_string.GetString();
        }

        //Project
        if (project != nullptr)
        {
            const JSONValue &projects = (*configuration)["projects"];
            for (auto _project = projects.Begin(); _project != projects.End(); _project++)
            {
                if (!_project->IsObject() || !_project->HasMember("name")) continue;
                const JSONValue &name = (*_project)["name"];
                if (!name.IsString()) continue;
                project->name = name.GetString();
            }
        }

        //Find targets
        const JSONValue &targets = (*configuration)["targets"];
        for (auto target = targets.Begin(); target != targets.End(); target++)
        {
            if (!target->IsObject() || !target->HasMember("jsonFile")) continue;
            const JSONValue &json_file = (*target)["jsonFile"];
            if (!json_file.IsString()) continue;

            const std::string target_file = reply_directory + json_file.GetString();
            JSONDocument target_document;
            if (!document_read(target_document, target_file)) throw std::runtime_error("Failed to read file " + target_file);

            if (!target_document.IsObject() || !target_document.HasMember("type")) continue;
            const JSONValue &typ = target_document["type"];
            if (!typ.IsString() || (
            std::strcmp(typ.GetString(), "EXECUTABLE") != 0
            && std::strcmp(typ.GetString(), "SHARED_LIBRARY") != 0
            && std::strcmp(typ.GetString(), "STATIC_LIBRARY") != 0)) continue;
            if (!target_document.HasMember("name")) continue;
            const JSONValue &name = target_document["name"];
            if (!name.IsString()) continue;
            if (!target_document.HasMember("paths")) continue;
            const JSONValue &paths = target_document["paths"];
            if (!paths.IsObject() || !paths.HasMember("build")) continue;
            const JSONValue &build = paths["build"];
            if (!build.IsString()) continue;
            if (!target_document.HasMember("artifacts")) continue;
            const JSONValue &artifacts = target_document["artifacts"];
            if (!artifacts.IsArray()) continue;
            const char *artifact_path = nullptr;
            for (auto artifact = artifacts.Begin(); artifact != artifacts.End() && artifact_path == nullptr; artifact++)
            {
                if (!artifact->IsObject() || !artifact->HasMember("path")) continue;
                const JSONValue &path = (*artifact)["path"];
                if (path.IsString()) artifact_path = path.GetString();
            }
            if (artifact_path == nullptr) continue;

            result.push_back(Target());
            Target &result_target = result.back();
            result_target.typ = typ.GetString();
            result_target.name = name.GetString();
            result_target.configuration_name = configuration_name;
            result_target.path = binary_directory;
            path_append(&result_target.path, build.GetString(), true);
            path_append(&result_target.path, artifact_path, false);
            if (!full) continue;

            if (target_document.HasMember("dependencies"))
            {
                const JSONValue &dependencies = target_document["dependencies"];
                if (dependencies.IsArray()) for (auto dependency = dependencies.Begin(); dependency != dependencies.End(); dependency++)
                {
                    if (!dependency->IsObject() || !dependency->HasMember("id")) continue;
                    const JSONValue &id = (*dependency)["id"];
                    if (!id.IsString()) continue;
                    const char *id_c = id.GetString();
                    const char *separator = std::strstr(id_c, "::@");
                    if (separator == nullptr) continue;
                    result_target.dependencies.insert(std::string(id_c, (separator - id_c)));
                }
            }

            if (target_document.HasMember("compileGroups"))
            {
                const JSONValue &compile_groups = target_document["compileGroups"];
                if (compile_groups.IsArray()) for (auto group = compile_groups.Begin(); group != compile_groups.End(); group++)
                {
                    if (!group->IsObject()) continue;
                    if (group->HasMember("language"))
                    {
                        const JSONValue &language = (*group)["language"];
                        if (language.IsString()) result_target.language = language.GetString();
                    }
                    if (group->HasMember("compileCommandFragments"))
                    {
                        const JSONValue &compile_fragments = (*group)["compileCommandFragments"];
                        if (compile_fragments.IsArray()) for (auto fragment = compile_fragments.Begin(); fragment != compile_fragments.End(); fragment++)
                        {
                            if (!fragment->IsObject() || !fragment->HasMember("fragment")) continue;
                            const JSONValue &fragment_string = (*fragment)["fragment"];
                            if (!fragment_string.IsString()) continue;
                            const auto fragment_strings = parse(fragment_string.GetString(), ' ');
                            result_target.options.insert(fragment_strings.cbegin(), fragment_strings.cend());
                        }
                    }
                    if (group->HasMember("defines"))
                    {
                        const JSONValue &defines = (*group)["defines"];
                        if (defines.IsArray()) for (auto define = defines.Begin(); define != defines.End(); define++)
                        {
                            if (!define->IsObject() || !define->HasMember("define")) continue;
                            const JSONValue &define_string = (*define)["define"];
                            if (!define_string.IsString()) continue;
                            const char *define_c = define_string.GetString();
                            const char *equal = std::strchr(define_c, '=');
                            if (equal == nullptr) result_target.defines.insert({define_c, ""});
                            else result_target.defines.insert({std::string(define_c, (equal - define_c)), equal + 1});
                        }
                    }
                    if (group->HasMember("includes"))
                    {
                        const JSONValue &includes = (*group)["includes"];
                        if (includes.IsArray()) for (auto include = includes.Begin(); include != includes.End(); include++)
                        {
                            if (!include->IsObject() || !include->HasMember("path")) continue;
                            const JSONValue &path_string = (*include)["path"];
                            if (!path_string.IsString()) continue;
                            std::string path = source_directory;
                            path_append(&path, path_string.GetString(), false);
                            result_target.directories.insert(path);
                        }
                    }
                }
            }

            if (target_document.HasMember("link"))
            {
                const JSONValue &link = target_document["link"];
                if (link.IsObject() && link.HasMember("commandFragments"))
                {
                    const JSONValue &fragments = link["commandFragments"];
                    if (fragments.IsArray()) for (auto fragment = fragments.Begin(); fragment != fragments.End(); fragment++)
                    {
                        if (!fragment->IsObject() || !fragment->HasMember("role") || !fragment->HasMember("fragment")) continue;
                        const JSONValue &role_string = (*fragment)["role"];
                        const JSONValue &fragment_string = (*fragment)["fragment"];
                        if (!role_string.IsString() || !fragment_string.IsString()) continue;
                        if (std::strcmp(role_string.GetString(), "flags") == 0)
                        {
                            const auto fragment_strings = parse(fragment_string.GetString(), ' ');
                            result_target.linker_options.insert(fragment_strings.cbegin(), fragment_strings.cend());
                        }
                        else if (std::strcmp(role_string.GetString(), "libraries") == 0 && std::strncmp(fragment_string.GetString(), "-W", 2) != 0) //TODO: GCC specific
                        {
                            const auto fragment_strings = parse(fragment_string.GetString(), ' ');
                            result_target.linker_sources.insert(fragment_strings.cbegin(), fragment_strings.cend());
                        }
                    }
                }
            }

            if (target_document.HasMember("sources"))
            {
                const JSONValue &sources = target_document["sources"];
                if (sources.IsArray()) for (auto source = sources.Begin(); source != sources.End(); source++)
                {
                    if (!source->IsObject() || !source->HasMember("compileGroupIndex") || !source->HasMember("path")) continue;
                    const JSONValue &path_string = (*source)["path"];
                    if (!path_string.IsString()) continue;
                    std::string path = source_directory;
                    path_append(&path, path_string.GetString(), false);
                    result_target.sources.insert(path);
                }
            }
        }
    }
    return result;
}