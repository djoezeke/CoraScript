#include "VMachine.hpp"

#include "../Runtime/Variable.hpp"
#include "BytecodeReader.hpp"

#include <iostream>
#include <stdexcept>

namespace cora::vmachine
{
    namespace
    {
        using cora::compiler::runtime::Value;

        bool isValidOpcode(OpCode op)
        {
            switch (op)
            {
            case OpCode::PushConst:
            case OpCode::LoadLocal:
            case OpCode::StoreLocal:
            case OpCode::Add:
            case OpCode::Mul:
            case OpCode::Sub:
            case OpCode::Div:
            case OpCode::Print:
            case OpCode::Jump:
            case OpCode::JumpIfFalse:
            case OpCode::Return:
            case OpCode::Halt:
                return true;
            }
            return false;
        }
    } // namespace

    VMachine::VMachine()
        : VMachine(&std::cout)
    {
    }

    VMachine::VMachine(std::ostream *out)
        : m_scope(nullptr, cora::compiler::runtime::ScopeKind::Module),
          m_output(out != nullptr ? out : &std::cout)
    {
        cora::compiler::runtime::GetGarbageCollector().RegisterRoot(&m_scope);
    }

    int VMachine::Run(const BytecodeProgram &program)
    {
        m_stack.clear();
        m_lastError.clear();
        m_returnValue = Value();

        std::size_t ip = 0;

        while (ip < program.code.size())
        {
            const Instruction &instruction = program.code[ip];
            if (!isValidOpcode(instruction.op))
            {
                SetRuntimeError("VMachine: invalid opcode at instruction " + std::to_string(ip));
                return 2;
            }

            switch (instruction.op)
            {
            case OpCode::PushConst:
            {
                Value value(static_cast<double>(instruction.a));
                value.SetValueKind(cora::compiler::runtime::ValueKind::Integer);
                m_stack.push_back(value);
                ++ip;
                break;
            }
            case OpCode::LoadLocal:
                if (!PushLocal(instruction.a))
                {
                    return 2;
                }
                ++ip;
                break;
            case OpCode::StoreLocal:
            {
                if (m_stack.empty())
                {
                    SetRuntimeError("VMachine: stack underflow on store_local");
                    return 2;
                }

                const Value value = m_stack.back();
                m_stack.pop_back();
                if (!StoreLocal(instruction.a, value))
                {
                    return 2;
                }
                ++ip;
                break;
            }
            case OpCode::Add:
            case OpCode::Mul:
            case OpCode::Sub:
            case OpCode::Div:
                if (!BinaryNumeric(instruction.op))
                {
                    return 2;
                }
                ++ip;
                break;
            case OpCode::Print:
            {
                if (m_stack.empty())
                {
                    SetRuntimeError("VMachine: stack underflow on print");
                    return 2;
                }

                const Value value = m_stack.back();
                m_stack.pop_back();
                if (m_output != nullptr)
                {
                    *m_output << value.AsString() << '\n';
                }
                ++ip;
                break;
            }
            case OpCode::Jump:
            {
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) >= program.code.size())
                {
                    SetRuntimeError("VMachine: jump target out of range");
                    return 2;
                }

                ip = static_cast<std::size_t>(instruction.a);
                break;
            }
            case OpCode::JumpIfFalse:
            {
                if (m_stack.empty())
                {
                    SetRuntimeError("VMachine: stack underflow on jump_if_false");
                    return 2;
                }

                const Value condition = m_stack.back();
                m_stack.pop_back();
                if (!condition.AsBool())
                {
                    if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) >= program.code.size())
                    {
                        SetRuntimeError("VMachine: jump_if_false target out of range");
                        return 2;
                    }
                    ip = static_cast<std::size_t>(instruction.a);
                }
                else
                {
                    ++ip;
                }
                break;
            }
            case OpCode::Return:
            {
                if (m_stack.empty())
                {
                    m_returnValue = Value(nullptr);
                }
                else
                {
                    m_returnValue = m_stack.back();
                    m_stack.pop_back();
                }

                cora::compiler::runtime::GetGarbageCollector().Collect();
                return 0;
            }
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
            BytecodeReader reader(bytecodeFile);
            reader.Read();
            return Run(reader.Instructions());
        }
        catch (const std::exception &error)
        {
            SetRuntimeError(error.what());
            return 2;
        }
    }

    const cora::compiler::runtime::Value &VMachine::GetReturnValue() const
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

    std::string VMachine::LocalName(std::int32_t slot)
    {
        return "$" + std::to_string(slot);
    }

    bool VMachine::PushLocal(std::int32_t slot)
    {
        cora::compiler::runtime::Variable *variable = m_scope.GetVariableValue(LocalName(slot));
        if (variable == nullptr || variable->GetValue() == nullptr)
        {
            SetRuntimeError("VMachine: undefined local slot " + std::to_string(slot));
            return false;
        }

        m_stack.push_back(*variable->GetValue());
        return true;
    }

    bool VMachine::StoreLocal(std::int32_t slot, const cora::compiler::runtime::Value &value)
    {
        try
        {
            m_scope.SetVariableValue(LocalName(slot), new cora::compiler::runtime::Value(value));
            return true;
        }
        catch (const std::exception &error)
        {
            SetRuntimeError(error.what());
            return false;
        }
    }

    bool VMachine::BinaryNumeric(OpCode op)
    {
        if (m_stack.size() < 2)
        {
            SetRuntimeError("VMachine: stack underflow on arithmetic op");
            return false;
        }

        const cora::compiler::runtime::Value rhs = m_stack.back();
        m_stack.pop_back();
        const cora::compiler::runtime::Value lhs = m_stack.back();
        m_stack.pop_back();

        double left = 0.0;
        double right = 0.0;
        try
        {
            left = lhs.AsNumber();
            right = rhs.AsNumber();
        }
        catch (const std::exception &error)
        {
            SetRuntimeError(error.what());
            return false;
        }

        if (op == OpCode::Div && right == 0.0)
        {
            SetRuntimeError("VMachine: division by zero");
            return false;
        }

        double result = 0.0;
        switch (op)
        {
        case OpCode::Add:
            result = left + right;
            break;
        case OpCode::Mul:
            result = left * right;
            break;
        case OpCode::Sub:
            result = left - right;
            break;
        case OpCode::Div:
            result = left / right;
            break;
        default:
            SetRuntimeError("VMachine: unsupported arithmetic opcode");
            return false;
        }

        cora::compiler::runtime::Value value(result);
        if (op != OpCode::Div)
        {
            value.SetValueKind(cora::compiler::runtime::ValueKind::Integer);
        }
        m_stack.push_back(value);
        return true;
    }

    void VMachine::SetRuntimeError(const std::string &message)
    {
        m_lastError = message;
    }

    VMachine::~VMachine()
    {
        cora::compiler::runtime::GetGarbageCollector().UnregisterRoot(&m_scope);
    }

} // namespace cora::vmachine
