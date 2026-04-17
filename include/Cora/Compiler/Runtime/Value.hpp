#ifndef CORA_COMPILER_RUNTIME_VALUE_H
#define CORA_COMPILER_RUNTIME_VALUE_H

#include <ostream>
#include <string>
#include <variant>

namespace cora::compiler
{
    namespace runtime
    {

        enum class ValueKind
        {
            Any,
            Null,
            Bool,
            Byte,
            Float,
            Array,
            Object,
            String,
            Integer,
            Pointer,
            Reference,
            Undefined,
        };

        class Value
        {
        public:
            using Data = std::variant<std::monostate, bool, double, std::string>;

            Value();
            explicit Value(ValueKind valuekind);
            Value(ValueKind valuekind, std::string kindstring);
            explicit Value(std::nullptr_t);
            explicit Value(bool value);
            explicit Value(double value);
            explicit Value(const std::string &value);
            explicit Value(const char *value);
            virtual ~Value() = default;

            virtual std::string Repr() const;

            ValueKind GetValueKind() const;
            std::string GetValueKindString() const;

            void SetValueKind(ValueKind valuekind) noexcept;
            void SetValueKindString(std::string kindstring) noexcept;

            const Data &GetData() const;
            void SetData(Data data);

            bool IsNull() const;
            bool IsBool() const;
            bool IsNumber() const;
            bool IsString() const;

            bool AsBool() const;
            double AsNumber() const;
            std::string AsString() const;

        private:
            ValueKind m_Kind;
            std::string m_KindString;
            Data m_Data;
        };

        struct Any : public Value
        {
            Any();
            std::string Repr() const override;
        };

        struct Null : public Value
        {
            Null();
            std::string Repr() const override;
        };

        struct Byte : public Value
        {
            Byte();
            std::string Repr() const override;
        };

        struct Float : public Value
        {
            Float();
            std::string Repr() const override;
        };

        struct Array : public Value
        {
            Array();
            std::string Repr() const override;
        };

        struct Object : public Value
        {
            Object();
            std::string Repr() const override;
        };

        struct String : public Value
        {
            String();
            explicit String(const std::string &value);
            std::string Repr() const override;
        };

        struct Integer : public Value
        {
            Integer();
            std::string Repr() const override;
        };

        struct Pointer : public Value
        {
            Pointer();
            std::string Repr() const override;
        };

        struct Reference : public Value
        {
            Reference();
            std::string Repr() const override;
        };

        struct Undefined : public Value
        {
            Undefined();
            std::string Repr() const override;
        };

        std::ostream &operator<<(std::ostream &ostream, const Value *value);

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_VALUE_H
