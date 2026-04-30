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
            }
        }
        return out.str();
    }

} // namespace cora::ir
