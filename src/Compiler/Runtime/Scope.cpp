#include "Cora/Compiler/Runtime/Scope.hpp"

#include "Cora/Compiler/Runtime/Value.hpp"
#include "Cora/Compiler/Runtime/Variable.hpp"

#include <stdexcept>

namespace cora
{
    namespace runtime
    {

        Scope::Scope()
        {
            m_Parent = nullptr;
            m_Kind = ScopeKind::Block;
        }

        Scope::~Scope()
        {
            for (auto &entry : m_Variables)
            {
                delete entry.second;
            }
        }

        Variable *Scope::GetVariable(const std::string &name) const
        {
            auto it = m_Variables.find(name);
            return it == m_Variables.end() ? nullptr : it->second;
        }

        void Scope::SetVariable(const std::string &name, Variable *variable)
        {
            auto it = m_Variables.find(name);
            if (it != m_Variables.end())
            {
                delete it->second;
                it->second = variable;
                return;
            }
            m_Variables.emplace(name, variable);
        }

        Variable *Scope::NewVariable(const std::string &name, Variable *variable)
        {
            m_Variables[name] = variable;
            return variable;
        }

        Variable *Scope::GetVariableValue(const std::string &name) const
        {
            return GetVariable(name);
        }

        void Scope::SetVariableValue(const std::string &name, Value *value, bool constant)
        {
            auto *variable = new Variable(value, VariableScope::Local, constant);
            SetVariable(name, variable);
        }

        Variable *Scope::NewVariableValue(const std::string &name, Value *value, bool constant)
        {
            auto *variable = new Variable(value, VariableScope::Local, constant);
            return NewVariable(name, variable);
        }

    } // namespace runtime

} // namespace cora
