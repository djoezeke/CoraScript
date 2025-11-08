#include "Cora/Runtime/Variable.hpp"

namespace cora
{
    namespace runtime
    {
        Variable::Variable(Value *value, VariableScope type, bool constant)
            : m_Value(value), m_Scope(type), m_Constant(constant) {}

    } // namespace runtime

} // namespace cora
