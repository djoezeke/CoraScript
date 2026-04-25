#ifndef CORA_VMACHINE_BYTECODEEMITTER_H
#define CORA_VMACHINE_BYTECODEEMITTER_H

#include "../IRGen/IRInstruction.hpp"
#include "Bytecode.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cora::vmachine
{
    class BytecodeEmitter final
    {
    public:
        BytecodeEmitter() = default;

        BytecodeProgram Emit(const std::vector<cora::ir::BasicBlock *> &blocks);

    private:
        std::vector<Instruction> output;
        std::map<cora::ir::Value *, std::int32_t> valueToLocal;
        std::map<std::string, std::int32_t> nameToLocal;
        std::int32_t nextLocal{0};

        std::int32_t allocateLocal();
        void emitValueLoad(cora::ir::Value *value);
        void emitInstruction(const cora::ir::Instruction *inst);
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODEEMITTER_H
