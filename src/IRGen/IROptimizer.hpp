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
        Value *foldConstants(Instruction::OpKind op, int left, int right);
        void performGVN(std::vector<Instruction *> &block);
        void performDCE(std::vector<Instruction *> &block);
        void performFoldingAndGVN(std::vector<Instruction *> &block);

    private:
        std::unordered_map<ValueKey, Instruction *, KeyHasher> valueTable;
    };

} // namespace cora::ir

#endif // CORA_IR_IROPTIMIZER_H
