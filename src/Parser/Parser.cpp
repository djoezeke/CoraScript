#include "Parser.hpp"

#include <stdexcept>

namespace cora::parser
{

    Parser::Parser()
        : m_Tokens({}), m_Current(0) {};

    Parser::Parser(const std::vector<Token> &tokens)
        : m_Tokens(tokens), m_Current(0) {};

    void Parser::SetFileName(std::string fileName)
    {
        m_FileName = fileName.empty() ? "<memory>" : std::move(fileName);
    }

    void Parser::SetModuleName(std::string moduleName)
    {
        m_ModuleName = std::move(moduleName);
    }

    std::vector<ast::Statement *> Parser::Parse()
    {
        std::vector<Statement *> program;

        if (m_Tokens.empty())
        {
        }
        else
        {
            SkipNewlines();
            while (!Check(TokenType::End))
            {
                const Token start = Peek();
                Statement *statement = ParseStatement();
                statement->SetStartPosition(start.GetStartPosition());
                program.push_back(statement);
                SkipNewlines();
            }
        }
        return program;
    }

    std::vector<ast::Statement *> Parser::Parse(const std::string &source)
    {
        Lexer lexer(source);
        lexer.SetFileName(m_FileName);
        lexer.SetModuleName(m_ModuleName);
        m_Tokens = lexer.Tokenize();
        m_Current = 0;
        return Parse();
    }

    std::vector<Statement *> Parser::ParseBlockBody()
    {
        std::vector<Statement *> stmts;
        SkipNewlines();
        while (!(Check(TokenType::Dedent) || Check(TokenType::RBrace)) && !Check(TokenType::End))
        {
            const Token start = Peek();
            Statement *statement = ParseStatement();
            statement->SetStartPosition(start.GetStartPosition());
            stmts.push_back(statement);
            SkipNewlines();
        };
        return stmts;
    }

    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////

    bool Parser::Match(TokenType type)
    {
        if (Check(type))
        {
            Advance();
            return true;
        }
        return false;
    }

    bool Parser::Check(TokenType type) const
    {
        return Peek().GetTokenType() == type;
    }

    TokenType Parser::Type() const
    {
        return Peek().GetTokenType();
    }

    bool Parser::CheckNext(TokenType type) const
    {
        if (m_Current + 1 >= m_Tokens.size())
        {
            return false;
        }
        return m_Tokens[m_Current + 1].GetTokenType() == type;
    }

    const Token &Parser::Advance()
    {
        if (!Check(TokenType::End))
        {
            ++m_Current;
        }
        return Previous();
    }

    const Token &Parser::Peek() const
    {
        return m_Tokens[m_Current];
    }

    const Token &Parser::Previous() const
    {
        return m_Tokens[m_Current - 1];
    }

    const Token &Parser::Consume(TokenType type, const std::string &message)
    {
        if (!Check(type))
        {
            const Token &token = Peek();
            RaiseParseError(message, token);
        }
        return Advance();
    }

    void Parser::ConsumeStatementTerminator()
    {
        if (Match(TokenType::Semicolon) || Match(TokenType::Newline))
        {
            return;
        }
        if (Check(TokenType::Dedent) || Check(TokenType::RBrace) || Check(TokenType::End))
        {
            return;
        }

        const Token &token = Peek();
        RaiseParseError("Expected statement terminator", token);
    }

    void Parser::SkipNewlines()
    {
        while (Match(TokenType::Newline))
        {
        }
    }

    error::DiagnosticContext Parser::MakeContext(const Token &token) const
    {
        error::DiagnosticContext context;
        context.fileName = m_FileName;
        context.moduleName = m_ModuleName;
        if (!m_ClassStack.empty())
        {
            context.className = m_ClassStack.back();
        }
        if (!m_FunctionStack.empty())
        {
            context.functionName = m_FunctionStack.back();
        }
        context.line = token.GetStartPosition().Line();
        context.column = token.GetStartPosition().Column();
        return context;
    }

    [[noreturn]] void Parser::RaiseParseError(const std::string &message, const Token &token) const
    {
        throw error::ParsingError(message, MakeContext(token));
    }

} // namespace cora::parser
