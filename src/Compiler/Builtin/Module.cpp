#include "Cora/Compiler/Builtin/Module.hpp"
#include "Cora/Compiler/Builtin/Builtin.hpp"
#include "Cora/Compiler/Builtin/Class.hpp"

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{
    namespace builtin
    {
        Module::Module(std::string name)
            : m_Classes(), m_Functions(), m_Variables(), m_Name(std::move(name)) {}

        Module::Module(std::string name, Classes classes, Functions functions, const Variables variables)
            : m_Classes(std::move(classes)), m_Functions(std::move(functions)), m_Variables(variables), m_Name(std::move(name)) {};

        const std::string &Module::Name() const
        {
            return m_Name;
        }

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