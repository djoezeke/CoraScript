#ifndef CORA_CORA_H
#define CORA_CORA_H

#include "Cora/Engine.hpp"

#include "Builtin.hpp"
#include "Module.hpp"
#include "Value.hpp"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace cora
{
    class module_ : public object
    {
    };

    class exception_ : public object
    {
    };

    namespace detail
    {

        class generic_type : public object
        {
        };

        /// Wraps an arbitrary C++ function/method/lambda function/.. into a callable Cora object
        class cpp_function : public function
        {
        };
    }

    template <typename Type>
    class class_ : public detail::generic_type
    {
    };

    template <typename Type>
    class enum_ : public class_<Type>
    {
    }

}

namespace cora
{

    namespace ffi
    {
        using Value = compiler::runtime::Value;
        using ValueKind = compiler::runtime::ValueKind;
        using Object = compiler::runtime::Object;
        using Callable = compiler::runtime::Callable;
        using Function = compiler::runtime::Function;

        enum class Operator
        {
            Add,
            Sub,
            Mul,
            Div,
            Mod,
            Eq,
            Ne,
            Lt,
            Le,
            Gt,
            Ge,
            Neg,
            Not
        };

        inline const char *OperatorMethodName(Operator op)
        {
            switch (op)
            {
            case Operator::Add:
                return "__add__";
            case Operator::Sub:
                return "__sub__";
            case Operator::Mul:
                return "__mul__";
            case Operator::Div:
                return "__div__";
            case Operator::Mod:
                return "__mod__";
            case Operator::Eq:
                return "__eq__";
            case Operator::Ne:
                return "__ne__";
            case Operator::Lt:
                return "__lt__";
            case Operator::Le:
                return "__le__";
            case Operator::Gt:
                return "__gt__";
            case Operator::Ge:
                return "__ge__";
            case Operator::Neg:
                return "__neg__";
            case Operator::Not:
                return "__not__";
            default:
                return "__op__";
            }
        }

        template <typename T, typename Enable = void>
        struct ValueConverter;

        template <typename T>
        using Decay = typename std::decay<T>::type;

        template <>
        struct ValueConverter<Value>
        {
            static Value To(const Value &value)
            {
                return value;
            }

            static Value From(const Value &value)
            {
                return value;
            }
        };

        template <>
        struct ValueConverter<bool>
        {
            static Value To(bool value)
            {
                return Value(value);
            }

            static bool From(const Value &value)
            {
                return value.AsBool();
            }
        };

        template <typename T>
        struct ValueConverter<T, std::enable_if_t<std::is_integral<Decay<T>>::value && !std::is_same<Decay<T>, bool>::value>>
        {
            static Value To(T value)
            {
                Value out(static_cast<double>(value));
                out.SetValueKind(ValueKind::Integer);
                return out;
            }

            static T From(const Value &value)
            {
                return static_cast<T>(value.AsNumber());
            }
        };

        template <typename T>
        struct ValueConverter<T, std::enable_if_t<std::is_floating_point<Decay<T>>::value>>
        {
            static Value To(T value)
            {
                return Value(static_cast<double>(value));
            }

            static T From(const Value &value)
            {
                return static_cast<T>(value.AsNumber());
            }
        };

        template <>
        struct ValueConverter<std::string>
        {
            static Value To(const std::string &value)
            {
                return Value(value);
            }

            static std::string From(const Value &value)
            {
                return value.AsString();
            }
        };

        template <>
        struct ValueConverter<const char *>
        {
            static Value To(const char *value)
            {
                return Value(value ? value : "");
            }
        };

        template <>
        struct ValueConverter<std::shared_ptr<Object>>
        {
            static Value To(std::shared_ptr<Object> object)
            {
                return Value(std::move(object));
            }

            static std::shared_ptr<Object> From(const Value &value)
            {
                return value.AsObject();
            }
        };

        template <typename T>
        struct ValueConverter<std::vector<T>>
        {
            static Value To(const std::vector<T> &values)
            {
                auto arrayObject = std::make_shared<Object>("Array");
                for (std::size_t i = 0; i < values.size(); ++i)
                {
                    arrayObject->fields[std::to_string(i)] = ValueConverter<T>::To(values[i]);
                }
                arrayObject->fields["__len__"] = Value(static_cast<double>(values.size()));
                return Value(arrayObject);
            }

            static std::vector<T> From(const Value &value)
            {
                auto object = value.AsObject();
                if (!object)
                {
                    throw std::runtime_error("Expected array-like object");
                }

                std::size_t count = 0;
                auto lenIt = object->fields.find("__len__");
                if (lenIt != object->fields.end())
                {
                    count = static_cast<std::size_t>(lenIt->second.AsNumber());
                }

                std::vector<T> out;
                out.reserve(count);
                for (std::size_t i = 0; i < count; ++i)
                {
                    const auto it = object->fields.find(std::to_string(i));
                    if (it == object->fields.end())
                    {
                        break;
                    }
                    out.push_back(ValueConverter<T>::From(it->second));
                }
                return out;
            }
        };

        template <typename T>
        inline Value ToValue(T &&value)
        {
            return ValueConverter<Decay<T>>::To(std::forward<T>(value));
        }

        template <typename T>
        inline T FromValue(const Value &value)
        {
            return ValueConverter<Decay<T>>::From(value);
        }

        namespace detail
        {
            template <typename R, typename Fn, typename Tuple, std::size_t... I>
            Value InvokeBound(Fn &function, const std::vector<Value> &arguments, std::index_sequence<I...>)
            {
                if constexpr (std::is_void<R>::value)
                {
                    function(FromValue<typename std::tuple_element<I, Tuple>::type>(arguments[I])...);
                    return Value(nullptr);
                }
                else
                {
                    auto result = function(FromValue<typename std::tuple_element<I, Tuple>::type>(arguments[I])...);
                    return ToValue(std::move(result));
                }
            }
        } // namespace detail

        template <typename R, typename... Args, typename Fn>
        inline std::shared_ptr<Callable> BindFunction(std::string name, Fn &&function, std::string doc = {})
        {
            auto wrapped = [func = std::forward<Fn>(function)](const std::vector<Value> &arguments) mutable -> Value
            {
                if (arguments.size() != sizeof...(Args))
                {
                    throw std::runtime_error("Argument count mismatch while calling native function");
                }

                using Tuple = std::tuple<Args...>;
                return detail::InvokeBound<R, decltype(func), Tuple>(
                    func,
                    arguments,
                    std::index_sequence_for<Args...>{});
            };

            auto callable = std::static_pointer_cast<Callable>(
                std::make_shared<Function>(std::move(name), wrapped, static_cast<int>(sizeof...(Args))));
            if (!doc.empty())
            {
                callable->SetDoc(std::move(doc));
            }
            return callable;
        }

        class ModuleBuilder
        {
        public:
            explicit ModuleBuilder(std::string name, std::string doc = {})
                : m_Module(std::move(name), {}, {}, {}, std::move(doc))
            {
            }

            ModuleBuilder &DefValue(const std::string &name, Value value)
            {
                m_Module.WithVariable(name, std::move(value));
                return *this;
            }

            template <typename T>
            ModuleBuilder &DefValue(const std::string &name, T &&value)
            {
                return DefValue(name, ToValue(std::forward<T>(value)));
            }

            ModuleBuilder &DefFunction(const std::string &name, std::shared_ptr<Callable> callable)
            {
                m_Module.WithFunction(name, std::move(callable));
                return *this;
            }

            template <typename R, typename... Args, typename Fn>
            ModuleBuilder &Def(const std::string &name, Fn &&function, std::string doc = {})
            {
                return DefFunction(name, BindFunction<R, Args...>(name, std::forward<Fn>(function), std::move(doc)));
            }

            std::shared_ptr<Object> Object() const
            {
                return m_Module.Object();
            }

            bool RegisterTo(embed::Engine &engine) const
            {
                const std::string moduleName = m_Module.Name();
                return engine.RegisterBuiltinModule(moduleName, [module = m_Module]() mutable
                                                    { return module.Object(); });
            }

        private:
            compiler::builtin::Module m_Module;
        };

        inline void BindOperator(const std::shared_ptr<Object> &object, Operator op, std::shared_ptr<Callable> function)
        {
            if (!object || !function)
            {
                return;
            }

            object->fields[OperatorMethodName(op)] = Value(std::move(function));
        }

        inline Value InvokeOperator(const Value &left, Operator op, const Value &right = Value(nullptr))
        {
            auto object = left.AsObject();
            if (object)
            {
                const auto it = object->fields.find(OperatorMethodName(op));
                if (it != object->fields.end() && it->second.IsCallable())
                {
                    auto callable = it->second.AsCallable();
                    if (callable)
                    {
                        if (op == Operator::Neg || op == Operator::Not)
                        {
                            return callable->Call({left});
                        }
                        return callable->Call({left, right});
                    }
                }
            }

            switch (op)
            {
            case Operator::Add:
                if (left.IsString() || right.IsString())
                {
                    return Value(left.AsString() + right.AsString());
                }
                return Value(left.AsNumber() + right.AsNumber());
            case Operator::Sub:
                return Value(left.AsNumber() - right.AsNumber());
            case Operator::Mul:
                return Value(left.AsNumber() * right.AsNumber());
            case Operator::Div:
                return Value(left.AsNumber() / right.AsNumber());
            case Operator::Mod:
                return Value(static_cast<double>(static_cast<long long>(left.AsNumber()) % static_cast<long long>(right.AsNumber())));
            case Operator::Eq:
                return Value(left.AsString() == right.AsString());
            case Operator::Ne:
                return Value(left.AsString() != right.AsString());
            case Operator::Lt:
                return Value(left.AsNumber() < right.AsNumber());
            case Operator::Le:
                return Value(left.AsNumber() <= right.AsNumber());
            case Operator::Gt:
                return Value(left.AsNumber() > right.AsNumber());
            case Operator::Ge:
                return Value(left.AsNumber() >= right.AsNumber());
            case Operator::Neg:
                return Value(-left.AsNumber());
            case Operator::Not:
                return Value(!left.AsBool());
            default:
                return Value(nullptr);
            }
        }
    } // namespace ffi

} // namespace cora

#endif // CORA_CORA_H