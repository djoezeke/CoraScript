#include "IRPrinter.hpp"

#include <sstream>

namespace cora::ir
{
    std::string IRPrinter::Print(const std::vector<BasicBlock *> &blocks) const
    {
        std::ostringstream out;
        for (const BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }
            out << block->name << ":\n";
            for (const Instruction *inst : block->insts)
            {
                if (inst == nullptr)
                {
                    continue;
                }
                out << "  " << inst->name << "\n";
                // Potentially add more detailed printing for specific instructions if needed
                // For example:
                // if (inst->opcode == Instruction::Opcode::GetField) {
                //     out << "  " << inst->name << " <field_name>\n";
                // }
            }
        }
        return out.str();
    }

} // namespace cora::ir

