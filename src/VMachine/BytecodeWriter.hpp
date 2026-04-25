/**
 * Bytecode Writer
 */

#ifndef CORA_VMACHINE_BYTECODEWRITER_H
#define CORA_VMACHINE_BYTECODEWRITER_H

#include "Bytecode.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace cora::vmachine
{
    class BytecodeWriter final
    {
    public:
        using RawBytecode = std::vector<std::uint8_t>;

        BytecodeWriter() = default;

        RawBytecode Write(const BytecodeProgram &program) const;
        void Write(const std::string &file, const BytecodeProgram &program) const;
        void Write(const std::string &file, const RawBytecode &bytecode) const;

    private:
        static constexpr std::uint32_t kMagic = 0xC0DEBEEF;
        static constexpr std::uint32_t kVersion = 1;
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODEWRITER_H
