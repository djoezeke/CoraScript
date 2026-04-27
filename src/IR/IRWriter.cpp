#include "IRWriter.hpp"

#include <sstream>

namespace cora::ir
{

    IRWriter::IRWriter(std::ostream &out)
        : m_out(out) {};

    void IRWriter::Write(const std::vector<BasicBlock *> &blocks)
    {
        for (const BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }

            m_out << block->name << ":\n";
            for (const PhiInstruction *phi : block->phis)
            {
            }

            for (const Instruction *inst : block->insts)
            {
                switch (inst->opcode)
                {
                case Instruction::Opcode::Add:
                case Instruction::Opcode::Mul:
                case Instruction::Opcode::Sub:
                case Instruction::Opcode::Div:
                    WriteBinaryInstruction();
                    break;
                case Instruction::Opcode::Alloca:
                    WriteAllocaInstruction();
                    break;
                case Instruction::Opcode::Call:
                    WriteBinaryInstruction();
                    break;
                case Instruction::Opcode::Load:
                    WriteLoadInstruction();
                    break;
                case Instruction::Opcode::Store:
                    WriteStoreInstruction();
                    break;
                case Instruction::Opcode::Br:
                    WriteBranchInstruction();
                    break;
                case Instruction::Opcode::Phi:
                    WritePhiInstruction();
                    break;
                case Instruction::Opcode::Ret:
                    WriteReturnInstruction();
                    break;
                case Instruction::Opcode::Jump:
                    WriteJumpInstruction();
                    break;

                default:
                    break;
                }
            }
        }
    };

    void IRWriter::Write(const std::vector<BasicBlock *> &blocks, std::ostream &out)
    {
        IRWriter writer(out);
        writer.Write(blocks);
    };

    void IRWriter::WriteFile(const std::vector<BasicBlock *> &blocks, std::string filename) {
    };

    void IRWriter::WritePhiInstruction() {};
    void IRWriter::WriteBinaryInstruction() {};
    void IRWriter::WriteLoadInstruction() {};
    void IRWriter::WriteStoreInstruction() {};
    void IRWriter::WriteCallInstruction() {};
    void IRWriter::WriteBranchInstruction() {};
    void IRWriter::WriteReturnInstruction() {};
    void IRWriter::WriteJumpInstruction() {};
    void IRWriter::WriteAllocaInstruction() {};

    IRWriter::~IRWriter() = default;

} // namespace cora::ir
