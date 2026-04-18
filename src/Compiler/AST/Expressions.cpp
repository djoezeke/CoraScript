#include "Cora/Compiler/AST/Expressions.hpp"

#include <sstream>

namespace cora::compiler
{
    namespace ast
    {
        Expression::Expression()
            : Node(NodeType::Expression), m_ExprType(ExpressionType::Null) {}

        Expression::Expression(ExpressionType kind)
            : Node(NodeType::Expression), m_ExprType(kind) {}

        ExpressionType Expression::GetExprType() const
        {
            return m_ExprType;
        }

        std::string Expression::Repr()
        {
            return "Expression";
        }

        Expression::~Expression() = default;

        LiteralExpr::LiteralExpr(LiteralValue value)
            : Expression(ExpressionType::Null), m_Value(std::move(value)) {}

        LiteralExpr::LiteralExpr(double value)
            : Expression(ExpressionType::Float), m_Value(value) {}

        LiteralExpr::LiteralExpr(bool value)
            : Expression(ExpressionType::Bool), m_Value(value) {}

        LiteralExpr::LiteralExpr(const std::string &value)
            : Expression(ExpressionType::String), m_Value(value) {}

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
            : Expression(ExpressionType::Identifier), m_Name(std::move(name)) {}

        const std::string &VariableExpr::GetName() const
        {
            return m_Name;
        }

        std::string VariableExpr::Repr()
        {
            return m_Name;
        }

        UnaryExpr::UnaryExpr(parser::TokenType op, Expression *rhs)
            : Expression(ExpressionType::PrefixUnaryExpr), m_Operator(op), m_Rhs(rhs) {}

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
            : Expression(ExpressionType::Null) {}

        Bool::Bool(bool const value)
            : Expression(ExpressionType::Bool), m_Value(value) {}

        bool Bool::GetValue() const
        {
            return m_Value;
        }

        Byte::Byte(char const value)
            : Expression(ExpressionType::Byte), m_Value(value) {}

        char Byte::GetValue() const
        {
            return m_Value;
        }

        Float::Float(double value)
            : Expression(ExpressionType::Float), m_Value(value) {}

        double Float::GetValue() const
        {
            return m_Value;
        }

        Array::Array(std::deque<Expression *> array)
            : Expression(ExpressionType::Array), m_Elements(std::move(array)) {}

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
            : Expression(ExpressionType::String), m_Value(std::move(value)) {}

        std::string String::GetValue() const
        {
            return m_Value;
        }

        Integer::Integer(long long value)
            : Expression(ExpressionType::Integer), m_Value(static_cast<int>(value)) {}

        int Integer::GetValue() const
        {
            return m_Value;
        }

        Identifier::Identifier(std::string name)
            : Expression(ExpressionType::Identifier), m_Value(std::move(name)) {}

        std::string Identifier::GetName() const
        {
            return m_Value;
        }

        TryExpr::TryExpr()
            : Expression(ExpressionType::TryExpr) {}

        TryExpr::~TryExpr() = default;

        CastExpr::CastExpr()
            : Expression(ExpressionType::CastExpr) {}

        CastExpr::~CastExpr() = default;

        CallExpr::CallExpr(Expression *callee, std::deque<Expression *> arguments)
            : Expression(ExpressionType::CallExpr), m_Callee(callee), m_Arguments(std::move(arguments)) {}

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
            : Expression(ExpressionType::MemberExpr), m_Object(object), m_Member(std::move(member)) {}

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
            : Expression(ExpressionType::NewExpr), m_ClassName(std::move(className)), m_Arguments(std::move(arguments)) {}

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
            : Expression(ExpressionType::BlockExpr) {}

        BlockExpr::~BlockExpr() = default;

        AssignExpr::AssignExpr()
            : Expression(ExpressionType::AssignExpr) {}

        AssignExpr::~AssignExpr() = default;

        BinaryExpr::BinaryExpr(Expression *left, parser::TokenType op, Expression *right)
            : Expression(ExpressionType::BinaryExpr), m_Left(left), m_Operator(op), m_Right(right) {}

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
            : Expression(ExpressionType::TernaryExpr), m_CondExpr(condExpr), m_ThenExpr(thenExpr), m_ElseExpr(elseExpr) {}

        TernaryExpr::~TernaryExpr()
        {
            delete m_CondExpr;
            delete m_ThenExpr;
            delete m_ElseExpr;
        }

        ArrayAccessExpr::ArrayAccessExpr()
            : Expression(ExpressionType::ArrayAccessExpr) {}

        ArrayAccessExpr::~ArrayAccessExpr() = default;

        PrefixUnaryExpr::PrefixUnaryExpr(parser::TokenType op, Expression *operand)
            : Expression(ExpressionType::PrefixUnaryExpr), m_Operator(op), m_Operand(operand) {}

        PrefixUnaryExpr::PrefixUnaryExpr(Expression *operand, parser::TokenType op)
            : PrefixUnaryExpr(op, operand) {}

        PrefixUnaryExpr::~PrefixUnaryExpr()
        {
            delete m_Operand;
        }

        PostfixUnaryExpr::PostfixUnaryExpr(parser::TokenType op, Expression *operand)
            : Expression(ExpressionType::PostfixUnaryExpr), m_Operator(op), m_Operand(operand) {}

        PostfixUnaryExpr::PostfixUnaryExpr(Expression *operand, parser::TokenType op)
            : PostfixUnaryExpr(op, operand) {}

        PostfixUnaryExpr::~PostfixUnaryExpr()
        {
            delete m_Operand;
        }

    } // namespace ast

} // namespace cora::compiler
