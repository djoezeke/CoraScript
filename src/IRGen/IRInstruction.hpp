#ifndef CORA_IR_IRINSTRUCTION_H
#define CORA_IR_IRINSTRUCTION_H

/**
 * SSA IR (Intermediate Representation) Format.
 */

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace cora::ir
{
    struct Instruction;

    struct Value
    {
        std::string name;
        std::vector<Instruction *> users;

        virtual ~Value() = default;

        void addUse(Instruction *user)
        {
            if (user != nullptr)
            {
                users.push_back(user);
            }
        }

        void removeUse(Instruction *user)
        {
            users.erase(std::remove(users.begin(), users.end(), user), users.end());
        }
    };

    struct Instruction : public Value
    {
        enum OpKind
        {
            CONST,
            LOAD,
            ADD,
            MUL,
            SUB,
            DIV,
            PHI,
            STORE,
            PRINT,
            RETURN,
            BRANCH
        };

        OpKind op{CONST};
        Value *result{nullptr};
        std::vector<Value *> operands;
        bool hasSideEffects{false};

        Instruction() = default;

        Instruction(OpKind kind, std::string instructionName, bool sideEffect = false)
            : op(kind), result(nullptr), hasSideEffects(sideEffect)
        {
            name = std::move(instructionName);
        }

        void addOperand(Value *value)
        {
            if (value == nullptr)
            {
                return;
            }

            operands.push_back(value);
            value->addUse(this);
        }
    };

    struct PhiInstruction : public Instruction
    {
        explicit PhiInstruction(std::string name)
            : Instruction(PHI, std::move(name)) {}
    };

    struct BasicBlock
    {
        std::string label;
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
        }

        void addPhi(PhiInstruction *phi)
        {
            if (phi != nullptr)
            {
                phis.push_back(phi);
            }
        }
    };

    using ValueKey = std::tuple<Instruction::OpKind, Value *, Value *>;

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

    struct ConstantInt : public Value
    {
        int value{0};

        explicit ConstantInt(int val)
            : value(val)
        {
            name = std::to_string(val);
        }
    };

    inline bool isConstant(Value *v)
    {
        return dynamic_cast<ConstantInt *>(v) != nullptr;
    }

    inline std::string opToString(Instruction::OpKind op)
    {
        switch (op)
        {
        case Instruction::CONST:
            return "const";
        case Instruction::LOAD:
            return "load";
        case Instruction::ADD:
            return "add";
        case Instruction::MUL:
            return "mul";
        case Instruction::SUB:
            return "sub";
        case Instruction::DIV:
            return "div";
        case Instruction::PHI:
            return "phi";
        case Instruction::STORE:
            return "store";
        case Instruction::PRINT:
            return "print";
        case Instruction::RETURN:
            return "return";
        case Instruction::BRANCH:
            return "branch";
        }

        return "unknown";
    }

} // namespace cora::ir

#endif // CORA_IR_IRINSTRUCTION_H
