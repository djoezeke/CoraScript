#ifndef CORA_COMPILER_PARSER_PARSER_H
#define CORA_COMPILER_PARSER_PARSER_H

#include "Lexer.hpp"
#include "Token.hpp"

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTNode.hpp"
#include "../AST/ASTStmt.hpp"
#include "Cora/Basic/Error.hpp"

#include <deque>
#include <optional>
#include <string>
#include <vector>

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

            void SetFileName(std::string fileName);
            void SetModuleName(std::string moduleName);

        private:
            std::deque<Statement *> ParseBlockBody(TokenType blockEnd, bool useIndent);

            Statement *ParseStatement();
            Statement *ParseAssignment(bool consumeTerminator = true);
            Statement *ParseVarDeclaration(bool consumeTerminator = true, bool constant = false,
                                           std::optional<ast::AccessModifier> access = std::nullopt);
            Statement *ParseIf();
            Statement *ParseWhile();
            Statement *ParseFor();
            Statement *ParseClassDecl();
            Statement *ParseNamespaceDecl();
            Statement *ParseImport();
            Statement *ParseTryCatch();
            Statement *ParseThrow();
            Statement *ParseFunctionDecl(bool requireName = true, ast::AccessModifier access = ast::AccessModifier::Public);
            Statement *ParseReturn();
            Statement *ParseVarDecl(std::optional<std::string> explicitType, bool consumeTerminator = true, ast::AccessModifier access = ast::AccessModifier::Public);
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
            bool IsFunctionDeclAhead() const;
            bool IsNomalAssignmentAhead() const;
            bool IsMemberAssignmentAhead() const;
            Expression *ParseAssignmentTarget();
            std::optional<ast::AccessModifier> ParseOptionalAccessModifier();
            static std::string ConsumeLeadingDocString(ast::BlockStmt *block);
            error::DiagnosticContext MakeContext(const Token &token) const;
            [[noreturn]] void RaiseParseError(const std::string &message, const Token &token) const;
            std::string CurrentNamespacePath() const;

        private:
            Lexer m_Lexer;
            std::deque<Token> m_Tokens;
            std::size_t m_Current{0};
            std::string m_FileName{"<memory>"};
            std::string m_ModuleName;
            std::vector<std::string> m_NamespaceStack;
            std::vector<std::string> m_ClassStack;
            std::vector<std::string> m_FunctionStack;
        };

    } // namespace parser

} // namespace cora::compiler

#endif // CORA_COMPILER_PARSER_PARSER_H
