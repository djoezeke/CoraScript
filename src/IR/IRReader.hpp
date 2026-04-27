#ifndef CORA_IR_IRREADER_H
#define CORA_IR_IRREADER_H

#include "IRInstruction.hpp"

#include <istream>
#include <string>
#include <vector>

namespace cora::ir
{
    class IRReader final
    {
    public:
        IRReader();
        IRReader(std::istream &in);
        std::vector<BasicBlock *> Read();

        static std::vector<BasicBlock *> Read(std::istream &in);
        static std::vector<BasicBlock *> ReadFile(std::string filename);

        ~IRReader();

    private:
        void ReadPhiInstruction();
        void ReadBinaryInstruction();
        void ReadLoadInstruction();
        void ReadStoreInstruction();
        void ReadCallInstruction();
        void ReadBranchInstruction();
        void ReadReturnInstruction();
        void ReadJumpInstruction();
        void ReadAllocaInstruction();

    private:
        std::istream m_in;
        std::vector<BasicBlock *> blocks;
    };

} // namespace cora::ir

#endif // CORA_IR_IRREADER_H
