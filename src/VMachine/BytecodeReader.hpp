#ifndef CORA_VMACHINE_BYTECODE_READER_H
#define CORA_VMACHINE_BYTECODE_READER_H

#include "Bytecode.hpp"

#include <cstdint>
#include <istream>
#include <string>

namespace cora::vmachine
{
    class BytecodeReader final
    {
    public:
        BytecodeReader() = default;

        void Read(const std::string &filePath);
        void Read(std::istream &in);

        const BytecodeProgram &GetProgram() const;

    private:
        BytecodeProgram m_program;
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODE_READER_H
