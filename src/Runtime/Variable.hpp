#ifndef CORA_COMPILER_RUNTIME_VARIABLE_H
#define CORA_COMPILER_RUNTIME_VARIABLE_H

#include "Value.hpp"

namespace cora::compiler
{
    namespace runtime
    {

        class value;

        enum class VariableScope
        {
            Local,
            Global,
        };

        class Variable
        {
        public:
            Variable(value *value, VariableScope type, bool constant);

            static Variable *LocalVariable(value *value, bool constant = false);

            static Variable *GlobalVariable(value *value, bool constant = false);

            /**
             * @brief Return the value object
             *
             * @return value *
             */
            value *GetValue() const;

            /**
             * @brief Set the value object
             *
             * @param value
             */
            void SetValue(value *value);

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
            value *m_Value;
            VariableScope m_Scope;
            bool m_Constant;
        };

    } // namespace runtime

} // namespace cora::compiler

#endif // CORA_COMPILER_RUNTIME_VARIABLE_H
