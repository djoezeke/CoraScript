#ifndef CORA_CORE_INTERNAL_BYTECODE_HPP
#define CORA_CORE_INTERNAL_BYTECODE_HPP

#include "../Runtime/Value.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace cora::embed::internal
{
    enum class OpCode : std::uint8_t
    {
        PushConst,
        PushNull,
        LoadGlobal,
        StoreGlobal,
        Pop,

        Add,
        Sub,
        Mul,
        Div,
        Mod,

        Negate,
        LogicalNot,

        Equal,
        NotEqual,
        Less,
        LessEqual,
        Greater,
        GreaterEqual,

        Jump,
        JumpIfFalse,

        Call,
        Return,
        Halt,
    };

    struct Instruction
    {
        OpCode op{OpCode::Halt};
        std::int32_t a{0};
        std::int32_t b{0};
    };

    struct BytecodeProgram
    {
        std::vector<Instruction> code;
        std::vector<cora::compiler::runtime::Value> constants;
        std::vector<std::string> names;

        std::int32_t AddConstant(cora::compiler::runtime::Value value)
        {
            constants.push_back(std::move(value));
            return static_cast<std::int32_t>(constants.size() - 1);
        }

        std::int32_t AddName(const std::string &name)
        {
            for (std::size_t i = 0; i < names.size(); ++i)
            {
                if (names[i] == name)
                {
                    return static_cast<std::int32_t>(i);
                }
            }

            names.push_back(name);
            return static_cast<std::int32_t>(names.size() - 1);
        }

        std::int32_t Emit(OpCode op, std::int32_t a = 0, std::int32_t b = 0)
        {
            code.push_back(Instruction{op, a, b});
            return static_cast<std::int32_t>(code.size() - 1);
        }
    };
}

#endif
