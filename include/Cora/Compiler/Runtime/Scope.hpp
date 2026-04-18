#ifndef CORA_COMPILER_RUNTIME_SCOPE_H
#define CORA_COMPILER_RUNTIME_SCOPE_H

#include <string>
#include <unordered_map>

namespace cora::compiler
{
    namespace runtime
    {

        class Value;
        class Variable;

        enum class ScopeKind
        {
            Class,
            Block,
            Module,
            Function,
            Namespace
        };

        class Scope
        {
        public:
            Scope();
            explicit Scope(Scope *parent, ScopeKind kind = ScopeKind::Block);

            Scope(const Scope &) = delete;
            Scope(Scope &&) = default;
            Scope &operator=(const Scope &) = delete;
            Scope &operator=(Scope &&) = default;

            virtual ~Scope();

            ScopeKind GetKind() const;
            void SetKind(ScopeKind kind);

            Scope *GetParent() const;
            void SetParent(Scope *parent);

            Scope *ResolveVariable(const std::string &name);

            Variable *GetVariable(const std::string &name) const;
            const std::unordered_map<std::string, Variable *> &GetVariables() const;
            void SetVariable(const std::string &name, Variable *variable);
            Variable *NewVariable(const std::string &name, Variable *variable);
            bool DeleteVariable(const std::string &name);

            Variable *GetVariableValue(const std::string &name) const;
            void SetVariableValue(const std::string &name, Value *value, bool constant = false);
            Variable *NewVariableValue(const std::string &name, Value *value, bool constant = false);

            [[nodiscard]] bool isLocal() const;
            [[nodiscard]] bool isGlobal() const;
            [[nodiscard]] bool isClass() const;
            [[nodiscard]] bool isBlock() const;
            [[nodiscard]] bool isModule() const;
            [[nodiscard]] bool isFunction() const;
            [[nodiscard]] bool isNamespace() const;

        protected:
            Scope *m_Parent;
            bool m_IsGlobal;
            ScopeKind m_Kind;
            std::unordered_map<std::string, Variable *> m_Variables;
        };

        class ClassScope final : public Scope
        {
        public:
            explicit ClassScope(std::string name = {}, Scope *parent = nullptr);
            const std::string &GetName() const;

        private:
            std::string m_Name;
        };

        class ModuleScope final : public Scope
        {
        public:
            explicit ModuleScope(std::string name = {}, Scope *parent = nullptr);
            const std::string &GetName() const;

        private:
            std::string m_Name;
        };

        class FunctionScope final : public Scope
        {
        public:
            explicit FunctionScope(std::string name = {}, Scope *parent = nullptr);
            const std::string &GetName() const;

        private:
            std::string m_Name;
        };

        class NamespaceScope final : public Scope
        {
        public:
            explicit NamespaceScope(std::string name = {}, Scope *parent = nullptr);
            const std::string &GetName() const;

        private:
            std::string m_Name;
        };

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_SCOPE_H
