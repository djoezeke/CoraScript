#ifndef CORA_COMPILER_RUNTIME_VARIABLE_H
#define CORA_COMPILER_RUNTIME_VARIABLE_H

#include "Cora/Compiler/Runtime/Value.hpp"

namespace cora::compiler
{
    namespace runtime
    {

        class Value;

        enum class VariableScope
        {
            Local,
            Global,
        };

        class Variable
        {
        public:
            Variable(Value *value, VariableScope type, bool constant);

            static Variable *LocalVariable(Value *value, bool constant = false);

            static Variable *GlobalVariable(Value *value, bool constant = false);

            /**
             * @brief Return the value object
             *
             * @return Value *
             */
            Value *GetValue() const;

            /**
             * @brief Set the value object
             *
             * @param value
             */
            void SetValue(Value *value);

            /**
             * @brief Return the value kind
             *
             * @return ValueKind
             */
            ValueKind GetKind() const;

            /**
             * @brief Return the value scope
             *
             * @return VariableScope
             */
            VariableScope GetScope() const;

            [[nodiscard]] bool IsConst() const;

            [[nodiscard]] bool isLocal() const;

            [[nodiscard]] bool isGlobal() const;

            ~Variable();

        private:
            Value *m_Value;
            VariableScope m_Scope;
            bool m_Constant;
        };

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_VARIABLE_H
