#ifndef CORA_VMACHINE_BYTECODE_EMITTER_H
#define CORA_VMACHINE_BYTECODE_EMITTER_H

#include "../IR/IRInstruction.hpp"
#include "Bytecode.hpp"

#include <vector>
#include <unordered_map>
#include <string>

namespace cora::vmachine
{
    // Moved Fixup struct out of anonymous namespace in .cpp and into here
    struct Fixup
    {
        std::size_t at;
        const cora::ir::BasicBlock *target;
    };

    class BytecodeEmitter final
    {
    public:
        BytecodeProgram Emit(const std::vector<cora::ir::BasicBlock *> &blocks);

    private:
        // Helper methods for bytecode generation
        std::int32_t allocateRegister();
        std::int32_t getNameIndex(const std::string &name);
        std::int32_t getRegister(const cora::ir::Value *value, bool emitGlobalLoad);

        // Member variables to replace captured variables in original lambdas
        BytecodeProgram m_program;
        std::unordered_map<const cora::ir::Value *, std::int32_t> m_valueToRegister;
        std::unordered_map<const cora::ir::Function *, std::int32_t> m_functionConstants;
        std::unordered_map<std::string, std::int32_t> m_nameToIndex;
        std::unordered_map<const cora::ir::BasicBlock *, std::size_t> m_blockToIp;
        std::vector<Fixup> m_fixups;
        std::int32_t m_nextRegister = 0;
    };

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODE_EMITTER_H
