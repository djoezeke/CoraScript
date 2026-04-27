#ifndef CORA_VMACHINE_BYTECODEEMITTER_H
#define CORA_VMACHINE_BYTECODEEMITTER_H

#include "../IRGen/IRInstruction.hpp"
#include "VMInstruction.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <list>

namespace cora::bc
{
    class BytecodeEmitter final
    {

        struct BytecodeProgram
        {
            std::vector<Instruction> code;
            std::vector<std::int64_t> constants;
            std::vector<std::string> names;
        };

    public:
        BytecodeEmitter(const std::vector<cora::ir::BasicBlock *> &program);

        std::vector<BytecodeBlock *> Emit();

    public:
        int entry{-1};
        int entry_end{-1};
        int global_var_len{-1};
        std::vector<ir::Function *> funcs;
        std::vector<Instruction *> vm_insts;
        std::vector<BytecodeBlock *> bytecode_blocks;
        ir::BasicBlock *global_vars{nullptr};

    private:
        Instruction *EmitValue(cora::ir::Value *value);
        Instruction *EmitArgument(cora::ir::Value *value);
        Instruction *EmitFunction(cora::ir::Value *value);

        Instruction *EmitInstruction(cora::ir::Instruction *inst);
        Instruction *EmitPhiInstruction(cora::ir::Instruction *inst);
        Instruction *EmitBinaryInstruction(cora::ir::Instruction *inst);
        Instruction *EmitUnaryInstruction(cora::ir::Instruction *inst);
        Instruction *EmitLoadInstruction(cora::ir::Instruction *inst);
        Instruction *EmitStoreInstruction(cora::ir::Instruction *inst);
        Instruction *EmitCallInstruction(cora::ir::Instruction *inst);
        Instruction *EmitBranchInstruction(cora::ir::Instruction *inst);
        Instruction *EmitReturnInstruction(cora::ir::Instruction *inst);
        Instruction *EmitJumpInstruction(cora::ir::Instruction *inst);
        Instruction *EmitAllocaInstruction(cora::ir::Instruction *inst);

    private:
        void addInstruction(Instruction *inst);

    private:
        std::vector<BytecodeBlock *> output;
        std::map<cora::ir::Value *, std::int32_t> valueToLocal;
        std::map<std::string, std::int32_t> nameToLocal;
        std::int32_t nextLocal{0};
    };

} // namespace cora::bc

#endif // CORA_VMACHINE_BYTECODEEMITTER_H
