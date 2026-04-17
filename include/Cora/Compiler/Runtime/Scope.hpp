#ifndef CORA_COMPILER_RUNTIME_SCOPE_H
#define CORA_COMPILER_RUNTIME_SCOPE_H

#include <string>
#include <unordered_map>

namespace cora::compiler
{
    namespace runtime
    {
        class Variable;
        class Value;
    }
}

namespace cora::compiler
{
    namespace runtime
    {

        enum class ScopeKind
        {
            Class,
            Block,
            Local,
            Global,
            Module,
            Function,
        };

        class Scope
        {
        public:
            Scope();
            explicit Scope(Scope *parent, ScopeKind kind = ScopeKind::Block);
            ~Scope();

            ScopeKind GetKind() const { return m_Kind; };
            void SetKind(ScopeKind kind) { m_Kind = kind; };

            Scope *GetParent() const { return m_Parent; };
            void SetParent(Scope *parent) { m_Parent = parent; };

            Scope *ResolveVariable(const std::string &name);

            Variable *GetVariable(const std::string &name) const;
            void SetVariable(const std::string &name, Variable *variable);
            Variable *NewVariable(const std::string &name, Variable *variable);

            Variable *GetVariableValue(const std::string &name) const;
            void SetVariableValue(const std::string &name, Value *value, bool constant = false);
            Variable *NewVariableValue(const std::string &name, Value *value, bool constant = false);

        private:
            Scope *m_Parent;
            ScopeKind m_Kind;
            std::unordered_map<std::string, Variable *> m_Variables;
        };

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_SCOPE_H
