#ifndef CORA_IR_IROPTIMIZER_H
#define CORA_IR_IROPTIMIZER_H

#include "IRInstruction.hpp"

#include <unordered_map>

namespace cora::ir
{
    class IROptimizer final
    {
    public:
        IROptimizer();

        void Optimize(std::vector<BasicBlock *> &blocks);

        ~IROptimizer();
    };

} // namespace cora::ir

#endif // CORA_IR_IROPTIMIZER_H
