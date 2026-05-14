#ifndef CORA_IR_IRINSTRUCTION_H
#define CORA_IR_IRINSTRUCTION_H

/**
 * SSA IRepresentation (Intermediate Representation) Format.
 *
 * -  Memory Access
 *      * alloca: Allocates memory on the stack.
 *      * load: Reads a value from memory into a register.
 *      * store: Writes a value from a register into memory.
 *
 * -  Control Flow
 *      * call: Call a function.
 *      * ret: Return a value from a function.
 *      * br: Branch to a label (conditional or unconditional).
 *
 * -  Special Constants
 *      * true:
 *      * false:
 *      * void:
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <list>

#include "IRValue.hpp"

namespace cora::ir
{

    using namespace cora::compiler;

    // Forward declarations
    struct Use;
    struct Type;
    struct User;
    struct Value;
    struct BasicBlock;

    // A User with an Opcode.
    struct Instruction : public User
    {
        enum class Opcode : uint8_t
        {
            /* Bitwise Operations. */

            /* Arithmetic Operations. */

            /* Loads and Stores. */

            /* Memory Operations. */

            /* Buffer Operations. */

            /* Memory Allocations. */

            /* Type Conversions. */

            /* Calls Operations */

            /* Miscellaneous Operations. */

            Alloca,
            Call,
            Load,
            Store,
            Add,
            Sub,
            Div,
            Mul,
            Eq,
            Ne,
            Lt,
            Le,
            Gt,
            Ge,
            Br,
            Ret,
            Phi,
            Jump,
            Import,
        };

    public:
        Instruction(Opcode op, std::string name, BasicBlock *block, int num_ops);

        std::string opcodeString();

        virtual std::string toString();

    public:
        Opcode opcode;
        BasicBlock *parent;
    };

    // PHI NODE: Specialized Instruction
    struct PhiInstruction : public Instruction
    {
    public:
        // Pairs of (Value, BasicBlock)
        PhiInstruction(std::string name, BasicBlock *block);

        void addIncoming(Value *value, BasicBlock *block);

        std::string toString() override;
    };

    // Import Instruction
    struct ImportInstruction : public Instruction
    {
    public:
        ImportInstruction(Value *moduleName, BasicBlock *block);

        std::string toString() override;
    };

    // Binary Operator (Add, Sub, Mul, etc.)
    // Syntax: %res = add i32 %op1, %op2
    struct BinaryInstruction : public Instruction
    {
    public:
        BinaryInstruction(Opcode op, Value *lhs, Value *rhs, std::string name, BasicBlock *block);
        std::string toString() override;
    };

    // Unary Operator (Neg, Not)
    // Syntax: %res = fneg float %op1
    struct UnaryInstruction : public Instruction
    {
    public:
        UnaryInstruction(Opcode op, Value *value, std::string name, BasicBlock *block);

        std::string toString() override;
    };

    // Load Instruction (Memory -> Register)
    // Syntax: %value = load i32, i32* %ptr
    struct LoadInstruction : public Instruction
    {
    public:
        LoadInstruction(Value *ptr, std::string name, BasicBlock *block);

        std::string toString() override;
    };

    // Store Instruction (Register -> Memory)
    // Syntax: store i32 %value, i32* %ptr
    // Note: Store is a "Void" instruction; it doesn't produce a value name.
    struct StoreInstruction : public Instruction
    {
    public:
        StoreInstruction(Value *value, Value *ptr, BasicBlock *block);

        std::string toString() override;
    };

    // Call Instruction
    // Syntax: %res = call i32 @func(%arg1, %arg2)
    struct CallInstruction : public Instruction
    {
    public:
        CallInstruction(Value *func, std::vector<Value *> args, std::string name, BasicBlock *block);

        std::string toString() override;
    };

    // Branch Instruction (Terminator)
    // Syntax: br label %dest OR br i1 %cond, label %true, label %false
    struct BranchInstruction : public Instruction
    {
    public:
        // Unconditional
        BranchInstruction(BasicBlock *dest, BasicBlock *block);

        // Conditional
        BranchInstruction(Value *cond, BasicBlock *ifTrue, BasicBlock *ifFalse, BasicBlock *block);

        std::string toString() override;

    public:
        bool is_conditional;
    };

    // RETURN: Terminator instruction that exits a function
    // Syntax: ret i32 %value  OR  ret void
    struct ReturnInstruction : public Instruction
    {
        ReturnInstruction(BasicBlock *block, Value *value = nullptr);

        std::string toString() override;
    };

    // JUMP (Unconditional Branch)
    // Syntax: br label %dest
    struct JumpInstruction : public Instruction
    {
        JumpInstruction(BasicBlock *dest, BasicBlock *block);

        std::string toString() override;
    };

    // ALLOCA: The "Variable" allocation on stack
    // Syntax: %x = alloca i32
    struct AllocaInstruction : public Instruction
    {
        AllocaInstruction(Type *ty, std::string name, BasicBlock *block);

        std::string toString() override;
    };

} // namespace cora::ir

#endif // CORA_IR_IRINSTRUCTION_H
