#include "IRVerifier.hpp"

#include <stdexcept>

namespace cora::ir
{
    IRVerifier::IRVerifier() = default;

    void IRVerifier::Verify(const std::vector<BasicBlock *> &blocks) const
    {
        for (const BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                throw std::runtime_error("IRVerifier: null basic block encountered");
            }

            for (const PhiInstruction *phi : block->phis)
            {
                if (phi == nullptr)
                {
                    throw std::runtime_error("IRVerifier: null phi instruction encountered");
                }
            }

            for (const Instruction *inst : block->instructions)
            {
                if (inst == nullptr)
                {
                    throw std::runtime_error("IRVerifier: null instruction encountered");
                }

                for (Value *operand : inst->operands)
                {
                    if (operand == nullptr)
                    {
                        throw std::runtime_error("IRVerifier: null instruction operand encountered");
                    }
                }
            }
        }
    }

    IRVerifier::~IRVerifier() = default;

} // namespace cora::ir
