#include "BytecodeEmitter.hpp"

#include <stdexcept>

namespace cora::vmachine
{
    namespace
    {
        static std::int32_t getLocalForValue(const std::map<cora::ir::Value *, std::int32_t> &valueToLocal, cora::ir::Value *value)
        {
            const auto found = valueToLocal.find(value);
            if (found == valueToLocal.end())
            {
                throw std::runtime_error("BytecodeEmitter: missing local for SSA value");
            }
            return found->second;
        }
    }

    std::int32_t BytecodeEmitter::allocateLocal()
    {
        return nextLocal++;
    }

    void BytecodeEmitter::emitValueLoad(cora::ir::Value *value)
    {
        if (value == nullptr)
        {
            output.push_back({OpCode::PushConst, 0, 0});
            return;
        }

        if (auto *constant = dynamic_cast<cora::ir::ConstantInt *>(value))
        {
            output.push_back({OpCode::PushConst, constant->value, 0});
            return;
        }

        const std::int32_t slot = getLocalForValue(valueToLocal, value);
        output.push_back({OpCode::LoadLocal, slot, 0});
    }

    void BytecodeEmitter::emitInstruction(const cora::ir::Instruction *inst)
    {
        if (inst == nullptr)
        {
            return;
        }

        switch (inst->op)
        {
        case cora::ir::Instruction::STORE:
        {
            if (!inst->operands.empty())
            {
                emitValueLoad(inst->operands.front());
            }

            const std::int32_t slot = allocateLocal();
            valueToLocal[const_cast<cora::ir::Instruction *>(inst)] = slot;
            if (!inst->name.empty())
            {
                nameToLocal[inst->name] = slot;
            }
            output.push_back({OpCode::StoreLocal, slot, 0});
            break;
        }
        case cora::ir::Instruction::PRINT:
            if (!inst->operands.empty())
            {
                emitValueLoad(inst->operands.front());
            }
            output.push_back({OpCode::Print, 0, 0});
            break;
        case cora::ir::Instruction::RETURN:
            if (!inst->operands.empty())
            {
                emitValueLoad(inst->operands.front());
            }
            output.push_back({OpCode::Return, 0, 0});
            break;
        case cora::ir::Instruction::ADD:
        case cora::ir::Instruction::MUL:
        case cora::ir::Instruction::SUB:
        case cora::ir::Instruction::DIV:
        {
            if (inst->operands.size() < 2)
            {
                throw std::runtime_error("BytecodeEmitter: binary instruction requires two operands");
            }

            emitValueLoad(inst->operands[0]);
            emitValueLoad(inst->operands[1]);

            const std::int32_t slot = allocateLocal();
            valueToLocal[const_cast<cora::ir::Instruction *>(inst)] = slot;

            OpCode op = OpCode::Add;
            if (inst->op == cora::ir::Instruction::MUL)
            {
                op = OpCode::Mul;
            }
            else if (inst->op == cora::ir::Instruction::SUB)
            {
                op = OpCode::Sub;
            }
            else if (inst->op == cora::ir::Instruction::DIV)
            {
                op = OpCode::Div;
            }

            output.push_back({op, slot, 0});
            output.push_back({OpCode::StoreLocal, slot, 0});
            break;
        }
        case cora::ir::Instruction::CONST:
        case cora::ir::Instruction::LOAD:
        case cora::ir::Instruction::PHI:
        case cora::ir::Instruction::BRANCH:
            break;
        }
    }

    BytecodeProgram BytecodeEmitter::Emit(const std::vector<cora::ir::BasicBlock *> &blocks)
    {
        output.clear();
        valueToLocal.clear();
        nameToLocal.clear();
        nextLocal = 0;

        for (const cora::ir::BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }

            for (const cora::ir::Instruction *inst : block->instructions)
            {
                emitInstruction(inst);
            }
        }

        output.push_back({OpCode::Halt, 0, 0});

        BytecodeProgram program;
        program.code = output;
        program.names.reserve(nameToLocal.size());
        for (const auto &entry : nameToLocal)
        {
            program.names.push_back(entry.first);
        }

        return program;
    }

} // namespace cora::vmachine
