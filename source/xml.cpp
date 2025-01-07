#include "../include/xml.h"
#include "../include/util.h"

#include <rapidxml/rapidxml.hpp>
namespace rapidxml { namespace internal {
template <class OutIt, class Ch> inline OutIt print_children(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_attributes(OutIt, const xml_node<Ch>*, int);
template <class OutIt, class Ch> inline OutIt print_data_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_cdata_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_element_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_declaration_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_comment_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_doctype_node(OutIt, const xml_node<Ch>*, int, int);
template <class OutIt, class Ch> inline OutIt print_pi_node(OutIt, const xml_node<Ch>*, int, int);
} }
#include <rapidxml/rapidxml_print.hpp>

#include <cstring>
#include <fstream>

rm::XMLEditor::XMLEditor()
{
    document = new XMLDocument();
    change = true;
}

void rm::XMLEditor::document_read(const std::string &path)
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
    try { document->parse<rapidxml::parse_full>(&content[0]); }
    catch(...) { return; }
    change = false;
}

void rm::XMLEditor::document_write(const std::string &path, size_t depth) const
{
    path_ensure(path, false, depth);
    std::ofstream file(path, std::ios::binary);
    std::basic_ostream<char> &basic_file = file;
    rapidxml::print(basic_file, *document); //Doesn't accept plain ofstream for some reason
    if (!file.good()) throw std::runtime_error("Failed to write file " + path);
}

rm::XMLNode &rm::XMLEditor::create_declaration()
{
    XMLNode *declaration = document->first_node();
    if (declaration == nullptr || declaration->type() != rapidxml::node_declaration)
    {
        document->remove_all_nodes();
        declaration = document->allocate_node(rapidxml::node_declaration);
        document->append_node(declaration);
        change = true;
    }
    return *declaration;
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name)
{
    XMLNode *child = parent.first_node(name);
    if (child == nullptr || child->type() != rapidxml::node_element)
    {
        if (child != nullptr) parent.remove_node(child);
        child = document->allocate_node(rapidxml::node_element, name);
        parent.append_node(child);
        change = true;
    }
    return *child;
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name, const char *attribute_name)
{
    XMLNode *child = parent.first_node(name);
    while (child != nullptr)
    {
        if (child->type() == rapidxml::node_element)
        {
            XMLAttribute *attribute = child->first_attribute(attribute_name);
            if (attribute != nullptr) break;
        }
        child = child->next_sibling(name);
    }
    if (child == nullptr)
    {
        child = document->allocate_node(rapidxml::node_element, name);
        parent.append_node(child);
        change = true;
    }
    return *child;
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name, const char *attribute_name, const char *attribute_value)
{
    XMLNode *child = parent.first_node(name);
    while (child != nullptr)
    {
        if (child->type() == rapidxml::node_element)
        {
            XMLAttribute *attribute = child->first_attribute(attribute_name);
            if (attribute != nullptr && std::strcmp(attribute->value(), attribute_value) == 0) break;
        }
        child = child->next_sibling(name);
    }
    if (child == nullptr)
    {
        child = document->allocate_node(rapidxml::node_element, name);
        parent.append_node(child);
        XMLAttribute *attribute = document->allocate_attribute(attribute_name, attribute_value); //do not allocate
        child->append_attribute(attribute);
        change = true;
    }
    return *child;
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name, const char *attribute_name, const std::string &attribute_value)
{
    XMLNode *child = parent.first_node(name);
    while (child != nullptr)
    {
        if (child->type() == rapidxml::node_element)
        {
            XMLAttribute *attribute = child->first_attribute(attribute_name);
            if (attribute != nullptr && attribute->value() == attribute_value) break;
        }
        child = child->next_sibling(name);
    }
    if (child == nullptr)
    {
        child = document->allocate_node(rapidxml::node_element, name);
        parent.append_node(child);
        const char *attribute_value_copy = document->allocate_string(attribute_value.c_str());
        XMLAttribute *attribute = document->allocate_attribute(attribute_name, attribute_value_copy); //allocate
        child->append_attribute(attribute);
        change = true;
    }
    return *child;
}

void rm::XMLEditor::checkout_attribute(XMLNode &node, const char *attribute_name, const char *attribute_value)
{
    XMLAttribute *attribute = node.first_attribute(attribute_name);
    if (attribute != nullptr && std::strcmp(attribute->value(), attribute_value) == 0) return;
    if (attribute != nullptr) node.remove_attribute(attribute);
    attribute = document->allocate_attribute(attribute_name, attribute_value); //do not allocate
    node.append_attribute(attribute);
    change = true;
}

void rm::XMLEditor::checkout_attribute(XMLNode &node, const char *attribute_name, const std::string &attribute_value)
{
    XMLAttribute *attribute = node.first_attribute(attribute_name);
    if (attribute != nullptr && attribute->value() == attribute_value) return;
    if (attribute != nullptr) node.remove_attribute(attribute);
    const char *attribute_value_copy = document->allocate_string(attribute_value.c_str());
    attribute = document->allocate_attribute(attribute_name, attribute_value_copy); //allocate
    node.append_attribute(attribute);
    change = true;
}

rm::XMLEditor::~XMLEditor()
{
    if (document != nullptr) { delete document; document = nullptr; }
}