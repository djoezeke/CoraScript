#ifndef CORA_IR_IRPRINTER_H
#define CORA_IR_IRPRINTER_H

#include "IRInstruction.hpp"

#include <string>
#include <vector>

namespace cora::ir
{
    class IRPrinter final
    {
    public:
        std::string Print(const std::vector<BasicBlock *> &blocks) const;
    };

} // namespace cora::ir

#endif // CORA_IR_IRPRINTER_H
