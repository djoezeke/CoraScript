/**
 * Bytecode Reader
 */

#ifndef CORA_VMACHINE_BYTECODEREADER_H
#define CORA_VMACHINE_BYTECODEREADER_H

#include "VMInstruction.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace cora::vmachine
{
    class BytecodeReader final
    {
    public:
        using Byte = std::uint8_t;
        using RawBytecode = std::vector<Byte>;

        BytecodeReader() = default;

        BytecodeReader(std::string filename)
            : filename(std::move(filename)) {};

        void Read() const;

    public:
        int entry{-1};
        int entry_end{-1};
        int global_var_len{-1};
        std::vector<Instruction *> vm_insts;

    private:
        std::uint32_t kMagic;
        std::uint32_t kVersion;
        Opcode cur_opcode;
        std::string filename;
        std::ifstream in;
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODEREADER_H
