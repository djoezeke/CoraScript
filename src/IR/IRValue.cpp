#include "IRValue.hpp"
#include "IRInstruction.hpp"

namespace cora::ir
{

    using namespace cora::compiler;

    //-----------------------------------------------------------------------------
    // [Class] Type
    //-----------------------------------------------------------------------------

    Type::Type(ID i) : id(i) {};

    Type *Type::Int() { return new Type(ID::Int); };
    Type *Type::Void() { return new Type(ID::Void); };
    Type *Type::Float() { return new Type(ID::Float); };
    Type *Type::Label() { return new Type(ID::Label); };
    Type *Type::Pointer(Type *element)
    {
        Type *t = new Type(ID::Pointer);
        t->pointee = element;
        return t;
    };

    std::string Type::toString()
    {
        // switch (id)
        // {
        // case ID::Int:
        //     return std::string("int");
        // case ID::Void:
        //     return std::string("void");
        // case ID::Float:
        //     return std::string("float");
        // case ID::Label:
        //     return std::string("label");
        // case ID::Pointer:
        //     return pointee->toString() + std::string("*");
        // default:
        //     return std::string("void");
        // }
        return std::string("void");
    }

    bool Type::is(Type type) const
    {
        return id == type.id;
    };
    bool Type::isInt() const { return id == ID::Int; };
    bool Type::isVoid() const { return id == ID::Void; };
    bool Type::isFloat() const { return id == ID::Float; };
    bool Type::isLabel() const { return id == ID::Label; };
    bool Type::isPointer() const { return id == ID::Pointer; };

    bool Type::operator==(Type *other) const
    {
        if (id != other->id)
            return false;
        if (id == ID::Pointer)
            return pointee == other->pointee;
        return true;
    };

    bool Type::operator!=(Type *other) const
    {
        return !((this) == other);
    };

    bool Type::operator==(const Type &other) const
    {
        if (id != other.id)
            return false;
        if (id == ID::Pointer)
            return pointee == other.pointee;
        return true;
    };

    bool Type::operator!=(const Type &other) const
    {
        return !((*this) == other);
    };

    //-----------------------------------------------------------------------------
    // [Class] Use
    //-----------------------------------------------------------------------------

    void Use::set(Value *value, User *use)
    {
        if (value)
        {
            // TODO: handle removing from old list if necessary.
        }

        this->value = value;
        this->user = use;
        if (this->value)
            this->value->addUse(*this);
    };

    //-----------------------------------------------------------------------------
    // [Class] User
    //-----------------------------------------------------------------------------

    User::User(Kind k, std::string name, int num_ops)
        : Value(k, name)
    {
        operands.resize(num_ops);
        for (auto &use : operands)
            use.user = this;
    };

    void User::setOperand(int i, Value *value)
    {
        operands[i].set(value, this);
    };

    Value *User::getOperand(int i) const
    {
        return operands[i].value;
    };

    std::string User::toString()
    {
        return std::string("");
    };

    //-----------------------------------------------------------------------------
    // [Class] Value
    //-----------------------------------------------------------------------------

    Value::Value(Kind k, std::string name)
        : kind(k), name(name), type(nullptr) {};

    Value::Value(Kind k, std::string name, Type *type)
        : kind(k), name(name), type(type) {};

    void Value::addUse(Use &use)
    {
        use.next = use_list;
        if (use_list)
            use_list->prev = &use;
        use_list = &use;
    };

    void Value::killUse(Use &use) {
    };

    // "Replace All Uses With"
    void Value::RAUW(Value *new_value) {};

    std::string Value::toString()
    {
        return std::string(type->toString() + " " + name);
    };

    Value::~Value() = default;

    //-----------------------------------------------------------------------------
    // [Class] ConstantValue
    //-----------------------------------------------------------------------------

    Constant::Constant(runtime::value value, std::string name)
        : Value(Kind::Constant, std::move(name)), value(std::move(value)) {};

    std::string Constant::toString()
    {
        return std::string(type->toString() + " " + value.toString());
    };

    //-----------------------------------------------------------------------------
    // [Class] BasicBlock
    //-----------------------------------------------------------------------------

    BasicBlock::BasicBlock(std::string name)
        : Value(Kind::BasicBlock, name) {};

    void BasicBlock::edge(BasicBlock *from, BasicBlock *to)
    {
        from->succs.push_back(to);
        to->preds.push_back(from);
    };

    void BasicBlock::addSuccessor(BasicBlock *block)
    {
        if (block != nullptr)
        {
            succs.push_back(block);
        }
    };

    void BasicBlock::addPredecessor(BasicBlock *block)
    {
        if (block != nullptr)
        {
            preds.push_back(block);
        }
    };

    void BasicBlock::addInstruction(Instruction *inst)
    {
        if (inst != nullptr)
        {
            insts.push_back(inst);
        }
    };

    void BasicBlock::addPhiInstruction(PhiInstruction *phi)
    {
        if (phi != nullptr)
        {
            phis.push_back(phi);
        }
    };

    std::string BasicBlock::toString()
    {
        std::ostringstream out;

        out << name << ":\n";

        for (Instruction *inst : insts)
        {
            if (inst == nullptr)
            {
                continue;
            }
            out << "  ";

            out << inst->toString();
        }

        out << "\n}";

        return out.str();
    };

    //-----------------------------------------------------------------------------
    // [Class] Argument
    //-----------------------------------------------------------------------------

    Argument::Argument(Type *t, Function *f, unsigned num)
        : Value(Kind::Argument, std::to_string(num), t), parent(f), argNo(num) {};

    std::string Argument::toString()
    {
        return std::string(type->toString() + " " + name);
    };

    //-----------------------------------------------------------------------------
    // [Class] Function
    //-----------------------------------------------------------------------------

    Function::Function(std::string name, Type *retTy, std::vector<Type *> argTypes, BasicBlock *body)
        : Value(Kind::Constant, name, retTy), body(body)
    {
        for (size_t i = 0; i < argTypes.size(); ++i)
        {
            args.push_back(new Argument(argTypes[i], this, i));
        }
    };

    std::string Function::toString()
    {
        std::ostringstream out;

        out << "func " << name << "(";

        for (size_t i = 1; i < args.size(); ++i)
        {
            out << args[i]->toString();
            out << (i == args.size() - 1 ? "" : ", ");
        }

        out << ") -> " << type->toString() << "{\n";

        out << body->toString();

        out << "\n}";

        return out.str();
    };

} // namespace cora::ir
