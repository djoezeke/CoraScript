#ifndef CORA_BYTECODE_BCINSTRUCTION_H
#define CORA_BYTECODE_BCINSTRUCTION_H

/**
 * V = variable slot
 * I = int const
 * B = bool const
 * F = float const
 * S = string const
 */

#include <list>
#include <memory>
#include <sstream>
#include <string>

#include "../Runtime/Value.hpp"
#include <string>

namespace cora::bc
{
    using namespace cora::compiler;

    enum class ArgType
    {
        MAP, // 0
        RAW  // 1
    };

    struct Instruction;

    struct BasicBlock
    {
        BasicBlock(std::string name)
            : name(std::move(name)) {};

        BasicBlock() = delete;

        std::list<Instruction *> insts;
        std::string name;
    };

    struct Instruction
    {

        enum class Opcode : uint8_t
        {
            Alloca = 0x01,
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
            Jump,

            CACHE,

            IMPORT_FROM,
            IMPORT_NAME,

            // Conversion
            CONVERT_VALUE,

            TO_STR,
            TO_STR_INT,
            TO_STR_STR,
            TO_STR_BOOL,
            TO_STR_LIST,
            TO_STR_NONE,
            TO_STR_FLOAT,

            TO_BOOL,
            TO_BOOL_INT,
            TO_BOOL_STR,
            TO_BOOL_BOOL,
            TO_BOOL_LIST,
            TO_BOOL_NONE,
            TO_BOOL_FLOAT,

            // Binary
            BINARY_OP_ADD_INT,
            BINARY_OP_ADD_STR,
            BINARY_OP_ADD_FLOAT,

            BINARY_OP_SUB_INT,
            BINARY_OP_SUB_STR,
            BINARY_OP_SUB_FLOAT,

            BINARY_OP_MUL_INT,
            BINARY_OP_MUL_STR,
            BINARY_OP_MUL_FLOAT,

            BINARY_OP_MUL_INT,
            BINARY_OP_DIV_STR,
            BINARY_OP_DIV_FLOAT,

            ADD,
            SUB,
            MUL,
            DIV,
            MOD,
            EXP,
            BAND,
            BOR,
            BXOR,
            SHL,
            SHR,
            LOR,
            // COMPARISION
            NE,
            EQ,
            LT,
            LE,
            GT,
            GE,
            LAND,

            // Unary
            UNARY_NOT,
            UNARY_INVERT,
            UNARY_NEGATIVE,
            LNOT,
            BNOT,

            // Load
            LOAD_BUILD_CLASS,
            LOAD_LOCALS,
            LOAD_ATTR,
            LOAD_COMMON_CONSTANT,
            LOAD_CONST,
            LOAD_DEREF,
            LOAD_FAST,
            LOAD_FAST_CHECK,
            LOAD_FAST_LOAD_FAST,
            LOAD_FAST_AND_CLEAR,
            LOAD_FAST_BORROW,
            LOAD_FROM_DICT_OR_DEREF,
            LOAD_FROM_DICT_OR_GLOBALS,
            LOAD_FAST_BORROW_LOAD_FAST_BORROW,
            LOAD_GLOBAL,
            LOAD_NAME,
            LOAD_SMALL_INT,
            LOAD_SPECIAL,
            LOAD_SUPER_ATTR,
            LOAD_ATTR_CLASS,
            LOAD_ATTR_CLASS_WITH_METACLASS_CHECK,
            LOAD_ATTR_GETATTRIBUTE_OVERRIDDEN,
            LOAD_ATTR_INSTANCE_VALUE,
            LOAD_ATTR_METHOD_LAZY_DICT,
            LOAD_ATTR_METHOD_NO_DICT,
            LOAD_ATTR_METHOD_WITH_VALUES,
            LOAD_ATTR_MODULE,
            LOAD_ATTR_NONDESCRIPTOR_NO_DICT,
            LOAD_ATTR_NONDESCRIPTOR_WITH_VALUES,
            LOAD_ATTR_PROPERTY,
            LOAD_ATTR_SLOT,
            LOAD_ATTR_WITH_HINT,
            LOAD_GLOBAL_BUILTIN,
            LOAD_GLOBAL_MODULE,
            LOAD_SUPER_ATTR_ATTR,
            LOAD_SUPER_ATTR_METHOD,
            LOADI,
            LOADD,
            LOADS,
            LOADA,
            LOADX,
            LOADXA,

            // Store
            STOREI,
            STORED,
            STORES,
            STOREA,
            STOREX,
            FUNC,
            ARG,
            PARAM,
            RETURN,
            RETURN_VALUE,

            // Delete
            DELETE_ATTR,
            DELETE_DEREF,
            DELETE_FAST,
            DELETE_GLOBAL,
            DELETE_NAME,
            DELETE_SUBSCR,

            // Call
            CALL,

            // Jump
            JUMP,
            JUMP_FORWARD,
            JUMP_IF_TRUE,
            JUMP_IF_FALSE,
            JUMP_BACKWARD,
            JUMP_NO_INTERRUPT,
            JUMP_BACKWARD_JIT,
            JUMP_BACKWARD_NO_JIT,
            JUMP_BACKWARD_NO_INTERRUPT,

            UNKNOWN = 0xff,
        };

        Instruction(Opcode k, int num_idxs)
        {
            reg_idxs.resize(num_idxs);
        };

        void setIndex(int idx, int value)
        {
            reg_idxs[idx] = value;
        };

        int getIndex(int idx) const
        {
            return reg_idxs[idx];
        };

        virtual std::string toString() = 0;
        virtual ~Instruction() = default;

    public:
        Opcode opcode;
        std::vector<uint8_t> reg_idxs;
    };

    //-----------------------------------------------------------------------------
    // [SECTION] Binary : Instruction
    //-----------------------------------------------------------------------------

    struct Binary : Instruction
    {
    public:
        Binary(Opcode opcode)
            : Instruction(opcode, 2) {};
    };

    struct Arithmetic : Binary
    {
    };

#define ARITHMETIC_INSTRUCTION(X, OP)

#undef ARITHMETIC_INSTRUCTION

    struct Cmp : Binary
    {
    };

#define COMPARISION_INSTRUCTION(X, OP)

#undef COMPARISION_INSTRUCTION

    //-----------------------------------------------------------------------------
    // [SECTION] Unary : Instruction
    //-----------------------------------------------------------------------------

    struct Unary : Instruction
    {
    public:
        Unary(Opcode opcode)
            : Instruction(opcode, 1) {};

        int reg_idx{-2};
        ArgType type;
        runtime::Value value;
        std::string name;
    };

#define UNARY_INSTRUCTION(X, OP)

#undef UNARY_INSTRUCTION

    //-----------------------------------------------------------------------------
    // [SECTION] Load : Instruction
    //-----------------------------------------------------------------------------

    struct Load : Instruction
    {
    public:
        Load(Opcode opcode)
            : Instruction(opcode, 1) {};

        int reg_idx{-1};
    };

#define LOAD_INSTRUCTION(X, OP, TYPE)
#undef LOAD_INSTRUCTION

    //-----------------------------------------------------------------------------
    // [SECTION] Store : Instruction
    //-----------------------------------------------------------------------------

    struct Store : Instruction
    {
    public:
        Store(Opcode opcode)
            : Instruction(opcode, 1) {};

        std::string name;
    };

#define STORE_INSTRUCTION(X, OP, TYPE)

#undef STORE_INSTRUCTION

    //-----------------------------------------------------------------------------
    // [SECTION] Call : Instruction
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // [SECTION] Jump : Instruction
    //-----------------------------------------------------------------------------

    //-----------------------------------------------------------------------------
    // [SECTION] Other : Instruction
    //-----------------------------------------------------------------------------

} // namespace cora::bc

#endif // CORA_BYTECODE_BCINSTRUCTION_H
