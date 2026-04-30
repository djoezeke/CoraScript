#ifndef CORA_AST_ASTEXPR_H
#define CORA_AST_ASTEXPR_H

#include "../Parser/Token.hpp"
#include "ASTNode.hpp"

#include <string>
#include <variant>
#include <vector>

namespace cora::ast
{

    using LiteralValue = std::variant<std::monostate, bool, double, std::string>;

    struct Expression : public Node
    {
    public:
        Expression()
            : Node(NodeType::Expression) {};

        Expression(NodeType kind)
            : Node(kind) {};

        std::string Repr() override;
        ~Expression() override;
    };

    struct NullExpr : public Expression
    {
    public:
        NullExpr()
            : Expression() {};

        std::string Repr() override;
    };

    struct BoolExpr : public Expression
    {
    public:
        BoolExpr(bool const value)
            : Expression(), value(value) {};

        std::string Repr() override;

    public:
        bool value;
    };

    struct ByteExpr : public Expression
    {
    public:
        ByteExpr(char const value)
            : Expression(), value(value) {};

        std::string Repr() override;

    public:
        char value;
    };

    struct FloatExpr : public Expression
    {
    public:
        FloatExpr(double value)
            : Expression(), value(value) {};

        std::string Repr() override;

    public:
        double value;
    };

    struct ArrayExpr : public Expression
    {
    public:
        ArrayExpr(std::vector<Statement *> value)
            : Expression(), value(value) {};

        ~ArrayExpr() override;

    public:
        std::vector<Statement *> value;
    };

    struct StringExpr : public Expression
    {
    public:
        StringExpr(std::string value)
            : Expression(), value(value) {};

    public:
        std::string value;
    };

    struct IntegerExpr : public Expression
    {
    public:
        IntegerExpr(int value)
            : Expression(), value(value) {};

    public:
        int value;
    };

    struct IdentifierExpr : public Expression
    {
    public:
        IdentifierExpr(std::string name)
            : Expression(), name(name) {};

        std::string Repr() override;

    public:
        std::string name;
    };

    struct ArrayIdExpr : public Expression
    {
    public:
        ArrayIdExpr(std::string name, std::vector<Statement *> value)
            : Expression(), name(name), value(value) {};

        ~ArrayIdExpr() override;

    public:
        std::string name;
        std::vector<Statement *> value;
    };

    class ParamExpr : public Expression
    {
    public:
        ParamExpr(IdentifierExpr *name, Expression *type)
            : Expression(), name(name), type(type) {};

        ~ParamExpr() override;

    public:
        IdentifierExpr *name;
        Expression *type;
    };

    struct UnaryExpr : public Expression
    {
    public:
        UnaryExpr(parser::TokenType op, Expression *expr)
            : Expression(), op(op), expr(expr) {};

        std::string Repr() override;
        ~UnaryExpr() override;

    public:
        parser::TokenType op;
        Expression *expr;
    };

    struct PrefixUnaryExpr : public UnaryExpr
    {
    public:
        PrefixUnaryExpr(parser::TokenType op, Expression *expr)
            : UnaryExpr(op, expr) { node_type = NodeType::PrefixUnaryExpr; };
    };

    struct PostfixUnaryExpr : public UnaryExpr
    {
    public:
        PostfixUnaryExpr(parser::TokenType op, Expression *rhs)
            : UnaryExpr(op, expr) { node_type = NodeType::PostfixUnaryExpr; };
    };

    struct BinaryExpr : public Expression
    {
    public:
        BinaryExpr(Expression *left, parser::TokenType op, Expression *right)
            : Expression(), left(left), op(op), right(right) {};

        std::string Repr() override;
        ~BinaryExpr() override;

    public:
        Expression *left;
        parser::TokenType op;
        Expression *right;
    };

    struct AssignExpr : public BinaryExpr
    {
    public:
        AssignExpr(Expression *left, Expression *right)
            : BinaryExpr(left, parser::TokenType::Equal, right) { node_type = NodeType::AssignExpr; };
    };

    struct FuncCallExpr : public Expression
    {
    public:
        FuncCallExpr(IdentifierExpr *name, std::vector<Statement *> args)
            : Expression(), name(name), args(args) {};

        std::string Repr() override;
        ~FuncCallExpr() override;

    public:
        IdentifierExpr *name;
        std::vector<Statement *> args;
    };

    struct TernaryExpr : public Expression
    {
    public:
        TernaryExpr(Expression *condExpr, Expression *thenExpr, Expression *elseExpr);
        ~TernaryExpr() override;

    public:
        Expression *cond;
        Expression *then;
        Expression *_else;
    };

} // namespace cora::ast

#endif // CORA_AST_ASTEXPR_H
