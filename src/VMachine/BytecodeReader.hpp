/**
 * Bytecode Reader
 */

#ifndef CORA_VMACHINE_BYTECODEREADER_H
#define CORA_VMACHINE_BYTECODEREADER_H

#include "Bytecode.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace cora::vmachine
{
    class BytecodeReader final
    {
    public:
        using RawBytecode = std::vector<std::uint8_t>;

        BytecodeReader() = default;

        void Read(const std::string &file);
        void Read(const RawBytecode &bytecode);

        const RawBytecode &GetRawBytecode() const;
        const BytecodeProgram &GetProgram() const;

    private:
        static constexpr std::uint32_t kMagic = 0xC0DEBEEF;
        static constexpr std::uint32_t kVersion = 1;

        RawBytecode m_bytecode;
        BytecodeProgram m_program;
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODEREADER_H
