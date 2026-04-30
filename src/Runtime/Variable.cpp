#include "Variable.hpp"
#include "Value.hpp"

#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {
        Variable::Variable(value *value, VariableScope type, bool constant)
            : m_Value(value), m_Scope(type), m_Constant(constant) {}

        Variable *Variable::LocalVariable(value *value, bool constant)
        {
            return new Variable(value, VariableScope::Local, constant);
        };

        Variable *Variable::GlobalVariable(value *value, bool constant)
        {
            return new Variable(value, VariableScope::Global, constant);
        };

        value *Variable::GetValue() const { return m_Value; };

        void Variable::SetValue(value *value)
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
