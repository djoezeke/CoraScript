#ifndef CORA_COMPILER_AST_EXPRESSIONS_H
#define CORA_COMPILER_AST_EXPRESSIONS_H

#include "../Parser/Token.hpp"
#include "ASTNode.hpp"

#include <deque>
#include <string>
#include <variant>

namespace cora::compiler
{
    namespace ast
    {

        using LiteralValue = std::variant<std::monostate, bool, double, std::string>;

        class Expression : public Node
        {
        public:
            Expression();
            explicit Expression(ExpressionType kind);
            ExpressionType GetExprType() const;
            std::string Repr() override;
            ~Expression() override;

        private:
            ExpressionType m_ExprType;
        };

        class LiteralExpr : public Expression
        {
        public:
            explicit LiteralExpr(LiteralValue value);
            explicit LiteralExpr(double value);
            explicit LiteralExpr(bool value);
            explicit LiteralExpr(const std::string &value);
            const LiteralValue &GetValue() const;
            std::string Repr() override;

        private:
            LiteralValue m_Value;
        };

        class VariableExpr : public Expression
        {
        public:
            explicit VariableExpr(std::string name);
            const std::string &GetName() const;
            std::string Repr() override;

        private:
            std::string m_Name;
        };

        class UnaryExpr : public Expression
        {
        public:
            UnaryExpr(parser::TokenType op, Expression *rhs);
            parser::TokenType GetOperator() const;
            Expression *GetRhs() const;
            std::string Repr() override;
            ~UnaryExpr() override;

        private:
            parser::TokenType m_Operator;
            Expression *m_Rhs;
        };

        class Null : public Expression
        {
        public:
            Null();
        };

        class Bool : public Expression
        {
        public:
            explicit Bool(bool const value);
            bool GetValue() const;

        private:
            bool m_Value{false};
        };

        class Byte : public Expression
        {
        public:
            explicit Byte(char const value);
            char GetValue() const;

        private:
            char m_Value{0};
        };

        class Float : public Expression
        {
        public:
            explicit Float(double value);
            double GetValue() const;

        private:
            double m_Value{0.0};
        };

        class Array : public Expression
        {
        public:
            explicit Array(std::deque<Expression *> array);
            std::deque<Expression *> GetElements() const;
            ~Array() override;

        private:
            std::deque<Expression *> m_Elements;
        };

        class String : public Expression
        {
        public:
            explicit String(std::string value);
            std::string GetValue() const;

        private:
            std::string m_Value;
        };

        class Integer : public Expression
        {
        public:
            explicit Integer(long long value);
            int GetValue() const;

        private:
            int m_Value{0};
        };

        class Identifier : public Expression
        {
        public:
            explicit Identifier(std::string name);
            std::string GetName() const;

        private:
            std::string m_Value;
        };

        class TryExpr : public Expression
        {
        public:
            TryExpr();
            ~TryExpr() override;
        };

        class CastExpr : public Expression
        {
        public:
            CastExpr();
            ~CastExpr() override;
        };

        class CallExpr : public Expression
        {
        public:
            CallExpr(Expression *callee, std::deque<Expression *> arguments);
            Expression *GetCallee() const;
            const std::deque<Expression *> &GetArguments() const;
            std::string Repr() override;
            ~CallExpr() override;

        private:
            Expression *m_Callee;
            std::deque<Expression *> m_Arguments;
        };

        class MemberExpr : public Expression
        {
        public:
            MemberExpr(Expression *object, std::string member);
            Expression *GetObject() const;
            const std::string &GetMember() const;
            std::string Repr() override;
            ~MemberExpr() override;

        private:
            Expression *m_Object;
            std::string m_Member;
        };

        class NewExpr : public Expression
        {
        public:
            NewExpr(std::string className, std::deque<Expression *> arguments);
            const std::string &GetClassName() const;
            const std::deque<Expression *> &GetArguments() const;
            std::string Repr() override;
            ~NewExpr() override;

        private:
            std::string m_ClassName;
            std::deque<Expression *> m_Arguments;
        };

        class BlockExpr : public Expression
        {
        public:
            BlockExpr();
            ~BlockExpr() override;
        };

        class AssignExpr : public Expression
        {
        public:
            AssignExpr();
            ~AssignExpr() override;
        };

        class BinaryExpr : public Expression
        {
        public:
            BinaryExpr(Expression *left, parser::TokenType op, Expression *right);
            Expression *GetLeft() const;
            parser::TokenType GetOperator() const;
            Expression *GetRight() const;
            std::string Repr() override;
            ~BinaryExpr() override;

        private:
            Expression *m_Left;
            parser::TokenType m_Operator;
            Expression *m_Right;
        };

        class TernaryExpr : public Expression
        {
        public:
            TernaryExpr(Expression *condExpr, Expression *thenExpr, Expression *elseExpr);
            ~TernaryExpr() override;

        private:
            Expression *m_CondExpr;
            Expression *m_ThenExpr;
            Expression *m_ElseExpr;
        };

        class ArrayAccessExpr : public Expression
        {
        public:
            ArrayAccessExpr();
            ~ArrayAccessExpr() override;
        };

        class PrefixUnaryExpr : public Expression
        {
        public:
            PrefixUnaryExpr(parser::TokenType op, Expression *operand);
            PrefixUnaryExpr(Expression *operand, parser::TokenType op);
            ~PrefixUnaryExpr() override;

        private:
            parser::TokenType m_Operator;
            Expression *m_Operand;
        };

        class PostfixUnaryExpr : public Expression
        {
        public:
            PostfixUnaryExpr(parser::TokenType op, Expression *operand);
            PostfixUnaryExpr(Expression *operand, parser::TokenType op);
            ~PostfixUnaryExpr() override;

        private:
            parser::TokenType m_Operator;
            Expression *m_Operand;
        };

    } // namespace ast

} // namespace cora::compiler

#endif // CORA_COMPILER_AST_EXPRESSIONS_H
