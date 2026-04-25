#ifndef CORA_IR_VMINSTRUCTION_H
#define CORA_IR_VMINSTRUCTION_H

#include <memory>
#include <sstream>
#include <string>

#include "../Runtime/Value.hpp"

namespace cora::vmachine
{
    using namespace cora::compiler;

    enum class ArgType
    {
        MAP, // 0
        RAW  // 1
    };

    enum Opcode
    {
        // binary
        ADD = 0x30,
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
        // CMP
        NE,
        EQ,
        LT,
        LE,
        GT,
        GE,
        LAND,
        // unary
        LNOT,
        BNOT,
        //
        LOADI,
        LOADD,
        LOADS,
        LOADA,
        LOADX,
        LOADXA,

        STOREI,
        STORED,
        STORES,
        STOREA,
        STOREX,
        CALL,
        FUNC,
        ARG,
        PARAM,
        RET,
        JMP, // unconditional jump
        JIF, // conditional jump, depend on `state`
             //

        UNKNOWN = 0xff,
    };

    struct Instruction
    {
        Opcode opcode{Opcode::UNKNOWN};
        virtual std::string toString() = 0;
        virtual ~Instruction() = default;
    };

    // for LOADX. STOREX
    using ArrIdx = std::variant<std::string, long long>;

    struct Load : Instruction
    {
        int reg_idx{-1};
    };

    struct LoadA : Load
    {
        LoadA()
        {
            opcode = Opcode::LOADA;
        }
        std::vector<runtime::Value> array;
        std::string toString() override
        {
            std::string str = "LOADA %" + std::to_string(reg_idx) + " [";
            for (int i = 0; i < array.size(); i++)
            {
                str += array[i].AsString() + (i != array.size() - 1 ? "," : "");
            }
            return str + "]";
        }
    };

    struct LoadX : Load
    {
        LoadX()
        {
            opcode = Opcode::LOADX;
        }
        std::string toString() override
        {
            std::string str = "LOADX %" + std::to_string(reg_idx) + " " + name;
            for (int i = 0; i < index.size(); i++)
            {
                str += "[";
                if (std::holds_alternative<long long>(index[i]))
                    str += std::to_string(std::get<long long>(index[i]));
                else
                    str += std::get<std::string>(index[i]);
                str += "]";
            }
            return str;
        }
        std::string name;
        std::vector<ArrIdx> index;
    };

    struct LoadXA : Load
    {
        LoadXA()
        {
            opcode = Opcode::LOADXA;
        }
        std::string toString() override
        {
            return "LOADXA %" + std::to_string(reg_idx) + "," + std::to_string(index) + " " + name;
        }
        std::string name;
        int index;
    };

#define LOAD_INST(X, OP, TYPE)            \
    struct X : Load                       \
    {                                     \
        X()                               \
        {                                 \
            opcode = Opcode::OP;          \
        }                                 \
        std::string toString() override   \
        {                                 \
            std::ostringstream oss;       \
            oss << #OP << " %";           \
            oss << reg_idx << " " << val; \
            return oss.str();             \
        }                                 \
        TYPE val{};                       \
    };

    LOAD_INST(LoadI, LOADI, long long)
    LOAD_INST(LoadD, LOADD, double)
    LOAD_INST(LoadS, LOADS, std::string)

#undef LOAD_INST

    struct Store : Instruction
    {
        std::string name;
    };

    struct StoreA : Store
    {
        StoreA()
        {
            opcode = Opcode::STOREA;
        }
        std::vector<ArrIdx> index;
        std::string toString() override
        {
            std::string str = "STOREA " + name;
            for (int i = 0; i < index.size(); i++)
            {
                str += "[";
                if (std::holds_alternative<long long>(index[i]))
                    str += std::to_string(std::get<long long>(index[i]));
                else
                    str += std::get<std::string>(index[i]);
                str += "] " + value.AsString();
            }
            return str;
        }
        runtime::Value value;
    };

    struct StoreX : Store
    {
        StoreX()
        {
            opcode = Opcode::STOREX;
        }
        std::string toString() override
        {
            std::string str = "STOREX " + name;
            for (int i = 0; i < index.size(); i++)
            {
                str += "[";
                if (std::holds_alternative<long long>(index[i]))
                    str += std::to_string(std::get<long long>(index[i]));
                else
                    str += std::get<std::string>(index[i]);
                str += "]";
            }
            return str + " %" + std::to_string(reg_idx);
        }
        int reg_idx{-1};
        std::vector<ArrIdx> index;
    };

#define STORE_INST(X, OP, TYPE)         \
    struct X : Store                    \
    {                                   \
        X()                             \
        {                               \
            opcode = Opcode::OP;        \
        }                               \
        std::string toString() override \
        {                               \
            std::ostringstream oss;     \
            oss << #OP << " " << name;  \
            oss << " " << val;          \
            return oss.str();           \
        }                               \
        TYPE val{};                     \
    };

    STORE_INST(StoreI, STOREI, long long)
    STORE_INST(StoreD, STORED, double)
    STORE_INST(StoreS, STORES, std::string)

#undef STORE_INST

    struct Arg : Instruction
    {
        Arg()
        {
            opcode = Opcode::ARG;
        }
        ArgType type{ArgType::RAW};
        std::string name;
        runtime::Value value;
        std::vector<ArrIdx> index;
        std::string toString() override
        {
            std::string str = type == ArgType::RAW ? "RAW " + value.AsString() : "MAP " + name;
            for (int i = 0; i < index.size(); i++)
            {
                str += "[";
                if (std::holds_alternative<long long>(index[i]))
                    str += std::to_string(std::get<long long>(index[i]));
                else
                    str += std::get<std::string>(index[i]);
                str += "]";
            }
            return str;
        }
    };

    struct Call : Instruction
    {
        Call()
        {
            opcode = Opcode::CALL;
        }
        std::string toString() override
        {
            std::string retval = "CALL " + name + "(" + std::to_string(target) + ")";
            for (auto arg : args)
            {
                retval += "\n" + arg.toString();
            }
            return retval;
        };
        std::string name;
        int target{-1};
        std::vector<Arg> args;
    };

    struct Func : Instruction
    {
        Func()
        {
            opcode = Opcode::FUNC;
        }
        std::string toString() override
        {
            return "FUNC " + name + " PARAM COUNT " + std::to_string(param_count);
        }

        std::string name;
        int param_count{0};
    };

    struct Param : Instruction
    {
        Param()
        {
            opcode = Opcode::PARAM;
        }
        std::string toString() override
        {
            return "PARAM " + name;
        }
        std::string name;
    };

    struct Ret : Instruction
    {
        Ret()
        {
            opcode = Opcode::RET;
        }
        int ret_size{0};
        std::vector<int> ret_regs;
        std::string toString() override
        {
            return "RET " + std::to_string(ret_size) + "retval";
        }
    };

    struct Jmp : Instruction
    {
        Jmp()
        {
            opcode = Opcode::JMP;
        }
        std::string basic_block_name;
        int target{-1};
        std::string toString() override
        {
            return "JMP " + std::to_string(target);
        }
    };

    struct Jif : Instruction
    {
        Jif()
        {
            opcode = Opcode::JIF;
        }
        std::string basic_block_name1;
        std::string basic_block_name2;
        int target1{-1};
        int target2{-1};
        std::string toString() override
        {
            return "JIF " + std::to_string(target1) + " " + std::to_string(target2);
        }
    };

    struct Binary : Instruction
    {
        int reg_idx1{-1};
        int reg_idx2{-1};
    };

    struct Arithmetic : Binary
    {
    };

#define ARITHMETIC_INST(X, OP)                                  \
    struct X : Arithmetic                                       \
    {                                                           \
        X()                                                     \
        {                                                       \
            opcode = Opcode::OP;                                \
        }                                                       \
        std::string toString() override                         \
        {                                                       \
            std::ostringstream oss;                             \
            oss << #OP << " %" << reg_idx1 << " %" << reg_idx2; \
            return oss.str();                                   \
        }                                                       \
    };

    ARITHMETIC_INST(Add, ADD)
    ARITHMETIC_INST(Sub, SUB)
    ARITHMETIC_INST(Mul, MUL)
    ARITHMETIC_INST(Div, DIV)
    ARITHMETIC_INST(Mod, MOD)
    ARITHMETIC_INST(Exp, EXP)
    ARITHMETIC_INST(Band, BAND)
    ARITHMETIC_INST(Bor, BOR)
    ARITHMETIC_INST(Bxor, BXOR)
    ARITHMETIC_INST(Shl, SHL)
    ARITHMETIC_INST(Shr, SHR)

#undef ARITHMETIC_INST

    struct Cmp : Binary
    {
    };

#define CMP_INST(X, OP)                          \
    struct X : Cmp                               \
    {                                            \
        X()                                      \
        {                                        \
            opcode = Opcode::OP;                 \
        }                                        \
        std::string toString() override          \
        {                                        \
            std::ostringstream oss;              \
            oss << #OP << " %";                  \
            oss << reg_idx1 << " %" << reg_idx2; \
            return oss.str();                    \
        }                                        \
    };

    CMP_INST(Lor, LOR)
    CMP_INST(Land, LAND)
    CMP_INST(Ge, GE)
    CMP_INST(Gt, GT)
    CMP_INST(Le, LE)
    CMP_INST(Lt, LT)
    CMP_INST(Ne, NE)
    CMP_INST(Eq, EQ)

#undef CMP_INST

    struct Unary : Instruction
    {
        int reg_idx{-2};
        ArgType type;
        runtime::Value value;
        std::string name;
    };

#define UNARY_INST(X, OP)               \
    struct X : Unary                    \
    {                                   \
        X()                             \
        {                               \
            opcode = Opcode::OP;        \
        }                               \
        std::string toString() override \
        {                               \
            return #OP;                 \
        }                               \
    };

    UNARY_INST(Lnot, LNOT)
    UNARY_INST(Bnot, BNOT)

#undef UNARY_INST

} // namespace cora::vmachine

#endif // CORA_IR_VMINSTRUCTION_H
