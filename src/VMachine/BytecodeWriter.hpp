/**
 * Bytecode Writer
 */

#ifndef CORA_VMACHINE_BYTECODEWRITER_H
#define CORA_VMACHINE_BYTECODEWRITER_H

#include "VMInstruction.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace cora::vmachine
{
    class BytecodeWriter final
    {
    public:
        using Byte = std::uint8_t;
        using RawBytecode = std::vector<Byte>;

        BytecodeWriter() = default;
         BytecodeWriter(std::string filename)
            : filename(std::move(filename)) {};

        void Write() const;

    public:
        int entry{-1};
        int entry_end{-1};
        int global_var_len{-1};
        std::vector<Instruction *> vm_insts;

    private:
        std::uint32_t kMagic;
        std::uint32_t kVersion;
        Instruction *cur_instr{nullptr};
        std::string filename;
        std::ofstream out;
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODEWRITER_H
