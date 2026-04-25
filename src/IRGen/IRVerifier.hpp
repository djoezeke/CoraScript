#ifndef CORA_IR_IRVERIFIER_H
#define CORA_IR_IRVERIFIER_H

#include "IRInstruction.hpp"

#include <vector>

namespace cora::ir
{
    class IRVerifier final
    {
    public:
        IRVerifier();

        void Verify(const std::vector<BasicBlock *> &blocks) const;

        ~IRVerifier();

    private:
    };

} // namespace cora::ir

#endif // CORA_IR_IRVERIFIER_H
