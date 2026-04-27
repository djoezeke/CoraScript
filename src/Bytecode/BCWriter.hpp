/**
 * Bytecode Writer
 */

#ifndef CORA_BYTECODE_BCWRITER_H
#define CORA_BYTECODE_BCWRITER_H

#include "BCInstruction.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace cora::bc
{
    class BCWriter final
    {
    public:
        using Byte = std::uint8_t;
        // A simple buffer to hold raw bytes
        using RawBytecode = std::vector<Byte>;

    public:
        BCWriter();
        BCWriter(std::ostream &out);

        void Write(const std::vector<BasicBlock *> &blocks);

        static void Write(const std::vector<BasicBlock *> &blocks, std::ostream &out);
        static void WriteFile(const std::vector<BasicBlock *> &blocks, std::string filename);

        ~BCWriter();

    private:
        void WriteBinaryInstruction();
        void WriteLoadInstruction();
        void WriteStoreInstruction();
        void WriteCallInstruction();
        void WriteReturnInstruction();
        void WriteJumpInstruction();
        void WriteAllocaInstruction();

    private:
        std::uint32_t kMagic;
        std::uint32_t kVersion;
        std::ostream &m_out;
        std::vector<BasicBlock *> *blocks;
    };

} // namespace cora::bc

#endif // CORA_BYTECODE_BCWRITER_H
