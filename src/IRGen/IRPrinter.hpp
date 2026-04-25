#ifndef CORA_IR_IRPRINTER_H
#define CORA_IR_IRPRINTER_H

#include "IRInstruction.hpp"

#include <ostream>
#include <string>
#include <vector>

namespace cora::ir
{
    class IRPrinter final
    {
    public:
        IRPrinter();

        std::string Print(const std::vector<BasicBlock *> &blocks) const;
        void Print(const std::vector<BasicBlock *> &blocks, std::ostream &out) const;

        ~IRPrinter();
    };

} // namespace cora::ir

#endif // CORA_IR_IRPRINTER_H
