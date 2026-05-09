#include "BytecodeEmitter.hpp"

#include <stdexcept>
#include <unordered_map>

namespace cora::vmachine
{
    namespace
    {
        struct Fixup
        {
            std::size_t at;
            const cora::ir::BasicBlock *target;
        };

        constexpr std::int32_t kMaxRegisters = 256;

        std::string ValueName(const cora::ir::Value *value)
        {
            if (value == nullptr)
            {
                return {};
            }
            return value->name;
        }
    } // namespace

    BytecodeProgram BytecodeEmitter::Emit(const std::vector<cora::ir::BasicBlock *> &blocks)
    {
        BytecodeProgram program;

        std::unordered_map<const cora::ir::Value *, std::int32_t> valueToRegister;
        std::unordered_map<const cora::ir::FunctionValue *, std::int32_t> functionConstants;
        std::unordered_map<std::string, std::int32_t> nameToIndex;
        std::unordered_map<const cora::ir::BasicBlock *, std::size_t> blockToIp;
        std::vector<Fixup> fixups;
        std::int32_t nextRegister = 0;

        auto allocateRegister = [&]() -> std::int32_t
        {
            if (nextRegister >= kMaxRegisters)
            {
                throw std::runtime_error("BytecodeEmitter: register limit exceeded");
            }
            return nextRegister++;
        };

        auto getNameIndex = [&](const std::string &name) -> std::int32_t
        {
            const auto found = nameToIndex.find(name);
            if (found != nameToIndex.end())
            {
                return found->second;
            }
            const std::int32_t index = static_cast<std::int32_t>(program.names.size());
            program.names.push_back(name);
            nameToIndex.emplace(name, index);
            return index;
        };

        auto getRegister = [&](const cora::ir::Value *value, bool emitGlobalLoad) -> std::int32_t
        {
            if (value == nullptr)
            {
                return -1;
            }
            const auto found = valueToRegister.find(value);
            if (found != valueToRegister.end())
            {
                return found->second;
            }

            if (const auto *arg = dynamic_cast<const cora::ir::Argument *>(value))
            {
                const std::int32_t reg = static_cast<std::int32_t>(arg->argNo);
                valueToRegister.emplace(value, reg);
                if (nextRegister <= reg)
                {
                    nextRegister = reg + 1;
                }
                return reg;
            }

            if (emitGlobalLoad && (value->kind == cora::ir::Value::Kind::Constant || dynamic_cast<const cora::ir::ConstantValue *>(value)))
            {
                const std::string name = ValueName(value);
                if (!name.empty() && name[0] != '%' && !dynamic_cast<const cora::ir::FunctionValue *>(value))
                {
                    const std::int32_t reg = allocateRegister();
                    const std::int32_t nameIndex = getNameIndex(name);
                    program.code.push_back({OpCode::LoadGlobal, reg, nameIndex, 0});
                    valueToRegister.emplace(value, reg);
                    return reg;
                }
            }

            if (const auto *constant = dynamic_cast<const cora::ir::ConstantValue *>(value))
            {
                const std::int32_t reg = allocateRegister();
                const std::int32_t index = static_cast<std::int32_t>(program.constants.size());
                program.constants.push_back(constant->value);
                program.code.push_back({OpCode::LoadConst, reg, index, 0});
                valueToRegister.emplace(value, reg);
                return reg;
            }

            if (const auto *function = dynamic_cast<const cora::ir::FunctionValue *>(value))
            {
                const std::int32_t reg = allocateRegister();
                const std::int32_t index = static_cast<std::int32_t>(program.constants.size());
                program.constants.emplace_back(0);
                program.code.push_back({OpCode::LoadConst, reg, index, 0});
                functionConstants.emplace(function, index);
                valueToRegister.emplace(value, reg);
                return reg;
            }

            const std::int32_t reg = allocateRegister();
            valueToRegister.emplace(value, reg);
            return reg;
        };

        for (const cora::ir::BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }

            blockToIp[block] = program.code.size();

            for (const cora::ir::Instruction *inst : block->insts)
            {
                if (inst == nullptr)
                {
                    continue;
                }

                switch (inst->opcode)
                {
                case cora::ir::Instruction::Opcode::Alloca:
                    getRegister(inst, false);
                    break;
                case cora::ir::Instruction::Opcode::Load:
                {
                    const std::int32_t dest = getRegister(inst, false);
                    const cora::ir::Value *ptr = inst->getOperand(0);
                    
                    if (dynamic_cast<const cora::ir::AllocaInstruction*>(ptr)) {
                        // Local variable
                        const std::int32_t src = getRegister(ptr, false);
                        if (dest != src) {
                            program.code.push_back({OpCode::Move, dest, src, 0});
                        }
                    } else {
                        // Global variable
                        const std::string name = ValueName(ptr);
                        if (!name.empty()) {
                            const std::int32_t nameIndex = getNameIndex(name);
                            program.code.push_back({OpCode::LoadGlobal, dest, nameIndex, 0});
                        }
                    }
                    break;
                }
                case cora::ir::Instruction::Opcode::Store:
                {
                    const cora::ir::Value *val = inst->getOperand(0);
                    const cora::ir::Value *ptr = inst->getOperand(1);
                    const std::int32_t source = getRegister(val, true);

                    if (dynamic_cast<const cora::ir::AllocaInstruction*>(ptr)) {
                        // Local variable
                        const std::int32_t dest = getRegister(ptr, false);
                        if (dest != source) {
                            program.code.push_back({OpCode::Move, dest, source, 0});
                        }
                    } else {
                        // Global variable
                        const std::string name = ValueName(ptr);
                        if (!name.empty()) {
                            const std::int32_t nameIndex = getNameIndex(name);
                            program.code.push_back({OpCode::StoreGlobal, source, nameIndex, 0});
                        }
                    }
                    break;
                }
                case cora::ir::Instruction::Opcode::Add:
                case cora::ir::Instruction::Opcode::Sub:
                case cora::ir::Instruction::Opcode::Mul:
                case cora::ir::Instruction::Opcode::Div:
                case cora::ir::Instruction::Opcode::Eq:
                case cora::ir::Instruction::Opcode::Ne:
                case cora::ir::Instruction::Opcode::Lt:
                case cora::ir::Instruction::Opcode::Le:
                case cora::ir::Instruction::Opcode::Gt:
                case cora::ir::Instruction::Opcode::Ge:
                {
                    const std::int32_t dest = getRegister(inst, false);
                    const std::int32_t lhs = getRegister(inst->getOperand(0), true);
                    const std::int32_t rhs = getRegister(inst->getOperand(1), true);
                    OpCode op = OpCode::Add;
                    switch (inst->opcode)
                    {
                    case cora::ir::Instruction::Opcode::Add:
                        op = OpCode::Add;
                        break;
                    case cora::ir::Instruction::Opcode::Sub:
                        op = OpCode::Sub;
                        break;
                    case cora::ir::Instruction::Opcode::Mul:
                        op = OpCode::Mul;
                        break;
                    case cora::ir::Instruction::Opcode::Div:
                        op = OpCode::Div;
                        break;
                    case cora::ir::Instruction::Opcode::Eq:
                        op = OpCode::Eq;
                        break;
                    case cora::ir::Instruction::Opcode::Ne:
                        op = OpCode::Ne;
                        break;
                    case cora::ir::Instruction::Opcode::Lt:
                        op = OpCode::Lt;
                        break;
                    case cora::ir::Instruction::Opcode::Le:
                        op = OpCode::Le;
                        break;
                    case cora::ir::Instruction::Opcode::Gt:
                        op = OpCode::Gt;
                        break;
                    case cora::ir::Instruction::Opcode::Ge:
                        op = OpCode::Ge;
                        break;
                    default:
                        break;
                    }
                    program.code.push_back({op, dest, lhs, rhs});
                    break;
                }
                case cora::ir::Instruction::Opcode::Call:
                {
                    const std::int32_t dest = getRegister(inst, false);
                    const std::int32_t calleeReg = getRegister(inst->getOperand(0), true);
                    const std::size_t argc = inst->operands.size() > 0 ? inst->operands.size() - 1 : 0;
                    const std::int32_t baseReg = allocateRegister();
                    if (baseReg != calleeReg)
                    {
                        program.code.push_back({OpCode::Move, baseReg, calleeReg, 0});
                    }
                    for (std::size_t i = 0; i < argc; ++i)
                    {
                        const std::int32_t argReg = getRegister(inst->getOperand(static_cast<int>(i + 1)), true);
                        const std::int32_t targetReg = allocateRegister();
                        if (targetReg != argReg)
                        {
                            program.code.push_back({OpCode::Move, targetReg, argReg, 0});
                        }
                    }
                    program.code.push_back({OpCode::Call, dest, baseReg, static_cast<std::int32_t>(argc)});
                    break;
                }
                case cora::ir::Instruction::Opcode::Ret:
                {
                    std::int32_t reg = -1;
                    if (!inst->operands.empty())
                    {
                        reg = getRegister(inst->getOperand(0), true);
                    }
                    program.code.push_back({OpCode::Return, reg, 0, 0});
                    break;
                }
                case cora::ir::Instruction::Opcode::Br:
                {
                    const auto *branch = static_cast<const cora::ir::BranchInstruction *>(inst);
                    if (branch->is_conditional)
                    {
                        const std::int32_t condReg = getRegister(inst->getOperand(0), true);
                        const auto *trueBlock = static_cast<const cora::ir::BasicBlock *>(inst->getOperand(1));
                        const auto *falseBlock = static_cast<const cora::ir::BasicBlock *>(inst->getOperand(2));
                        const std::size_t jumpIfIndex = program.code.size();
                        program.code.push_back({OpCode::JumpIfTrue, 0, condReg, 0});
                        fixups.push_back({jumpIfIndex, trueBlock});

                        const std::size_t jumpIndex = program.code.size();
                        program.code.push_back({OpCode::Jump, 0, 0, 0});
                        fixups.push_back({jumpIndex, falseBlock});
                    }
                    else
                    {
                        const auto *dest = static_cast<const cora::ir::BasicBlock *>(inst->getOperand(0));
                        const std::size_t jumpIndex = program.code.size();
                        program.code.push_back({OpCode::Jump, 0, 0, 0});
                        fixups.push_back({jumpIndex, dest});
                    }
                    break;
                }
                case cora::ir::Instruction::Opcode::Jump:
                {
                    const auto *dest = static_cast<const cora::ir::BasicBlock *>(inst->getOperand(0));
                    const std::size_t jumpIndex = program.code.size();
                    program.code.push_back({OpCode::Jump, 0, 0, 0});
                    fixups.push_back({jumpIndex, dest});
                    break;
                }
                case cora::ir::Instruction::Opcode::Phi:
                    getRegister(inst, false);
                    break;
                }
            }
        }

        for (const auto &fixup : fixups)
        {
            auto found = blockToIp.find(fixup.target);
            if (found == blockToIp.end())
            {
                std::string targetName = fixup.target ? fixup.target->name : "nullptr";
                throw std::runtime_error("BytecodeEmitter: missing block target: " + targetName);
            }
            program.code[fixup.at].a = static_cast<std::int32_t>(found->second);
        }

        for (const auto &entry : functionConstants)
        {
            const auto found = blockToIp.find(entry.first->entry);
            if (found == blockToIp.end())
            {
                throw std::runtime_error("BytecodeEmitter: missing function entry block");
            }
            program.constants[static_cast<std::size_t>(entry.second)] = cora::compiler::runtime::value(static_cast<std::int64_t>(found->second));
        }

        // To support functions properly, we should ideally emit them separately.
        // But with the current flat structure, we just need to ensure targets are resolved.
        // Actually, the issue might be that functions start with their own register 0.
        // If we want to support that, we need to reset valueToRegister and nextRegister
        // when we start emitting a block that is a function entry.
        // For now, let's keep it simple and just ensure all blocks are present.

        program.code.push_back({OpCode::Halt, 0, 0, 0});
        return program;
    }

} // namespace cora::vmachine
