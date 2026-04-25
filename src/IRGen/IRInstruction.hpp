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

    enum IROpcode : char
    {
        CONST,  // constant included above
        BINARY, // binary expr => a+b
        RETURN, // return
        JMP,
        CALL,   // function call
        VAR,    // variable
        FUNC,   // function decl
        PARAM,  // function params
        BRANCH, // if then else.
        ASSIGN, // a = binary / constant / call / var / vardef
        PHI,    // phi node
        ARRAY,  // [1,a,[a,2]]
        INDEX,  // a[1], a[f()][a]

        /* Constants. */

        /* Bit Operations. */
        CONST,
        LOAD,

        /* Arithmetic Operations. */
        ADD,
        MUL,
        SUB,
        DIV,
        MOD,
        POW,
        NEG,

        ABS,
        LDEXP,
        MIN,
        MAX,
        FPMATH,

        /* Loads and Stores. */

        /* Memory Operations. */

        /* Buffer Operations. */

        /* Memory Allocations. */

        /* Type Conversions. */

        /* Calls Operations */

        /* Miscellaneous Operations. */
        NOP,
        BASE,
        PVAL,
        GCSTEP,
        HIOP,
        LOOP,
        USE,
        PHI,
        RENAME,
        PROF,

        PHI,
        STORE,
        PRINT,
        RETURN,
        BRANCH,
        INVALID,
    };

    struct Instruction;

    struct BasicBlock
    {
        std::string name;
        std::vector<PhiInstruction *> phis;
        std::vector<Instruction *> instructions;
        std::vector<BasicBlock *> predecessors;
        std::vector<BasicBlock *> successors;

        ~BasicBlock() = default;

        void addInstruction(Instruction *inst)
        {
            if (inst != nullptr)
            {
                instructions.push_back(inst);
            }
        };

        void addPhiInstruction(PhiInstruction *phi)
        {
            if (phi != nullptr)
            {
                phis.push_back(phi);
            }
        };

        void addPredecessor(BasicBlock *block)
        {
            if (block != nullptr)
            {
                predecessors.push_back(block);
            }
        };

        void addSuccessor(BasicBlock *block)
        {
            if (block != nullptr)
            {
                successors.push_back(block);
            }
        };
    };

    struct IRepresentation
    {
        enum OpKind
        {
            CONST,  // constant included above
            BINARY, // binary expr => a+b
            RETURN, // return
            JMP,
            CALL,   // function call
            VAR,    // variable
            FUNC,   // function decl
            PARAM,  // function params
            BRANCH, // if then else.
            ASSIGN, // a = binary / constant / call / var / vardef
            PHI,    // phi node
            ARRAY,  // [1,a,[a,2]]
            INDEX,  // a[1], a[f()][a]
        };

    public:
        explicit IRepresentation(OpKind opkind)
            : opkind(opkind) {};

        IRepresentation() = default;

        virtual ~IRepresentation() = default;
        virtual std::string toString() = 0;

        OpKind opkind{CONST};
    };

    struct Instruction : public IRepresentation
    {
        explicit Instruction(OpKind opkind)
            : IRepresentation(opkind) {};

        BasicBlock *block{nullptr};
        int id{-1}; // instruction ID
    };

    // IR conversion
    template <typename T, IRepresentation::OpKind Kind>
    static inline T *as(IRepresentation *inst)
    {
        if (inst == nullptr)
            return nullptr;
        if (inst->opkind == Kind)
            return static_cast<T *>(inst);
        return nullptr;
    };

    template <typename T, typename... U>
    static inline bool inOr(T lhs, U... args)
    {
        return ((lhs == args) || ...);
    }

    struct IRValue : public IRepresentation
    {
        explicit IRValue(OpKind opkind)
            : IRepresentation(opkind) {};

        Instruction *instruction{nullptr};
    };

    struct ConstInstruction : public IRValue
    {
        explicit ConstInstruction(std::string name)
            : IRValue(CONST) {};

        std::string toString() override {
        };

        runtime::Value value;
    };

    class BinaryInstruction : public IRValue
    {
    public:
        using IRValue::IRValue;
        BinaryInstruction()
            : IRValue(BINARY) {};

        std::string toString() override
        {
            std::string retval;
            if (lhs != nullptr)
                retval += lhs->toString();
            // retval += " " + opcode + " ";
            if (rhs != nullptr)
                retval += rhs->toString();
            return retval;
        }

    public:
        // IROpcode opcode{IROpcode::IR_INVALID};
        IRValue *lhs{nullptr};
        IRValue *rhs{nullptr};
    };

    class ReturnInstruction : public Instruction
    {
    public:
        using Instruction::Instruction;
        ReturnInstruction()
            : Instruction(RETURN) {};

        std::string toString() override
        {
            return "RETURN " + (ret ? ret->toString() : "");
        }

    public:
        IRValue *ret{nullptr};
    };

    class JumpInstruction : public Instruction
    {
    public:
        using Instruction::Instruction;
        JumpInstruction()
            : Instruction(JMP) {};

        std::string toString() override
        {
            return (id >= 0 ? std::to_string(id) + " " : "") + "jmp " + target->name;
        }

    public:
        BasicBlock *target{nullptr};
    };

    class FunctionInstruction : public IRepresentation
    {
    public:
        using IRepresentation::IRepresentation;
        FunctionInstruction()
            : IRepresentation(FUNC) {};

        std::string toString() override
        {
            return "@" + name + "";
        }

    public:
        std::string name;
        std::vector<VarInstruction *> params;
        std::list<BasicBlock *> blocks;
    };

    class CallInstruction : public Instruction
    {
    public:
        using Instruction::Instruction;
        CallInstruction()
            : Instruction(CALL) {};

        std::string toString() override
        {
            std::string retval = (func ? func->toString() : name) + "(";
            for (auto *arg : args)
            {
                retval += arg->toString() + ", ";
            }
            return retval + ")";
        }

    public:
        std::string name;
        FunctionInstruction *func{nullptr};
        std::vector<IRepresentation *> args;
    };

    class ArrayInstruction : public IRValue
    {
    public:
        using IRValue::IRValue;
        ArrayInstruction()
            : IRValue(ARRAY) {};

        // [VarInstruction,ArrayInstruction,CallInstruction,ConstInstruction]
        std::vector<IRepresentation *> content;
        std::string toString() override
        {
            std::string str = "[";
            for (auto *x : content)
            {
                str += x->toString() + ", ";
            }
            return str + "]";
        }
    };

    class VarInstruction : public IRValue
    {
    public:
        using IRValue::IRValue;
        VarInstruction()
            : IRValue(VAR) {};

        std::string ssaName()
        {
            return is_ir_gen ? name : name + std::to_string(ssa_index);
        };

        std::string toString() override
        {
            std::string str = name;
            if (is_array)
            {
                for (auto x : index)
                {
                    str += "[" + x->toString() + "]";
                }
            }
            else
            {
                str += (is_ir_gen ? "" : std::to_string(ssa_index));
            }
            return str;
        }

        void addUse(VarInstruction *value)
        {
            if (std::find(use.begin(), use.end(), value) == use.end())
            {
                use.push_back(value);
            }
        }
        void killUse(VarInstruction *value)
        {
            use.remove(value);
        }
        bool operator<(const VarInstruction &var) const
        {
            return false;
        }

    public:
        bool is_array{false};
        std::vector<IRepresentation *> index;
        //
        bool is_ir_gen{false};
        std::string name;
        int ssa_index{0};
        VarInstruction *def{nullptr};
        std::list<VarInstruction *> use;
    };

    class ParamsInstruction : public Instruction
    {
    public:
        using Instruction::Instruction;
        ParamsInstruction()
            : Instruction(PARAM) {};

        std::vector<std::string> params;

    public:
        std::string toString() override
        {
            std::string retval = "PARAMS ";
            for (const auto &param : params)
            {
                retval += param + " ";
            }
            return retval;
        }
    };

    class PhiInstruction : public IRValue
    {
    public:
        using IRValue::IRValue;
        PhiInstruction()
            : IRValue(PHI) {};

        std::string toString() override
        {
            std::string str = "phi(";
            for (const auto &arg : args)
            {
                str += arg->toString() + " ";
            }
            return str + ")";
        }
        std::vector<IRValue *> args;
    };

    class AssignInstruction : public Instruction
    {
    public:
        using Instruction::Instruction;
        AssignInstruction()
            : Instruction(ASSIGN) {};

        void setDest(VarInstruction *dest)
        {
            this->_dest = dest;
            dest->instruction = this;
        };

        void setSrc(IRepresentation *src)
        {
            if (inOr(src->opkind, IRepresentation::OpKind::BINARY, IRepresentation::OpKind::VAR, IRepresentation::OpKind::PHI))
            {
                auto *binary = as<BinaryInstruction, IRepresentation::OpKind::BINARY>(src);
                auto *var = as<VarInstruction, IRepresentation::OpKind::VAR>(src);
                auto *phi = as<PhiInstruction, IRepresentation::OpKind::PHI>(src);
                if (binary != nullptr)
                {
                    binary->instruction = this;
                    auto *lhs_const = as<ConstInstruction, IRepresentation::OpKind::CONST>(binary->lhs);
                    auto *lhs_var = as<VarInstruction, IRepresentation::OpKind::VAR>(binary->lhs);
                    auto *rhs_const = as<ConstInstruction, IRepresentation::OpKind::CONST>(binary->rhs);
                    auto *rhs_var = as<VarInstruction, IRepresentation::OpKind::VAR>(binary->rhs);
                    if (lhs_const != nullptr)
                        lhs_const->instruction = this;
                    if (lhs_var != nullptr)
                        lhs_var->instruction = this;
                    if (rhs_const != nullptr)
                        rhs_const->instruction = this;
                    if (rhs_var != nullptr)
                        rhs_var->instruction = this;
                }
                else if (var != nullptr)
                {
                    var->instruction = this;
                }
                else if (phi != nullptr)
                {
                    phi->instruction = this;
                    for (auto *v : phi->args)
                    {
                        v->instruction = this;
                    }
                }
            }
            _src = src;
        }

        VarInstruction *dest()
        {
            return _dest;
        }

        IRepresentation *src()
        {
            return _src;
        }

        std::string toString() override
        {
            std::string retval = (id >= 0 ? std::to_string(id) + " " : "");
            if (_dest != nullptr)
                retval += _dest->toString();
            if (_src != nullptr)
                retval += " = " + _src->toString();
            return retval;
        }

    private:
        VarInstruction *_dest{nullptr}; // a.k.a lhs
        IRepresentation *_src{nullptr}; // a.k.a rhs
    };

    class BranchInstruction : public Instruction
    {
    public:
        using Instruction::Instruction;
        BranchInstruction()
            : Instruction(BRANCH) {};

        VarInstruction *cond{nullptr};
        BasicBlock *true_block{nullptr};
        BasicBlock *false_block{nullptr};

        std::string toString() override
        {
            std::string retval = (id >= 0 ? std::to_string(id) + " " : "") + "if ";
            if (cond)
                retval += cond->toString() + " ";
            if (true_block != nullptr)
                retval += "then goto " + true_block->name + " ";
            if (false_block != nullptr)
                retval += "else goto " + false_block->name + " ";
            return retval;
        }
    };

    static bool forceRemoveVar(VarInstruction *var)
    {
        if (var == nullptr)
            return false;
        if (!var->use.empty())
        {
            // if this is vardef. set it uses's def to nullptr.
            for (auto x : var->use)
            {
                x->def = nullptr;
            }
        }
        if (var->def != nullptr)
        {
            var->def->killUse(var);
        }
        delete var;
        return true;
    };

    using ValueKey = std::tuple<Instruction::OpKind, IRValue *, IRValue *>;

    struct KeyHasher
    {
        std::size_t operator()(const ValueKey &k) const
        {
            const auto opHash = static_cast<std::size_t>(std::get<0>(k));
            const auto lhsHash = reinterpret_cast<std::size_t>(std::get<1>(k));
            const auto rhsHash = reinterpret_cast<std::size_t>(std::get<2>(k));
            return opHash ^ (lhsHash << 1U) ^ (rhsHash << 2U);
        }
    };

    inline std::string opToString(Instruction::OpKind opkind)
    {
        switch (opkind)
        {
        case Instruction::CONST:
            return "const";
            return "div";
        case Instruction::PHI:
            return "phi";
        case Instruction::RETURN:
            return "return";
        case Instruction::BRANCH:
            return "branch";
        }
        return "unknown";
    }

} // namespace cora::ir

#endif // CORA_IR_IRINSTRUCTION_H
