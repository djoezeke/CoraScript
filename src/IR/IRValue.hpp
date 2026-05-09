#ifndef CORA_IR_IRVALUE_H
#define CORA_IR_IRVALUE_H

/**
 * SSA IRepresentation (Intermediate Representation) Format.
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

#include "../Runtime/Value.hpp"

namespace cora::ir
{

    using namespace cora::compiler;

    // Forward declarations
    struct User;
    struct Value;
    struct Instruction;
    struct PhiInstruction;

    struct Type
    {
        enum class ID
        {
            Int,
            Void,
            Label,
            Float,
            Pointer,
        };

    public:
        Type(ID i);

        static Type *Int();
        static Type *Void();
        static Type *Float();
        static Type *Label();
        static Type *Pointer(Type *element);

        bool is(Type type) const;
        bool isInt() const;
        bool isVoid() const;
        bool isFloat() const;
        bool isLabel() const;
        bool isPointer() const;

        std::string toString();

        bool operator==(Type *other) const;
        bool operator!=(Type *other) const;
        bool operator==(const Type &other) const;
        bool operator!=(const Type &other) const;

    public:
        ID id;
        // Only used if id == Pointer
        Type *pointee = nullptr;
    };

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
        Value(Kind k, std::string name);
        Value(Kind k, std::string name, Type *type);

        void addUse(Use &use);
        void killUse(Use &use);

        // "Replace All Uses With"
        void RAUW(Value *new_value);

        virtual std::string toString();

        virtual ~Value();

    public:
        Kind kind;
        Type *type;
        std::string name;

        // Head of the intrusive linked list of Uses
        Use *use_list = nullptr;
    };

    /**
     * @brief A Value that consumes other Values.
     */
    struct User : public Value
    {
    public:
        User(Kind k, std::string name, int num_ops);

        void setOperand(int i, Value *value);
        Value *getOperand(int i) const;

        std::string toString() override;

    public:
        // The operands are stored as Use objects
        std::vector<Use> operands;
    };

    struct Constant : public Value
    {
    public:
        Constant(runtime::value value, std::string name = "");

        std::string toString() override;

    public:
        runtime::value value;
    };

    struct BasicBlock : public Value
    {
    public:
        std::vector<PhiInstruction *> phis;
        std::vector<Instruction *> insts;
        std::vector<BasicBlock *> preds;
        std::vector<BasicBlock *> succs;

        BasicBlock(std::string name);

        static void edge(BasicBlock *from, BasicBlock *to);

        void addSuccessor(BasicBlock *block);
        void addPredecessor(BasicBlock *block);
        void addInstruction(Instruction *inst);
        void addPhiInstruction(PhiInstruction *phi);

        std::string toString() override;
    };

    // ARGUMENT: Represents a function parameter (e.g., i32 %0)
    struct Argument : public Value
    {
        struct Function *parent;
        unsigned argNo;

        Argument(Type *t, Function *f, unsigned num);

        std::string toString() override;
    };

    // FUNCTION: A global value containing BasicBlocks and Arguments
    struct Function : public Value
    {

        Function(std::string name, Type *retTy, std::vector<Type *> argTypes,  BasicBlock *body);

        std::string toString() override;

    public:
        BasicBlock * body;
        std::vector<Argument *> args;
        Type *returnType;
    };

} // namespace cora::ir

#endif // CORA_IR_IRVALUE_H
