#ifndef CORA_COMPILER_RUNTIME_INTERPRETER_H
#define CORA_COMPILER_RUNTIME_INTERPRETER_H

#include "Bytecode.hpp"
#include "VMachine.hpp"

#include <iosfwd>
#include <string>

namespace cora::vmachine
{
    class Interpreter
    {
    public:
        Interpreter();
        explicit Interpreter(std::ostream *out);

        int Run(const BytecodeProgram &program);
        int RunBytecodeFile(const std::string &filePath);

        std::string LastError() const;

        ~Interpreter();

    private:
        VMachine m_vm;
    };

}

#endif // CORA_COMPILER_RUNTIME_INTERPRETER_H
