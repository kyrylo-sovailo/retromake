#pragma once
#include <string>

namespace rapidxml
{
    template <typename Ch> class xml_document;
    template <typename Ch> class xml_node;
    template <typename Ch> class xml_attribute;
}

namespace rm
{
    typedef rapidxml::xml_document<char> XMLDocument;
    typedef rapidxml::xml_node<char> XMLNode;
    typedef rapidxml::xml_attribute<char> XMLAttribute;

    struct XMLEditor
    {
        XMLDocument *document;
        bool change;

        XMLEditor();
        void document_read(const std::string &path);
        void document_write(const std::string &path, size_t depth) const;

        XMLNode &create_declaration();
        XMLNode &create_node(XMLNode &parent, const char *name);
        XMLNode &create_node(XMLNode &parent, const char *name, const char *attribute_name);
        XMLNode &create_node(XMLNode &parent, const char *name, const char *attribute_name, const char *attribute_value);
        XMLNode &create_node(XMLNode &parent, const char *name, const char *attribute_name, const std::string &attribute_value);
        void checkout_attribute(XMLNode &node, const char *attribute_name, const char *attribute_value);
        void checkout_attribute(XMLNode &node, const char *attribute_name, const std::string &attribute_value);
        
        ~XMLEditor();
    };
}