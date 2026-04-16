#ifndef CORASCRIPT_INTERPRETER_HPP
#define CORASCRIPT_INTERPRETER_HPP

#include "Cora/Compiler/AST/ScriptAST.hpp"
#include "Cora/Compiler/Parser/ScriptToken.hpp"

#include <deque>
#include <optional>
#include <string>
#include <unordered_map>

namespace cora
{
    namespace script
    {
        class Interpreter
        {
        public:
            using Value = ScriptValue;

            void Run(const std::string &source);
            void RunFile(const std::string &path);

        private:
            std::deque<Token> Lex(const std::string &source) const;
            std::deque<Stmt *> Parse(const std::deque<Token> &tokens);
            void Execute(const std::deque<Stmt *> &program);

            class Parser
            {
            public:
                explicit Parser(const std::deque<Token> &tokens) : m_Tokens(tokens) {}

                std::deque<Stmt *> ParseProgram();

            private:
                std::deque<Stmt *> ParseBlockBody(TokenType blockEnd, bool useIndent);
                BlockStmt *ParseBlock();
                Stmt *ParseStatement();
                Stmt *ParseIf();
                Stmt *ParseWhile();
                Stmt *ParseFor();
                Stmt *ParseVarDecl(std::optional<std::string> explicitType, bool consumeTerminator = true);
                Stmt *ParseAssignOrExpr(bool consumeTerminator = true);
                Stmt *ParsePrint();

                Expr *ParseExpression();
                Expr *ParseOr();
                Expr *ParseAnd();
                Expr *ParseEquality();
                Expr *ParseComparison();
                Expr *ParseTerm();
                Expr *ParseFactor();
                Expr *ParseUnary();
                Expr *ParsePrimary();

                bool Match(TokenType type);
                bool Check(TokenType type) const;
                bool CheckNext(TokenType type) const;
                const Token &Advance();
                const Token &Peek() const;
                const Token &Previous() const;
                const Token &Consume(TokenType type, const std::string &message);
                void ConsumeStatementTerminator();
                void SkipNewlines();

                const std::deque<Token> &m_Tokens;
                std::size_t m_Current{0};
            };

            Value EvalExpr(Expr *expr);
            void ExecStmt(Stmt *stmt);
            void ExecBlock(BlockStmt *block);

            bool IsTruthy(const Value &value) const;
            double AsNumber(const Value &value) const;
            std::string ToString(const Value &value) const;
            bool ValuesEqual(const Value &lhs, const Value &rhs) const;
            Value ApplyBinary(TokenType op, const Value &lhs, const Value &rhs) const;
            Value ApplyUnary(TokenType op, const Value &rhs) const;
            void CheckTypeCompatibility(const std::optional<std::string> &declaredType, const Value &value, const std::string &name) const;

            struct Symbol
            {
                Value value;
                std::optional<std::string> declaredType;
            };

            std::unordered_map<std::string, Symbol> m_Symbols;
        };
    }
}

#endif
