#include "Cora/Compiler/Builtin/Builtin.hpp"

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"
#include "Modules.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace cora::compiler
{
    namespace builtin
    {
        std::shared_ptr<runtime::Object> MakeObject(const std::string &className)
        {
            return std::make_shared<runtime::Object>(className.empty() ? "Object" : className);
        }

        std::shared_ptr<runtime::Method> MakeMethod(const std::shared_ptr<runtime::Object> &object, std::string name, runtime::Method::Func method)
        {
            return std::make_shared<runtime::Method>(object, std::move(name), std::move(method));
        }

        std::shared_ptr<runtime::Function> MakeFunction(std::string name, runtime::Function::Func function)
        {
            return std::make_shared<runtime::Function>(std::move(name), std::move(function));
        }

        void RegisterClass(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant)
        {
            scope.SetVariableValue(name, new runtime::Value(object), constant);
        }

        void RegisterModule(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant)
        {
            scope.SetVariableValue(name, new runtime::Value(object), constant);
        }

        void RegisterFunction(runtime::Scope &scope, const std::string &name, std::function<runtime::Value(const std::vector<runtime::Value> &)> function, bool constant)
        {
            auto callable = std::static_pointer_cast<runtime::Callable>(std::make_shared<runtime::Function>(name, std::move(function)));
            scope.SetVariableValue(name, new runtime::Value(callable), constant);
        }

        void RegisterVariable(runtime::Scope &scope, const std::string &name, runtime::Value value, bool constant)
        {
            scope.SetVariableValue(name, new runtime::Value(std::move(value)), constant);
        }

        static std::string FormatValue(const runtime::Value &value)
        {
            if (value.IsObject())
            {
                auto object = value.AsObject();
                if (!object)
                {
                    return "<object null>";
                }

                auto it = object->fields.find("__str__");
                if (it != object->fields.end() && it->second.IsCallable())
                {
                    auto callable = it->second.AsCallable();
                    if (callable)
                    {
                        return callable->Call({runtime::Value(object)}).AsString();
                    }
                }
            }

            return value.AsString();
        }

        static std::string ReprValue(const runtime::Value &value)
        {
            if (value.IsObject())
            {
                auto object = value.AsObject();
                if (!object)
                {
                    return "<object null>";
                }

                auto reprIt = object->fields.find("__repr__");
                if (reprIt != object->fields.end() && reprIt->second.IsCallable())
                {
                    auto callable = reprIt->second.AsCallable();
                    if (callable)
                    {
                        return callable->Call({runtime::Value(object)}).AsString();
                    }
                }
            }

            return FormatValue(value);
        }

        void Builtins(runtime::Scope &scope)
        {
            RegisterFunction(scope, "print", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 bool first = true;
                                 for (const auto &argument : arguments)
                                 {
                                     if (!first)
                                     {
                                         std::cout << ' ';
                                     }
                                     std::cout << FormatValue(argument);
                                     first = false;
                                 }
                                 std::cout << '\n';
                                 return runtime::Value(nullptr); });

            RegisterFunction(scope, "str", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty())
                                 {
                                     return runtime::Value("");
                                 }
                                 return runtime::Value(FormatValue(arguments.front())); });

            RegisterFunction(scope, "repr", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty())
                                 {
                                     return runtime::Value("null");
                                 }
                                 return runtime::Value(ReprValue(arguments.front())); });

            RegisterFunction(scope, "dir", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty() || !arguments.front().IsObject())
                                 {
                                     return runtime::Value("");
                                 }

                                 auto object = arguments.front().AsObject();
                                 if (!object)
                                 {
                                     return runtime::Value("");
                                 }

                                 std::vector<std::string> names;
                                 names.reserve(object->fields.size());
                                 for (const auto &entry : object->fields)
                                 {
                                     names.push_back(entry.first);
                                 }
                                 std::sort(names.begin(), names.end());

                                 std::ostringstream out;
                                 for (std::size_t i = 0; i < names.size(); ++i)
                                 {
                                     if (i != 0)
                                     {
                                         out << ",";
                                     }
                                     out << names[i];
                                 }

                                 return runtime::Value(out.str()); });

            RegisterFunction(scope, "doc", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty())
                                 {
                                     return runtime::Value("");
                                 }

                                 const runtime::Value &value = arguments.front();
                                 if (value.IsCallable())
                                 {
                                     auto callable = value.AsCallable();
                                     if (!callable)
                                     {
                                         return runtime::Value("");
                                     }
                                     return runtime::Value(callable->Doc());
                                 }

                                 if (value.IsObject())
                                 {
                                     auto object = value.AsObject();
                                     if (!object)
                                     {
                                         return runtime::Value("");
                                     }

                                     auto it = object->fields.find("__doc__");
                                     if (it != object->fields.end())
                                     {
                                         return runtime::Value(it->second.AsString());
                                     }
                                 }

                                 return runtime::Value(""); });

            RegisterFunction(scope, "bool", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty())
                                 {
                                     return runtime::Value(false);
                                 }
                                 return runtime::Value(arguments.front().AsBool()); });

            RegisterFunction(scope, "float", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty())
                                 {
                                     return runtime::Value(0.0);
                                 }
                                 if (arguments.front().IsNumber())
                                 {
                                     return runtime::Value(arguments.front().AsNumber());
                                 }
                                 if (arguments.front().IsBool())
                                 {
                                     return runtime::Value(arguments.front().AsBool() ? 1.0 : 0.0);
                                 }
                                 return runtime::Value(std::stod(arguments.front().AsString())); });

            RegisterFunction(scope, "int", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty())
                                 {
                                     return runtime::Value(0.0);
                                 }
                                 if (arguments.front().IsNumber())
                                 {
                                     return runtime::Value(static_cast<double>(static_cast<long long>(arguments.front().AsNumber())));
                                 }
                                 if (arguments.front().IsBool())
                                 {
                                     return runtime::Value(arguments.front().AsBool() ? 1.0 : 0.0);
                                 }
                                 return runtime::Value(static_cast<double>(std::stoll(arguments.front().AsString()))); });

            RegisterFunction(scope, "string", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                             {
                                 if (arguments.empty())
                                 {
                                     return runtime::Value("");
                                 }
                                 return runtime::Value(FormatValue(arguments.front())); });

            RegisterModule(scope, "io", CoraGetIOModuleObject(), true);
            RegisterModule(scope, "math", CoraGetMathModuleObject(), true);
            RegisterModule(scope, "os", CoraGetOSModuleObject(), true);
            RegisterModule(scope, "exception", CoraGetExceptionModuleObject(), true);
        }

    } // namespace builtin

} // namespace cora::compiler
