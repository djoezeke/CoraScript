#include "Cora/Compiler/Runtime/Value.hpp"

#include <sstream>
#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {
        Value::Value()
            : m_Kind(ValueKind::Undefined), m_Data(std::monostate{}) {}

        Value::Value(ValueKind valuekind)
            : m_Kind(valuekind), m_Data(std::monostate{}) {}

        Value::Value(ValueKind valuekind, std::string kindstring)
            : m_Kind(valuekind), m_KindString(std::move(kindstring)), m_Data(std::monostate{}) {}

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
            return m_KindString;
        }

        void Value::SetValueKind(ValueKind valuekind) noexcept
        {
            m_Kind = valuekind;
        }

        void Value::SetValueKindString(std::string kindstring) noexcept
        {
            m_KindString = std::move(kindstring);
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
            return !std::get<std::string>(m_Data).empty();
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
            return std::get<std::string>(m_Data);
        }

        Any::Any()
            : Value(ValueKind::Any, "Any") {}

        std::string Any::Repr() const
        {
            return "Any";
        }

        Null::Null()
            : Value(nullptr) {}

        std::string Null::Repr() const
        {
            return "null";
        }

        Byte::Byte()
            : Value(ValueKind::Byte, "Byte") {}

        std::string Byte::Repr() const
        {
            return "Byte";
        }

        Float::Float()
            : Value(ValueKind::Float, "Float") {}

        std::string Float::Repr() const
        {
            return "Float";
        }

        Array::Array()
            : Value(ValueKind::Array, "Array") {}

        std::string Array::Repr() const
        {
            return "Array";
        }

        Object::Object()
            : Value(ValueKind::Object, "Object") {}

        std::string Object::Repr() const
        {
            return "Object";
        }

        String::String()
            : Value(std::string())
        {
            SetValueKind(ValueKind::String);
            SetValueKindString("String");
        }

        String::String(const std::string &value)
            : Value(value)
        {
            SetValueKind(ValueKind::String);
            SetValueKindString("String");
        }

        std::string String::Repr() const
        {
            return AsString();
        }

        Integer::Integer()
            : Value(ValueKind::Integer, "Integer") {}

        std::string Integer::Repr() const
        {
            return "Integer";
        }

        Pointer::Pointer()
            : Value(ValueKind::Pointer, "Pointer") {}

        std::string Pointer::Repr() const
        {
            return "Pointer";
        }

        Reference::Reference()
            : Value(ValueKind::Reference, "Reference") {}

        std::string Reference::Repr() const
        {
            return "Reference";
        }

        Undefined::Undefined()
            : Value(ValueKind::Undefined, "Undefined") {}

        std::string Undefined::Repr() const
        {
            return "Undefined";
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
