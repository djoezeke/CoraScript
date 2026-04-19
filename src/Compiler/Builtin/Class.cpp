#include "Cora/Compiler/Builtin/Class.hpp"
#include "Cora/Compiler/Builtin/Builtin.hpp"

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{
    namespace builtin
    {

        Class::Class(std::string name)
            : m_Name(std::move(name)), m_Fields(), m_Methods(), m_PrivateMembers() {}

        Class::Class(std::string name, const Methods methods, const Fields fields)
            : m_Name(std::move(name)), m_Fields(fields), m_Methods(methods), m_PrivateMembers() {};

        const std::string &Class::Name() const
        {
            return m_Name;
        }

        std::shared_ptr<runtime::Scope> Class::Scope() const
        {
            auto scope = std::make_shared<runtime::ClassScope>(m_Name, nullptr);
            auto object = Object();
            for (const auto &field : object->fields)
            {
                scope->SetVariableValue(field.first, new runtime::Value(field.second), false);
            }
            return scope;
        };

        std::shared_ptr<runtime::Object> Class::Object() const
        {
            auto object = MakeObject(m_Name);

            for (auto &method : m_Methods)
            {

                AddMethod(object, method.first, method.second);
            }

            for (auto &field : m_Fields)
            {
                AddField(object, field.first, field.second);
            }

            return object;
        };

        Class &Class::WithField(std::string name, Field field, bool isPrivate)
        {
            if (isPrivate)
            {
                m_PrivateMembers.insert(name);
            }
            else
            {
                m_PrivateMembers.erase(name);
            }

            m_Fields[std::move(name)] = std::move(field);
            return *this;
        }

        Class &Class::WithMethod(std::string name, runtime::Function::Func function, bool isPrivate)
        {
            const std::string methodName = name;
            return WithMethod(std::move(name), std::static_pointer_cast<runtime::Callable>(MakeFunction(methodName, std::move(function))), isPrivate);
        }

        Class &Class::WithMethod(std::string name, Method method, bool isPrivate)
        {
            if (isPrivate)
            {
                m_PrivateMembers.insert(name);
            }
            else
            {
                m_PrivateMembers.erase(name);
            }

            m_Methods[std::move(name)] = std::move(method);
            return *this;
        }

        void Class::AddField(const Obj &object, const std::string &name, Field field, bool isPrivate)
        {
            if (!object)
            {
                return;
            }

            object->fields[name] = std::move(field);
            object->SetMemberVisibility(name, isPrivate);
        };

        void Class::AddMethod(const Obj &object, const std::string &name, Method method, bool isPrivate)
        {
            if (!object)
            {
                return;
            }

            object->fields[name] = runtime::Value(std::move(method));
            object->SetMemberVisibility(name, isPrivate);
        };

        Class::~Class() {};

    } // namespace builtin

} // namespace cora::compiler