#include "../include/xml.h"
#include "../include/util.h"
#include <stdexcept>

#ifdef RETROMAKE_USE_TINYXML
#include <tinyxml.h>
#else
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
#endif

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
    #ifdef RETROMAKE_USE_TINYXML
    const char *result = document->Parse(content.c_str());
    if (result == nullptr) return;
    #else
    try { document->parse<rapidxml::parse_full>(&content[0]); }
    catch(...) { return; }
    #endif
    change = false;
}

void rm::XMLEditor::document_write(const std::string &path, size_t depth) const
{
    path_ensure(path, false, depth);
    std::ofstream file(path, std::ios::binary);
    #ifdef RETROMAKE_USE_TINYXML
    file << *document;
    #else
    std::basic_ostream<char> &basic_file = file;
    rapidxml::print(basic_file, *document); //Doesn't accept plain ofstream for some reason
    #endif
    if (!file.good()) throw std::runtime_error("Failed to write file " + path);
}

void rm::XMLEditor::create_declaration(const char *version, const char *encoding, const char *standalone)
{
    #ifdef RETROMAKE_USE_TINYXML
    XMLNode *declaration = document->FirstChild();
    bool do_change = true;
    if (declaration != nullptr && declaration->Type() == TiXmlNode::TINYXML_DECLARATION)
    {
        TiXmlDeclaration *cast = static_cast<TiXmlDeclaration*>(declaration);
        if (std::strcmp(cast->Version(), version) == 0
        && std::strcmp(cast->Encoding(), encoding) == 0
        && std::strcmp(cast->Standalone(), standalone) == 0)
            do_change = false;
    }
    if (do_change)
    {
        document->Clear();
        declaration = new TiXmlDeclaration(version, encoding, standalone);
        document->LinkEndChild(declaration);
        change = true;
    }
    #else
    XMLNode *declaration = document->first_node();
    if (declaration == nullptr || declaration->type() != rapidxml::node_declaration)
    {
        document->remove_all_nodes();
        declaration = document->allocate_node(rapidxml::node_declaration);
        document->append_node(declaration);
        change = true;
    }
    checkout_attribute(*declaration, "version", version);
    checkout_attribute(*declaration, "encoding", encoding);
    checkout_attribute(*declaration, "standalone", standalone);
    #endif
}

rm::XMLNode &rm::XMLEditor::create_root_node(const char *name)
{
    #ifdef RETROMAKE_USE_TINYXML
    XMLNode *child = document->FirstChild(name);
    if (child == nullptr || child->Type() != TiXmlNode::TINYXML_ELEMENT)
    {
        if (child != nullptr) document->RemoveChild(child);
        child = new TiXmlElement(name);
        document->LinkEndChild(child);
        change = true;
    }
    return *child;
    #else
    return create_node(*document, name);
    #endif
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name)
{
    #ifdef RETROMAKE_USE_TINYXML
    XMLNode *child = parent.FirstChild(name);
    if (child == nullptr || child->Type() != TiXmlNode::TINYXML_ELEMENT)
    {
        if (child != nullptr) parent.RemoveChild(child);
        child = new TiXmlElement(name);
        parent.LinkEndChild(child);
        change = true;
    }
    #else
    XMLNode *child = parent.first_node(name);
    if (child == nullptr || child->type() != rapidxml::node_element)
    {
        if (child != nullptr) parent.remove_node(child);
        child = document->allocate_node(rapidxml::node_element, name);
        parent.append_node(child);
        change = true;
    }
    #endif
    return *child;
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name, const char *attribute_name)
{
    #ifdef RETROMAKE_USE_TINYXML
    XMLNode *child = parent.FirstChild(name);
    while (child != nullptr)
    {
        if (child->Type() == TiXmlNode::TINYXML_ELEMENT)
        {
            const char *attribute = static_cast<TiXmlElement*>(child)->Attribute(attribute_name);
            if (attribute != nullptr) break;
        }
        child = child->NextSibling();
    }
    if (child == nullptr)
    {
        child = new TiXmlElement(name);
        parent.LinkEndChild(child);
        change = true;
    }
    #else
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
    #endif
    return *child;
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name, const char *attribute_name, const char *attribute_value)
{
    #ifdef RETROMAKE_USE_TINYXML
    XMLNode *child = parent.FirstChild(name);
    while (child != nullptr)
    {
        if (child->Type() == TiXmlNode::TINYXML_ELEMENT)
        {
            const char *attribute = static_cast<TiXmlElement*>(child)->Attribute(attribute_name);
            if (attribute != nullptr && std::strcmp(attribute, attribute_value) == 0) break;
        }
        child = child->NextSibling();
    }
    if (child == nullptr)
    {
        TiXmlElement *cast = new TiXmlElement(name);
        parent.LinkEndChild(cast);
        cast->SetAttribute(attribute_name, attribute_value);
        change = true;
        child = cast;
    }
    #else
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
    #endif
    return *child;
}

rm::XMLNode &rm::XMLEditor::create_node(XMLNode &parent, const char *name, const char *attribute_name, const std::string &attribute_value)
{
    #ifdef RETROMAKE_USE_TINYXML
    return create_node(parent, name, attribute_name, attribute_value.c_str());
    #else
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
    #endif
}

void rm::XMLEditor::checkout_attribute(XMLNode &node, const char *attribute_name, const char *attribute_value)
{
    #ifdef RETROMAKE_USE_TINYXML
    if (node.Type() != TiXmlNode::TINYXML_ELEMENT) throw std::logic_error("Cannot set attribute of non-element node");
    TiXmlElement& cast = static_cast<TiXmlElement&>(node);
    const char *attribute = cast.Attribute(attribute_name);
    if (attribute != nullptr && std::strcmp(attribute, attribute_value) == 0) return;
    cast.SetAttribute(attribute_name, attribute_value);
    #else
    XMLAttribute *attribute = node.first_attribute(attribute_name);
    if (attribute != nullptr && std::strcmp(attribute->value(), attribute_value) == 0) return;
    if (attribute != nullptr) node.remove_attribute(attribute);
    attribute = document->allocate_attribute(attribute_name, attribute_value); //do not allocate
    node.append_attribute(attribute);
    #endif
    change = true;
}

void rm::XMLEditor::checkout_attribute(XMLNode &node, const char *attribute_name, const std::string &attribute_value)
{
    #ifdef RETROMAKE_USE_TINYXML
    checkout_attribute(node, attribute_name, attribute_value.c_str());
    #else
    XMLAttribute *attribute = node.first_attribute(attribute_name);
    if (attribute != nullptr && attribute->value() == attribute_value) return;
    if (attribute != nullptr) node.remove_attribute(attribute);
    const char *attribute_value_copy = document->allocate_string(attribute_value.c_str());
    attribute = document->allocate_attribute(attribute_name, attribute_value_copy); //allocate
    node.append_attribute(attribute);
    change = true;
    #endif
}

rm::XMLEditor::~XMLEditor()
{
    if (document != nullptr) { delete document; document = nullptr; }
}