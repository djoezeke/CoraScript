#ifndef CORA_RUNTIME_VALUE_H
#define CORA_RUNTIME_VALUE_H

#include <string>
#include <vector>

namespace cora
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
            Value() = default;
            virtual ~Value() {};

            Value(ValueKind valuekind) { m_Kind = valuekind; };

            Value(ValueKind valuekind, std::string kinsdtring)
            {
                m_Kind = valuekind;
                m_KindString = kinsdtring;
            };

            /**
             * @brief Compute a representation of the value.
             * @return String representation of the value
             */
            virtual std::string Repr() = 0;

            /**
             * @brief Return the value kind
             *
             * @return ValueKind
             */
            ValueKind GetValueKind() const { return m_Kind; };

            /**
             * @brief Return the value kind string
             *
             * @return const std::string&
             */
            std::string GetValueKindString() const { return m_KindString; };

            /**
             * @brief Set the value kind object
             *
             * @param valuekind
             */
            void SetValueKind(ValueKind valuekind) noexcept { m_Kind = valuekind; };

            /**
             * @brief Set the value kind string
             *
             * @param kindstring
             */
            void SetValueKindString(std::string kindstring) noexcept { m_KindString = kindstring; };

        private:
            ValueKind m_Kind;
            std::string m_KindString;
        };

        struct Any : public Value
        {
            virtual std::string Repr() override;
        };

        struct Null : public Value
        {
            virtual std::string Repr() override;
        };

        struct Byte : public Value
        {
            virtual std::string Repr() override;
        };

        struct Null : public Value
        {
            virtual std::string Repr() override;
        };

        struct Float : public Value
        {
            virtual std::string Repr() override;
        };

        struct Array : public Value
        {
            virtual std::string Repr() override;
        };

        struct Object : public Value
        {
            virtual std::string Repr() override;
        };

        struct String : public Value
        {
            virtual std::string Repr() override;
        };

        struct Integer : public Value
        {
            virtual std::string Repr() override;
        };

        struct Pointer : public Value
        {
            virtual std::string Repr() override;
        };

        struct Reference : public Value
        {
            virtual std::string Repr() override;
        };

        struct Undefined : public Value
        {
            virtual std::string Repr() override;
        };

        std::ostream &operator<<(std::ostream &ostream, const Value *value);

    } // namespace runtime

} // namespace cora

#endif // CORA_RUNTIME_VALUE_H
