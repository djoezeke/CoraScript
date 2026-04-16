#ifndef CORASCRIPT_SCRIPT_AST_HPP
#define CORASCRIPT_SCRIPT_AST_HPP

#include "Cora/Compiler/Parser/ScriptToken.hpp"

#include <deque>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace cora
{
    namespace script
    {
        using ScriptValue = std::variant<std::monostate, double, bool, std::string>;

        struct Expr
        {
            virtual ~Expr() = default;
        };

        struct LiteralExpr : Expr
        {
            explicit LiteralExpr(ScriptValue v) : value(std::move(v)) {}
            ScriptValue value;
        };

        struct VariableExpr : Expr
        {
            explicit VariableExpr(std::string n) : name(std::move(n)) {}
            std::string name;
        };

        struct UnaryExpr : Expr
        {
            UnaryExpr(TokenType op, Expr *rhs) : op(op), rhs(rhs) {}
            ~UnaryExpr() override { delete rhs; }
            TokenType op;
            Expr *rhs;
        };

        struct BinaryExpr : Expr
        {
            BinaryExpr(Expr *lhs, TokenType op, Expr *rhs) : lhs(lhs), op(op), rhs(rhs) {}
            ~BinaryExpr() override
            {
                delete lhs;
                delete rhs;
            }
            Expr *lhs;
            TokenType op;
            Expr *rhs;
        };

        struct Stmt
        {
            virtual ~Stmt() = default;
        };

        struct PrintStmt : Stmt
        {
            ~PrintStmt() override
            {
                for (Expr *expr : expressions)
                {
                    delete expr;
                }
            }
            std::deque<Expr *> expressions;
        };

        struct ExprStmt : Stmt
        {
            explicit ExprStmt(Expr *e) : expr(e) {}
            ~ExprStmt() override { delete expr; }
            Expr *expr;
        };

        struct AssignStmt : Stmt
        {
            AssignStmt(std::string n, Expr *e) : name(std::move(n)), expr(e) {}
            ~AssignStmt() override { delete expr; }
            std::string name;
            Expr *expr;
        };

        struct VarDeclStmt : Stmt
        {
            VarDeclStmt(std::string n, std::optional<std::string> t, Expr *e)
                : name(std::move(n)), declaredType(std::move(t)), expr(e) {}
            ~VarDeclStmt() override { delete expr; }
            std::string name;
            std::optional<std::string> declaredType;
            Expr *expr;
        };

        struct BlockStmt : Stmt
        {
            ~BlockStmt() override;
            std::deque<Stmt *> statements;
        };

        struct IfStmt : Stmt
        {
            ~IfStmt() override;
            std::deque<std::pair<Expr *, BlockStmt *>> branches;
            BlockStmt *elseBlock{nullptr};
        };

        struct WhileStmt : Stmt
        {
            WhileStmt(Expr *c, BlockStmt *b) : condition(c), block(b) {}
            ~WhileStmt() override;
            Expr *condition;
            BlockStmt *block;
        };

        struct ForRangeStmt : Stmt
        {
            ForRangeStmt(std::string n, Expr *s, Expr *e, Expr *st, BlockStmt *b)
                : name(std::move(n)), start(s), end(e), step(st), block(b) {}
            ~ForRangeStmt() override;
            std::string name;
            Expr *start;
            Expr *end;
            Expr *step;
            BlockStmt *block;
        };

        struct ForCStyleStmt : Stmt
        {
            ForCStyleStmt(Stmt *i, Expr *c, Stmt *u, BlockStmt *b)
                : init(i), condition(c), update(u), block(b) {}
            ~ForCStyleStmt() override;
            Stmt *init;
            Expr *condition;
            Stmt *update;
            BlockStmt *block;
        };

        struct BreakStmt : Stmt
        {
        };

        struct ContinueStmt : Stmt
        {
        };

        struct PassStmt : Stmt
        {
        };
    }
}

#endif
