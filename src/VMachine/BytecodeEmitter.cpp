#include "BytecodeEmitter.hpp"

#include <stdexcept>
#include <unordered_map>

namespace cora::vmachine
{
    namespace
    {
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

    std::int32_t BytecodeEmitter::allocateRegister()
    {
        if (m_nextRegister >= kMaxRegisters)
        {
            throw std::runtime_error("BytecodeEmitter: register limit exceeded");
        }
        return m_nextRegister++;
    }

    std::int32_t BytecodeEmitter::getNameIndex(const std::string &name)
    {
        const auto found = m_nameToIndex.find(name);
        if (found != m_nameToIndex.end())
        {
            return found->second;
        }
        const std::int32_t index = static_cast<std::int32_t>(m_program.names.size());
        m_program.names.push_back(name);
        m_nameToIndex.emplace(name, index);
        return index;
    }

    std::int32_t BytecodeEmitter::getRegister(const cora::ir::Value *value, bool emitGlobalLoad)
    {
        if (value == nullptr)
        {
            return -1;
        }
        const auto found = m_valueToRegister.find(value);
        if (found != m_valueToRegister.end())
        {
            return found->second;
        }

        if (const auto *arg = dynamic_cast<const cora::ir::Argument *>(value))
        {
            const std::int32_t reg = static_cast<std::int32_t>(arg->argNo);
            m_valueToRegister.emplace(value, reg);
            if (m_nextRegister <= reg)
            {
                m_nextRegister = reg + 1;
            }
            return reg;
        }

        if (emitGlobalLoad && (value->kind == cora::ir::Value::Kind::Constant || dynamic_cast<const cora::ir::Constant *>(value)))
        {
            const std::string name = ValueName(value);
            if (!name.empty() && name[0] != '%' && !dynamic_cast<const cora::ir::Function *>(value))
            {
                const std::int32_t reg = allocateRegister();
                const std::int32_t nameIndex = getNameIndex(name);
                m_program.code.push_back({OpCode::LoadGlobal, reg, nameIndex, 0});
                m_valueToRegister.emplace(value, reg);
                return reg;
            }
        }

        if (const auto *constant = dynamic_cast<const cora::ir::Constant *>(value))
        {
            const std::int32_t reg = allocateRegister();
            const std::int32_t index = static_cast<std::int32_t>(m_program.constants.size());
            m_program.constants.push_back(constant->value);
            m_program.code.push_back({OpCode::LoadConst, reg, index, 0});
            m_valueToRegister.emplace(value, reg);
            return reg;
        }

        if (const auto *function = dynamic_cast<const cora::ir::Function *>(value))
        {
            const std::int32_t reg = allocateRegister();
            const std::int32_t index = static_cast<std::int32_t>(m_program.constants.size());
            m_program.constants.emplace_back(0);
            m_program.code.push_back({OpCode::LoadConst, reg, index, 0});
            m_functionConstants.emplace(function, index);
            m_valueToRegister.emplace(value, reg);
            return reg;
        }

        const std::int32_t reg = allocateRegister();
        m_valueToRegister.emplace(value, reg);
        return reg;
    }

    BytecodeProgram BytecodeEmitter::Emit(const std::vector<cora::ir::BasicBlock *> &blocks)
    {
        m_program = BytecodeProgram(); // Reset for each emit call
        m_valueToRegister.clear();
        m_functionConstants.clear();
        m_nameToIndex.clear();
        m_blockToIp.clear();
        m_fixups.clear();
        m_nextRegister = 0;


        for (const cora::ir::BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }

            m_blockToIp[block] = m_program.code.size();

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

                    if (dynamic_cast<const cora::ir::AllocaInstruction *>(ptr))
                    {
                        // Local variable
                        const std::int32_t src = getRegister(ptr, false);
                        if (dest != src)
                        {
                            m_program.code.push_back({OpCode::Move, dest, src, 0});
                        }
                    }
                    else
                    {
                        // Global variable
                        const std::string name = ValueName(ptr);
                        if (!name.empty())
                        {
                            const std::int32_t nameIndex = getNameIndex(name);
                            m_program.code.push_back({OpCode::LoadGlobal, dest, nameIndex, 0});
                        }
                    }
                    break;
                }
                case cora::ir::Instruction::Opcode::Store:
                {
                    const cora::ir::Value *val = inst->getOperand(0);
                    const cora::ir::Value *ptr = inst->getOperand(1);
                    const std::int32_t source = getRegister(val, true);

                    if (dynamic_cast<const cora::ir::AllocaInstruction *>(ptr))
                    {
                        // Local variable
                        const std::int32_t dest = getRegister(ptr, false);
                        if (dest != source)
                        {
                            m_program.code.push_back({OpCode::Move, dest, source, 0});
                        }
                    }
                    else
                    {
                        // Global variable
                        const std::string name = ValueName(ptr);
                        if (!name.empty())
                        {
                            const std::int32_t nameIndex = getNameIndex(name);
                            m_program.code.push_back({OpCode::StoreGlobal, source, nameIndex, 0});
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
                    m_program.code.push_back({op, dest, lhs, rhs});
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
                        m_program.code.push_back({OpCode::Move, baseReg, calleeReg, 0});
                    }
                    for (std::size_t i = 0; i < argc; ++i)
                    {
                        const std::int32_t argReg = getRegister(inst->getOperand(static_cast<int>(i + 1)), true);
                        const std::int32_t targetReg = allocateRegister();
                        if (targetReg != argReg)
                        {
                            m_program.code.push_back({OpCode::Move, targetReg, argReg, 0});
                        }
                    }
                    m_program.code.push_back({OpCode::Call, dest, baseReg, static_cast<std::int32_t>(argc)});
                    break;
                }
                case cora::ir::Instruction::Opcode::Ret:
                {
                    std::int32_t reg = -1;
                    if (!inst->operands.empty())
                    {
                        reg = getRegister(inst->getOperand(0), true);
                    }
                    m_program.code.push_back({OpCode::Return, reg, 0, 0});
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
                        const std::size_t jumpIfIndex = m_program.code.size();
                        m_program.code.push_back({OpCode::JumpIfTrue, 0, condReg, 0});
                        m_fixups.push_back(Fixup{jumpIfIndex, trueBlock});

                        const std::size_t jumpIndex = m_program.code.size();
                        m_program.code.push_back({OpCode::Jump, 0, 0, 0});
                        m_fixups.push_back(Fixup{jumpIndex, falseBlock});
                    }
                    else
                    {
                        const auto *dest = static_cast<const cora::ir::BasicBlock *>(inst->getOperand(0));
                        const std::size_t jumpIndex = m_program.code.size();
                        m_program.code.push_back({OpCode::Jump, 0, 0, 0});
                        m_fixups.push_back(Fixup{jumpIndex, dest});
                    }
                    break;
                }
                case cora::ir::Instruction::Opcode::Jump:
                {
                    const auto *dest = static_cast<const cora::ir::BasicBlock *>(inst->getOperand(0));
                    const std::size_t jumpIndex = m_program.code.size();
                    m_program.code.push_back({OpCode::Jump, 0, 0, 0});
                    m_fixups.push_back(Fixup{jumpIndex, dest});
                    break;
                }
                case cora::ir::Instruction::Opcode::Phi:
                    getRegister(inst, false);
                    break;
                }
            }
        }

        for (const auto &fixup : m_fixups)
        {
            auto found = m_blockToIp.find(fixup.target);
            if (found == m_blockToIp.end())
            {
                std::string targetName = fixup.target ? fixup.target->name : "nullptr";
                throw std::runtime_error("BytecodeEmitter: missing block target: " + targetName);
            }
            m_program.code[fixup.at].a = static_cast<std::int32_t>(found->second);
        }

        for (const auto &function : m_functionConstants)
        {
            const auto found = m_blockToIp.find(function.first->body);
            if (found == m_blockToIp.end())
            {
                throw std::runtime_error("BytecodeEmitter: missing function entry block");
            }
            m_program.constants[static_cast<std::size_t>(function.second)] = cora::compiler::runtime::value(static_cast<std::int64_t>(found->second));
        }

        m_program.code.push_back({OpCode::Halt, 0, 0, 0});
        return m_program;
    }
} // namespace cora::vmachine
