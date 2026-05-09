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
                out << "  ";
                if (!inst->name.empty()) out << inst->name << " = ";
                
                switch (inst->opcode) {
                    case Instruction::Opcode::Br: {
                        auto *br = static_cast<const BranchInstruction*>(inst);
                        if (br->is_conditional) {
                            out << "br " << br->getOperand(0)->name << ", label " << br->getOperand(1)->name << ", label " << br->getOperand(2)->name << "\n";
                        } else {
                            out << "br label " << br->getOperand(0)->name << "\n";
                        }
                        break;
                    }
                    case Instruction::Opcode::Jump:
                        out << "jump label " << inst->getOperand(0)->name << "\n";
                        break;
                    case Instruction::Opcode::Ret:
                        out << "ret";
                        if (!inst->operands.empty()) out << " " << inst->getOperand(0)->name;
                        out << "\n";
                        break;
                    case Instruction::Opcode::Call: {
                        out << "call " << inst->getOperand(0)->name << "(";
                        for (size_t i = 1; i < inst->operands.size(); ++i) {
                            out << inst->getOperand(static_cast<int>(i))->name << (i == inst->operands.size() - 1 ? "" : ", ");
                        }
                        out << ")\n";
                        break;
                    }
                    default:
                        out << inst->name << " (opcode " << static_cast<int>(inst->opcode) << ")\n";
                        break;
                }
            }
        }
        return out.str();
    }

} // namespace cora::ir

