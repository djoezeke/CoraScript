#ifndef CORA_VMACHINE_BYTECODE_EMITTER_H
#define CORA_VMACHINE_BYTECODE_EMITTER_H

#include "../IR/IRInstruction.hpp"
#include "Bytecode.hpp"

#include <vector>

namespace cora::vmachine
{
    class BytecodeEmitter final
    {
    public:
        BytecodeProgram Emit(const std::vector<cora::ir::BasicBlock *> &blocks);
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODE_EMITTER_H
