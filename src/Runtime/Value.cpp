#include "Value.hpp"

#include <sstream>
#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {

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

        value::value(value::null_type value)
            : m_value(value)
        {
        }

        value::value(value::integer_type value)
            : m_value(value)
        {
        }

        value::value(int value)
            : m_value(static_cast<integer_type>(value))
        {
        }

        value::value(value::boolean_type value)
            : m_value(value)
        {
        }

        value::value(value::floating_type value)
            : m_value(value)
        {
        }

        value::value(const value::array_type &value)
            : m_value(value)
        {
        }

        value::value(value::array_type &&value)
            : m_value(std::move(value))
        {
        }

        value::value(const value::object_type &value)
            : m_value(value)
        {
        }

        value::value(value::object_type &&value)
            : m_value(std::move(value))
        {
        }

        value::value(const value::string_type &value)
            : m_value(value)
        {
        }

        value::value(value::string_type &&value)
            : m_value(std::move(value))
        {
        }

        value::value(const char *value)
            : m_value(string_type(value != nullptr ? value : ""))
        {
        }

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

        value::~value() = default;

        value &value::operator=(const value &other) = default;

        value &value::operator=(value &&other) = default;

        value &value::operator=(null_type value)
        {
            m_value = value;
            return *this;
        }

        value &value::operator=(bool value)
        {
            m_value = value;
            return *this;
        }

        value &value::operator=(integer_type value)
        {
            m_value = value;
            return *this;
        }

        value &value::operator=(int value)
        {
            m_value = static_cast<integer_type>(value);
            return *this;
        }

        value &value::operator=(floating_type value)
        {
            m_value = value;
            return *this;
        }

        value &value::operator=(string_type value)
        {
            m_value = std::move(value);
            return *this;
        }

        value &value::operator=(const char *value)
        {
            m_value = string_type(value != nullptr ? value : "");
            return *this;
        }

        value &value::operator=(array_type value)
        {
            m_value = std::move(value);
            return *this;
        }

        value &value::operator=(object_type value)
        {
            m_value = std::move(value);
            return *this;
        }

        //========== Type Information ==========

        value::value_type value::type() const
        {
            return m_type;
        };

        template <class T>
        bool value::is() { return std::holds_alternative<T>(m_value); };

        bool value::is_null() const { return type() == value_type::null; };

        bool value::is_array() const { return type() == value_type::object; };

        bool value::is_object() const { return type() == value_type::object; };

        bool value::is_string() const { return type() == value_type::string; };

        bool value::is_integer() const { return type() == value_type::integer; };

        bool value::is_boolean() const { return type() == value_type::boolean; };

        bool value::is_floating() const { return type() == value_type::floating; };

        bool value::is_callable() const { return type() == value_type::callable; };

        //========== Type Conversions ==========

        template <class T>
        T value::as(const T &default_value)
        {
            if (is<T>())
                return value<T>();
            if constexpr (std::is_same<T, std::string>::value)
                return asString();
            if constexpr (std::is_same<T, long long>::value)
                return asInt();
            if constexpr (std::is_same<T, double>::value)
                return asDouble();
        };

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

        value value::clone() const { return value(*this); }

        std::ostream &operator<<(std::ostream &ostream, const Value *value)
        {
            if (value == nullptr)
            {
                return ostream << "<null>";
            }

            return ostream << value->Repr();
        }

    } // namespace runtime

} // namespace cora::compiler
