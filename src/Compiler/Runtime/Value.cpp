#include "Cora/Compiler/Runtime/Value.hpp"

#include <sstream>
#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {

        Object::Object(std::string className)
            : className(std::move(className)), fields(), privateMembers(), m_Name(this->className) {}

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

        ///////////////////////////////////////////

        Method::Method(std::shared_ptr<Object> object, std::string name, Func function)
            : m_Name(std::move(name)), m_Object(std::move(object)), m_Function(std::move(function)) {};

        Value Method::Call(const std::vector<Value> &arguments)
        {
            return m_Function(arguments);
        };

        std::string Method::Name() const
        {
            return m_Object->Name() + m_Name;
        };

        Function::Function(std::string name, Func function)
            : m_Name(std::move(name)), m_Function(std::move(function)) {};

        Value Function::Call(const std::vector<Value> &arguments)
        {
            return m_Function(arguments);
        };

        std::string Function::Name() const
        {
            return m_Name;
        };

        ///////////////////////////////////////////

        NativeFunction::NativeFunction(std::string name, Fn fn)
            : m_Name(std::move(name)), m_Fn(std::move(fn)) {}

        Value NativeFunction::Call(const std::vector<Value> &arguments)
        {
            return m_Fn(arguments);
        }

        std::string NativeFunction::Name() const
        {
            return m_Name;
        }

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

        Value::Value()
            : m_Kind(ValueKind::Undefined), m_Data(std::monostate{}) {}

        Value::Value(ValueKind valuekind)
            : m_Kind(valuekind), m_Data(std::monostate{}) {}

        Value::Value(std::nullptr_t)
            : m_Kind(ValueKind::Null), m_Data(std::monostate{}) {}

        Value::Value(bool value)
            : m_Kind(ValueKind::Bool), m_Data(value) {}

        Value::Value(double value)
            : m_Kind(ValueKind::Float), m_Data(value) {}

        Value::Value(const std::string &value)
            : m_Kind(ValueKind::String), m_Data(value) {}

        Value::Value(const char *value)
            : m_Kind(ValueKind::String), m_Data(std::string(value)) {}

        Value::Value(ObjectPtr object)
            : m_Kind(ValueKind::Object), m_Data(std::move(object)) {}

        Value::Value(CallablePtr callable)
            : m_Kind(ValueKind::Callable), m_Data(std::move(callable)) {}

        std::string Value::Repr() const
        {
            return AsString();
        }

        ValueKind Value::GetValueKind() const
        {
            return m_Kind;
        }

        std::string Value::GetValueKindString() const
        {
            switch (m_Kind)
            {
            case ValueKind::Null:
                return "null";
            case ValueKind::Bool:
                return "bool";
            case ValueKind::Byte:
                return "byte";
            case ValueKind::Float:
                return "float";
            case ValueKind::Array:
                return "array";
            case ValueKind::Object:
                return "object";
            case ValueKind::String:
                return "string";
            case ValueKind::Integer:
                return "integer";
            case ValueKind::Pointer:
                return "pointer";
            case ValueKind::Reference:
                return "reference";
            case ValueKind::Callable:
                return "callable";
            case ValueKind::Undefined:
                return "undefined";
            case ValueKind::Any:
            default:
                return "any";
            }
        }

        void Value::SetValueKind(ValueKind valuekind) noexcept
        {
            m_Kind = valuekind;
        }

        const Value::Data &Value::GetData() const
        {
            return m_Data;
        }

        void Value::SetData(Data data)
        {
            m_Data = std::move(data);
        }

        bool Value::IsNull() const
        {
            return std::holds_alternative<std::monostate>(m_Data);
        }

        bool Value::IsBool() const
        {
            return std::holds_alternative<bool>(m_Data);
        }

        bool Value::IsNumber() const
        {
            return std::holds_alternative<double>(m_Data);
        }

        bool Value::IsString() const
        {
            return std::holds_alternative<std::string>(m_Data);
        }

        bool Value::IsObject() const
        {
            return std::holds_alternative<ObjectPtr>(m_Data);
        }

        bool Value::IsCallable() const
        {
            return std::holds_alternative<CallablePtr>(m_Data);
        }

        bool Value::AsBool() const
        {
            if (IsNull())
            {
                return false;
            }
            if (IsBool())
            {
                return std::get<bool>(m_Data);
            }
            if (IsNumber())
            {
                return std::get<double>(m_Data) != 0.0;
            }
            if (IsString())
            {
                return !std::get<std::string>(m_Data).empty();
            }
            if (IsObject() || IsCallable())
            {
                return true;
            }
            return false;
        }

        double Value::AsNumber() const
        {
            if (!IsNumber())
            {
                throw std::runtime_error("Expected numeric value");
            }
            return std::get<double>(m_Data);
        }

        std::string Value::AsString() const
        {
            if (IsNull())
            {
                return "null";
            }
            if (IsBool())
            {
                return std::get<bool>(m_Data) ? "true" : "false";
            }
            if (IsNumber())
            {
                std::ostringstream out;
                out << std::get<double>(m_Data);
                return out.str();
            }
            if (IsString())
            {
                return std::get<std::string>(m_Data);
            }
            if (IsObject())
            {
                auto object = std::get<ObjectPtr>(m_Data);
                return "<object " + (object ? object->className : std::string("null")) + ">";
            }
            if (IsCallable())
            {
                auto callable = std::get<CallablePtr>(m_Data);
                return "<callable " + (callable ? callable->Name() : std::string("null")) + ">";
            }
            return "<value>";
        }

        Value::ObjectPtr Value::AsObject() const
        {
            if (!IsObject())
            {
                return nullptr;
            }
            return std::get<ObjectPtr>(m_Data);
        }

        Value::CallablePtr Value::AsCallable() const
        {
            if (!IsCallable())
            {
                return nullptr;
            }
            return std::get<CallablePtr>(m_Data);
        }

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
