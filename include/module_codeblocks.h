#pragma once
#include "module.h"
#include "codemodel.h"
#include "util.h"
#include "xml.h"
#include <map>
#include <set>

namespace rm
{
    class CodeBlocksModule : public Module
    {
    public:
        struct Checkout : XMLEditor
        {
            const CodeBlocksModule &owner;
            const std::string compiler;

            Checkout(const CodeBlocksModule &owner, const std::string &compiler);
            void checkout_target(XMLNode &target_node, const Target &target);
            void checkout_build(XMLNode &build_node, const std::vector<Target> &targets);
            void checkout_document(const std::vector<Target> &targets, const Project &project);
        };

        static Module *create_module(const std::string &requested_module);

        CodeBlocksModule();
        int order() const override;
        std::string id() const override;
        std::string name() const override;
        std::string help() const override;
        std::vector<std::string> slots() const override;
        void check(const RetroMake *system) const override;
        void pre_work(RetroMake *system) override;
        void post_work(const RetroMake *system) override;
    };
}