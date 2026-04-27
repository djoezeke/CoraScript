/**
 * Bytecode Reader
 */

#ifndef CORA_BYTECODE_BCREADER_H
#define CORA_BYTECODE_BCREADER_H

#include "BCInstruction.hpp"

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

namespace cora::bc
{
    class BCReader final
    {
    public:
        using Byte = std::uint8_t;
        using RawBytecode = std::vector<Byte>;

    public:
        BCReader();
        BCReader(std::istream &in);
        std::vector<BasicBlock *> Read();

        static std::vector<BasicBlock *> Read(std::istream &in);
        static std::vector<BasicBlock *> ReadFile(std::string filename);

        ~BCReader();

    private:
        void ReadBinaryInstruction();
        void ReadLoadInstruction();
        void ReadStoreInstruction();
        void ReadCallInstruction();
        void ReadReturnInstruction();
        void ReadJumpInstruction();
        void ReadAllocaInstruction();

    private:
        std::uint32_t kMagic;
        std::uint32_t kVersion;
        std::istream m_in;
        std::vector<BasicBlock *> blocks;
    };

} // namespace cora::bc

#endif // CORA_BYTECODE_BCREADER_H
