#ifndef CORA_CORE_INTERNAL_BYTECODEVM_HPP
#define CORA_CORE_INTERNAL_BYTECODEVM_HPP

#include "../Runtime/GarbageCollector.hpp"
#include "../Runtime/Scope.hpp"
#include "../Runtime/Value.hpp"
#include "VMInstruction.hpp"

#include <iosfwd>
#include <string>
#include <vector>

namespace cora::vmachine
{
    class VMachine final
    {
    public:
        VMachine();
        explicit VMachine(std::ostream *out);

        int Run(const std::vector<Instruction *> &program);
        int RunFile(const std::string &bytecodeFile);

        const cora::compiler::runtime::Value &GetReturnValue() const;
        std::string LastError() const;

        void SetOutput(std::ostream *out);

        ~VMachine();

    private:
        static std::string LocalName(std::int32_t slot);
        bool PushLocal(std::int32_t slot);
        bool StoreLocal(std::int32_t slot, const cora::compiler::runtime::Value &value);
        bool BinaryNumeric(VMOpcode op);
        void SetRuntimeError(const std::string &message);

    private:
        std::vector<cora::compiler::runtime::Value> m_stack;
        cora::compiler::runtime::Scope m_scope;
        cora::compiler::runtime::Value m_returnValue;
        std::ostream *m_output;
        std::string m_lastError;
    };

}

#endif
