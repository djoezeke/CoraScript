#include "Class.hpp"
#include "Builtin.hpp"

#include "../Runtime/Scope.hpp"
#include "../Runtime/Value.hpp"

namespace cora::compiler
{
    namespace builtin
    {

        Class::Class(std::string name)
            : Class(std::move(name), {}, {}, {}) {}

        Class::Class(std::string name, const Methods methods, const Fields fields, std::string doc)
            : m_Doc(std::move(doc)), m_Name(std::move(name)), m_Fields(fields), m_Methods(methods), m_PrivateMembers() {};

        const std::string &Class::Name() const
        {
            return m_Name;
        };

        const std::string &Class::Doc() const
        {
            return m_Name;
        };

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

            AddField(object, "__doc__", runtime::Value(Doc()));

            auto str_method = MakeMethod(object, "__str__", [object](const std::vector<runtime::Value> &) -> runtime::Value
                                         { return runtime::Value("<object " + object->Name() + ">"); });

            auto get_method = MakeMethod(object, "__get__", [object](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                         {                         if (arguments.empty())
                        {
                            return runtime::Value(nullptr);
                        }
                        const std::string key = arguments.front().AsString();
                        auto it = object->fields.find(key);
                        if (it == object->fields.end())
                        {
                            return runtime::Value(nullptr);
                        }
                        return it->second; });

            auto set_method = MakeMethod(object, "__set__", [object](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                         {                         
                        if (arguments.size() < 2)
                        {
                            return runtime::Value(nullptr);
                        }
                        const std::string key = arguments[0].AsString();
                        object->fields[key] = arguments[1];
                        return arguments[1]; });

            auto dir_method = MakeMethod(object, "__dir__", [object](const std::vector<runtime::Value> &) -> runtime::Value
                                         {                         
                                            std::string result;
                        bool first = true;
                        for (const auto &entry : object->fields)
                        {
                            if (!first)
                            {
                                result += ",";
                            }
                            result += entry.first;
                            first = false;
                        }
                        return runtime::Value(result); });

            AddMethod(object, "__str__", str_method);
            AddMethod(object, "__set__", set_method);
            AddMethod(object, "__get__", get_method);
            AddMethod(object, "__dir__", dir_method);

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