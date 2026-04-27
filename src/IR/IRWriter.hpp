#ifndef CORA_IR_IRWRITER_H
#define CORA_IR_IRWRITER_H

#include "IRInstruction.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace cora::ir
{
    class IRWriter final
    {
    public:
        IRWriter();
        IRWriter(std::ostream &out);

        void Write(const std::vector<BasicBlock *> &blocks);

        static void Write(const std::vector<BasicBlock *> &blocks, std::ostream &out);
        static void WriteFile(const std::vector<BasicBlock *> &blocks, std::string filename);

        ~IRWriter();

    private:
        void WritePhiInstruction();
        void WriteBinaryInstruction();
        void WriteLoadInstruction();
        void WriteStoreInstruction();
        void WriteCallInstruction();
        void WriteBranchInstruction();
        void WriteReturnInstruction();
        void WriteJumpInstruction();
        void WriteAllocaInstruction();

    private:
        std::ostream &m_out;
        std::vector<BasicBlock *> *blocks;
    };

} // namespace cora::ir

#endif // CORA_IR_IRWRITER_H
