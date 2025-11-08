#ifndef CORA_RUNTIME_VARIABLE_H
#define CORA_RUNTIME_VARIABLE_H

#include "Cora/Runtime/Scope.hpp"
#include "Cora/Runtime/Value.hpp"

namespace cora
{
    namespace runtime
    {

        enum class VariableScope
        {
            Local,
            Global,
        };

        class Variable
        {
        public:
            Variable(Value *value, VariableScope type, bool constant);

            static Variable *LocalVariable(Value *value, bool constant = false)
            {
                return new Variable(value, VariableScope::Local, constant);
            }

            static Variable *GlobalVariable(Value *value, bool constant = false)
            {
                return new Variable(value, VariableScope::Global, constant);
            }

            /**
             * @brief Return the value object
             *
             * @return Value *
             */
            Value *GetValue() const { return m_Value; }

            /**
             * @brief Set the value object
             *
             * @param value
             */
            void SetValue(Value *value) noexcept { m_Value = value; }

            /**
             * @brief Return the value kind
             *
             * @return ValueKind
             */
            ValueKind GetKind() const { return m_Value->GetValueKind(); }

            /**
             * @brief Return the value scope
             *
             * @return VariableScope
             */
            VariableScope GetScope() const { return m_Scope; }

            bool IsConst() const { return m_Constant; }

            bool IsLocal() const { return m_Scope == VariableScope::Local; }

            bool IsGlobal() const { return m_Scope == VariableScope::Global; }

        private:
            Value *m_Value;
            VariableScope m_Scope;
            bool m_Constant;
        };

    } // namespace runtime

} // namespace cora

#endif // CORA_RUNTIME_VARIABLE_H
