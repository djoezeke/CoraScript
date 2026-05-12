#ifndef CORA_VMACHINE_BYTECODE_H
#define CORA_VMACHINE_BYTECODE_H

#include "../Runtime/Value.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace cora::vmachine
{
    enum class OpCode : std::uint8_t
    {
        Nop = 0,
        LoadConst,
        Move,
        Add,
        Sub,
        Mul,
        Div,
        Mod,
        Neg,
        Not,
        Eq,
        Ne,
        Lt,
        Le,
        Gt,
        Ge,
        Jump,
        JumpIfTrue,
        JumpIfFalse,
        LoadGlobal,
        StoreGlobal,
        Call,
        Return,
        Print,
        Halt,
        Import
    };

    struct Instruction
    {
        OpCode op{OpCode::Nop};
        std::int32_t a{0};
        std::int32_t b{0};
        std::int32_t c{0};
    };

    struct BytecodeProgram
    {
        std::vector<Instruction> code;
        std::vector<cora::compiler::runtime::value> constants;
        std::vector<std::string> names;
    };

    inline const char *ToString(OpCode op)
    {
        switch (op)
        {
        case OpCode::Nop:
            return "NOP";
        case OpCode::LoadConst:
            return "LOAD_CONST";
        case OpCode::Move:
            return "MOVE";
        case OpCode::Add:
            return "ADD";
        case OpCode::Sub:
            return "SUB";
        case OpCode::Mul:
            return "MUL";
        case OpCode::Div:
            return "DIV";
        case OpCode::Mod:
            return "MOD";
        case OpCode::Neg:
            return "NEG";
        case OpCode::Not:
            return "NOT";
        case OpCode::Eq:
            return "EQ";
        case OpCode::Ne:
            return "NE";
        case OpCode::Lt:
            return "LT";
        case OpCode::Le:
            return "LE";
        case OpCode::Gt:
            return "GT";
        case OpCode::Ge:
            return "GE";
        case OpCode::Jump:
            return "JUMP";
        case OpCode::JumpIfTrue:
            return "JUMP_IF_TRUE";
        case OpCode::JumpIfFalse:
            return "JUMP_IF_FALSE";
        case OpCode::LoadGlobal:
            return "LOAD_GLOBAL";
        case OpCode::StoreGlobal:
            return "STORE_GLOBAL";
        case OpCode::Call:
            return "CALL";
        case OpCode::Return:
            return "RETURN";
        case OpCode::Print:
            return "PRINT";
        case OpCode::Halt:
            return "HALT";
        case OpCode::Import:
            return "IMPORT";
        }
        return "UNKNOWN";
    }

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODE_H
