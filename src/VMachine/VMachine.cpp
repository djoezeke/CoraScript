#include "VMachine.hpp"


namespace cora::vmachine
{

    VMachine::VMachine()
    {
    }

    VMachine::~VMachine()
    {
    }

}

#include "../JITCom/JitPipeline.hpp"

#include "../Builtin/Builtin.hpp"
#include "../Runtime/Scope.hpp"
#include "../Runtime/Variable.hpp"

#include <cmath>
#include <stdexcept>

namespace cora::embed::internal
{
    using namespace cora::compiler;

    BytecodeVm::BytecodeVm(JitPipeline *jit)
        : m_Jit(jit)
    {
        runtime::Scope scope;
        builtin::Builtins(scope);

        for (const auto &entry : scope.GetVariables())
        {
            if (!entry.second || !entry.second->GetValue())
            {
                continue;
            }

            m_Globals[entry.first] = *(entry.second->GetValue());
        }
    }

    void BytecodeVm::SetGlobal(const std::string &name, runtime::Value value)
    {
        m_Globals[name] = std::move(value);
    }

    bool BytecodeVm::HasGlobal(const std::string &name) const
    {
        return m_Globals.find(name) != m_Globals.end();
    }

    runtime::Value BytecodeVm::Execute(const BytecodeProgram &program)
    {
        if (m_Jit)
        {
            m_Jit->ResetExecutionState();
        }

        m_Stack.clear();
        std::size_t ip = 0;

        auto popValue = [this]() -> runtime::Value
        {
            if (m_Stack.empty())
            {
                throw std::runtime_error("VM stack underflow");
            }

            runtime::Value value = m_Stack.back();
            m_Stack.pop_back();
            return value;
        };

        while (ip < program.code.size())
        {
            const Instruction &instruction = program.code[ip++];
            switch (instruction.op)
            {
            case OpCode::PushConst:
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) >= program.constants.size())
                {
                    throw std::runtime_error("VM constant index out of range");
                }
                m_Stack.push_back(program.constants[static_cast<std::size_t>(instruction.a)]);
                break;

            case OpCode::PushNull:
                m_Stack.emplace_back(nullptr);
                break;

            case OpCode::LoadGlobal:
            {
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) >= program.names.size())
                {
                    throw std::runtime_error("VM name index out of range");
                }

                const std::string &name = program.names[static_cast<std::size_t>(instruction.a)];
                const auto found = m_Globals.find(name);
                if (found == m_Globals.end())
                {
                    throw std::runtime_error("Unknown global: " + name);
                }

                m_Stack.push_back(found->second);
                break;
            }

            case OpCode::StoreGlobal:
            {
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) >= program.names.size())
                {
                    throw std::runtime_error("VM name index out of range");
                }

                if (m_Stack.empty())
                {
                    throw std::runtime_error("VM stack underflow on store");
                }

                const std::string &name = program.names[static_cast<std::size_t>(instruction.a)];
                m_Globals[name] = m_Stack.back();
                break;
            }

            case OpCode::Pop:
                (void)popValue();
                break;

            case OpCode::Add:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.push_back(Add(lhs, rhs));
                break;
            }

            case OpCode::Sub:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.push_back(Sub(lhs, rhs));
                break;
            }

            case OpCode::Mul:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.push_back(Mul(lhs, rhs));
                break;
            }

            case OpCode::Div:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.push_back(Div(lhs, rhs));
                break;
            }

            case OpCode::Mod:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.push_back(Mod(lhs, rhs));
                break;
            }

            case OpCode::Negate:
            {
                runtime::Value rhs = popValue();
                m_Stack.emplace_back(-rhs.AsNumber());
                break;
            }

            case OpCode::LogicalNot:
            {
                runtime::Value rhs = popValue();
                m_Stack.emplace_back(!IsTruthy(rhs));
                break;
            }

            case OpCode::Equal:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.emplace_back(ValuesEqual(lhs, rhs));
                break;
            }

            case OpCode::NotEqual:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.emplace_back(!ValuesEqual(lhs, rhs));
                break;
            }

            case OpCode::Less:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.emplace_back(lhs.AsNumber() < rhs.AsNumber());
                break;
            }

            case OpCode::LessEqual:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.emplace_back(lhs.AsNumber() <= rhs.AsNumber());
                break;
            }

            case OpCode::Greater:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.emplace_back(lhs.AsNumber() > rhs.AsNumber());
                break;
            }

            case OpCode::GreaterEqual:
            {
                runtime::Value rhs = popValue();
                runtime::Value lhs = popValue();
                m_Stack.emplace_back(lhs.AsNumber() >= rhs.AsNumber());
                break;
            }

            case OpCode::Jump:
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) > program.code.size())
                {
                    throw std::runtime_error("VM jump target out of range");
                }

                if (m_Jit)
                {
                    const std::size_t currentIp = ip == 0 ? 0 : ip - 1;
                    const std::size_t targetIp = static_cast<std::size_t>(instruction.a);
                    if (targetIp < currentIp)
                    {
                        const bool compiled = m_Jit->OnLoopBackEdge(
                            program,
                            static_cast<std::int32_t>(targetIp),
                            static_cast<std::int32_t>(currentIp));
                        if (compiled)
                        {
                            m_Jit->RecordCompiledTraceExecution(static_cast<std::int32_t>(targetIp));
                        }
                    }
                }

                ip = static_cast<std::size_t>(instruction.a);
                break;

            case OpCode::JumpIfFalse:
            {
                if (instruction.a < 0 || static_cast<std::size_t>(instruction.a) > program.code.size())
                {
                    throw std::runtime_error("VM jump target out of range");
                }

                if (!IsTruthy(m_Stack.empty() ? runtime::Value(nullptr) : m_Stack.back()))
                {
                    ip = static_cast<std::size_t>(instruction.a);
                }
                break;
            }

            case OpCode::Call:
            {
                const std::int32_t argCount = instruction.a;
                if (argCount < 0 || m_Stack.size() < static_cast<std::size_t>(argCount + 1))
                {
                    throw std::runtime_error("Invalid VM call frame");
                }

                std::vector<runtime::Value> args;
                args.reserve(static_cast<std::size_t>(argCount));
                for (std::int32_t i = 0; i < argCount; ++i)
                {
                    args.push_back(m_Stack[m_Stack.size() - static_cast<std::size_t>(argCount) + static_cast<std::size_t>(i)]);
                }

                runtime::Value callee = m_Stack[m_Stack.size() - static_cast<std::size_t>(argCount + 1)];
                m_Stack.resize(m_Stack.size() - static_cast<std::size_t>(argCount + 1));

                if (!callee.IsCallable())
                {
                    throw std::runtime_error("Attempted to call non-callable value");
                }

                auto callable = callee.AsCallable();
                m_Stack.push_back(callable->Call(args));
                break;
            }

            case OpCode::Return:
                return m_Stack.empty() ? runtime::Value(nullptr) : popValue();

            case OpCode::Halt:
                return m_Stack.empty() ? runtime::Value(nullptr) : m_Stack.back();
            }
        }

        return runtime::Value(nullptr);
    }

    bool BytecodeVm::IsTruthy(const runtime::Value &value)
    {
        if (value.IsNull())
        {
            return false;
        }

        if (value.IsBool())
        {
            return value.AsBool();
        }

        if (value.IsNumber())
        {
            return value.AsNumber() != 0.0;
        }

        if (value.IsString())
        {
            return !value.AsString().empty();
        }

        return true;
    }

    bool BytecodeVm::ValuesEqual(const runtime::Value &lhs, const runtime::Value &rhs)
    {
        if (lhs.IsNull() && rhs.IsNull())
        {
            return true;
        }

        if (lhs.IsNull() || rhs.IsNull())
        {
            return false;
        }

        if (lhs.IsNumber() && rhs.IsNumber())
        {
            return lhs.AsNumber() == rhs.AsNumber();
        }

        if (lhs.IsBool() && rhs.IsBool())
        {
            return lhs.AsBool() == rhs.AsBool();
        }

        return lhs.AsString() == rhs.AsString();
    }

    runtime::Value BytecodeVm::Add(const runtime::Value &lhs, const runtime::Value &rhs)
    {
        if (lhs.IsString() || rhs.IsString())
        {
            return runtime::Value(lhs.AsString() + rhs.AsString());
        }
        return runtime::Value(lhs.AsNumber() + rhs.AsNumber());
    }

    runtime::Value BytecodeVm::Sub(const runtime::Value &lhs, const runtime::Value &rhs)
    {
        return runtime::Value(lhs.AsNumber() - rhs.AsNumber());
    }

    runtime::Value BytecodeVm::Mul(const runtime::Value &lhs, const runtime::Value &rhs)
    {
        return runtime::Value(lhs.AsNumber() * rhs.AsNumber());
    }

    runtime::Value BytecodeVm::Div(const runtime::Value &lhs, const runtime::Value &rhs)
    {
        return runtime::Value(lhs.AsNumber() / rhs.AsNumber());
    }

    runtime::Value BytecodeVm::Mod(const runtime::Value &lhs, const runtime::Value &rhs)
    {
        return runtime::Value(std::fmod(lhs.AsNumber(), rhs.AsNumber()));
    }
}
