#include "IROptimizer.hpp"

#include <algorithm>
#include <stdexcept>

namespace cora::ir
{
    IROptimizer::IROptimizer() = default;

    IRValue *IROptimizer::foldConstants(IROpcode op, runtime::Value left, runtime::Value right)
    {
        switch (op)
        {
        case IROpcode::ADD:
            return new ConstInstruction(runtime::Value(left.AsNumber() + right.AsNumber()));
        case IROpcode::MUL:
            return new ConstInstruction(runtime::Value(left.AsNumber() * right.AsNumber()));
        case IROpcode::SUB:
            return new ConstInstruction(runtime::Value(left.AsNumber() - right.AsNumber()));
        case IROpcode::DIV:
            return new ConstInstruction(runtime::Value(left.AsNumber() == 0 ? 0 : left.AsNumber() / right.AsNumber()));
        default:
            return nullptr;
        }
    };

    void IROptimizer::performGVN(std::vector<IRInstruction *> &block)
    {
        valueTable.clear();

        for (auto it = block.begin(); it != block.end();)
        {
            IRInstruction *inst = *it;
            if (inst == nullptr || inst->hasSideEffects || inst->operands.size() < 2)
            {
                ++it;
                continue;
            }

            IRValue *left = inst->operands[0];
            IRValue *right = inst->operands[1];
            if ((inst->op == IROpcode::ADD || inst->op == IROpcode::MUL) && left > right)
            {
                std::swap(left, right);
            }

            ValueKey key = {inst->op, left, right};
            const auto found = valueTable.find(key);
            if (found != valueTable.end())
            {
                IRInstruction *existing = found->second;
                std::vector<IRInstruction *> usersToUpdate = inst->users;
                for (IRInstruction *user : usersToUpdate)
                {
                    for (IRValue *&operand : user->operands)
                    {
                        if (operand == inst)
                        {
                            operand = existing;
                            existing->addUse(user);
                        }
                    }
                }

                for (IRValue *operand : inst->operands)
                {
                    operand->removeUse(inst);
                }

                it = block.erase(it);
                continue;
            }

            valueTable[key] = inst;
            ++it;
        }
    }

    void IROptimizer::performDCE(std::vector<IRInstruction *> &block)
    {
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto it = block.begin(); it != block.end();)
            {
                IRInstruction *inst = *it;
                if (inst != nullptr && inst->users.empty() && !inst->hasSideEffects)
                {
                    for (IRValue *operand : inst->operands)
                    {
                        operand->removeUse(inst);
                    }
                    it = block.erase(it);
                    changed = true;
                    continue;
                }

                ++it;
            }
        }
    }

    void IROptimizer::performFoldingAndGVN(std::vector<IRInstruction *> &block)
    {
        for (auto it = block.begin(); it != block.end();)
        {
            IRInstruction *inst = *it;
            if (inst == nullptr)
            {
                it = block.erase(it);
                continue;
            }

            if (inst->operands.size() == 2 && isConstant(inst->operands[0]) && isConstant(inst->operands[1]))
            {
                const int left = static_cast<ConstantInt *>(inst->operands[0])->value;
                const int right = static_cast<ConstantInt *>(inst->operands[1])->value;
                IRValue *foldedValue = foldConstants(inst->op, left, right);
                if (foldedValue != nullptr)
                {
                    std::vector<IRInstruction *> usersToUpdate = inst->users;
                    for (IRInstruction *user : usersToUpdate)
                    {
                        for (IRValue *&operand : user->operands)
                        {
                            if (operand == inst)
                            {
                                operand = foldedValue;
                                foldedValue->addUse(user);
                            }
                        }
                    }

                    for (IRValue *operand : inst->operands)
                    {
                        operand->removeUse(inst);
                    }

                    it = block.erase(it);
                    continue;
                }
            }

            ++it;
        }

        performGVN(block);
        performDCE(block);
    }

    void IROptimizer::Optimize(std::vector<BasicBlock *> &blocks)
    {
        for (BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }

            performFoldingAndGVN(block->instructions);
        }
    }

    IROptimizer::~IROptimizer() = default;
}