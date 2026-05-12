#ifndef CORA_CORE_INTERNAL_BYTECODEVM_HPP
#define CORA_CORE_INTERNAL_BYTECODEVM_HPP

#include "../Runtime/GarbageCollector.hpp"
#include "../Runtime/Scope.hpp"
#include "../Runtime/Value.hpp"
#include "Bytecode.hpp"
#include "SharedLibrary.hpp"

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

        int Run(const BytecodeProgram &program);
        int RunFile(const std::string &bytecodeFile);

        const cora::compiler::runtime::value &GetReturnValue() const;
        std::string LastError() const;

        void SetOutput(std::ostream *out);

        bool LoadPlugin(const std::string &name);

        ~VMachine();

    private:
        static constexpr std::size_t kRegisterCount = 256;

        bool BinaryOp(OpCode op, std::int32_t dest, std::int32_t left, std::int32_t right);
        bool CompareOp(OpCode op, std::int32_t dest, std::int32_t left, std::int32_t right);
        bool UnaryOp(OpCode op, std::int32_t dest, std::int32_t source);
        bool LoadGlobal(const BytecodeProgram &program, std::int32_t dest, std::int32_t nameIndex);
        bool StoreGlobal(const BytecodeProgram &program, std::int32_t source, std::int32_t nameIndex);
        bool IsRegisterValid(std::int32_t index) const;
        void SetRuntimeError(const std::string &message);

        struct CallFrame
        {
            std::size_t returnIp{0};
            std::vector<cora::compiler::runtime::value> registers;
            std::int32_t destRegister{-1};
        };

    private:
        std::vector<cora::compiler::runtime::value> m_registers;
        std::vector<CallFrame> m_callStack;
        cora::compiler::runtime::Scope m_globals;
        cora::compiler::runtime::value m_returnValue;
        std::ostream *m_output;
        std::string m_lastError;
        std::vector<SharedLibrary> m_loadedPlugins;
    };

}

#endif
