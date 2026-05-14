#include "VMachine.hpp"

#include "../Builtin/Builtin.hpp"
#include "../Runtime/Variable.hpp"
#include "BytecodeReader.hpp"

#include <iostream>
#include <fstream>
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
            case OpCode::Import:
                if (!IsRegisterValid(instruction.b))
                {
                    SetRuntimeError("VMachine: invalid import register");
                    return 2;
                }
                {
                    const std::string name = m_registers[static_cast<std::size_t>(instruction.b)].AsString();
                    if (!LoadPlugin(name))
                    {
                        SetRuntimeError("VMachine: failed to load module " + name);
                        return 2;
                    }
                }
                ++ip;
                break;
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

    bool VMachine::LoadPlugin(const std::string &name)
    {
        std::string libName = name;
#if defined(CORA_OS_WINDOWS)
        libName += ".dll";
#elif defined(CORA_OS_LINUX)
        libName = "lib" + name + ".so";
#endif

        std::vector<std::string> searchPaths = {".", "./plugins", "./lib", "plugins", "lib"};
        std::string foundPath;
        for (const auto &path : searchPaths)
        {
            std::string fullPath = path + "/" + libName;
            std::ifstream f(fullPath);
            if (f.good())
            {
                foundPath = fullPath;
                break;
            }
        }

        if (foundPath.empty())
        {
            // Try without prefix/suffix just in case it's a full path
            std::ifstream f(name);
            if (f.good())
            {
                foundPath = name;
            }
            else
            {
                return false;
            }
        }

        try
        {
            SharedLibrary lib(foundPath);
            typedef void (*InitFunc)(cora::compiler::runtime::Scope &);
            std::string initName = "CoraInit_" + name;
            auto init = lib.Get<InitFunc>(initName);
            if (init)
            {
                init(m_globals);
                m_loadedPlugins.push_back(std::move(lib));
                return true;
            }
        }
        catch (...)
        {
            return false;
        }

        return false;
    }

    VMachine::~VMachine()
    {
        cora::compiler::runtime::GetGarbageCollector().UnregisterRoot(&m_globals);
    }

} // namespace cora::vmachine
