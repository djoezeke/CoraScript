#include "Scope.hpp"
#include "Value.hpp"
#include "Variable.hpp"

#include <stdexcept>
#include <utility>

namespace cora::compiler
{
    namespace runtime
    {

        Scope::Scope()
            : m_Parent(nullptr), m_IsGlobal(true), m_Kind(ScopeKind::Module), m_Variables() {}

        Scope::Scope(Scope *parent, ScopeKind kind)
            : m_Parent(parent), m_IsGlobal(parent == nullptr), m_Kind(kind), m_Variables() {}

        ScopeKind Scope::GetKind() const { return m_Kind; }
        void Scope::SetKind(ScopeKind kind) { m_Kind = kind; }
        Scope *Scope::GetParent() const { return m_Parent; }
        void Scope::SetParent(Scope *parent)
        {
            m_Parent = parent;
            m_IsGlobal = (parent == nullptr);
        }

        Scope *Scope::ResolveVariable(const std::string &name)
        {
            if (m_Variables.find(name) != m_Variables.end())
            {
                return this;
            }
            if (m_Parent == nullptr)
            {
                return nullptr;
            }
            return m_Parent->ResolveVariable(name);
        }

        Variable *Scope::GetVariable(const std::string &name) const
        {
            auto it = m_Variables.find(name);
            return it == m_Variables.end() ? nullptr : it->second;
        }

        const std::unordered_map<std::string, Variable *> &Scope::GetVariables() const
        {
            return m_Variables;
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

        bool Scope::DeleteVariable(const std::string &name)
        {
            auto it = m_Variables.find(name);
            if (it == m_Variables.end())
            {
                return false;
            }

            delete it->second;
            m_Variables.erase(it);
            return true;
        }

        Variable *Scope::GetVariableValue(const std::string &name) const
        {
            return GetVariable(name);
        }

        void Scope::SetVariableValue(const std::string &name, value *value, bool constant)
        {
            Scope *scope = ResolveVariable(name);
            if (scope == nullptr)
            {
                SetVariable(name, new Variable(value, VariableScope::Local, constant));
                return;
            }

            Variable *target = scope->GetVariable(name);
            if (target == nullptr)
            {
                scope->SetVariable(name, new Variable(value, VariableScope::Local, constant));
                return;
            }

            target->SetValue(value);
        }

        Variable *Scope::NewVariablevalue(const std::string &name, value *value, bool constant)
        {
            return NewVariable(name, new Variable(value, VariableScope::Local, constant));
        }

        bool Scope::isLocal() const { return !m_IsGlobal; }
        bool Scope::isGlobal() const { return m_IsGlobal; }
        bool Scope::isClass() const { return m_Kind == ScopeKind::Class; }
        bool Scope::isBlock() const { return m_Kind == ScopeKind::Block; }
        bool Scope::isModule() const { return m_Kind == ScopeKind::Module; }
        bool Scope::isFunction() const { return m_Kind == ScopeKind::Function; }
        bool Scope::isNamespace() const { return m_Kind == ScopeKind::Namespace; }

        Scope::~Scope()
        {
            for (auto &entry : m_Variables)
            {
                delete entry.second;
            }
        }

        ClassScope::ClassScope(std::string name, Scope *parent)
            : Scope(parent, ScopeKind::Class), m_Name(std::move(name)) {}
        const std::string &ClassScope::GetName() const { return m_Name; }

        ModuleScope::ModuleScope(std::string name, Scope *parent)
            : Scope(parent, ScopeKind::Module), m_Name(std::move(name)) {}
        const std::string &ModuleScope::GetName() const { return m_Name; }

        FunctionScope::FunctionScope(std::string name, Scope *parent)
            : Scope(parent, ScopeKind::Function), m_Name(std::move(name)) {}
        const std::string &FunctionScope::GetName() const { return m_Name; }

        NamespaceScope::NamespaceScope(std::string name, Scope *parent)
            : Scope(parent, ScopeKind::Namespace), m_Name(std::move(name)) {}
        const std::string &NamespaceScope::GetName() const { return m_Name; }

    } // namespace runtime

} // namespace cora::compiler
