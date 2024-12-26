#pragma once
#include "module.h"
#include "codemodel.h"
#include "util.h"
#include <map>
#include <set>

namespace rm
{
    class CodeBlocksModule : public Module
    {
    public:
        struct Checkout
        {
            const CodeBlocksModule &owner;
            XMLDocument &document;
            bool change;

            Checkout(const CodeBlocksModule &owner, XMLDocument &document);
            XMLNode *create_node(XMLNode *parent, const char *name);
            XMLNode *create_node(XMLNode *parent, const char *name, const char *attribute_name);
            XMLNode *create_node(XMLNode *parent, const char *name, const char *attribute_name, const char *attribute_value);
            void checkout_attribute(XMLNode *node, const char *attribute_name, const char *attribute_value);
            void checkout_target(XMLNode *target_node, const Target &target);
            void checkout_build(XMLNode *build_node, const std::vector<Target> &targets);
            void checkout_document(const std::vector<Target> &targets, const Project &project);
        };

        static Module *create_module(const std::string &requested_module);

        CodeBlocksModule();
        int order() const override;
        std::string id() const override;
        std::string name() const override;
        std::string help() const override;
        std::vector<std::string> slots() const override;
        void check(const std::vector<Module*> &modules) const override;
        void pre_work(RetroMake *system) override;
        void post_work(RetroMake *system) override;
    };
}