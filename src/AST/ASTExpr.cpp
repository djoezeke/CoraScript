#include "ASTExpr.hpp"

#include <sstream>

namespace cora::ast
{
    Expression::Expression()
        : Node(NodeType::Expression) {}

    Expression::Expression(NodeType kind)
        : Node(NodeType::Expression) {}

    std::string Expression::Repr()
    {
        return GetNodeTypeString();
    }

    Expression::~Expression() = default;

    LiteralExpr::LiteralExpr(LiteralValue value)
        : Expression(NodeType::Null), m_Value(std::move(value)) {}

    LiteralExpr::LiteralExpr(double value)
        : Expression(NodeType::Float), m_Value(value) {}

    LiteralExpr::LiteralExpr(bool value)
        : Expression(NodeType::Bool), m_Value(value) {}

    LiteralExpr::LiteralExpr(const std::string &value)
        : Expression(NodeType::String), m_Value(value) {}

    const LiteralValue &LiteralExpr::GetValue() const
    {
        return m_Value;
    }

    std::string LiteralExpr::Repr()
    {
        if (std::holds_alternative<std::monostate>(m_Value))
        {
            return "null";
        }
        if (std::holds_alternative<bool>(m_Value))
        {
            return std::get<bool>(m_Value) ? "true" : "false";
        }
        if (std::holds_alternative<double>(m_Value))
        {
            std::ostringstream out;
            out << std::get<double>(m_Value);
            return out.str();
        }
        return std::get<std::string>(m_Value);
    }

    VariableExpr::VariableExpr(std::string name)
        : Expression(NodeType::Identifier), m_Name(std::move(name)) {}

    const std::string &VariableExpr::GetName() const
    {
        return m_Name;
    }

    std::string VariableExpr::Repr()
    {
        return m_Name;
    }

    UnaryExpr::UnaryExpr(parser::TokenType op, Expression *rhs)
        : Expression(NodeType::PrefixUnaryExpr), m_Operator(op), m_Rhs(rhs) {}

    parser::TokenType UnaryExpr::GetOperator() const
    {
        return m_Operator;
    }

    Expression *UnaryExpr::GetRhs() const
    {
        return m_Rhs;
    }

    std::string UnaryExpr::Repr()
    {
        return "UnaryExpr";
    }

    UnaryExpr::~UnaryExpr()
    {
        delete m_Rhs;
    }

    Null::Null()
        : Expression(NodeType::Null) {}

    Bool::Bool(bool const value)
        : Expression(NodeType::Bool), m_Value(value) {}

    bool Bool::GetValue() const
    {
        return m_Value;
    }

    Byte::Byte(char const value)
        : Expression(NodeType::Byte), m_Value(value) {}

    char Byte::GetValue() const
    {
        return m_Value;
    }

    Float::Float(double value)
        : Expression(NodeType::Float), m_Value(value) {}

    double Float::GetValue() const
    {
        return m_Value;
    }

    Array::Array(std::deque<Expression *> array)
        : Expression(NodeType::Array), m_Elements(std::move(array)) {}

    std::deque<Expression *> Array::GetElements() const
    {
        return m_Elements;
    }

    Array::~Array()
    {
        for (Expression *expr : m_Elements)
        {
            delete expr;
        }
    }

    String::String(std::string value)
        : Expression(NodeType::String), m_Value(std::move(value)) {}

    std::string String::GetValue() const
    {
        return m_Value;
    }

    Integer::Integer(long long value)
        : Expression(NodeType::Integer), m_Value(static_cast<int>(value)) {}

    int Integer::GetValue() const
    {
        return m_Value;
    }

    Identifier::Identifier(std::string name)
        : Expression(NodeType::Identifier), m_Value(std::move(name)) {}

    std::string Identifier::GetName() const
    {
        return m_Value;
    }

    TryExpr::TryExpr()
        : Expression(NodeType::TryExpr) {}

    TryExpr::~TryExpr() = default;

    CastExpr::CastExpr()
        : Expression(NodeType::CastExpr) {}

    CastExpr::~CastExpr() = default;

    CallExpr::CallExpr(Expression *callee, std::deque<Expression *> arguments)
        : Expression(NodeType::CallExpr), m_Callee(callee), m_Arguments(std::move(arguments)) {}

    Expression *CallExpr::GetCallee() const
    {
        return m_Callee;
    }

    const std::deque<Expression *> &CallExpr::GetArguments() const
    {
        return m_Arguments;
    }

    std::string CallExpr::Repr()
    {
        return "CallExpr(args=" + std::to_string(m_Arguments.size()) + ")";
    }

    CallExpr::~CallExpr()
    {
        delete m_Callee;
        for (Expression *argument : m_Arguments)
        {
            delete argument;
        }
    }

    MemberExpr::MemberExpr(Expression *object, std::string member)
        : Expression(NodeType::MemberExpr), m_Object(object), m_Member(std::move(member)) {}

    Expression *MemberExpr::GetObject() const
    {
        return m_Object;
    }

    const std::string &MemberExpr::GetMember() const
    {
        return m_Member;
    }

    std::string MemberExpr::Repr()
    {
        return "MemberExpr(" + m_Member + ")";
    }

    MemberExpr::~MemberExpr()
    {
        delete m_Object;
    }

    NewExpr::NewExpr(std::string className, std::deque<Expression *> arguments)
        : Expression(NodeType::NewExpr), m_ClassName(std::move(className)), m_Arguments(std::move(arguments)) {}

    const std::string &NewExpr::GetClassName() const
    {
        return m_ClassName;
    }

    const std::deque<Expression *> &NewExpr::GetArguments() const
    {
        return m_Arguments;
    }

    std::string NewExpr::Repr()
    {
        return "NewExpr(" + m_ClassName + ")";
    }

    NewExpr::~NewExpr()
    {
        for (Expression *argument : m_Arguments)
        {
            delete argument;
        }
    }

    BlockExpr::BlockExpr()
        : Expression(NodeType::BlockExpr) {}

    BlockExpr::~BlockExpr() = default;

    AssignExpr::AssignExpr()
        : Expression(NodeType::AssignExpr) {}

    AssignExpr::~AssignExpr() = default;

    BinaryExpr::BinaryExpr(Expression *left, parser::TokenType op, Expression *right)
        : Expression(NodeType::BinaryExpr), m_Left(left), m_Operator(op), m_Right(right) {}

    Expression *BinaryExpr::GetLeft() const
    {
        return m_Left;
    }

    parser::TokenType BinaryExpr::GetOperator() const
    {
        return m_Operator;
    }

    Expression *BinaryExpr::GetRight() const
    {
        return m_Right;
    }

    std::string BinaryExpr::Repr()
    {
        return "BinaryExpr";
    }

    BinaryExpr::~BinaryExpr()
    {
        delete m_Left;
        delete m_Right;
    }

    TernaryExpr::TernaryExpr(Expression *condExpr, Expression *thenExpr, Expression *elseExpr)
        : Expression(NodeType::TernaryExpr), m_CondExpr(condExpr), m_ThenExpr(thenExpr), m_ElseExpr(elseExpr) {}

    TernaryExpr::~TernaryExpr()
    {
        delete m_CondExpr;
        delete m_ThenExpr;
        delete m_ElseExpr;
    }

    ArrayAccessExpr::ArrayAccessExpr()
        : Expression(NodeType::ArrayAccessExpr) {}

    ArrayAccessExpr::~ArrayAccessExpr() = default;

    PrefixUnaryExpr::PrefixUnaryExpr(parser::TokenType op, Expression *operand)
        : Expression(NodeType::PrefixUnaryExpr), m_Operator(op), m_Operand(operand) {}

    PrefixUnaryExpr::PrefixUnaryExpr(Expression *operand, parser::TokenType op)
        : PrefixUnaryExpr(op, operand) {}

    PrefixUnaryExpr::~PrefixUnaryExpr()
    {
        delete m_Operand;
    }

    PostfixUnaryExpr::PostfixUnaryExpr(parser::TokenType op, Expression *operand)
        : Expression(NodeType::PostfixUnaryExpr), m_Operator(op), m_Operand(operand) {}

    PostfixUnaryExpr::PostfixUnaryExpr(Expression *operand, parser::TokenType op)
        : PostfixUnaryExpr(op, operand) {}

    PostfixUnaryExpr::~PostfixUnaryExpr()
    {
        delete m_Operand;
    }

} // namespace cora::ast
