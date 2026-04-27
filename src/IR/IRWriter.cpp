#include "IRPrinter.hpp"

#include <sstream>

namespace cora::ir
{
    namespace
    {
        std::string formatValue(const IRValue *value)
        {
            if (value == nullptr)
            {
                return "<null>";
            }

            if (const auto *constant = dynamic_cast<const ConstantInt *>(value))
            {
                return std::to_string(constant->value);
            }

            return value->name.empty() ? "<unnamed>" : value->name;
        }
    }

    IRPrinter::IRPrinter() = default;

    std::string IRPrinter::Print(const std::vector<BasicBlock *> &blocks) const
    {
        std::ostringstream out;
        Print(blocks, out);
        return out.str();
    }

    void IRPrinter::Print(const std::vector<BasicBlock *> &blocks, std::ostream &out) const
    {
        for (const BasicBlock *block : blocks)
        {
            if (block == nullptr)
            {
                continue;
            }

            out << block->label << ":\n";
            for (const PhiInstruction *phi : block->phis)
            {
                out << "  " << phi->name << " = phi";
                for (IRValue *operand : phi->operands)
                {
                    out << ' ' << formatValue(operand);
                }
                out << '\n';
            }

            for (const IRInstruction *inst : block->instructions)
            {
                out << "  " << inst->name << " = " << opToString(inst->op);
                for (IRValue *operand : inst->operands)
                {
                    out << ' ' << formatValue(operand);
                }
                out << '\n';
            }
        }
    }

    IRPrinter::~IRPrinter() = default;

} // namespace cora::ir
