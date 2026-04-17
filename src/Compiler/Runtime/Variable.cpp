#include "Cora/Compiler/Runtime/Variable.hpp"

#include <stdexcept>

namespace cora::compiler
{
    namespace runtime
    {
        Variable::Variable(Value *value, VariableScope type, bool constant)
            : m_Value(value), m_Scope(type), m_Constant(constant) {}

        Variable::~Variable()
        {
            delete m_Value;
        }

        void Variable::SetValue(Value *value)
        {
            if (m_Constant)
            {
                delete value;
                throw std::runtime_error("Cannot assign to constant variable");
            }

            delete m_Value;
            m_Value = value;
        }

    } // namespace runtime

} // namespace cora::compiler
