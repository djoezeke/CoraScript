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

    struct MatchStmt : public Statement
    {
    public:
        MatchStmt(Expression *cond, BlockStmt *block)
            : Statement(), cond(cond), block(block) {};

    public:
        Expression *cond{nullptr};
        BlockStmt *block{nullptr};
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
        VarDeclStmt(IdentifierExpr *name, Expression *value, IdentifierExpr *type = nullptr)
            : Statement(NodeType::VarDeclaration), name(name), value(value), type(type) {};

        ~VarDeclStmt() override;

    public:
        IdentifierExpr *name{nullptr};
        Expression *value{nullptr};
        IdentifierExpr *type{nullptr};
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

    struct ReturnStmt : public Statement
    {
    public:
        ReturnStmt(Expression *value)
            : Statement(NodeType::ReturnStmt), value(value) {};

        ~ReturnStmt() override;

    public:
        Expression *value{nullptr};
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

    struct ImportStmt : public Statement
    {
    public:
        ImportStmt(IdentifierExpr *moduleName)
            : Statement(NodeType::ImportStmt), moduleName(moduleName) {};

        ~ImportStmt() override;

    public:
        IdentifierExpr *moduleName{nullptr};
    };

    struct ClassDecl : public Statement
    {
    public:
        ClassDecl(IdentifierExpr *name, std::vector<FuncDeclStmt *> methods)
            : Statement(NodeType::ClassDecl), name(name), methods(methods) {};

        ~ClassDecl() override;

    public:
        IdentifierExpr *name{nullptr};
        std::vector<FuncDeclStmt *> methods;
        // Potentially add std::vector<VarDeclStmt*> members;
    };

    struct EnumDecl : public Statement
    {
    public:
        EnumDecl(IdentifierExpr *name, std::vector<IdentifierExpr *> variants)
            : Statement(NodeType::EnumDecl), name(name), variants(std::move(variants)) {};

        ~EnumDecl() override;

    public:
        IdentifierExpr *name{nullptr};
        std::vector<IdentifierExpr *> variants;
    };

    struct StructDecl : public Statement
    {
    public:
        StructDecl(IdentifierExpr *name, std::vector<VarDeclStmt *> fields)
            : Statement(NodeType::StructDecl), name(name), fields(std::move(fields)) {};

        ~StructDecl() override;

    public:
        IdentifierExpr *name{nullptr};
        std::vector<VarDeclStmt *> fields;
    };

    struct ForInStmt : public Statement
    {
    public:
        ForInStmt(VarDeclStmt *variable, Expression *iterable, BlockStmt *block)
            : Statement(NodeType::ForInStmt), variable(variable), iterable(iterable), block(block) {};

        ~ForInStmt() override;

    public:
        VarDeclStmt *variable{nullptr};
        Expression *iterable{nullptr};
        BlockStmt *block{nullptr};
    };

    struct TryCatchStmt : public Statement
    {
    public:
        TryCatchStmt(BlockStmt *tryBlock, IdentifierExpr *catchVar, BlockStmt *catchBlock)
            : Statement(NodeType::TryCatchStmt), tryBlock(tryBlock), catchVar(catchVar), catchBlock(catchBlock) {};

        ~TryCatchStmt() override;

    public:
        BlockStmt *tryBlock{nullptr};
        IdentifierExpr *catchVar{nullptr}; // Optional: for 'catch (e)'
        BlockStmt *catchBlock{nullptr};
    };

} // namespace cora::ast

#endif // CORA_AST_STATEMENTS_H
