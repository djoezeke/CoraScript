#ifndef CORA_COMPILER_AST_STATEMENTS_H
#define CORA_COMPILER_AST_STATEMENTS_H

#include "Cora/Compiler/AST/Expressions.hpp"

#include <deque>
#include <optional>
#include <string>
#include <utility>

namespace cora::compiler
{
    namespace ast
    {

        class Statement : public Node
        {
        public:
            Statement();
            explicit Statement(StatementType kind);
            StatementType GetStmtType() const;
            std::string Repr() override;
            ~Statement() override;

        private:
            StatementType m_StmtType;
        };

        class ExprStmt : public Statement
        {
        public:
            explicit ExprStmt(Expression *expr);
            Expression *GetExpression() const;
            ~ExprStmt() override;

        private:
            Expression *m_Expr;
        };

        class PrintStmt : public Statement
        {
        public:
            PrintStmt();
            std::deque<Expression *> expressions;
            ~PrintStmt() override;
        };

        class AssignStmt : public Statement
        {
        public:
            AssignStmt(std::string name, Expression *expr);
            const std::string &GetName() const;
            Expression *GetExpression() const;
            ~AssignStmt() override;

        private:
            std::string m_Name;
            Expression *m_Expr;
        };

        class VarDeclStmt : public Statement
        {
        public:
            VarDeclStmt(std::string name, std::optional<std::string> declaredType, Expression *expr);
            const std::string &GetName() const;
            const std::optional<std::string> &GetDeclaredType() const;
            Expression *GetExpression() const;
            ~VarDeclStmt() override;

        private:
            std::string m_Name;
            std::optional<std::string> m_DeclaredType;
            Expression *m_Expr;
        };

        class IfStmt : public Statement
        {
        public:
            IfStmt();
            std::deque<std::pair<Expression *, class BlockStmt *>> branches;
            class BlockStmt *elseBlock;
            ~IfStmt() override;
        };

        class DoStmt : public Statement
        {
        public:
            DoStmt();
            ~DoStmt() override;
        };

        class ForStmt : public Statement
        {
        public:
            ForStmt();
            ~ForStmt() override;
        };

        class NewStmt : public Statement
        {
        public:
            NewStmt();
            ~NewStmt() override;
        };

        class PassStmt : public Statement
        {
        public:
            PassStmt();
        };

        class WhileStmt : public Statement
        {
        public:
            WhileStmt();
            WhileStmt(Expression *condition, class BlockStmt *block);
            Expression *condition;
            class BlockStmt *block;
            ~WhileStmt() override;
        };

        class BreakStmt : public Statement
        {
        public:
            BreakStmt();
        };

        class BlockStmt : public Statement
        {
        public:
            BlockStmt();
            std::deque<Statement *> statements;
            ~BlockStmt() override;
        };

        class YieldStmt : public Statement
        {
        public:
            YieldStmt();
            ~YieldStmt() override;
        };

        class ThrowStmt : public Statement
        {
        public:
            ThrowStmt();
            ~ThrowStmt() override;
        };

        class DeleteStmt : public Statement
        {
        public:
            explicit DeleteStmt(Expression *target);
            Expression *GetTarget() const;
            ~DeleteStmt() override;

        private:
            Expression *m_Target;
        };

        class SwitchStmt : public Statement
        {
        public:
            SwitchStmt();
            ~SwitchStmt() override;
        };

        class ReturnStmt : public Statement
        {
        public:
            explicit ReturnStmt(Expression *value);
            Expression *GetValue() const;
            ~ReturnStmt() override;

        private:
            Expression *m_Value;
        };

        class FunctionDeclStmt : public Statement
        {
        public:
            FunctionDeclStmt(std::string name, std::deque<std::string> parameters, class BlockStmt *body);
            const std::string &GetName() const;
            const std::deque<std::string> &GetParameters() const;
            class BlockStmt *GetBody() const;
            ~FunctionDeclStmt() override;

        private:
            std::string m_Name;
            std::deque<std::string> m_Parameters;
            class BlockStmt *m_Body;
        };

        class ClassDeclStmt : public Statement
        {
        public:
            ClassDeclStmt(std::string name, std::deque<FunctionDeclStmt *> methods);
            const std::string &GetName() const;
            const std::deque<FunctionDeclStmt *> &GetMethods() const;
            ~ClassDeclStmt() override;

        private:
            std::string m_Name;
            std::deque<FunctionDeclStmt *> m_Methods;
        };

        class ForEachStmt : public Statement
        {
        public:
            ForEachStmt();
            ~ForEachStmt() override;
        };

        class ContinueStmt : public Statement
        {
        public:
            ContinueStmt();
        };

        class ForRangeStmt : public Statement
        {
        public:
            ForRangeStmt(std::string name, Expression *start, Expression *end, Expression *step, BlockStmt *block);
            std::string name;
            Expression *start;
            Expression *end;
            Expression *step;
            BlockStmt *block;
            ~ForRangeStmt() override;
        };

        class ForCStyleStmt : public Statement
        {
        public:
            ForCStyleStmt(Statement *init, Expression *condition, Statement *update, BlockStmt *block);
            Statement *init;
            Expression *condition;
            Statement *update;
            BlockStmt *block;
            ~ForCStyleStmt() override;
        };

    } // namespace ast

} // namespace cora::compiler

#endif // CORA_COMPILER_AST_STATEMENTS_H
