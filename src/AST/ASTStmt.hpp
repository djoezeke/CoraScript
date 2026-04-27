#ifndef CORA_AST_STATEMENTS_H
#define CORA_AST_STATEMENTS_H

#include "ASTExpr.hpp"

#include <deque>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cora::ast
{
    struct Statement : public Node
    {
    public:
        Statement();
        Statement(NodeType kind);
        std::string Repr() override;
        ~Statement() override;
    };

    struct ExprStmt : public Statement
    {
    public:
        ExprStmt(Expression *expr)
            : Statement(), expr(expr) {};

        ~ExprStmt() override;

    public:
        Expression *expr;
    };

    struct BlockStmt : public Statement
    {
    public:
        BlockStmt(std::vector<Statement *> stmts)
            : Statement(), stmts(std::move(stmts)) {};

    public:
        std::vector<Statement *> stmts;
    };

    struct IfStmt : public Statement
    {
    public:
        IfStmt(Expression *cond, BlockStmt *true_block, BlockStmt *false_block)
            : Statement(), cond(cond), true_block(true_block), false_block(false_block) {};

        ~IfStmt() override;

    public:
        Expression *cond{nullptr};
        BlockStmt *true_block{nullptr};
        BlockStmt *false_block{nullptr};
        // std::deque<std::pair<Expression *, struct BlockStmt *>> branches;
    };

    struct ForStmt : public Statement
    {
    public:
        ForStmt(Statement *init, Expression *condition, Statement *update, BlockStmt *block)
            : Statement(), init(init), condition(condition), update(update), block(block) {};

        ~ForStmt() override;

    public:
        Statement *init;
        Statement *update;
        BlockStmt *block;
        Expression *condition;
    };

    struct WhileStmt : public Statement
    {
    public:
        WhileStmt(Expression *condition, BlockStmt *block)
            : Statement(), condition(condition), block(block) {};

        ~WhileStmt() override;

    public:
        Expression *condition;
        BlockStmt *block;
    };

    struct SwitchStmt : public Statement
    {
    public:
        SwitchStmt(Expression *cond, std::vector<MatchStmt *> matches)
            : Statement(), cond(cond), matches(matches) {};

    public:
        Expression *cond{nullptr};
        std::vector<MatchStmt *> matches;
    };

    struct MatchStmt : public Statement
    {
    public:
        MatchStmt(Expression *cond, BlockStmt *block)
            : Statement(), cond(cond), block(block) {};

    public:
        Expression *cond{nullptr};
        BlockStmt *block{nullptr};
    };

    struct FuncDeclStmt : public Statement
    {
    public:
        FuncDeclStmt(IdentifierExpr *name, std::vector<ParamExpr *> params, BlockStmt *block, IdentifierExpr *type)
            : Statement(), name(name), params(params), block(block), type(type) {};

    public:
        IdentifierExpr *name{nullptr};
        IdentifierExpr *type{nullptr};
        std::vector<ParamExpr *> params;
        BlockStmt *block{nullptr};
    };

    struct VarDeclStmt : public Statement
    {
    public:
        VarDeclStmt(IdentifierExpr *name, Statement *value)
            : Statement(), name(name), value(value) {};

        ~VarDeclStmt() override;

    public:
        IdentifierExpr *name{nullptr};
        Statement *value{nullptr};
    };

    struct PassStmt : public Statement
    {
    public:
        PassStmt()
            : Statement() {};
    };

    struct BreakStmt : public Statement
    {
    public:
        BreakStmt()
            : Statement() {};
    };

    struct ContinueStmt : public Statement
    {
    public:
        ContinueStmt()
            : Statement() {};
    };

    struct ThrowStmt : public Statement
    {
    public:
        ThrowStmt(Expression *value)
            : Statement(), value(value) {};

        ~ThrowStmt() override;

    public:
        Expression *value;
    };

} // namespace cora::ast

#endif // CORA_AST_STATEMENTS_H
