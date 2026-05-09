#ifndef CORA_PARSER_PARSER_H
#define CORA_PARSER_PARSER_H

#include "Lexer.hpp"
#include "Token.hpp"

#include "../AST/ASTExpr.hpp"
#include "../AST/ASTNode.hpp"
#include "../AST/ASTStmt.hpp"
#include "Cora/Basic/Error.hpp"

#include <optional>
#include <string>
#include <vector>

namespace cora::parser
{
    using namespace ast;
    using namespace cora;

    class Parser
    {
        using Statement = ast::Statement;
        using Expression = ast::Expression;

    public:
        Parser();

        Parser(const std::vector<Token> &tokens);

        std::vector<Statement *> Parse();
        std::vector<Statement *> Parse(const std::string &source);

        void SetFileName(std::string fileName);
        void SetModuleName(std::string moduleName);

    private:
        std::vector<Statement *> ParseBlockBody();
        std::vector<Statement *> ParseArguments();

        Statement *ParseStatement();
        Statement *ParseExprStmt();
        Statement *ParseBlockStmt();
        Statement *ParseIfStmt();
        Statement *ParseForStmt();
        Statement *ParseWhileStmt();
        Statement *ParseSwitchStmt();
        Statement *ParseMatchStmt();
        Statement *ParseFuncDeclStmt();
        Statement *ParseVarDeclStmt();
        Statement *ParsePassStmt();
        Statement *ParseBreakStmt();
        Statement *ParseContinueStmt();
        Statement *ParseThrowStmt();

        Statement *ParseClassDeclStmt();
        Statement *ParseImportStmt();
        Statement *ParseTryCatch();
        Statement *ParseEnumDeclStmt();      // Added
        Statement *ParseStructDeclStmt();     // Added
        Statement *ParseForInStmt(); // Added

        Expression *ParseExpression();
        Expression *ParseGroupExpr();
        Expression *ParseArrayExpr();
        Expression *ParseArrayIdExpr();
        Expression *ParseStructLiteralExpr(); // Added
        Expression *ParseParamExpr();
        Expression *ParseUnaryExpr();
        Expression *PrefixUnaryExpr();
        Expression *PostfixUnaryExpr();
        Expression *ParseBinaryExpr();
        Expression *AssignExpr();
        Expression *ParseTernaryExpr();
        Expression *ParseFuncCallExpr();

        Expression *ParseOr();
        Expression *ParseAnd();
        Expression *ParseEquality();
        Expression *ParseComparison();
        Expression *ParseTerm();
        Expression *ParseFactor();
        Expression *ParseUnary();
        Expression *ParsePrimary();

        bool Match(TokenType type);
        bool Check(TokenType type) const;
        bool CheckNext(TokenType type) const;
        TokenType Type() const;
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
        error::DiagnosticContext MakeContext(const Token &token) const;
        [[noreturn]] void RaiseParseError(const std::string &message, const Token &token) const;
        std::string CurrentNamespacePath() const;
        constexpr int OperatorPriority(TokenType op);

    private:
        Lexer m_Lexer;
        std::vector<Token> m_Tokens;
        std::size_t m_Current{0};
        std::string m_FileName{"<memory>"};
        std::string m_ModuleName;
        std::vector<std::string> m_ClassStack;
        std::vector<std::string> m_FunctionStack;
        std::vector<std::string> m_NamespaceStack;
    };

} // namespace cora::parser

#endif // CORA_PARSER_PARSER_H
