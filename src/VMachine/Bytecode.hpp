// Source -> AST -> SSA IR -> Optimized IR -> Bytecode.

#ifndef CORA_VMACHINE_BYTECODE_H
#define CORA_VMACHINE_BYTECODE_H

#include <cstdint>
#include <string>
#include <vector>

namespace cora::vmachine
{
    enum class OpCode : std::uint8_t
    {
        PushConst,
        LoadLocal,
        StoreLocal,
        Add,
        Mul,
        Sub,
        Div,
        Print,
        Jump,
        JumpIfFalse,
        Return,
        Halt
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
        std::vector<std::int64_t> constants;
        std::vector<std::string> names;
    };

    inline const char *ToString(OpCode op)
    {
        switch (op)
        {
        case OpCode::PushConst:
            return "push_const";
        case OpCode::LoadLocal:
            return "load_local";
        case OpCode::StoreLocal:
            return "store_local";
        case OpCode::Add:
            return "add";
        case OpCode::Mul:
            return "mul";
        case OpCode::Sub:
            return "sub";
        case OpCode::Div:
            return "div";
        case OpCode::Print:
            return "print";
        case OpCode::Jump:
            return "jump";
        case OpCode::JumpIfFalse:
            return "jump_if_false";
        case OpCode::Return:
            return "return";
        case OpCode::Halt:
            return "halt";
        }

        return "unknown";
    }

} // namespace cora::vmachine

#endif // CORA_VMACHINE_BYTECODE_H
