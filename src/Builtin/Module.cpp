#include "Module.hpp"
#include "Builtin.hpp"
#include "Class.hpp"

#include "../Runtime/Scope.hpp"
#include "../Runtime/Value.hpp"

namespace cora::compiler
{
    namespace builtin
    {

        Module::Module(std::string name, Classes classes, Functions functions, const Variables variables, std::string doc)
            : m_Name(std::move(name)), m_Classes(std::move(classes)), m_Functions(std::move(functions)), m_Variables(variables), m_Doc(std::move(doc)) {};

        const std::string &Module::Name() const
        {
            return m_Name;
        };

        const std::string &Module::Doc() const
        {
            return m_Doc;
        };

        std::shared_ptr<runtime::Scope> Module::Scope() const
        {
            auto scope = std::make_shared<runtime::ModuleScope>(m_Name, nullptr);
            auto object = Object();
            for (const auto &field : object->fields)
            {
                scope->SetVariableValue(field.first, new runtime::Value(field.second), false);
            }
            return scope;
        };

        std::shared_ptr<runtime::Object> Module::Object() const
        {
            auto object = MakeObject(m_Name);

            AddVariable(object, "__doc__", runtime::Value(Doc()));

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

            AddFunction(object, "__str__", str_method);
            AddFunction(object, "__set__", set_method);
            AddFunction(object, "__get__", get_method);
            AddFunction(object, "__dir__", dir_method);

            for (auto &cls : m_Classes)
            {
                AddClass(object, cls.first, cls.second);
            }

            for (auto &function : m_Functions)
            {
                AddFunction(object, function.first, function.second);
            }

            for (auto &variable : m_Variables)
            {
                AddVariable(object, variable.first, variable.second);
            }

            return object;
        };

        Module &Module::WithClass(std::string name, Class cls)
        {
            m_Classes[std::move(name)] = std::move(cls);
            return *this;
        }

        Module &Module::WithFunction(std::string name, runtime::Function::Func function)
        {
            const std::string functionName = name;
            return WithFunction(std::move(name), std::static_pointer_cast<runtime::Callable>(MakeFunction(functionName, std::move(function))));
        }

        Module &Module::WithFunction(std::string name, Function function)
        {
            m_Functions[std::move(name)] = std::move(function);
            return *this;
        }

        Module &Module::WithVariable(std::string name, Variable variable)
        {
            m_Variables[std::move(name)] = std::move(variable);
            return *this;
        }

        void Module::AddClass(const Obj &object, const std::string &name, Class cls)
        {
            if (!object)
            {
                return;
            }

            object->fields[name] = runtime::Value(cls.Object());
            object->SetMemberVisibility(name, false);
        };

        void Module::AddFunction(const Obj &object, const std::string &name, Function function)
        {
            if (!object)
            {
                return;
            }

            object->fields[name] = runtime::Value(std::move(function));
            object->SetMemberVisibility(name, false);
        };

        void Module::AddVariable(const Obj &object, const std::string &name, Variable variable)
        {
            if (!object)
            {
                return;
            }

            object->fields[name] = std::move(variable);
            object->SetMemberVisibility(name, false);
        };

        Module::~Module() {};

    } // namespace builtin

} // namespace cora::compiler