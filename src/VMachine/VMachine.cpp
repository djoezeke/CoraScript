#include "VMachine.hpp"

#include "../Builtin/Builtin.hpp"
#include "../Runtime/Variable.hpp"
#include "BytecodeReader.hpp"

#include <iostream>
#include <stdexcept>

namespace cora::vmachine
{
    namespace
    {
        using cora::compiler::runtime::value;
    } // namespace

    VMachine::VMachine()
        : VMachine(&std::cout)
    {
    }

    VMachine::VMachine(std::ostream *out)
        : m_registers(kRegisterCount),
          m_globals(nullptr, cora::compiler::runtime::ScopeKind::Module),
          m_output(out != nullptr ? out : &std::cout)
    {
        cora::compiler::builtin::Builtins(m_globals);
        cora::compiler::runtime::GetGarbageCollector().RegisterRoot(&m_globals);
    }

    int VMachine::Run(const BytecodeProgram &program)
    {
        m_registers.assign(kRegisterCount, value(nullptr));
        m_callStack.clear();
        m_lastError.clear();
        m_returnValue = value(nullptr);

        std::size_t ip = 0;
        while (ip < program.code.size())
        {
            const Instruction &instruction = program.code[ip];
            switch (instruction.op)
            {
            case OpCode::Nop:
                ++ip;
                break;
            case OpCode::LoadConst:
                if (!IsRegisterValid(instruction.a) || instruction.b < 0 ||
                    static_cast<std::size_t>(instruction.b) >= program.constants.size())
                {
                    SetRuntimeError("VMachine: invalid load_const operand");
                    return 2;
                }
                m_registers[static_cast<std::size_t>(instruction.a)] = program.constants[static_cast<std::size_t>(instruction.b)];
                ++ip;
                break;
            case OpCode::Move:
                if (!IsRegisterValid(instruction.a) || !IsRegisterValid(instruction.b))
                {
                    SetRuntimeError("VMachine: invalid move operand");
                    return 2;
                }
                m_registers[static_cast<std::size_t>(instruction.a)] = m_registers[static_cast<std::size_t>(instruction.b)];
                ++ip;
                break;
            case OpCode::Add:
            case OpCode::Sub:
            case OpCode::Mul:
            case OpCode::Div:
            case OpCode::Mod:
                if (!BinaryOp(instruction.op, instruction.a, instruction.b, instruction.c))
                {
                    return 2;
                }
                ++ip;
                break;
            case OpCode::Neg:
            case OpCode::Not:
                if (!UnaryOp(instruction.op, instruction.a, instruction.b))
                {
                    return 2;
                }
                ++ip;
                break;
            case OpCode::Eq:
            case OpCode::Ne:
            case OpCode::Lt:
            case OpCode::Le:
            case OpCode::Gt:
            case OpCode::Ge:
                if (!CompareOp(instruction.op, instruction.a, instruction.b, instruction.c))
                {
                    return 2;
                }
                ++ip;
                break;
            case OpCode::Jump:
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) >= program.code.size())
                {
                    SetRuntimeError("VMachine: jump target out of range");
                    return 2;
                }
                ip = static_cast<std::size_t>(instruction.a);
                break;
            case OpCode::JumpIfTrue:
            case OpCode::JumpIfFalse:
                if (!IsRegisterValid(instruction.b))
                {
                    SetRuntimeError("VMachine: invalid jump condition register");
                    return 2;
                }
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) >= program.code.size())
                {
                    SetRuntimeError("VMachine: jump target out of range");
                    return 2;
                }
                if ((instruction.op == OpCode::JumpIfTrue && m_registers[static_cast<std::size_t>(instruction.b)].AsBool()) ||
                    (instruction.op == OpCode::JumpIfFalse && !m_registers[static_cast<std::size_t>(instruction.b)].AsBool()))
                {
                    ip = static_cast<std::size_t>(instruction.a);
                }
                else
                {
                    ++ip;
                }
                break;
            case OpCode::LoadGlobal:
                if (!LoadGlobal(program, instruction.a, instruction.b))
                {
                    return 2;
                }
                ++ip;
                break;
            case OpCode::StoreGlobal:
                if (!StoreGlobal(program, instruction.a, instruction.b))
                {
                    return 2;
                }
                ++ip;
                break;
            case OpCode::Call:
            {
                if (!IsRegisterValid(instruction.a) || !IsRegisterValid(instruction.b) || instruction.c < 0)
                {
                    SetRuntimeError("VMachine: invalid call operands");
                    return 2;
                }
                const auto &calleeValue = m_registers[static_cast<std::size_t>(instruction.b)];
                // std::cerr << "Calling reg " << instruction.b << ", value type: " << calleeValue.AsString() << "\n";
                if (calleeValue.IsCallable())
                {
                    std::vector<value> args;
                    args.reserve(static_cast<std::size_t>(instruction.c));
                    for (std::int32_t i = 0; i < instruction.c; ++i)
                    {
                        const std::int32_t argReg = instruction.b + 1 + i;
                        if (!IsRegisterValid(argReg))
                        {
                            SetRuntimeError("VMachine: invalid call argument register");
                            return 2;
                        }
                        args.push_back(m_registers[static_cast<std::size_t>(argReg)]);
                    }

                    try
                    {
                        auto callable = calleeValue.AsCallable();
                        m_registers[static_cast<std::size_t>(instruction.a)] = callable->Call(args);
                    }
                    catch (const std::exception &error)
                    {
                        SetRuntimeError(error.what());
                        return 2;
                    }
                    ++ip;
                    break;
                }

                if (!calleeValue.IsInteger() && !calleeValue.IsFloat())
                {
                    SetRuntimeError("VMachine: call target is not callable or function address");
                    return 2;
                }

                const std::size_t entryIp = static_cast<std::size_t>(calleeValue.AsNumber());
                if (entryIp >= program.code.size())
                {
                    SetRuntimeError("VMachine: call target out of range");
                    return 2;
                }

                CallFrame frame;
                frame.returnIp = ip + 1;
                frame.destRegister = instruction.a;
                frame.registers = std::move(m_registers);

                m_registers.assign(kRegisterCount, value(nullptr));
                for (std::int32_t i = 0; i < instruction.c; ++i)
                {
                    const std::int32_t argReg = instruction.b + 1 + i;
                    if (!IsRegisterValid(argReg))
                    {
                        SetRuntimeError("VMachine: invalid call argument register");
                        return 2;
                    }
                    m_registers[static_cast<std::size_t>(i)] = frame.registers[static_cast<std::size_t>(argReg)];
                }

                m_callStack.push_back(std::move(frame));
                ip = entryIp;
                break;
            }
            case OpCode::Return:
                if (instruction.a >= 0 && IsRegisterValid(instruction.a))
                {
                    m_returnValue = m_registers[static_cast<std::size_t>(instruction.a)];
                }
                else
                {
                    m_returnValue = value(nullptr);
                }
                if (!m_callStack.empty())
                {
                    CallFrame frame = std::move(m_callStack.back());
                    m_callStack.pop_back();
                    m_registers = std::move(frame.registers);
                    if (IsRegisterValid(frame.destRegister))
                    {
                        m_registers[static_cast<std::size_t>(frame.destRegister)] = m_returnValue;
                    }
                    ip = frame.returnIp;
                    break;
                }
                cora::compiler::runtime::GetGarbageCollector().Collect();
                return 0;
            case OpCode::Print:
                if (!IsRegisterValid(instruction.a))
                {
                    SetRuntimeError("VMachine: invalid print register");
                    return 2;
                }
                if (m_output != nullptr)
                {
                    *m_output << m_registers[static_cast<std::size_t>(instruction.a)].AsString() << '\n';
                }
                ++ip;
                break;
            case OpCode::Halt:
                cora::compiler::runtime::GetGarbageCollector().Collect();
                return 0;
            }
        }

        cora::compiler::runtime::GetGarbageCollector().Collect();
        return 0;
    }

    int VMachine::RunFile(const std::string &bytecodeFile)
    {
        try
        {
            BytecodeReader reader;
            reader.Read(bytecodeFile);
            return Run(reader.GetProgram());
        }
        catch (const std::exception &error)
        {
            SetRuntimeError(error.what());
            return 2;
        }
    }

    const cora::compiler::runtime::value &VMachine::GetReturnValue() const
    {
        return m_returnValue;
    }

    std::string VMachine::LastError() const
    {
        return m_lastError;
    }

    void VMachine::SetOutput(std::ostream *out)
    {
        m_output = out != nullptr ? out : &std::cout;
    }

    bool VMachine::BinaryOp(OpCode op, std::int32_t dest, std::int32_t left, std::int32_t right)
    {
        if (!IsRegisterValid(dest) || !IsRegisterValid(left) || !IsRegisterValid(right))
        {
            SetRuntimeError("VMachine: invalid arithmetic register");
            return false;
        }

        const value &lhs = m_registers[static_cast<std::size_t>(left)];
        const value &rhs = m_registers[static_cast<std::size_t>(right)];

        if (op == OpCode::Add && (lhs.IsString() || rhs.IsString()))
        {
            m_registers[static_cast<std::size_t>(dest)] = value(lhs.AsString() + rhs.AsString());
            return true;
        }

        const double leftValue = lhs.AsNumber();
        const double rightValue = rhs.AsNumber();
        double result = 0.0;

        switch (op)
        {
        case OpCode::Add:
            result = leftValue + rightValue;
            break;
        case OpCode::Sub:
            result = leftValue - rightValue;
            break;
        case OpCode::Mul:
            result = leftValue * rightValue;
            break;
        case OpCode::Div:
            if (rightValue == 0.0)
            {
                SetRuntimeError("VMachine: division by zero");
                return false;
            }
            result = leftValue / rightValue;
            break;
        case OpCode::Mod:
            result = static_cast<double>(static_cast<long long>(leftValue) % static_cast<long long>(rightValue));
            break;
        default:
            SetRuntimeError("VMachine: unsupported arithmetic opcode");
            return false;
        }

        m_registers[static_cast<std::size_t>(dest)] = value(result);
        return true;
    }

    bool VMachine::CompareOp(OpCode op, std::int32_t dest, std::int32_t left, std::int32_t right)
    {
        if (!IsRegisterValid(dest) || !IsRegisterValid(left) || !IsRegisterValid(right))
        {
            SetRuntimeError("VMachine: invalid comparison register");
            return false;
        }

        const value &lhs = m_registers[static_cast<std::size_t>(left)];
        const value &rhs = m_registers[static_cast<std::size_t>(right)];

        bool result = false;
        if (lhs.IsString() || rhs.IsString())
        {
            const std::string leftValue = lhs.AsString();
            const std::string rightValue = rhs.AsString();
            switch (op)
            {
            case OpCode::Eq:
                result = leftValue == rightValue;
                break;
            case OpCode::Ne:
                result = leftValue != rightValue;
                break;
            default:
                SetRuntimeError("VMachine: unsupported string comparison opcode");
                return false;
            }
        }
        else
        {
            const double leftValue = lhs.AsNumber();
            const double rightValue = rhs.AsNumber();
            switch (op)
            {
            case OpCode::Eq:
                result = leftValue == rightValue;
                break;
            case OpCode::Ne:
                result = leftValue != rightValue;
                break;
            case OpCode::Lt:
                result = leftValue < rightValue;
                break;
            case OpCode::Le:
                result = leftValue <= rightValue;
                break;
            case OpCode::Gt:
                result = leftValue > rightValue;
                break;
            case OpCode::Ge:
                result = leftValue >= rightValue;
                break;
            default:
                SetRuntimeError("VMachine: unsupported comparison opcode");
                return false;
            }
        }

        m_registers[static_cast<std::size_t>(dest)] = value(result);
        return true;
    }

    bool VMachine::UnaryOp(OpCode op, std::int32_t dest, std::int32_t source)
    {
        if (!IsRegisterValid(dest) || !IsRegisterValid(source))
        {
            SetRuntimeError("VMachine: invalid unary register");
            return false;
        }

        const value &val = m_registers[static_cast<std::size_t>(source)];
        switch (op)
        {
        case OpCode::Neg:
            m_registers[static_cast<std::size_t>(dest)] = value(-val.AsNumber());
            return true;
        case OpCode::Not:
            m_registers[static_cast<std::size_t>(dest)] = value(!val.AsBool());
            return true;
        default:
            SetRuntimeError("VMachine: unsupported unary opcode");
            return false;
        }
    }

    bool VMachine::LoadGlobal(const BytecodeProgram &program, std::int32_t dest, std::int32_t nameIndex)
    {
        if (!IsRegisterValid(dest) || nameIndex < 0 || static_cast<std::size_t>(nameIndex) >= program.names.size())
        {
            SetRuntimeError("VMachine: invalid global load");
            return false;
        }

        const std::string &name = program.names[static_cast<std::size_t>(nameIndex)];
        
        // Handle qualified names like io.print
        std::vector<std::string> parts;
        std::string part;
        for (char c : name)
        {
            if (c == '.')
            {
                if (!part.empty()) parts.push_back(part);
                part.clear();
            }
            else
            {
                part += c;
            }
        }
        if (!part.empty()) parts.push_back(part);

        if (parts.empty()) return false;

        cora::compiler::runtime::Variable *variable = m_globals.GetVariableValue(parts[0]);
        if (variable == nullptr || variable->GetValue() == nullptr)
        {
            SetRuntimeError("VMachine: undefined global variable " + parts[0]);
            return false;
        }

        value current = *variable->GetValue();
        for (std::size_t i = 1; i < parts.size(); ++i)
        {
            if (!current.IsObject())
            {
                SetRuntimeError("VMachine: " + parts[i-1] + " is not an object");
                return false;
            }
            auto obj = current.AsObject();
            auto it = obj->fields.find(parts[i]);
            if (it == obj->fields.end())
            {
                SetRuntimeError("VMachine: undefined member " + parts[i] + " in " + parts[i-1]);
                return false;
            }
            current = it->second;
        }

        m_registers[static_cast<std::size_t>(dest)] = current;
        return true;
    }

    bool VMachine::StoreGlobal(const BytecodeProgram &program, std::int32_t source, std::int32_t nameIndex)
    {
        if (!IsRegisterValid(source) || nameIndex < 0 || static_cast<std::size_t>(nameIndex) >= program.names.size())
        {
            SetRuntimeError("VMachine: invalid global store");
            return false;
        }

        const std::string &name = program.names[static_cast<std::size_t>(nameIndex)];
        try
        {
            m_globals.SetVariableValue(name, new value(m_registers[static_cast<std::size_t>(source)]));
        }
        catch (const std::exception &error)
        {
            SetRuntimeError(error.what());
            return false;
        }
        return true;
    }

    void VMachine::SetRuntimeError(const std::string &message)
    {
        m_lastError = message;
    }

    bool VMachine::IsRegisterValid(std::int32_t index) const
    {
        return index >= 0 && static_cast<std::size_t>(index) < kRegisterCount;
    }

    VMachine::~VMachine()
    {
        cora::compiler::runtime::GetGarbageCollector().UnregisterRoot(&m_globals);
    }

} // namespace cora::vmachine
