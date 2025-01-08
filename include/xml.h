#pragma once
#include <string>

#ifdef RETROMAKE_USE_TINYXML
class TiXmlDocument;
class TiXmlNode;
class TiXmlAttribute;
#else
namespace rapidxml
{
    template <typename Ch> class xml_document;
    template <typename Ch> class xml_node;
    template <typename Ch> class xml_attribute;
}
#endif

namespace rm
{
    #ifdef RETROMAKE_USE_TINYXML
    typedef TiXmlDocument XMLDocument;
    typedef TiXmlNode XMLNode;
    typedef TiXmlAttribute XMLAttribute;
    #else
    typedef rapidxml::xml_document<char> XMLDocument;
    typedef rapidxml::xml_node<char> XMLNode;
    typedef rapidxml::xml_attribute<char> XMLAttribute;
    #endif

    struct XMLEditor
    {
        XMLDocument *document;
        bool change;

        XMLEditor();
        void document_read(const std::string &path);
        void document_write(const std::string &path, size_t depth) const;

        void create_declaration(const char *version, const char *encoding, const char *standalone);
        XMLNode &create_root_node(const char *name);
        XMLNode &create_node(XMLNode &parent, const char *name);
        XMLNode &create_node(XMLNode &parent, const char *name, const char *attribute_name);
        XMLNode &create_node(XMLNode &parent, const char *name, const char *attribute_name, const char *attribute_value);
        XMLNode &create_node(XMLNode &parent, const char *name, const char *attribute_name, const std::string &attribute_value);
        void checkout_attribute(XMLNode &node, const char *attribute_name, const char *attribute_value);
        void checkout_attribute(XMLNode &node, const char *attribute_name, const std::string &attribute_value);
        
        ~XMLEditor();
    };
}