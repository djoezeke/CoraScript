#ifndef CORA_CORE_INTERNAL_BYTECODEVM_HPP
#define CORA_CORE_INTERNAL_BYTECODEVM_HPP

#include "../IRGen/Bytecode.hpp"

#include "../Runtime/Value.hpp"

#include <string>
#include <unordered_map>
#include <vector>

namespace cora::vmachine
{
    class VMachine
    {
    private:
    public:
        VMachine();
        ~VMachine();
    };

}

namespace cora::embed::internal
{
    class JitPipeline;

    class BytecodeVm
    {
    public:
        explicit BytecodeVm(JitPipeline *jit = nullptr);

        void SetGlobal(const std::string &name, cora::compiler::runtime::Value value);
        bool HasGlobal(const std::string &name) const;

        cora::compiler::runtime::Value Execute(const BytecodeProgram &program);

    private:
        static bool IsTruthy(const cora::compiler::runtime::Value &value);
        static bool ValuesEqual(const cora::compiler::runtime::Value &lhs, const cora::compiler::runtime::Value &rhs);
        static cora::compiler::runtime::Value Add(const cora::compiler::runtime::Value &lhs, const cora::compiler::runtime::Value &rhs);
        static cora::compiler::runtime::Value Sub(const cora::compiler::runtime::Value &lhs, const cora::compiler::runtime::Value &rhs);
        static cora::compiler::runtime::Value Mul(const cora::compiler::runtime::Value &lhs, const cora::compiler::runtime::Value &rhs);
        static cora::compiler::runtime::Value Div(const cora::compiler::runtime::Value &lhs, const cora::compiler::runtime::Value &rhs);
        static cora::compiler::runtime::Value Mod(const cora::compiler::runtime::Value &lhs, const cora::compiler::runtime::Value &rhs);

    private:
        JitPipeline *m_Jit{nullptr};
        std::unordered_map<std::string, cora::compiler::runtime::Value> m_Globals;
        std::vector<cora::compiler::runtime::Value> m_Stack;
    };
}

#endif
