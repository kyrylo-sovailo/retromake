#include "../include/json.h"
#include "../include/util.h"

#include <rapidjson/allocators.h>
#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/stringbuffer.h>

#include <fstream>

const rm::JSONValue *rm::find_string(const JSONValue &parent, const char *name)
{
    if (!parent.HasMember(name)) return nullptr;
    const JSONValue &value = parent[name];
    if (!value.IsString()) return nullptr;
    return &value;
}

const rm::JSONValue *rm::find_array(const JSONValue &parent, const char *name)
{
    if (!parent.HasMember(name)) return nullptr;
    const JSONValue &value = parent[name];
    if (!value.IsArray()) return nullptr;
    return &value;
}

const rm::JSONValue *rm::find_object(const JSONValue &parent, const char *name)
{
    if (!parent.HasMember(name)) return nullptr;
    const JSONValue &value = parent[name];
    if (!value.IsObject()) return nullptr;
    return &value;
}

rm::JSONEditor::JSONEditor()
{
    document = new JSONDocument();
    allocator = &document->GetAllocator();
    change = true;
}

void rm::JSONEditor::document_read(const std::string &path)
{
    change = true;

    //Read file
    if (!path_exists(path, false, nullptr)) return;
    std::string content;
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open()) return;
        file.seekg(0, std::ios::end);
        content.resize(file.tellg());
        file.seekg(0, std::ios::beg);
        if (!file.read(&content[0], content.size())) return;
    }

    //Parse file
    try { document->Parse(content.c_str()); }
    catch(...) { return; }
    change = false;
}

void rm::JSONEditor::document_write(const std::string &path, size_t depth) const
{
    rapidjson::StringBuffer buffer;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    document->Accept(writer);

    path_ensure(path, false, depth);
    std::ofstream file(path, std::ios::binary);
    if (!file.write(buffer.GetString(), buffer.GetSize())) throw std::runtime_error("Failed to write file " + path);
}

rm::JSONValue &rm::JSONEditor::create_string(JSONValue &parent, const char *name)
{
    if (!parent.HasMember(name))
    {
        change = true;
        return parent.AddMember(rapidjson::GenericStringRef<char>(name), JSONValue(rapidjson::kStringType), *allocator);
    }
    JSONValue &value = parent[name];
    if (!value.IsString())
    {
        change = true;
        value.SetString("");
    }
    return value;
}

rm::JSONValue &rm::JSONEditor::create_array(JSONValue &parent, const char *name)
{
    if (!parent.HasMember(name))
    {
        change = true;
        return parent.AddMember(rapidjson::GenericStringRef<char>(name), JSONValue(rapidjson::kArrayType), *allocator);
    }
    JSONValue &value = parent[name];
    if (!value.IsArray())
    {
        change = true;
        value.SetArray();
    }
    return value;
}

rm::JSONValue &rm::JSONEditor::create_object(JSONValue &parent, const char *name)
{
    if (!parent.HasMember(name))
    {
        change = true;
        return parent.AddMember(rapidjson::GenericStringRef<char>(name), JSONValue(rapidjson::kObjectType), *allocator);
    }
    JSONValue &value = parent[name];
    if (!value.IsObject())
    {
        change = true;
        value.SetObject();
    }
    return value;
}

void rm::JSONEditor::checkout_string(JSONValue &node, const char *value)
{
    if (!node.IsString() || std::strcmp(node.GetString(), value) != 0)
    {
        node.SetString(rapidjson::GenericStringRef<char>(value)); //do not allocate
        change = true;
    }
}

void rm::JSONEditor::checkout_string(JSONValue &node, const std::string &value)
{
    if (!node.IsString() || node.GetString() != value)
    {
        node.SetString(value.c_str(), *allocator); //allocate
        change = true;
    }
}

rm::JSONEditor::~JSONEditor()
{
    if (document != nullptr) { delete document; document = nullptr; }
}