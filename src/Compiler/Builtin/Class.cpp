#include "Cora/Compiler/Builtin/Builtin.hpp"

#include "Cora/Compiler/Runtime/Scope.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unordered_set>

namespace cora::compiler
{
    namespace builtin
    {

        namespace
        {
            using Value = runtime::Value;
            using Object = runtime::Object;
            using NativeFn = std::function<Value(const std::vector<Value> &)>;

            static double ToNumberOrZero(const Value &value)
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

            static std::string ToLower(std::string text)
            {
                for (char &ch : text)
                {
                    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                }
                return text;
            }

            static std::string ToUpper(std::string text)
            {
                for (char &ch : text)
                {
                    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
                }
                return text;
            }

            static std::string Trim(std::string text)
            {
                std::size_t start = 0;
                while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0)
                {
                    ++start;
                }

                std::size_t end = text.size();
                while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0)
                {
                    --end;
                }

                text = text.substr(start, end - start);
                return text;
            }

            static bool IsHiddenMember(const std::string &name)
            {
                return name.rfind("__", 0) == 0;
            }

            static std::shared_ptr<Object> RequireObject(const std::vector<Value> &arguments, std::size_t index = 0)
            {
                if (index >= arguments.size())
                {
                    return nullptr;
                }

                if (!arguments[index].IsObject())
                {
                    return nullptr;
                }

                return arguments[index].AsObject();
            }

            static std::size_t ToIndex(const Value &value)
            {
                const double number = ToNumberOrZero(value);
                if (number <= 0.0)
                {
                    return 0;
                }
                return static_cast<std::size_t>(number);
            }

            static std::string JoinNames(const std::vector<std::string> &names)
            {
                std::ostringstream out;
                out << '[';
                for (std::size_t i = 0; i < names.size(); ++i)
                {
                    if (i != 0)
                    {
                        out << ", ";
                    }
                    out << names[i];
                }
                out << ']';
                return out.str();
            }

            static std::vector<std::string> VisibleNames(const Object &object, const std::unordered_set<std::string> &ignored = {})
            {
                std::vector<std::string> names;
                for (const auto &entry : object.fields)
                {
                    if (IsHiddenMember(entry.first))
                    {
                        continue;
                    }
                    if (ignored.find(entry.first) != ignored.end())
                    {
                        continue;
                    }
                    names.push_back(entry.first);
                }

                std::sort(names.begin(), names.end());
                return names;
            }

            static std::size_t VisibleCount(const Object &object, const std::unordered_set<std::string> &ignored = {})
            {
                return VisibleNames(object, ignored).size();
            }

            static bool GetNumericField(const std::shared_ptr<Object> &object, const std::string &name, double &out)
            {
                if (!object)
                {
                    return false;
                }

                auto it = object->fields.find(name);
                if (it == object->fields.end() || !it->second.IsNumber())
                {
                    return false;
                }

                out = it->second.AsNumber();
                return true;
            }

            static double GetCountFieldOrVisibleSize(const std::shared_ptr<Object> &object, const std::unordered_set<std::string> &ignored = {})
            {
                if (!object)
                {
                    return 0.0;
                }

                double count = 0.0;
                if (GetNumericField(object, "__length__", count))
                {
                    return count;
                }

                return static_cast<double>(VisibleCount(*object, ignored));
            }

            static Value GetFieldOrNull(const std::shared_ptr<Object> &object, const std::string &name)
            {
                if (!object)
                {
                    return Value(nullptr);
                }

                auto it = object->fields.find(name);
                if (it == object->fields.end())
                {
                    return Value(nullptr);
                }
                return it->second;
            }

            static void SetField(const std::shared_ptr<Object> &object, const std::string &name, const Value &value)
            {
                if (!object)
                {
                    return;
                }
                object->fields[name] = value;
            }

            static void ClearVisibleFields(const std::shared_ptr<Object> &object, const std::unordered_set<std::string> &ignored = {})
            {
                if (!object)
                {
                    return;
                }

                std::vector<std::string> names;
                for (const auto &entry : object->fields)
                {
                    if (IsHiddenMember(entry.first))
                    {
                        continue;
                    }
                    if (ignored.find(entry.first) != ignored.end())
                    {
                        continue;
                    }
                    names.push_back(entry.first);
                }

                for (const std::string &name : names)
                {
                    object->fields.erase(name);
                }
            }

            static std::string ObjectToString(const std::shared_ptr<Object> &object, const std::unordered_set<std::string> &ignored = {})
            {
                if (!object)
                {
                    return "<object null>";
                }

                std::ostringstream out;
                out << '<' << object->className;
                const std::vector<std::string> names = VisibleNames(*object, ignored);
                if (!names.empty())
                {
                    out << ' ' << JoinNames(names);
                }
                out << '>';
                return out.str();
            }

            static std::string ListItemKey(std::size_t index)
            {
                return "__item_" + std::to_string(index);
            }

            static std::size_t ListLength(const std::shared_ptr<Object> &object)
            {
                return static_cast<std::size_t>(GetCountFieldOrVisibleSize(object));
            }

            static Value ListGetItem(const std::shared_ptr<Object> &object, std::size_t index)
            {
                if (!object)
                {
                    return Value(nullptr);
                }

                const std::string key = ListItemKey(index);
                auto it = object->fields.find(key);
                if (it == object->fields.end())
                {
                    return Value(nullptr);
                }
                return it->second;
            }

            static void ListSetItem(const std::shared_ptr<Object> &object, std::size_t index, const Value &value)
            {
                if (!object)
                {
                    return;
                }

                object->fields[ListItemKey(index)] = value;
                const double newLength = static_cast<double>(index + 1);
                double currentLength = 0.0;
                if (!GetNumericField(object, "__length__", currentLength) || newLength > currentLength)
                {
                    object->fields["__length__"] = Value(newLength);
                }
            }

            static void ListAppend(const std::shared_ptr<Object> &object, const Value &value)
            {
                if (!object)
                {
                    return;
                }

                const std::size_t index = ListLength(object);
                ListSetItem(object, index, value);
            }

            static Value ListPop(const std::shared_ptr<Object> &object)
            {
                if (!object)
                {
                    return Value(nullptr);
                }

                const std::size_t length = ListLength(object);
                if (length == 0)
                {
                    return Value(nullptr);
                }

                const std::size_t index = length - 1;
                Value value = ListGetItem(object, index);
                object->fields.erase(ListItemKey(index));
                object->fields["__length__"] = Value(static_cast<double>(index));
                return value;
            }

            static std::string ListToString(const std::shared_ptr<Object> &object)
            {
                if (!object)
                {
                    return "[]";
                }

                std::ostringstream out;
                out << '[';
                const std::size_t length = ListLength(object);
                for (std::size_t i = 0; i < length; ++i)
                {
                    if (i != 0)
                    {
                        out << ", ";
                    }
                    out << ListGetItem(object, i).AsString();
                }
                out << ']';
                return out.str();
            }

            static std::size_t RangeLength(double start, double stop, double step)
            {
                if (step == 0.0)
                {
                    return 0;
                }

                std::size_t count = 0;
                if (step > 0.0)
                {
                    for (double current = start; current < stop; current += step)
                    {
                        ++count;
                    }
                }
                else
                {
                    for (double current = start; current > stop; current += step)
                    {
                        ++count;
                    }
                }
                return count;
            }

            static std::shared_ptr<Object> CreateContainerObject(const std::string &className)
            {
                auto object = MakeBuiltinObject(className);

                AddBuiltinMethod(object, "get", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 2)
                    {
                        return Value(nullptr);
                    }

                    const std::string key = arguments[1].AsString();
                    return GetFieldOrNull(object, key); });

                AddBuiltinMethod(object, "set", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 3)
                    {
                        return Value(nullptr);
                    }

                    const std::string key = arguments[1].AsString();
                    const Value &value = arguments[2];
                    SetField(object, key, value);
                    return value; });

                AddBuiltinMethod(object, "has", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 2)
                    {
                        return Value(false);
                    }

                    const std::string key = arguments[1].AsString();
                    return Value(object->fields.find(key) != object->fields.end()); });

                AddBuiltinMethod(object, "delete", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 2)
                    {
                        return Value(false);
                    }

                    const std::string key = arguments[1].AsString();
                    return Value(object->fields.erase(key) > 0); });

                AddBuiltinMethod(object, "clear", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value(nullptr);
                    }

                    ClearVisibleFields(object, {"get", "set", "has", "delete", "clear", "keys", "len", "toString", "contains", "update"});
                    return Value(nullptr); });

                AddBuiltinMethod(object, "keys", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("[]");
                    }

                    const std::vector<std::string> keys = VisibleNames(*object, {"get", "set", "has", "delete", "clear", "keys", "len", "toString", "contains", "update"});
                    return Value(JoinNames(keys)); });

                AddBuiltinMethod(object, "contains", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 2)
                    {
                        return Value(false);
                    }

                    const std::string key = arguments[1].AsString();
                    return Value(object->fields.find(key) != object->fields.end()); });

                AddBuiltinMethod(object, "update", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    auto source = RequireObject(arguments, 1);
                    if (!object || !source)
                    {
                        return Value(nullptr);
                    }

                    for (const auto &entry : source->fields)
                    {
                        if (IsHiddenMember(entry.first))
                        {
                            continue;
                        }
                        object->fields[entry.first] = entry.second;
                    }
                    return Value(nullptr); });

                AddBuiltinMethod(object, "len", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    return Value(object ? static_cast<double>(VisibleCount(*object, {"get", "set", "has", "delete", "clear", "keys", "len", "toString", "contains", "update"})) : 0.0); });

                AddBuiltinMethod(object, "toString", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    return Value(ObjectToString(object, {"get", "set", "has", "delete", "clear", "keys", "len", "toString", "contains", "update"})); });

                return object;
            }

            static std::shared_ptr<Object> CreateListObject(const std::vector<Value> &initialValues)
            {
                auto object = MakeBuiltinObject("list");
                object->fields["__length__"] = Value(0.0);

                for (const Value &value : initialValues)
                {
                    ListAppend(object, value);
                }

                AddBuiltinMethod(object, "append", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 2)
                    {
                        return Value(nullptr);
                    }

                    ListAppend(object, arguments[1]);
                    return arguments[1]; });

                AddBuiltinMethod(object, "get", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 2)
                    {
                        return Value(nullptr);
                    }

                    const std::size_t index = ToIndex(arguments[1]);
                    return ListGetItem(object, index); });

                AddBuiltinMethod(object, "set", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 3)
                    {
                        return Value(nullptr);
                    }

                    const std::size_t index = ToIndex(arguments[1]);
                    ListSetItem(object, index, arguments[2]);
                    return arguments[2]; });

                AddBuiltinMethod(object, "pop", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value(nullptr);
                    }

                    return ListPop(object); });

                AddBuiltinMethod(object, "clear", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value(nullptr);
                    }

                    const std::size_t length = ListLength(object);
                    for (std::size_t i = 0; i < length; ++i)
                    {
                        object->fields.erase(ListItemKey(i));
                    }
                    object->fields["__length__"] = Value(0.0);
                    return Value(nullptr); });

                AddBuiltinMethod(object, "keys", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("[]");
                    }

                    std::vector<std::string> indices;
                    const std::size_t length = ListLength(object);
                    for (std::size_t i = 0; i < length; ++i)
                    {
                        indices.push_back(std::to_string(i));
                    }
                    return Value(JoinNames(indices)); });

                AddBuiltinMethod(object, "len", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    return Value(object ? static_cast<double>(ListLength(object)) : 0.0); });

                AddBuiltinMethod(object, "toString", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    return Value(ListToString(object)); });

                return object;
            }

            static std::shared_ptr<Object> CreateStringObject(const std::vector<Value> &arguments)
            {
                const std::string text = arguments.empty() ? std::string() : arguments.front().AsString();
                auto object = MakeBuiltinObject("string");
                object->fields["value"] = Value(text);
                object->fields["__length__"] = Value(static_cast<double>(text.size()));

                AddBuiltinMethod(object, "length", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value(0.0);
                    }

                    double length = 0.0;
                    if (GetNumericField(object, "__length__", length))
                    {
                        return Value(length);
                    }

                    const Value value = GetFieldOrNull(object, "value");
                    return Value(static_cast<double>(value.AsString().size())); });

                AddBuiltinMethod(object, "upper", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("");
                    }

                    return Value(ToUpper(GetFieldOrNull(object, "value").AsString())); });

                AddBuiltinMethod(object, "lower", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("");
                    }

                    return Value(ToLower(GetFieldOrNull(object, "value").AsString())); });

                AddBuiltinMethod(object, "strip", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("");
                    }

                    return Value(Trim(GetFieldOrNull(object, "value").AsString())); });

                AddBuiltinMethod(object, "contains", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 2)
                    {
                        return Value(false);
                    }

                    const std::string text = GetFieldOrNull(object, "value").AsString();
                    return Value(text.find(arguments[1].AsString()) != std::string::npos); });

                AddBuiltinMethod(object, "replace", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object || arguments.size() < 3)
                    {
                        return Value("");
                    }

                    std::string text = GetFieldOrNull(object, "value").AsString();
                    const std::string from = arguments[1].AsString();
                    const std::string to = arguments[2].AsString();

                    if (from.empty())
                    {
                        return Value(text);
                    }

                    std::size_t pos = 0;
                    while ((pos = text.find(from, pos)) != std::string::npos)
                    {
                        text.replace(pos, from.size(), to);
                        pos += to.size();
                    }
                    return Value(text); });

                AddBuiltinMethod(object, "toString", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("");
                    }

                    return GetFieldOrNull(object, "value"); });

                return object;
            }

            static std::shared_ptr<Object> CreateRangeObject(const std::vector<Value> &arguments)
            {
                double start = 0.0;
                double stop = 0.0;
                double step = 1.0;

                if (arguments.size() == 1)
                {
                    stop = ToNumberOrZero(arguments[0]);
                }
                else if (arguments.size() >= 2)
                {
                    start = ToNumberOrZero(arguments[0]);
                    stop = ToNumberOrZero(arguments[1]);
                    if (arguments.size() >= 3)
                    {
                        step = ToNumberOrZero(arguments[2]);
                    }
                }

                if (step == 0.0)
                {
                    step = 1.0;
                }

                auto object = MakeBuiltinObject("range");
                object->fields["__start__"] = Value(start);
                object->fields["__stop__"] = Value(stop);
                object->fields["__step__"] = Value(step);
                object->fields["__current__"] = Value(start);
                object->fields["__length__"] = Value(static_cast<double>(RangeLength(start, stop, step)));

                AddBuiltinMethod(object, "hasNext", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value(false);
                    }

                    const double start = GetFieldOrNull(object, "__start__").AsNumber();
                    const double stop = GetFieldOrNull(object, "__stop__").AsNumber();
                    const double step = GetFieldOrNull(object, "__step__").AsNumber();
                    const double current = GetFieldOrNull(object, "__current__").AsNumber();
                    if (step > 0.0)
                    {
                        return Value(current < stop);
                    }
                    return Value(current > stop); });

                AddBuiltinMethod(object, "next", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value(nullptr);
                    }

                    const double start = GetFieldOrNull(object, "__start__").AsNumber();
                    const double stop = GetFieldOrNull(object, "__stop__").AsNumber();
                    const double step = GetFieldOrNull(object, "__step__").AsNumber();
                    double current = GetFieldOrNull(object, "__current__").AsNumber();

                    if ((step > 0.0 && current >= stop) || (step < 0.0 && current <= stop))
                    {
                        return Value(nullptr);
                    }

                    const double result = current;
                    current += step;
                    object->fields["__current__"] = Value(current);
                    (void)start;
                    return Value(result); });

                AddBuiltinMethod(object, "reset", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value(nullptr);
                    }

                    object->fields["__current__"] = GetFieldOrNull(object, "__start__");
                    return Value(nullptr); });

                AddBuiltinMethod(object, "len", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    return Value(object ? GetCountFieldOrVisibleSize(object) : 0.0); });

                AddBuiltinMethod(object, "toString", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("range(0, 0, 1)");
                    }

                    const std::string start = GetFieldOrNull(object, "__start__").AsString();
                    const std::string stop = GetFieldOrNull(object, "__stop__").AsString();
                    const std::string step = GetFieldOrNull(object, "__step__").AsString();
                    return Value("range(" + start + ", " + stop + ", " + step + ")"); });

                return object;
            }

            static std::shared_ptr<Object> CreateCreatorObject()
            {
                auto object = MakeBuiltinObject("creator");
                AddBuiltinField(object, "name", Value("creator demo"));
                AddBuiltinField(object, "language", Value("CoraScript"));

                AddBuiltinMethod(object, "greet", [](const std::vector<Value> &) -> Value
                                 { return Value("Hello from CoraScript"); });

                AddBuiltinMethod(object, "program", [](const std::vector<Value> &) -> Value
                                 { return Value("Computer Science"); });

                AddBuiltinMethod(object, "describe", [](const std::vector<Value> &arguments) -> Value
                                 {
                    auto object = RequireObject(arguments);
                    if (!object)
                    {
                        return Value("");
                    }

                    return Value(ObjectToString(object, {"greet", "program", "describe"})); });

                return object;
            }
        } // namespace

        std::shared_ptr<runtime::NativeFunction> MakeBuiltinFunction(std::string name, NativeFn function)
        {
            return std::make_shared<runtime::NativeFunction>(std::move(name), std::move(function));
        }

        std::shared_ptr<runtime::Object> MakeBuiltinObject(const std::string &className)
        {
            return std::make_shared<runtime::Object>(className);
        }

        void AddBuiltinField(const std::shared_ptr<runtime::Object> &object, const std::string &name, runtime::Value value, bool isPrivate)
        {
            if (!object)
            {
                return;
            }

            object->fields[name] = std::move(value);
            object->SetMemberVisibility(name, isPrivate);
        }

        void AddBuiltinMethod(const std::shared_ptr<runtime::Object> &object, const std::string &name, NativeFn function, bool isPrivate)
        {
            if (!object)
            {
                return;
            }

            auto callable = MakeBuiltinFunction(object->className + "." + name, std::move(function));
            object->fields[name] = runtime::Value(std::static_pointer_cast<runtime::Callable>(callable));
            object->SetMemberVisibility(name, isPrivate);
        }

        BuiltinClassBuilder::BuiltinClassBuilder(std::string className)
            : m_Object(MakeBuiltinObject(std::move(className))) {}

        BuiltinClassBuilder &BuiltinClassBuilder::Field(const std::string &name, runtime::Value value, bool isPrivate)
        {
            AddBuiltinField(m_Object, name, std::move(value), isPrivate);
            return *this;
        }

        BuiltinClassBuilder &BuiltinClassBuilder::Method(const std::string &name, NativeFn function, bool isPrivate)
        {
            AddBuiltinMethod(m_Object, name, std::move(function), isPrivate);
            return *this;
        }

        std::shared_ptr<runtime::Object> BuiltinClassBuilder::Build() const
        {
            return m_Object;
        }

        BuiltinClassBuilder MakeBuiltinClass(std::string className)
        {
            return BuiltinClassBuilder(std::move(className));
        }

        void RegisterBuiltinFunction(runtime::Scope &scope, const std::string &name, NativeFn function, bool constant)
        {
            auto callable = MakeBuiltinFunction(name, std::move(function));
            scope.SetVariableValue(name, new runtime::Value(std::static_pointer_cast<runtime::Callable>(callable)), constant);
        }

        void RegisterBuiltinValue(runtime::Scope &scope, const std::string &name, runtime::Value value, bool constant)
        {
            scope.SetVariableValue(name, new runtime::Value(std::move(value)), constant);
        }

        void RegisterBuiltinObject(runtime::Scope &scope, const std::string &name, const std::shared_ptr<runtime::Object> &object, bool constant)
        {
            scope.SetVariableValue(name, new runtime::Value(object), constant);
        }

        static std::shared_ptr<Object> CreateIOModule()
        {
            auto module = MakeBuiltinObject("io");
            AddBuiltinField(module, "__module__", Value(true), true);

            AddBuiltinMethod(module, "print", [](const std::vector<Value> &arguments) -> Value
                             {
                bool first = true;
                for (const Value &argument : arguments)
                {
                    if (!first)
                    {
                        std::cout << ' ';
                    }
                    std::cout << argument.AsString();
                    first = false;
                }
                std::cout << '\n';
                return Value(nullptr); });

            AddBuiltinMethod(module, "println", [](const std::vector<Value> &arguments) -> Value
                             {
                bool first = true;
                for (const Value &argument : arguments)
                {
                    if (!first)
                    {
                        std::cout << ' ';
                    }
                    std::cout << argument.AsString();
                    first = false;
                }
                std::cout << '\n';
                return Value(nullptr); });

            AddBuiltinMethod(module, "input", [](const std::vector<Value> &arguments) -> Value
                             {
                if (!arguments.empty())
                {
                    std::cout << arguments.front().AsString();
                    std::cout.flush();
                }

                std::string line;
                std::getline(std::cin, line);
                return Value(line); });

            AddBuiltinMethod(module, "read_file", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(nullptr);
                }

                std::ifstream input(arguments.front().AsString());
                if (!input)
                {
                    return Value(nullptr);
                }

                std::ostringstream buffer;
                buffer << input.rdbuf();
                return Value(buffer.str()); });

            AddBuiltinMethod(module, "write_file", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.size() < 2)
                {
                    return Value(false);
                }

                std::ofstream output(arguments[0].AsString(), std::ios::binary);
                if (!output)
                {
                    return Value(false);
                }

                output << arguments[1].AsString();
                return Value(true); });

            AddBuiltinMethod(module, "exists", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(false);
                }

                std::ifstream input(arguments.front().AsString());
                return Value(static_cast<bool>(input)); });

            return module;
        }

        static std::shared_ptr<Object> CreateMathModule()
        {
            auto module = MakeBuiltinObject("math");
            AddBuiltinField(module, "__module__", Value(true), true);

            AddBuiltinField(module, "pi", Value(std::acos(-1.0)));
            AddBuiltinField(module, "tau", Value(2.0 * std::acos(-1.0)));
            AddBuiltinField(module, "e", Value(std::exp(1.0)));

            AddBuiltinMethod(module, "abs", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::fabs(ToNumberOrZero(arguments.front()))); });

            AddBuiltinMethod(module, "pow", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.size() < 2)
                {
                    return Value(0.0);
                }
                return Value(std::pow(ToNumberOrZero(arguments[0]), ToNumberOrZero(arguments[1]))); });

            AddBuiltinMethod(module, "sqrt", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::sqrt(std::max(0.0, ToNumberOrZero(arguments.front())))); });

            AddBuiltinMethod(module, "min", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }

                double result = ToNumberOrZero(arguments.front());
                for (std::size_t i = 1; i < arguments.size(); ++i)
                {
                    result = std::min(result, ToNumberOrZero(arguments[i]));
                }
                return Value(result); });

            AddBuiltinMethod(module, "max", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }

                double result = ToNumberOrZero(arguments.front());
                for (std::size_t i = 1; i < arguments.size(); ++i)
                {
                    result = std::max(result, ToNumberOrZero(arguments[i]));
                }
                return Value(result); });

            AddBuiltinMethod(module, "sum", [](const std::vector<Value> &arguments) -> Value
                             {
                double total = 0.0;
                for (const Value &argument : arguments)
                {
                    total += ToNumberOrZero(argument);
                }
                return Value(total); });

            AddBuiltinMethod(module, "floor", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::floor(ToNumberOrZero(arguments.front()))); });

            AddBuiltinMethod(module, "ceil", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::ceil(ToNumberOrZero(arguments.front()))); });

            AddBuiltinMethod(module, "round", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::round(ToNumberOrZero(arguments.front()))); });

            AddBuiltinMethod(module, "sin", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::sin(ToNumberOrZero(arguments.front()))); });

            AddBuiltinMethod(module, "cos", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::cos(ToNumberOrZero(arguments.front()))); });

            AddBuiltinMethod(module, "tan", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::tan(ToNumberOrZero(arguments.front()))); });

            return module;
        }

        static std::shared_ptr<Object> CreateStringModule()
        {
            auto module = MakeBuiltinObject("strings");
            AddBuiltinField(module, "__module__", Value(true), true);

            AddBuiltinMethod(module, "lower", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value("");
                }
                return Value(ToLower(arguments.front().AsString())); });

            AddBuiltinMethod(module, "upper", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value("");
                }
                return Value(ToUpper(arguments.front().AsString())); });

            AddBuiltinMethod(module, "trim", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value("");
                }
                return Value(Trim(arguments.front().AsString())); });

            AddBuiltinMethod(module, "contains", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.size() < 2)
                {
                    return Value(false);
                }
                return Value(arguments[0].AsString().find(arguments[1].AsString()) != std::string::npos); });

            AddBuiltinMethod(module, "replace", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.size() < 3)
                {
                    return Value("");
                }

                std::string text = arguments[0].AsString();
                const std::string from = arguments[1].AsString();
                const std::string to = arguments[2].AsString();

                if (from.empty())
                {
                    return Value(text);
                }

                std::size_t pos = 0;
                while ((pos = text.find(from, pos)) != std::string::npos)
                {
                    text.replace(pos, from.size(), to);
                    pos += to.size();
                }

                return Value(text); });

            return module;
        }

        static std::shared_ptr<Object> CreateTimeModule()
        {
            auto module = MakeBuiltinObject("time");
            AddBuiltinField(module, "__module__", Value(true), true);

            AddBuiltinMethod(module, "now_ms", [](const std::vector<Value> &) -> Value
                             {
                const auto now = std::chrono::system_clock::now();
                const auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
                return Value(static_cast<double>(millis)); });

            AddBuiltinMethod(module, "now_seconds", [](const std::vector<Value> &) -> Value
                             {
                const auto now = std::chrono::system_clock::now();
                const auto secs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                return Value(static_cast<double>(secs)); });

            AddBuiltinMethod(module, "sleep_ms", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(nullptr);
                }

                const double duration = ToNumberOrZero(arguments.front());
                if (duration > 0.0)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(duration)));
                }
                return Value(nullptr); });

            AddBuiltinMethod(module, "sleep_seconds", [](const std::vector<Value> &arguments) -> Value
                             {
                if (arguments.empty())
                {
                    return Value(nullptr);
                }

                const double duration = ToNumberOrZero(arguments.front());
                if (duration > 0.0)
                {
                    std::this_thread::sleep_for(std::chrono::duration<double>(duration));
                }
                return Value(nullptr); });

            return module;
        }

        static std::shared_ptr<Object> CreateStdModule(const std::shared_ptr<Object> &ioModule, const std::shared_ptr<Object> &mathModule, const std::shared_ptr<Object> &stringModule, const std::shared_ptr<Object> &timeModule)
        {
            auto module = MakeBuiltinObject("std");
            AddBuiltinField(module, "__module__", Value(true), true);
            AddBuiltinField(module, "io", Value(ioModule));
            AddBuiltinField(module, "math", Value(mathModule));
            AddBuiltinField(module, "maths", Value(mathModule));
            AddBuiltinField(module, "strings", Value(stringModule));
            AddBuiltinField(module, "time", Value(timeModule));
            return module;
        }

        void Builtins(runtime::Scope &scope)
        {
            RegisterBuiltinFunction(scope, "print", [](const std::vector<Value> &arguments) -> Value
                                    {
                bool first = true;
                for (const Value &argument : arguments)
                {
                    if (!first)
                    {
                        std::cout << ' ';
                    }
                    std::cout << argument.AsString();
                    first = false;
                }
                std::cout << '\n';
                return Value(nullptr); });

            RegisterBuiltinFunction(scope, "len", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value(0.0);
                }

                const Value &value = arguments.front();
                if (value.IsString())
                {
                    return Value(static_cast<double>(value.AsString().size()));
                }
                if (value.IsObject() && value.AsObject() != nullptr)
                {
                    return Value(GetCountFieldOrVisibleSize(value.AsObject(), {"get", "set", "has", "delete", "clear", "keys", "len", "toString", "contains", "update", "append", "pop", "upper", "lower", "strip", "replace", "length", "hasNext", "next", "reset"}));
                }
                return Value(0.0); });

            RegisterBuiltinFunction(scope, "type", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value("undefined");
                }
                return Value(arguments.front().GetValueKindString()); });

            RegisterBuiltinFunction(scope, "repr", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value("null");
                }
                return Value(arguments.front().AsString()); });

            RegisterBuiltinFunction(scope, "str", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value("");
                }
                return Value(arguments.front().AsString()); });

            RegisterBuiltinFunction(scope, "int", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                const double number = ToNumberOrZero(arguments.front());
                return Value(static_cast<double>(static_cast<long long>(number))); });

            RegisterBuiltinFunction(scope, "float", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(ToNumberOrZero(arguments.front())); });

            RegisterBuiltinFunction(scope, "bool", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value(false);
                }
                return Value(arguments.front().AsBool()); });

            RegisterBuiltinFunction(scope, "abs", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value(0.0);
                }
                return Value(std::fabs(ToNumberOrZero(arguments.front()))); });

            RegisterBuiltinFunction(scope, "pow", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.size() < 2)
                {
                    return Value(0.0);
                }
                return Value(std::pow(ToNumberOrZero(arguments[0]), ToNumberOrZero(arguments[1]))); });

            RegisterBuiltinFunction(scope, "min", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value(0.0);
                }

                double result = ToNumberOrZero(arguments.front());
                for (std::size_t i = 1; i < arguments.size(); ++i)
                {
                    result = std::min(result, ToNumberOrZero(arguments[i]));
                }
                return Value(result); });

            RegisterBuiltinFunction(scope, "max", [](const std::vector<Value> &arguments) -> Value
                                    {
                if (arguments.empty())
                {
                    return Value(0.0);
                }

                double result = ToNumberOrZero(arguments.front());
                for (std::size_t i = 1; i < arguments.size(); ++i)
                {
                    result = std::max(result, ToNumberOrZero(arguments[i]));
                }
                return Value(result); });

            RegisterBuiltinFunction(scope, "sum", [](const std::vector<Value> &arguments) -> Value
                                    {
                double total = 0.0;
                for (const Value &argument : arguments)
                {
                    total += ToNumberOrZero(argument);
                }
                return Value(total); });

            RegisterBuiltinFunction(scope, "range_obj", [](const std::vector<Value> &arguments) -> Value
                                    { return Value(CreateRangeObject(arguments)); });

            RegisterBuiltinFunction(scope, "object", [](const std::vector<Value> &) -> Value
                                    { return Value(CreateContainerObject("object")); });

            RegisterBuiltinFunction(scope, "dict", [](const std::vector<Value> &) -> Value
                                    { return Value(CreateContainerObject("dict")); });

            RegisterBuiltinFunction(scope, "list", [](const std::vector<Value> &arguments) -> Value
                                    { return Value(CreateListObject(arguments)); });

            RegisterBuiltinFunction(scope, "creator", [](const std::vector<Value> &) -> Value
                                    { return Value(CreateCreatorObject()); });

            RegisterBuiltinValue(scope, "true_value", Value(true));
            RegisterBuiltinValue(scope, "false_value", Value(false));

            RegisterStandardModules(scope);
        }

        void RegisterStandardModules(runtime::Scope &scope)
        {
            auto ioModule = CreateIOModule();
            auto mathModule = CreateMathModule();
            auto stringModule = CreateStringModule();
            auto timeModule = CreateTimeModule();
            auto stdModule = CreateStdModule(ioModule, mathModule, stringModule, timeModule);

            RegisterBuiltinObject(scope, "io", ioModule);
            RegisterBuiltinObject(scope, "math", mathModule);
            RegisterBuiltinObject(scope, "maths", mathModule);
            RegisterBuiltinObject(scope, "strings", stringModule);
            RegisterBuiltinObject(scope, "time", timeModule);
            RegisterBuiltinObject(scope, "std", stdModule);
        }

    } // namespace builtin

} // namespace cora::compiler