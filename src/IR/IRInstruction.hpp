#ifndef CORA_IR_IRINSTRUCTION_H
#define CORA_IR_IRINSTRUCTION_H

/**
 * SSA IRepresentation (Intermediate Representation) Format.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <list>

#include "../Runtime/Value.hpp"

namespace cora::ir
{

    using namespace cora::compiler;

    struct Type
    {
        enum ID
        {
            Int32,
            Float,
            Pointer,
            Void,
            Label
        };

        ID id;
        Type *pointee = nullptr; // Only used if id == Pointer

        Type(ID i) : id(i) {}
        static Type *getInt32Ty() { return new Type(Int32); }
        static Type *getVoidTy() { return new Type(Void); }
        static Type *getPtrTy(Type *element)
        {
            Type *t = new Type(Pointer);
            t->pointee = element;
            return t;
        };

        bool isPointer() const { return id == Pointer; }
        bool equals(Type *other) const
        {
            if (id != other->id)
                return false;
            if (id == Pointer)
                return pointee->equals(other->pointee);
            return true;
        };
    };

    // Forward declarations
    struct User;
    struct Value;

    /**
     * @brief THE USE: The "glue" object.
     * @note This allows O(1) removal of a user from a value's use-list.
     */
    struct Use
    {
    public:
        void set(Value *value, User *use);

    public:
        Value *value = nullptr;
        User *user = nullptr;

        // Links in the circular list of a Value's users
        Use *next = nullptr;
        Use *prev = nullptr;
    };

    /**
     * @brief The Value class.
     */
    struct Value
    {
    public:
        enum class Kind
        {
            Constant,
            Instruction,
            Argument,
            BasicBlock
        };

    public:
        Value(Kind k, std::string name)
            : kind(k), name(name), type(nullptr) {};

        Value(Kind k, std::string name, Type *type)
            : kind(k), name(name), type(type) {};

        void addUse(Use &use)
        {
            use.next = use_list;
            if (use_list)
                use_list->prev = &use;
            use_list = &use;
        };

        void killUse(Use &use) {
        };

        // "Replace All Uses With" - Essential SSA optimization tool
        void RAUW(Value *NewV);

    public:
        Kind kind;
        Type *type;
        std::string name;

        // Head of the intrusive linked list of Uses
        Use *use_list = nullptr;
    };

    struct BasicBlock : public Value
    {
        std::vector<PhiInstruction *> phis;
        std::vector<Instruction *> insts;
        std::vector<BasicBlock *> preds;
        std::vector<BasicBlock *> succs;

        BasicBlock(std::string name)
            : Value(Kind::BasicBlock, name) {};

        static void edge(BasicBlock *from, BasicBlock *to)
        {
            from->succs.push_back(to);
            to->preds.push_back(from);
        };

        void addSuccessor(BasicBlock *block)
        {
            if (block != nullptr)
            {
                succs.push_back(block);
            }
        };

        void addPredecessor(BasicBlock *block)
        {
            if (block != nullptr)
            {
                preds.push_back(block);
            }
        };

        void addInstruction(Instruction *inst)
        {
            if (inst != nullptr)
            {
                insts.push_back(inst);
            }
        };

        void addPhiInstruction(PhiInstruction *phi)
        {
            if (phi != nullptr)
            {
                phis.push_back(phi);
            }
        };
    };

    /**
     * @brief A Value that consumes other Values.
     */
    struct User : public Value
    {
    public:
        User(Kind k, std::string name, int num_ops)
            : Value(k, name)
        {
            op_storage.resize(num_ops);
            for (auto &use : op_storage)
                use.user = this;
        };

        void setOperand(int i, Value *value)
        {
            op_storage[i].set(value, this);
        };

        Value *getOperand(int i) const
        {
            return op_storage[i].value;
        };

    public:
        // The operands are stored as Use objects
        std::vector<Use> op_storage;
    };

    // ARGUMENT: Represents a function parameter (e.g., i32 %0)
    struct Argument : public Value
    {
        struct Function *parent;
        unsigned argNo;

        Argument(Type *t, Function *f, unsigned num)
            : Value(Kind::Argument, std::to_string(num), t), parent(f), argNo(num) {};
    };

    // FUNCTION: A global value containing BasicBlocks and Arguments
    struct Function : public Value
    {

        Function(std::string name, Type *retTy, std::vector<Type *> argTypes)
            : Value(Kind::Constant, name, retTy), returnType(retTy)
        {
            for (size_t i = 0; i < argTypes.size(); ++i)
            {
                args.push_back(new Argument(argTypes[i], this, i));
            }
        };

        void addBlock(BasicBlock *bb) { blocks.push_back(bb); };

    public:
        std::vector<BasicBlock *> blocks;
        std::vector<Argument *> args;
        Type *returnType;
    };

    // A User with an Opcode.
    struct Instruction : public User
    {
        enum class Opcode
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
            Br,
            Ret,
            Phi,
        };

    public:
        Instruction(Opcode op, std::string name, BasicBlock *block, int num_ops)
            : User(Kind::Instruction, name, num_ops), opcode(op), parent(block)
        {
            parent->insts.push_back(this);
        };

    public:
        Opcode opcode;
        struct BasicBlock *parent;
    };

    // IMPLEMENTATION OF USE SETTING
    void Use::set(Value *value, User *use)
    {
        if (value)
        { /* handle removing from old list if necessary */
        }
        value = value;
        user = use;
        if (value)
            value->addUse(*this);
    };

    // PHI NODE: Specialized Instruction
    struct PhiInstruction : public Instruction
    {
    public:
        // Pairs of (Value, BasicBlock)
        PhiInstruction(std::string name, BasicBlock *block)
            : Instruction(Opcode::Phi, name, block, 0) {} // Phi grows dynamically

        void addIncoming(Value *value, BasicBlock *block)
        {
            Use use;
            use.set(value, this);
            op_storage.push_back(use);
            // Additional logic to track which block matches which operand
        };
    };

    // Binary Operator (Add, Sub, Mul, etc.)
    // Syntax: %res = add i32 %op1, %op2
    struct BinaryInstruction : public Instruction
    {
    public:
        BinaryInstruction(Opcode op, Value *lhs, Value *rhs, std::string name, BasicBlock *block)
            : Instruction(op, name, block, 2)
        {
            setOperand(0, lhs);
            setOperand(1, rhs);
        };
    };

    // Unary Operator (Neg, Not)
    // Syntax: %res = fneg float %op1
    struct UnaryInstruction : public Instruction
    {
    public:
        UnaryInstruction(Opcode op, Value *value, std::string name, BasicBlock *block)
            : Instruction(op, name, block, 1)
        {
            setOperand(0, value);
        };
    };

    // Load Instruction (Memory -> Register)
    // Syntax: %value = load i32, i32* %ptr
    struct LoadInstruction : public Instruction
    {
    public:
        LoadInstruction(Value *ptr, std::string name, BasicBlock *block)
            : Instruction(Opcode::Load, name, block, 1)
        {
            setOperand(0, ptr); // The source address
        };
    };

    // Store Instruction (Register -> Memory)
    // Syntax: store i32 %value, i32* %ptr
    // Note: Store is a "Void" instruction; it doesn't produce a value name.
    struct StoreInstruction : public Instruction
    {
    public:
        StoreInstruction(Value *value, Value *ptr, BasicBlock *block)
            : Instruction(Opcode::Store, "", block, 2)
        {
            setOperand(0, value); // Value to store
            setOperand(1, ptr);   // Destination address
        };
    };

    // Call Instruction
    // Syntax: %res = call i32 @func(%arg1, %arg2)
    struct CallInstruction : public Instruction
    {
    public:
        CallInstruction(Value *func, std::vector<Value *> args, std::string name, BasicBlock *block)
            : Instruction(Opcode::Call, name, block, args.size() + 1)
        {
            setOperand(0, func); // First operand is usually the function pointer
            for (size_t i = 0; i < args.size(); ++i)
            {
                setOperand(i + 1, args[i]);
            }
        };
    };

    // Branch Instruction (Terminator)
    // Syntax: br label %dest OR br i1 %cond, label %true, label %false
    struct BranchInstruction : public Instruction
    {
    public:
        // Unconditional
        BranchInstruction(BasicBlock *dest, BasicBlock *block)
            : Instruction(Opcode::Br, "", block, 1), is_conditional(false)
        {
            setOperand(0, (Value *)dest);
        };

        // Conditional
        BranchInstruction(Value *cond, BasicBlock *ifTrue, BasicBlock *ifFalse, BasicBlock *block)
            : Instruction(Opcode::Br, "", block, 3), is_conditional(true)
        {
            setOperand(0, cond);
            setOperand(1, (Value *)ifTrue);
            setOperand(2, (Value *)ifFalse);
        };

    public:
        bool is_conditional;
    };

    // RETURN: Terminator instruction that exits a function
    // Syntax: ret i32 %value  OR  ret void
    struct ReturnInstruction : public Instruction
    {
        ReturnInstruction(BasicBlock *block, Value *value = nullptr)
            : Instruction(Opcode::Ret, "ret", block, value ? 1 : 0)
        {
            if (value)
            {
                setOperand(0, value);
                this->type = value->type;
            }
            else
            {
                this->type = Type::getVoidTy();
            }
        };
    };

    // JUMP (Unconditional Branch)
    // Syntax: br label %dest
    struct JumpInstruction : public Instruction
    {
        JumpInstruction(BasicBlock *dest, BasicBlock *block)
            : Instruction(Opcode::Br, "", block, 1)
        {
            setOperand(0, (Value *)dest);

            // Link CFG
            block->succs.push_back(dest);
            dest->preds.push_back(block);
            this->type = Type::getVoidTy();
        };
    };

    // ALLOCA: The "Variable" allocation on stack
    // Syntax: %x = alloca i32
    struct AllocaInstruction : public Instruction
    {
        Type *type;
        AllocaInstruction(Type *ty, std::string name, BasicBlock *block)
            : Instruction(Opcode::Alloca, name, block, 0), type(ty)
        {
            // Result is a pointer to the type
            this->type = Type::getPtrTy(ty);
        };
    };

} // namespace cora::ir

#endif // CORA_IR_IRINSTRUCTION_H
