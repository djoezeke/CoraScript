#include "IROptimizer.hpp"

#include <algorithm>
#include <stdexcept>

namespace cora::ir
{
    IROptimizer::IROptimizer() = default;

    Value *IROptimizer::foldConstants(Instruction::OpKind op, int left, int right)
    {
        switch (op)
        {
        case Instruction::ADD:
            return new ConstantInt(left + right);
        case Instruction::MUL:
            return new ConstantInt(left * right);
        case Instruction::SUB:
            return new ConstantInt(left - right);
        case Instruction::DIV:
            return new ConstantInt(right == 0 ? 0 : left / right);
        default:
            return nullptr;
        }
    }

    void IROptimizer::performGVN(std::vector<Instruction *> &block)
    {
        valueTable.clear();

        for (auto it = block.begin(); it != block.end();)
        {
            Instruction *inst = *it;
            if (inst == nullptr || inst->hasSideEffects || inst->operands.size() < 2)
            {
                ++it;
                continue;
            }

            Value *left = inst->operands[0];
            Value *right = inst->operands[1];
            if ((inst->op == Instruction::ADD || inst->op == Instruction::MUL) && left > right)
            {
                std::swap(left, right);
            }

            ValueKey key = {inst->op, left, right};
            const auto found = valueTable.find(key);
            if (found != valueTable.end())
            {
                Instruction *existing = found->second;
                std::vector<Instruction *> usersToUpdate = inst->users;
                for (Instruction *user : usersToUpdate)
                {
                    for (Value *&operand : user->operands)
                    {
                        if (operand == inst)
                        {
                            operand = existing;
                            existing->addUse(user);
                        }
                    }
                }

                for (Value *operand : inst->operands)
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

    void IROptimizer::performDCE(std::vector<Instruction *> &block)
    {
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (auto it = block.begin(); it != block.end();)
            {
                Instruction *inst = *it;
                if (inst != nullptr && inst->users.empty() && !inst->hasSideEffects)
                {
                    for (Value *operand : inst->operands)
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

    void IROptimizer::performFoldingAndGVN(std::vector<Instruction *> &block)
    {
        for (auto it = block.begin(); it != block.end();)
        {
            Instruction *inst = *it;
            if (inst == nullptr)
            {
                it = block.erase(it);
                continue;
            }

            if (inst->operands.size() == 2 && isConstant(inst->operands[0]) && isConstant(inst->operands[1]))
            {
                const int left = static_cast<ConstantInt *>(inst->operands[0])->value;
                const int right = static_cast<ConstantInt *>(inst->operands[1])->value;
                Value *foldedValue = foldConstants(inst->op, left, right);
                if (foldedValue != nullptr)
                {
                    std::vector<Instruction *> usersToUpdate = inst->users;
                    for (Instruction *user : usersToUpdate)
                    {
                        for (Value *&operand : user->operands)
                        {
                            if (operand == inst)
                            {
                                operand = foldedValue;
                                foldedValue->addUse(user);
                            }
                        }
                    }

                    for (Value *operand : inst->operands)
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