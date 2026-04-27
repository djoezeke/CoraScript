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

    private:
        IRValue *foldConstants(IROpcode op, runtime::Value left, runtime::Value right);
        void performGVN(std::vector<IRInstruction *> &block);
        void performDCE(std::vector<IRInstruction *> &block);
        void performFoldingAndGVN(std::vector<IRInstruction *> &block);

    private:
        std::unordered_map<ValueKey, IRInstruction *, KeyHasher> valueTable;
    };

} // namespace cora::ir

#endif // CORA_IR_IROPTIMIZER_H
