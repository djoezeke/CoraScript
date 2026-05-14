#include "IRInstruction.hpp"
#include "IRValue.hpp"

namespace cora::ir
{

    using namespace cora::compiler;

    //-----------------------------------------------------------------------------
    // [Class] Instruction
    //-----------------------------------------------------------------------------

    Instruction::Instruction(Opcode op, std::string name, BasicBlock *block, int num_ops)
        : User(Value::Kind::Instruction, name, num_ops), opcode(op), parent(block)
    {
        parent->insts.push_back(this);
    };

    std::string Instruction::opcodeString()
    {
        switch (opcode)
        {
        case Opcode::Add:
            return std::string("add");
        case Opcode::Import:
            return std::string("import");
        default:
            std::ostringstream out;
            out << std::string("(opcode ") << std::to_string(static_cast<uint8_t>(opcode)) << ")\n";
            return out.str();
        }
    };

    std::string Instruction::toString() { return opcodeString(); };

    //-----------------------------------------------------------------------------
    // [Class] PhiInstruction
    //-----------------------------------------------------------------------------

    PhiInstruction::PhiInstruction(std::string name, BasicBlock *block)
        : Instruction(Opcode::Phi, name, block, 0) {
              // Phi grows dynamically
          };

    void PhiInstruction::addIncoming(Value *value, BasicBlock *block)
    {
        Use use;
        use.set(value, this);
        operands.push_back(use);
        // Additional logic to track which block matches which operand
    };

    std::string PhiInstruction::toString() { return std::string(""); };

    //-----------------------------------------------------------------------------
    // [Class] BinaryInstruction
    //-----------------------------------------------------------------------------

    BinaryInstruction::BinaryInstruction(Opcode op, Value *lhs, Value *rhs, std::string name, BasicBlock *block)
        : Instruction(op, name, block, 2)
    {
        setOperand(0, lhs);
        setOperand(1, rhs);
    };

    std::string BinaryInstruction::toString()
    {
        std::ostringstream out;
        out << opcodeString() << " " << type->toString() << " ";
        out << getOperand(0)->toString() << ", " << getOperand(0)->toString() << "\n";
        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] UnaryInstruction
    //-----------------------------------------------------------------------------

    UnaryInstruction::UnaryInstruction(Opcode op, Value *value, std::string name, BasicBlock *block)
        : Instruction(op, name, block, 1)
    {
        setOperand(0, value);
    };

    std::string UnaryInstruction::toString()
    {
        std::ostringstream out;
        out << opcodeString() << " " << type->toString() << " ";
        out << getOperand(0)->toString() << ", " << "\n";
        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] LoadInstruction
    //-----------------------------------------------------------------------------

    LoadInstruction::LoadInstruction(Value *ptr, std::string name, BasicBlock *block)
        : Instruction(Opcode::Load, name, block, 1)
    {
        setOperand(0, ptr); // The source address
    };

    std::string LoadInstruction::toString()
    {
        std::ostringstream out;

        out << "load " << type->toString() << "\n";

        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] StoreInstruction
    //-----------------------------------------------------------------------------

    StoreInstruction::StoreInstruction(Value *value, Value *ptr, BasicBlock *block)
        : Instruction(Opcode::Store, "", block, 2)
    {
        setOperand(0, value); // Value to store
        setOperand(1, ptr);   // Destination address
    };

    std::string StoreInstruction::toString()
    {
        std::ostringstream out;

        out << "store " << type->toString() << " ";
        out << getOperand(0)->toString() << ", ";
        out << getOperand(1)->toString() << "\n";

        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] CallInstruction
    //-----------------------------------------------------------------------------

    CallInstruction::CallInstruction(Value *func, std::vector<Value *> args, std::string name, BasicBlock *block)
        : Instruction(Opcode::Call, name, block, args.size() + 1)
    {
        setOperand(0, func); // First operand is usually the function pointer
        for (size_t i = 0; i < args.size(); ++i)
        {
            setOperand(i + 1, args[i]);
        }
    };

    std::string CallInstruction::toString()
    {
        std::ostringstream out;

        out << "call " << type->toString() << " @";
        out << getOperand(0)->toString() << "(";

        for (size_t i = 1; i < operands.size(); ++i)
        {
            out << getOperand(static_cast<int>(i))->toString();
            out << (i == operands.size() - 1 ? "" : ", ");
        }
        out << ")\n";

        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] BranchInstruction
    //-----------------------------------------------------------------------------

    // Unconditional
    BranchInstruction::BranchInstruction(BasicBlock *dest, BasicBlock *block)
        : Instruction(Opcode::Br, "", block, 1), is_conditional(false)
    {
        setOperand(0, dest);
        block->addSuccessor(dest);
        dest->addPredecessor(block);
    };

    // Conditional
    BranchInstruction::BranchInstruction::BranchInstruction(Value *cond, BasicBlock *ifTrue, BasicBlock *ifFalse, BasicBlock *block)
        : Instruction(Opcode::Br, "", block, 3), is_conditional(true)
    {
        setOperand(0, cond);
        setOperand(1, ifTrue);
        setOperand(2, ifFalse);
        block->addSuccessor(ifTrue);
        block->addSuccessor(ifFalse);
        ifTrue->addPredecessor(block);
        ifFalse->addPredecessor(block);
    };

    std::string BranchInstruction::toString()
    {
        std::ostringstream out;

        if (is_conditional)
        {
            out << "br " << getOperand(0)->toString() << getOperand(1)->toString() << this->type->toString() << getOperand(2)->toString() << "\n";
        }
        else
        {
            out << "br " << this->type->toString() << " " << getOperand(0)->toString() << "\n";
        }

        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] ReturnInstruction
    //-----------------------------------------------------------------------------

    ReturnInstruction::ReturnInstruction(BasicBlock *block, Value *value)
        : Instruction(Opcode::Ret, "ret", block, value ? 1 : 0)
    {
        if (value)
        {
            setOperand(0, value);
            this->type = value->type;
        }
        else
        {
            this->type = Type::Void();
        }
    };

    std::string ReturnInstruction::toString()
    {
        std::ostringstream out;

        out << name << " " << type->toString();

        // if (getOperand(0) != nullptr)
        // {
        //     out << " " << getOperand(0)->toString();
        // }

        out << "\n";

        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] JumpInstruction
    //-----------------------------------------------------------------------------

    JumpInstruction::JumpInstruction(BasicBlock *dest, BasicBlock *block)
        : Instruction(Opcode::Jump, "", block, 1)
    {
        setOperand(0, dest);

        // Link CFG
        block->addSuccessor(dest);
        dest->addPredecessor(block);
        this->type = Type::Void();
    };

    std::string JumpInstruction::toString()
    {
        return std::string("jump");
    };

    //-----------------------------------------------------------------------------
    // [Class] AllocaInstruction
    //-----------------------------------------------------------------------------

    AllocaInstruction::AllocaInstruction(Type *ty, std::string name, BasicBlock *block)
        : Instruction(Opcode::Alloca, name, block, 0)
    {
        // Result is a pointer to the type
        this->type = Type::Pointer(ty);
    };

    std::string AllocaInstruction::toString()
    {
        std::ostringstream out;

        out << "%" << name << " =  alloca ";
        out << type->toString() << "\n";

        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] ImportInstruction
    //-----------------------------------------------------------------------------

    ImportInstruction::ImportInstruction(Value *moduleName, BasicBlock *block)
        : Instruction(Opcode::Import, "", block, 1)
    {
        setOperand(0, moduleName);
        this->type = Type::Void();
    };

    std::string ImportInstruction::toString()
    {
        return "import " + getOperand(0)->toString() + "\n";
    }

} // namespace cora::ir
