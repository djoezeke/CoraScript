#ifndef CORA_COMPILER_PARSER_PARSER_H
#define CORA_COMPILER_PARSER_PARSER_H

#include "Cora/Compiler/Parser/Lexer.hpp"
#include "Cora/Compiler/Parser/Token.hpp"

#include "Cora/Compiler/AST/Expressions.hpp"
#include "Cora/Compiler/AST/Nodes.hpp"
#include "Cora/Compiler/AST/Statements.hpp"

#include <deque>
#include <optional>
#include <string>

namespace cora::compiler
{
    namespace parser
    {
        using namespace ast;

        class Parser
        {
            using Statement = ast::Statement;
            using Expression = ast::Expression;

        public:
            Parser();

            Parser(const std::deque<Token> &tokens);

            std::deque<Statement *> ParseProgram(const std::string &source);
            std::deque<Statement *> ParseProgram();

        private:
            std::deque<Statement *> ParseBlockBody(TokenType blockEnd, bool useIndent);

            Statement *ParseStatement();
            Statement *ParseIf();
            Statement *ParseWhile();
            Statement *ParseFor();
            Statement *ParseClassDecl();
            Statement *ParseFunctionDecl(bool requireName = true);
            Statement *ParseReturn();
            Statement *ParseVarDecl(std::optional<std::string> explicitType, bool consumeTerminator = true);
            Statement *ParseAssignOrExpr(bool consumeTerminator = true);
            Statement *ParsePrint();
            Statement *ParseDelete();
            Statement *ParseBlock();

            Expression *ParseExpression();
            Expression *ParseOr();
            Expression *ParseAnd();
            Expression *ParseEquality();
            Expression *ParseComparison();
            Expression *ParseTerm();
            Expression *ParseFactor();
            Expression *ParseUnary();
            Expression *ParsePrimary();
            Expression *ParseCall();
            Expression *ParseMember();

            bool Match(TokenType type);
            bool Check(TokenType type) const;
            bool CheckNext(TokenType type) const;
            const Token &Advance();
            const Token &Peek() const;
            const Token &Previous() const;
            const Token &Consume(TokenType type, const std::string &message);
            void ConsumeStatementTerminator();
            void SkipNewlines();

        private:
            Lexer m_Lexer;
            std::deque<Token> m_Tokens;
            std::size_t m_Current{0};
        };

    } // namespace parser

} // namespace cora::compiler

#endif // CORA_COMPILER_PARSER_PARSER_H
