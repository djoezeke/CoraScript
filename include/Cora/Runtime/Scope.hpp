#ifndef CORA_RUNTIME_SCOPE_H
#define CORA_RUNTIME_SCOPE_H

#include "Cora/Runtime/Variable.hpp"

#include <string>
#include <unordered_map>

namespace cora
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
            ~Scope();

            ScopeKind GetKind() const { return m_Kind; };
            void SetKind(ScopeKind kind) { m_Kind = kind; };

            Scope *GetParent() const { return m_Parent; };
            void SetParent(Scope *parent) { m_Parent = parent; };

            Scope *ResolveVariable(const std::string &name)
            {
                if (m_Variables.find(name) != m_Variables.end())
                {
                    return this;
                }

                if (m_Parent == nullptr)
                {
                    // throw std::runtime_error("Undefined variable referenced in scope: " + name);
                }

                return m_Parent->ResolveVariable(name);
            }

            Variable *GetVariable(const std::string &name) const;
            void SetVariable(const std::string &name, Variable *variable);
            Variable *NewVariable(const std::string &name, Variable *variable);

            Variable *GetVariableValue(const std::string &name) const;
            void SetVariableValue(const std::string &name, Value *value, bool constant = false);
            Variable *NewVariableValue(const std::string &name, Value *value, bool constant = false);

        private:
            Scope *m_Parent;
            ScopeKind m_Kind;
            std::unordered_map<std::string, Variable> m_Variables;
        };

    } // namespace runtime

} // namespace cora

#endif // CORA_RUNTIME_SCOPE_H
