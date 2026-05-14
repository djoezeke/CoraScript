#ifndef CORA_VMACHINE_BYTECODE_WRITER_H
#define CORA_VMACHINE_BYTECODE_WRITER_H

#include "Bytecode.hpp"

#include <istream>
#include <ostream>
#include <string>

namespace cora::vmachine
{
    class BytecodeWriter final
    {
    public:
        BytecodeWriter() = default;

        void Write(const std::string &filePath, const BytecodeProgram &program);
        void Write(std::ostream &out, const BytecodeProgram &program);
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODE_WRITER_H
