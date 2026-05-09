#include "Value.hpp"

#include <sstream>
#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {
        namespace
        {
            ValueKind KindFromType(value::value_type type)
            {
                switch (type)
                {
                case value::value_type::null:
                    return ValueKind::Null;
                case value::value_type::boolean:
                    return ValueKind::Bool;
                case value::value_type::integer:
                    return ValueKind::Integer;
                case value::value_type::floating:
                    return ValueKind::Float;
                case value::value_type::string:
                    return ValueKind::String;
                case value::value_type::array:
                    return ValueKind::Array;
                case value::value_type::object:
                    return ValueKind::Object;
                case value::value_type::callable:
                    return ValueKind::Callable;
                }
                return ValueKind::Undefined;
            }
        }

        int Callable::Arity() const
        {
            return -1;
        }

        std::string Callable::Doc() const
        {
            return m_Doc;
        }

        void Callable::SetDoc(std::string doc)
        {
            m_Doc = std::move(doc);
        }

        Object::Object(std::string className)
            : className(std::move(className)), fields(), privateMembers(), constMembers(), initializedConstMembers(), m_Name(this->className) {}

        std::string Object::Name() const
        {
            return m_Name;
        };

        bool Object::IsPrivateMember(const std::string &member) const
        {
            return privateMembers.find(member) != privateMembers.end();
        }

        void Object::SetMemberVisibility(const std::string &member, bool isPrivate)
        {
            if (isPrivate)
            {
                privateMembers.insert(member);
                return;
            }
            privateMembers.erase(member);
        }

        bool Object::IsConstMember(const std::string &member) const
        {
            return constMembers.find(member) != constMembers.end();
        }

        bool Object::IsConstMemberInitialized(const std::string &member) const
        {
            return initializedConstMembers.find(member) != initializedConstMembers.end();
        }

        void Object::SetMemberConstness(const std::string &member, bool isConst, bool initialized)
        {
            if (!isConst)
            {
                constMembers.erase(member);
                initializedConstMembers.erase(member);
                return;
            }

            constMembers.insert(member);
            if (initialized)
            {
                initializedConstMembers.insert(member);
                return;
            }

            initializedConstMembers.erase(member);
        }

        void Object::MarkConstMemberInitialized(const std::string &member)
        {
            if (IsConstMember(member))
            {
                initializedConstMembers.insert(member);
            }
        }

        ///////////////////////////////////////////

        Method::Method(std::shared_ptr<Object> object, std::string name, Func function, int arity)
            : m_Name(std::move(name)), m_Object(std::move(object)), m_Function(std::move(function)), m_Arity(arity) {};

        Value Method::Call(const std::vector<Value> &arguments)
        {
            return m_Function(arguments);
        };

        std::string Method::Name() const
        {
            return m_Object->Name() + m_Name;
        };

        int Method::Arity() const
        {
            return m_Arity;
        }

        Function::Function(std::string name, Func function, int arity)
            : m_Name(std::move(name)), m_Function(std::move(function)), m_Arity(arity) {};

        Value Function::Call(const std::vector<Value> &arguments)
        {
            return m_Function(arguments);
        };

        std::string Function::Name() const
        {
            return m_Name;
        };

        int Function::Arity() const
        {
            return m_Arity;
        }

        ///////////////////////////////////////////

        BoundMethod::BoundMethod(std::shared_ptr<Object> receiver, std::shared_ptr<Callable> callable, std::string name)
            : m_Receiver(std::move(receiver)), m_Callable(std::move(callable)), m_Name(std::move(name)) {}

        Value BoundMethod::Call(const std::vector<Value> &arguments)
        {
            return m_Callable->Call(arguments);
        }

        std::string BoundMethod::Name() const
        {
            return m_Name;
        }

        int BoundMethod::Arity() const
        {
            return m_Callable ? m_Callable->Arity() : -1;
        }

        //-----------------------------------------------------------------------------
        // [Class] value
        //-----------------------------------------------------------------------------

        value::value()
            : m_type(value_type::null), m_kind(ValueKind::Null), m_value(null_type{})
        {
        }

        value::value(value_type value)
        {
            SetType(value);
            switch (value)
            {
            case value_type::null:
                m_value = null_type{};
                break;
            case value_type::boolean:
                m_value = boolean_type{false};
                break;
            case value_type::integer:
                m_value = integer_type{0};
                break;
            case value_type::floating:
                m_value = floating_type{0.0};
                break;
            case value_type::string:
                m_value = string_type{};
                break;
            case value_type::array:
                m_value = array_type{};
                break;
            case value_type::object:
                m_value = object_type{};
                break;
            case value_type::callable:
                m_value = callable_type{};
                break;
            }
        }

        value::value(value::null_type value)
            : m_value(value)
        {
            SetType(value_type::null);
        }

        value::value(value::integer_type value)
            : m_value(value)
        {
            SetType(value_type::integer);
        }

        value::value(int value)
            : m_value(static_cast<integer_type>(value))
        {
            SetType(value_type::integer);
        }

        value::value(value::boolean_type value)
            : m_value(value)
        {
            SetType(value_type::boolean);
        }

        value::value(value::floating_type value)
            : m_value(value)
        {
            SetType(value_type::floating);
        }

        value::value(const value::array_type &value)
            : m_value(value)
        {
            SetType(value_type::array);
        }

        value::value(value::array_type &&value)
            : m_value(std::move(value))
        {
            SetType(value_type::array);
        }

        value::value(const value::object_type &value)
            : m_value(value)
        {
            SetType(value_type::object);
        }

        value::value(value::object_type &&value)
            : m_value(std::move(value))
        {
            SetType(value_type::object);
        }

        value::value(value::callable_type value)
            : m_value(std::move(value))
        {
            SetType(value_type::callable);
        }

        value::value(const value::string_type &value)
            : m_value(value)
        {
            SetType(value_type::string);
        }

        value::value(value::string_type &&value)
            : m_value(std::move(value))
        {
            SetType(value_type::string);
        }

        value::value(const char *value)
            : m_value(string_type(value != nullptr ? value : ""))
        {
            SetType(value_type::string);
        }

        value::value(const value &other) = default;

        value::value(value &&other) = default;

        // Value::Value(std::shared_ptr<Method> method)
        //     : m_Kind(ValueKind::Callable), m_Data(std::move(std::static_pointer_cast<runtime::Callable>(method))) {};

        // Value::Value(std::shared_ptr<Function> function)
        //     : m_Kind(ValueKind::Callable), m_Data(std::move(std::static_pointer_cast<runtime::Callable>(function))) {};

        // Value::Value(std::shared_ptr<BoundMethod> method)
        //     : m_Kind(ValueKind::Callable), m_Data(std::move(std::static_pointer_cast<runtime::Callable>(method))) {};

        // Value::Value(Method method)
        //     : Value(std::make_shared<Method>(method)) {};

        // Value::Value(Function function)
        //     : Value(std::make_shared<Function>(function)) {};

        // Value::Value(BoundMethod method)
        //     : Value(std::make_shared<BoundMethod>(method)) {};

        //========== Assignment ==========

        value &value::operator=(const value &other) = default;

        value &value::operator=(value &&other) = default;

        value &value::operator=(null_type value)
        {
            SetType(value_type::null);
            m_value = value;
            return *this;
        }

        value &value::operator=(bool value)
        {
            SetType(value_type::boolean);
            m_value = value;
            return *this;
        }

        value &value::operator=(integer_type value)
        {
            SetType(value_type::integer);
            m_value = value;
            return *this;
        }

        value &value::operator=(int value)
        {
            SetType(value_type::integer);
            m_value = static_cast<integer_type>(value);
            return *this;
        }

        value &value::operator=(floating_type value)
        {
            SetType(value_type::floating);
            m_value = value;
            return *this;
        }

        value &value::operator=(string_type value)
        {
            SetType(value_type::string);
            m_value = std::move(value);
            return *this;
        }

        value &value::operator=(const char *value)
        {
            SetType(value_type::string);
            m_value = string_type(value != nullptr ? value : "");
            return *this;
        }

        value &value::operator=(array_type value)
        {
            SetType(value_type::array);
            m_value = std::move(value);
            return *this;
        }

        value &value::operator=(object_type value)
        {
            SetType(value_type::object);
            m_value = std::move(value);
            return *this;
        }

        void value::SetType(value_type type)
        {
            m_type = type;
            m_kind = KindFromType(type);
        }

        ValueKind value::GetValueKind() const { return m_kind; }

        void value::SetValueKind(ValueKind kind) { m_kind = kind; }

        bool value::IsNull() const { return is_null(); }
        bool value::IsBool() const { return is_boolean(); }
        bool value::IsString() const { return is_string(); }
        bool value::IsInteger() const { return is_integer(); }
        bool value::IsFloat() const { return is_floating(); }
        bool value::IsNumber() const { return is_integer() || is_floating(); }
        bool value::IsArray() const { return is_array(); }
        bool value::IsObject() const { return is_object(); }
        bool value::IsCallable() const { return is_callable(); }

        double value::AsNumber() const
        {
            switch (m_type)
            {
            case value_type::integer:
                return static_cast<double>(as_integer());
            case value_type::floating:
                return as_floating();
            case value_type::boolean:
                return as_boolean() ? 1.0 : 0.0;
            case value_type::string:
                return std::stod(as_string());
            case value_type::null:
                return 0.0;
            default:
                throw std::runtime_error("Cannot convert value to number");
            }
        }

        std::string value::AsString() const
        {
            switch (m_type)
            {
            case value_type::string:
                return as_string();
            case value_type::null:
                return "null";
            case value_type::boolean:
                return as_boolean() ? "true" : "false";
            case value_type::integer:
                return std::to_string(as_integer());
            case value_type::floating:
            {
                std::ostringstream out;
                out << as_floating();
                return out.str();
            }
            case value_type::object:
            {
                auto object = as_object();
                return object ? object->Name() : "<object>";
            }
            case value_type::array:
                return "<array>";
            case value_type::callable:
                return "<callable>";
            }
            return "<unknown>";
        }

        bool value::AsBool() const
        {
            switch (m_type)
            {
            case value_type::boolean:
                return as_boolean();
            case value_type::integer:
                return as_integer() != 0;
            case value_type::floating:
                return as_floating() != 0.0;
            case value_type::string:
                return !as_string().empty();
            case value_type::null:
                return false;
            case value_type::object:
            case value_type::array:
            case value_type::callable:
                return true;
            }
            return false;
        }

        value::object_type value::AsObject() const
        {
            if (!is_object())
            {
                throw std::runtime_error("Expected object value");
            }
            return as_object();
        }

        value::callable_type value::AsCallable() const
        {
            if (!is_callable())
            {
                throw std::runtime_error("Expected callable value");
            }
            return std::get<callable_type>(m_value);
        }

        //========== Type Information ==========

        value::value_type value::type() const
        {
            return m_type;
        };

        template <class T>
        bool value::is() { return std::holds_alternative<T>(m_value); };

        bool value::is_null() const { return type() == value_type::null; };

        bool value::is_array() const { return type() == value_type::array; };

        bool value::is_object() const { return type() == value_type::object; };

        bool value::is_string() const { return type() == value_type::string; };

        bool value::is_integer() const { return type() == value_type::integer; };

        bool value::is_boolean() const { return type() == value_type::boolean; };

        bool value::is_floating() const { return type() == value_type::floating; };

        bool value::is_callable() const { return type() == value_type::callable; };

        //========== Type Conversions ==========

        // template <class T>
        // T value::as(const T &default_value)
        // {
        //     // if (is<T>())
        //     //     return value<T>();
        //     if constexpr (std::is_same<T, std::string>::value)
        //         return asString();
        //     if constexpr (std::is_same<T, long long>::value)
        //         return asInt();
        //     if constexpr (std::is_same<T, double>::value)
        //         return asDouble();
        // };

        value::array_type &value::as_array()
        {
            if (!is_array())
                throw(std::runtime_error("Cannot access as array"));
            return std::get<array_type>(m_value);
        }

        value::object_type &value::as_object()
        {
            if (!is_object())
                throw(std::runtime_error("Cannot access as object"));
            return std::get<object_type>(m_value);
        }

        value::string_type &value::as_string()
        {
            if (!is_string())
                throw(std::runtime_error("Cannot access as string"));
            return std::get<string_type>(m_value);
        }

        value::integer_type &value::as_integer()
        {
            if (!is_integer())
                throw(std::runtime_error("Cannot access as integer"));
            return std::get<integer_type>(m_value);
        }

        value::boolean_type &value::as_boolean()
        {
            if (!is_boolean())
                throw(std::runtime_error("Cannot access as boolean"));
            return std::get<boolean_type>(m_value);
        }

        value::floating_type &value::as_floating()
        {
            if (!is_floating())
                throw(std::runtime_error("Cannot access as floating"));
            return std::get<floating_type>(m_value);
        }

        const value::array_type &value::as_array() const
        {
            if (!is_array())
                throw(std::runtime_error("Cannot access as array"));
            return std::get<array_type>(m_value);
        }

        const value::object_type &value::as_object() const
        {
            if (!is_object())
                throw(std::runtime_error("Cannot access as object"));
            return std::get<object_type>(m_value);
        }

        const value::string_type &value::as_string() const
        {
            if (!is_string())
                throw(std::runtime_error("Cannot access as string"));
            return std::get<string_type>(m_value);
        }

        const value::integer_type &value::as_integer() const
        {
            if (!is_integer())
                throw(std::runtime_error("Cannot access as integer"));
            return std::get<integer_type>(m_value);
        }

        const value::boolean_type &value::as_boolean() const
        {
            if (!is_boolean())
                throw(std::runtime_error("Cannot access as boolean"));
            return std::get<boolean_type>(m_value);
        }

        const value::floating_type &value::as_floating() const
        {
            if (!is_floating())
                throw(std::runtime_error("Cannot access as floating"));
            return std::get<floating_type>(m_value);
        }

        //========== Comparison ==========

        bool value::operator<(const value &other) const
        {
            if (type() != other.type())
                return static_cast<uint8_t>(type()) < static_cast<uint8_t>(other.type());

            switch (type())
            {
            case value_type::null:
                return false;
            case value_type::boolean:
                return as_boolean() < other.as_boolean();
            case value_type::integer:
                return as_integer() < other.as_integer();
            case value_type::floating:
                return as_floating() < other.as_floating();
            case value_type::string:
                return as_string() < other.as_string();
            case value_type::array:
                return as_array() < other.as_array();
            case value_type::object:
                return as_object() < other.as_object();
            }

            return false;
        };

        bool value::operator>(const value &other) const { return other < *this; }
        bool value::operator==(const value &other) const { return m_value == other.m_value; }
        bool value::operator!=(const value &other) const { return !(*this == other); }
        bool value::operator<=(const value &other) const { return (*this < other) || (*this == other); }
        bool value::operator>=(const value &other) const { return !(*this < other); }

        //========== Utility ==========

        std::string value::toString(){
            return as_string();
        };

        value value::clone() const { return value(*this); }

        std::string value::Repr() const
        {
            return AsString();
        }

        std::ostream &operator<<(std::ostream &ostream, const Value *value)
        {
            if (value == nullptr)
            {
                return ostream << "<null>";
            }

            return ostream << value->Repr();
        }

        std::ostream &operator<<(std::ostream &ostream, const Value &value)
        {
            return ostream << value.Repr();
        }

    } // namespace runtime

} // namespace cora::compiler
