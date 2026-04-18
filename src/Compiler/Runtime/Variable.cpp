#include "Cora/Compiler/Runtime/Variable.hpp"
#include "Cora/Compiler/Runtime/Value.hpp"

#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {
        Variable::Variable(Value *value, VariableScope type, bool constant)
            : m_Value(value), m_Scope(type), m_Constant(constant) {}

        Variable *Variable::LocalVariable(Value *value, bool constant)
        {
            return new Variable(value, VariableScope::Local, constant);
        };

        Variable *Variable::GlobalVariable(Value *value, bool constant)
        {
            return new Variable(value, VariableScope::Global, constant);
        };

        Value *Variable::GetValue() const { return m_Value; };

        void Variable::SetValue(Value *value)
        {
            if (m_Constant)
            {
                delete value;
                throw std::runtime_error("Cannot assign to constant variable");
            }

            delete m_Value;
            m_Value = value;
        };

        ValueKind Variable::GetKind() const { return m_Value->GetValueKind(); };

        VariableScope Variable::GetScope() const { return m_Scope; };

        bool Variable::IsConst() const { return m_Constant; };

        bool Variable::isLocal() const { return m_Scope == VariableScope::Local; };

        bool Variable::isGlobal() const { return m_Scope == VariableScope::Global; };

        Variable::~Variable()
        {
            delete m_Value;
        };

    } // namespace runtime

} // namespace cora::compiler
