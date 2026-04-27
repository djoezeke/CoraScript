#include "BCWriter.hpp"

#include <sstream>

namespace cora::bc
{

    namespace
    {
        void WriteU32(BCWriter::RawBytecode &out, std::uint32_t value)
        {
            out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
            out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
            out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
            out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
        }

        void WriteI32(BCWriter::RawBytecode &out, std::int32_t value)
        {
            WriteU32(out, static_cast<std::uint32_t>(value));
        }

        void WriteI64(BCWriter::RawBytecode &out, std::int64_t value)
        {
            const auto *bytes = reinterpret_cast<const std::uint8_t *>(&value);
            out.insert(out.end(), bytes, bytes + sizeof(value));
        }

        void WriteString(BCWriter::RawBytecode &out, const std::string &value)
        {
            WriteU32(out, static_cast<std::uint32_t>(value.size()));
            out.insert(out.end(), value.begin(), value.end());
        }
    }

    BCWriter::BCWriter(std::ostream &out)
        : m_out(out) {};

    void BCWriter::Write(const std::vector<BasicBlock *> &blocks)
    {
        for (const BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }

            m_out << block->name << ":\n";

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

    void BCWriter::Write(const std::vector<BasicBlock *> &blocks, std::ostream &out)
    {
        BCWriter writer(out);
        writer.Write(blocks);
    };

    void BCWriter::WriteFile(const std::vector<BasicBlock *> &blocks, std::string filename) {
    };

    void BCWriter::WriteBinaryInstruction() {};
    void BCWriter::WriteLoadInstruction() {};
    void BCWriter::WriteStoreInstruction() {};
    void BCWriter::WriteCallInstruction() {};
    void BCWriter::WriteReturnInstruction() {};
    void BCWriter::WriteJumpInstruction() {};
    void BCWriter::WriteAllocaInstruction() {};

    BCWriter::~BCWriter() = default;

} // namespace cora::bc
