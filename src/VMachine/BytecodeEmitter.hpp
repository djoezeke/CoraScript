#ifndef CORA_VMACHINE_BYTECODEEMITTER_H
#define CORA_VMACHINE_BYTECODEEMITTER_H

#include "../IRGen/IRInstruction.hpp"
#include "VMInstruction.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <list>

namespace cora::vmachine
{
    class BytecodeEmitter final
    {
        struct BytecodeBlock
        {
            BytecodeBlock(std::string name)
                : name(std::move(name)) {};

            BytecodeBlock() = delete;
            std::list<Instruction *> instructions;
            std::string name;
        };

        struct BytecodeProgram
        {
            std::vector<Instruction> code;
            std::vector<std::int64_t> constants;
            std::vector<std::string> names;
        };

    public:
        BytecodeEmitter() = default;

        BytecodeProgram Emit(const std::vector<cora::ir::BasicBlock *> &blocks);

    private:
        void addInstruction(Instruction *inst);

        void emitBinary(ir::BinaryInstruction *ptr);
        void emitAssign(ir::AssignInstruction *ptr);

    public:
        int entry{-1};
        int entry_end{-1};
        int global_var_len{-1};
        std::vector<ir::FunctionInstruction *> funcs;
        std::vector<Instruction *> vm_insts;
        std::vector<BytecodeBlock *> bytecode_blocks;
        ir::BasicBlock *global_vars{nullptr};

    private:
        std::vector<Instruction> output;
        std::map<cora::ir::IRValue *, std::int32_t> valueToLocal;
        std::map<std::string, std::int32_t> nameToLocal;
        std::int32_t nextLocal{0};

        std::int32_t allocateLocal();
        void emitValueLoad(cora::ir::IRValue *value);
        void emitInstruction(const cora::ir::Instruction *inst);
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODEEMITTER_H
