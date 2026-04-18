#include "Cora/Compiler/Builtin/Builtin.hpp"

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

#include <iostream>
#include <stdexcept>

namespace cora::compiler
{
    namespace builtin
    {

        namespace
        {
            static double ToNumberOrZero(const runtime::Value &value)
            {
                if (value.IsNumber())
                {
                    return value.AsNumber();
                }
                if (value.IsBool())
                {
                    return value.AsBool() ? 1.0 : 0.0;
                }
                if (value.IsNull())
                {
                    return 0.0;
                }

                try
                {
                    return std::stod(value.AsString());
                }
                catch (...)
                {
                    return 0.0;
                }
            }
        } // namespace

        void RegisterBuiltinFunctions(runtime::Scope &scope)
        {
            scope.NewVariableValue("print", new runtime::Value(std::make_shared<runtime::NativeFunction>("print", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                                                                                         {
                bool first = true;
                for (const runtime::Value &argument : arguments)
                {
                    if (!first)
                    {
                        std::cout << ' ';
                    }
                    std::cout << argument.AsString();
                    first = false;
                }
                std::cout << '\n';
                return runtime::Value(nullptr); })),
                                   true);

            scope.NewVariableValue("len", new runtime::Value(std::make_shared<runtime::NativeFunction>("len", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                                                                                       {
                if (arguments.empty())
                {
                    return runtime::Value(0.0);
                }

                const runtime::Value &value = arguments.front();
                if (value.IsString())
                {
                    return runtime::Value(static_cast<double>(value.AsString().size()));
                }
                if (value.IsObject() && value.AsObject() != nullptr)
                {
                    return runtime::Value(static_cast<double>(value.AsObject()->fields.size()));
                }
                return runtime::Value(0.0); })),
                                   true);

            scope.NewVariableValue("type", new runtime::Value(std::make_shared<runtime::NativeFunction>("type", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                                                                                        {
                if (arguments.empty())
                {
                    return runtime::Value("undefined");
                }
                return runtime::Value(arguments.front().GetValueKindString()); })),
                                   true);

            scope.NewVariableValue("str", new runtime::Value(std::make_shared<runtime::NativeFunction>("str", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                                                                                       {
                if (arguments.empty())
                {
                    return runtime::Value("");
                }
                return runtime::Value(arguments.front().AsString()); })),
                                   true);

            scope.NewVariableValue("int", new runtime::Value(std::make_shared<runtime::NativeFunction>("int", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                                                                                       {
                if (arguments.empty())
                {
                    return runtime::Value(0.0);
                }
                const double number = ToNumberOrZero(arguments.front());
                return runtime::Value(static_cast<double>(static_cast<long long>(number))); })),
                                   true);

            scope.NewVariableValue("float", new runtime::Value(std::make_shared<runtime::NativeFunction>("float", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                                                                                         {
                if (arguments.empty())
                {
                    return runtime::Value(0.0);
                }
                return runtime::Value(ToNumberOrZero(arguments.front())); })),
                                   true);

            scope.NewVariableValue("bool", new runtime::Value(std::make_shared<runtime::NativeFunction>("bool", [](const std::vector<runtime::Value> &arguments) -> runtime::Value
                                                                                                        {
                if (arguments.empty())
                {
                    return runtime::Value(false);
                }
                return runtime::Value(arguments.front().AsBool()); })),
                                   true);

            scope.NewVariableValue("object", new runtime::Value(std::make_shared<runtime::NativeFunction>("object", [](const std::vector<runtime::Value> &) -> runtime::Value
                                                                                                          {
                auto object = std::make_shared<runtime::Object>("object");
                object->fields["__init__"] = runtime::Value(std::make_shared<runtime::NativeFunction>("object.__init__", [](const std::vector<runtime::Value> &) -> runtime::Value
                                                                                                         { return runtime::Value(nullptr); }));
                object->fields["__del__"] = runtime::Value(std::make_shared<runtime::NativeFunction>("object.__del__", [](const std::vector<runtime::Value> &) -> runtime::Value
                                                                                                        { return runtime::Value(nullptr); }));
                return runtime::Value(object); })),
                                   true);

            scope.NewVariableValue("dict", new runtime::Value(std::make_shared<runtime::NativeFunction>("dict", [](const std::vector<runtime::Value> &) -> runtime::Value
                                                                                                        {
                auto object = std::make_shared<runtime::Object>("dict");
                object->fields["__init__"] = runtime::Value(std::make_shared<runtime::NativeFunction>("dict.__init__", [](const std::vector<runtime::Value> &) -> runtime::Value
                                                                                                       { return runtime::Value(nullptr); }));
                object->fields["__del__"] = runtime::Value(std::make_shared<runtime::NativeFunction>("dict.__del__", [](const std::vector<runtime::Value> &) -> runtime::Value
                                                                                                      { return runtime::Value(nullptr); }));
                return runtime::Value(object); })),
                                   true);
        }

    } // namespace builtin

} // namespace cora::compiler