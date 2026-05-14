#include "Interpreter.hpp"

namespace cora::vmachine
{

    Interpreter::Interpreter()
        : m_vm()
    {
    }

    Interpreter::Interpreter(std::ostream *out)
        : m_vm(out)
    {
    }

    int Interpreter::Run(const BytecodeProgram &program)
    {
        return m_vm.Run(program);
    }

    int Interpreter::RunBytecodeFile(const std::string &filePath)
    {
        return m_vm.RunFile(filePath);
    }

    std::string Interpreter::LastError() const
    {
        return m_vm.LastError();
    }

    Interpreter::~Interpreter()
    {
    }

}
