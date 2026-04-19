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

        class Value;

        class Callable
        {
        public:
            virtual ~Callable() = default;
            virtual Value Call(const std::vector<Value> &arguments) = 0;
            virtual std::string Name() const = 0;
        };

        class Object
        {
        public:
            explicit Object(std::string className = "Object");

            std::string className;
            std::unordered_map<std::string, Value> fields;
            std::unordered_set<std::string> privateMembers;

            std::string Name() const;

            bool IsPrivateMember(const std::string &member) const;
            void SetMemberVisibility(const std::string &member, bool isPrivate);

        private:
            std::string m_Name;
        };

        class Method final : public Callable
        {
        public:
            using Func = std::function<Value(const std::vector<Value> &)>;

        public:
            Method(std::shared_ptr<Object> object, std::string name, Func function);

            Value Call(const std::vector<Value> &arguments) override;
            std::string Name() const override;

        private:
            std::string m_Name;
            Func m_Function;
            std::shared_ptr<Object> m_Object;
        };

        class Function final : public Callable
        {
        public:
            using Func = std::function<Value(const std::vector<Value> &)>;

        public:
            Function(std::string name, Func function);

            Value Call(const std::vector<Value> &arguments) override;
            std::string Name() const override;

        private:
            std::string m_Name;
            Func m_Function;
        };

        class BoundMethod final : public Callable
        {
        public:
            BoundMethod(std::shared_ptr<Object> receiver, std::shared_ptr<Callable> callable, std::string name);

            Value Call(const std::vector<Value> &arguments) override;
            std::string Name() const override;

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

        class Value
        {
        public:
            using ObjectPtr = std::shared_ptr<Object>;
            using CallablePtr = std::shared_ptr<Callable>;
            using Data = std::variant<std::monostate, bool, double, std::string, ObjectPtr, CallablePtr>;

        public:
            Value();
            explicit Value(ValueKind valuekind);
            explicit Value(std::nullptr_t);
            explicit Value(bool value);
            explicit Value(double value);
            explicit Value(const std::string &value);
            explicit Value(const char *value);
            explicit Value(ObjectPtr object);
            explicit Value(CallablePtr callable);

            explicit Value(Method method);
            explicit Value(Function function);
            explicit Value(BoundMethod method);

            explicit Value(std::shared_ptr<Method> method);
            explicit Value(std::shared_ptr<Function> function);
            explicit Value(std::shared_ptr<BoundMethod> method);

            virtual std::string Repr() const;

            ValueKind GetValueKind() const;
            std::string GetValueKindString() const;

            void SetValueKind(ValueKind valuekind) noexcept;

            const Data &GetData() const;
            void SetData(Data data);

            bool IsNull() const;
            bool IsBool() const;
            bool IsNumber() const;
            bool IsString() const;
            bool IsObject() const;
            bool IsCallable() const;

            bool AsBool() const;
            double AsNumber() const;
            std::string AsString() const;
            ObjectPtr AsObject() const;
            CallablePtr AsCallable() const;

            virtual ~Value() = default;

        private:
            ValueKind m_Kind;
            Data m_Data;
        };

        std::ostream &operator<<(std::ostream &ostream, const Value *value);

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_VALUE_H
