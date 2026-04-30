#ifndef CORA_COMPILER_RUNTIME_VALUE_H
#define CORA_COMPILER_RUNTIME_VALUE_H

#include <functional>
#include <memory>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

namespace cora::compiler
{
    namespace runtime
    {

        class value;

        class Callable
        {
        public:
            virtual ~Callable() = default;
            virtual value Call(const std::vector<value> &arguments) = 0;
            virtual std::string Name() const = 0;
            virtual int Arity() const;
            virtual std::string Doc() const;
            virtual void SetDoc(std::string doc);

        protected:
            std::string m_Doc;
        };

        class Object
        {
        public:
            Object(std::string className = "Object");

            std::string className;
            std::unordered_map<std::string, value> fields;
            std::unordered_set<std::string> privateMembers;
            std::unordered_set<std::string> constMembers;
            std::unordered_set<std::string> initializedConstMembers;

            std::string Name() const;

            bool IsPrivateMember(const std::string &member) const;
            void SetMemberVisibility(const std::string &member, bool isPrivate);
            bool IsConstMember(const std::string &member) const;
            bool IsConstMemberInitialized(const std::string &member) const;
            void SetMemberConstness(const std::string &member, bool isConst, bool initialized);
            void MarkConstMemberInitialized(const std::string &member);

        private:
            std::string m_Name;
        };

        class Method final : public Callable
        {
        public:
            using Func = std::function<value(const std::vector<value> &)>;

        public:
            Method(std::shared_ptr<Object> object, std::string name, Func function, int arity = -1);

            value Call(const std::vector<value> &arguments) override;
            std::string Name() const override;
            int Arity() const override;

        private:
            std::string m_Name;
            Func m_Function;
            std::shared_ptr<Object> m_Object;
            int m_Arity;
        };

        class Function final : public Callable
        {
        public:
            using Func = std::function<value(const std::vector<value> &)>;

        public:
            Function(std::string name, Func function, int arity = -1);

            value Call(const std::vector<value> &arguments) override;
            std::string Name() const override;
            int Arity() const override;

        private:
            std::string m_Name;
            Func m_Function;
            int m_Arity;
        };

        class BoundMethod final : public Callable
        {
        public:
            BoundMethod(std::shared_ptr<Object> receiver, std::shared_ptr<Callable> callable, std::string name);

            value Call(const std::vector<value> &arguments) override;
            std::string Name() const override;
            int Arity() const override;

        private:
            std::shared_ptr<Object> m_Receiver;
            std::shared_ptr<Callable> m_Callable;
            std::string m_Name;
        };

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
            Callable,
            Undefined,
        };

        /**
         * @enum value_t
         * @brief Defines all possible value types.
         */
        enum class value_t : uint8_t
        {
            null,     ///< null value
            object,   ///< object
            array,    ///< array (vector of values)
            string,   ///< string value
            integer,  ///< integer value
            boolean,  ///< boolean value
            floating, ///< numeric value (floating point)
            callable, ///< callable value
        };

        class value
        {
        public:
            /**
             * @name container
             * {@
             */

            /**
             * @brief A type for a value object.
             */
            using value_type = value_t;

            /**
             * @brief A type for a pointer to a value object.
             */
            using pointer = value *;

            /**
             * @brief A type for a reference to a value object.
             */
            using reference = value &;

            /**
             * @brief A type for a constant pointer to a value object.
             */
            using const_pointer = const value *;

            /**
             * @brief A type for a constant reference to a value object.
             */
            using const_reference = const value &;

            /**
             * @brief A type to represent differences between value iterators.
             */
            using difference_type = std::ptrdiff_t;

            /**
             * @brief A type to represent value sizes.
             */
            using size_type = std::size_t;

            /* @} container */

            /**
             * @name types
             * @brief Type aliases for convenience.
             * {@
             */

            /**
             * @brief A type for a null value.
             */
            using null_type = std::nullptr_t;

            /**
             * @brief A type for an array value.
             */
            using array_type = std::vector<value>;

            /**
             * @brief A type for an object value.
             */
            using object_type = std::shared_ptr<Object>;

            /**
             * @brief A type for a string value.
             */
            using string_type = std::string;

            /**
             * @brief A type for a integer value.
             */
            using integer_type = int64_t;

            /**
             * @brief A type for a boolean value.
             */
            using boolean_type = bool;

            /**
             * @brief A type for a float number value.
             */
            using floating_type = double;

            /**
             * @brief A type for a callable value.
             */
            using callable_type = std::shared_ptr<Callable>;

            /* @} types */

        public:
            //========== Constructors ==========

            /**
             * @brief Construct a null value.
             */
            value();

            // value(Method method);
            // value(Function function);
            // value(BoundMethod method);

            // value(std::shared_ptr<Method> method);
            // value(std::shared_ptr<Function> function);
            // value(std::shared_ptr<BoundMethod> method);

            /**
             * @brief Construct an empty value of type.
             */
            value(value_type value);

            /**
             * @brief Construct null explicitly.
             */
            value(null_type value);

            /**
             * @brief Construct a integer value.
             */
            value(integer_type value);

            /**
             * @brief Construct a integer value from int.
             */
            value(int value);

            /**
             * @brief Construct a boolean value.
             */
            value(boolean_type value);

            /**
             * @brief Construct a floating-point value.
             */
            value(floating_type value);

            /**
             * @brief Construct a array value from a copy an array object.
             * @param[in] value A lvalue array node value.
             * @return A value array node.
             */
            value(const array_type &value);

            /**
             * @brief Construct a array value by moving a array object.
             * @param[in] value A rvalue array node value.
             * @return A value array node.
             */
            value(array_type &&value);

            /**
             * @brief Construct a object by copy.
             * @param[in] value A lvalue object node value.
             * @return A value object node.
             */
            value(const object_type &value);

            /**
             * @brief Construct a object by move.
             * @param[in] value A rvalue object node value.
             * @return A value object node.
             */
            value(object_type &&value);

            /**
             * @brief Construct a callable value.
             */
            value(callable_type value);

            /**
             * @brief Construct a string by copy.
             * @param[in] value A lvalue string node value.
             * @return A value string node.
             */
            value(const string_type &value);

            /**
             * @brief Construct a string by move.
             * @param[in] value A rvalue string node value.
             * @return A value string node.
             */
            value(string_type &&value);

            /**
             * @brief Construct a string from a C string.
             */
            value(const char *value);

            /**
             * @brief Copy constructor.
             */
            value(const value &other);

            /**
             * @brief Move constructor.
             */
            value(value &&other);

            // Convenience API for the VM/FFI layer.
            ValueKind GetValueKind() const;
            void SetValueKind(ValueKind kind);
            bool IsNull() const;
            bool IsBool() const;
            bool IsString() const;
            bool IsInteger() const;
            bool IsFloat() const;
            bool IsNumber() const;
            bool IsArray() const;
            bool IsObject() const;
            bool IsCallable() const;

            double AsNumber() const;
            std::string AsString() const;
            bool AsBool() const;
            object_type AsObject() const;
            callable_type AsCallable() const;

            virtual std::string Repr() const;

            //========== Type Information ==========

            template <typename T>
            bool is();

            /**
             * @brief Check this value type.
             * @return the value value type.
             */
            value_type type() const;

            /**
             * @brief Check whether this value is null.
             * @return true if value is null else false.
             */
            bool is_null() const;

            /**
             * @brief Check whether this value is an object.
             * @return true if value is an object else false.
             */
            bool is_object() const;

            /**
             * @brief Check whether this value is an array.
             * @return true if value is an array else false.
             */
            bool is_array() const;

            /**
             * @brief Check whether this value is a string.
             * @return true if value is a string else false.
             */
            bool is_string() const;

            /**
             * @brief Check whether this value is an integer.
             * @return true if value is an integer else false.
             */
            bool is_integer() const;

            /**
             * @brief Check whether this value is a boolean.
             * @return true if value is null else false.
             */
            bool is_boolean() const;

            /**
             * @brief Check whether this value is a float number.
             * @return true if value is a float number else false.
             */
            bool is_floating() const;

            /**
             * @brief Check whether this value is a callable.
             * @return true if value is callable else false.
             */
            bool is_callable() const;

            //========== Type Conversions ==========

            /**
             * @brief Get value as target type T with default fallback.
             * @tparam T The target type (bool, int, double, string, etc.)
             * @param default_value The value to return if conversion fails.
             * @return Converted value or default_value.
             */
            // template <typename T>
            // T as(const T &default_value = T()) const;

            /**
             * @brief Get value as an array type .
             * @return The converted array value.
             * @throws cast_error if conversion fails.
             */
            array_type &as_array();

            /**
             * @brief Get value as an object type .
             * @return The converted object value.
             * @throws cast_error if conversion fails.
             */
            object_type &as_object();

            /**
             * @brief Get value as a string type .
             * @return The converted string value.
             * @throws cast_error if conversion fails.
             */
            string_type &as_string();

            /**
             * @brief Get value as an integer type .
             * @return The converted integer value.
             * @throws cast_error if conversion fails.
             */
            integer_type &as_integer();

            /**
             * @brief Get value as a boolean type .
             * @return The converted boolean value.
             * @throws cast_error if conversion fails.
             */
            boolean_type &as_boolean();

            /**
             * @brief Get value as a float number type .
             * @return The converted float number value.
             * @throws cast_error if conversion fails.
             */
            floating_type &as_floating();

            /**
             * @brief Get value as an array type .
             * @return The converted array value (const).
             * @throws cast_error if conversion fails.
             */
            const array_type &as_array() const;

            /**
             * @brief Get value as an object type .
             * @return The converted object value (const).
             * @throws cast_error if conversion fails.
             */
            const object_type &as_object() const;

            /**
             * @brief Get value as a string type .
             * @return The converted string value (const).
             * @throws cast_error if conversion fails.
             */
            const string_type &as_string() const;

            /**
             * @brief Get value as an integer type .
             * @return The converted integer value (const).
             * @throws cast_error if conversion fails.
             */
            const integer_type &as_integer() const;

            /**
             * @brief Get value as a boolean type .
             * @return The converted boolean value (const).
             * @throws cast_error if conversion fails.
             */
            const boolean_type &as_boolean() const;

            /**
             * @brief Get value as a float number type .
             * @return The converted float number value (const).
             * @throws cast_error if conversion fails.
             */
            const floating_type &as_floating() const;

            //========== Assignment ==========

            /**
             * @brief Copy assignment.
             */
            reference operator=(const value &other);

            /**
             * @brief Move assignment.
             */
            reference operator=(value &&other);

            /**
             * @brief Assign an int value.
             */
            reference operator=(int value);

            /**
             * @brief Assign a C-string value.
             * @note nullptr becomes empty string.
             */
            reference operator=(const char *value);

            /**
             * @brief Assign a null value.
             */
            reference operator=(null_type value);

            /**
             * @brief Assign an array value.
             */
            reference operator=(array_type value);

            /**
             * @brief Assign an object value.
             */
            reference operator=(object_type value);

            /**
             * @brief Assign a string value.
             */
            reference operator=(string_type value);

            /**
             * @brief Assign a integer value.
             */
            reference operator=(integer_type value);

            /**
             * @brief Assign a boolean value.
             */
            reference operator=(boolean_type value);

            /**
             * @brief Assign a floating-point numeric value.
             */
            reference operator=(floating_type value);

            //========== Comparison ==========

            bool operator<(const value &other) const;
            bool operator>(const value &other) const;
            bool operator==(const value &other) const;
            bool operator!=(const value &other) const;
            bool operator<=(const value &other) const;
            bool operator>=(const value &other) const;
            bool operator&&(const value &other);
            bool operator||(const value &other);

            //========== operator ==========

            value operator+(value &rhs);
            value operator-(value &rhs);
            value operator*(value &rhs);
            value operator/(value &rhs);
            value operator%(value &rhs);
            value operator&(value &rhs);
            value operator|(value &rhs);
            value operator^(value &rhs);
            value operator<<(value &rhs);
            value operator>>(value &rhs);

            value operator~();
            value operator!();

            //========== Utility ==========
            template <typename T>
            T *value_ptr();
            bool has_value();

            void reset();

            /**
             * @brief Deep-copy this value.
             */
            value clone() const;

            virtual ~value() = default;

        private:
            using data_type = std::variant<std::monostate, null_type, boolean_type, integer_type, floating_type, string_type, object_type, array_type, callable_type>;

            value_type m_type;
            ValueKind m_kind{ValueKind::Undefined};
            data_type m_value;

            void SetType(value_type type);
        };

        using Value = value;

        std::ostream &operator<<(std::ostream &ostream, const value &value);
        std::ostream &operator<<(std::ostream &ostream, const value *value);

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_VALUE_H
