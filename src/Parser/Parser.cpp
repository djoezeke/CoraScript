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
            return program;
        }

        SkipNewlines();
        while (!Check(TokenType::End))
        {
            const Token start = Peek();
            Statement *statement = ParseStatement();
            if (statement != nullptr)
            {
                statement->SetStartPosition(start.GetStartPosition());
                program.push_back(statement);
            }
            SkipNewlines();
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
            if (statement != nullptr)
            {
                statement->SetStartPosition(start.GetStartPosition());
                stmts.push_back(statement);
            }
            SkipNewlines();
        }
        return stmts;
    }

    std::vector<Statement *> Parser::ParseArguments()
    {
        std::vector<Statement *> args;
        if (!Check(TokenType::RParen))
        {
            do
            {
                args.push_back(new ExprStmt(ParseExpression()));
            } while (Match(TokenType::Comma));
        }
        return args;
    }

    Statement *Parser::ParseStatement()
    {
        SkipNewlines();

        if (Match(TokenType::Import))
        {
            return ParseImportStmt();
        }
        if (Match(TokenType::If))
        {
            return ParseIfStmt();
        }
        if (Match(TokenType::While))
        {
            return ParseWhileStmt();
        }
        if (Match(TokenType::For))
        {
            return ParseForStmt();
        }
        if (Match(TokenType::Do))
        {
            Statement *body = ParseBlockStmt();
            if (Match(TokenType::While))
            {
                Consume(TokenType::LParen, "Expected '(' after while in do-while");
                Expression *condition = ParseExpression();
                delete condition;
                Consume(TokenType::RParen, "Expected ')' after do-while condition");
            }
            ConsumeStatementTerminator();
            return body;
        }
        if (Match(TokenType::Func) || (Check(TokenType::Identifier) && CheckNext(TokenType::LParen) && IsFunctionDeclAhead()))
        {
            return ParseFuncDeclStmt();
        }
        if (Match(TokenType::Return))
        {
            Expression *value = nullptr;
            if (!Check(TokenType::Semicolon) && !Check(TokenType::Newline) && !Check(TokenType::Dedent) && !Check(TokenType::RBrace) && !Check(TokenType::End))
            {
                value = ParseExpression();
            }
            ConsumeStatementTerminator();
            return new ReturnStmt(value);
        }
        if (Match(TokenType::Break))
        {
            ConsumeStatementTerminator();
            return new BreakStmt();
        }
        if (Match(TokenType::Continue))
        {
            ConsumeStatementTerminator();
            return new ContinueStmt();
        }
        if (Match(TokenType::Pass))
        {
            ConsumeStatementTerminator();
            return new PassStmt();
        }
        if (Match(TokenType::Let) || Match(TokenType::Const))
        {
            return ParseVarDeclStmt();
        }

        if (Match(TokenType::Class) || Match(TokenType::Enum) || Match(TokenType::Struct) ||
            Match(TokenType::Switch) || Match(TokenType::Try) || Match(TokenType::Catch) ||
            Match(TokenType::Match) || Match(TokenType::Default))
        {
            if (Check(TokenType::Identifier))
            {
                Advance();
            }
            if (Match(TokenType::LParen))
            {
                int depth = 1;
                while (depth > 0 && !Check(TokenType::End))
                {
                    if (Match(TokenType::LParen))
                    {
                        ++depth;
                    }
                    else if (Match(TokenType::RParen))
                    {
                        --depth;
                    }
                    else
                    {
                        Advance();
                    }
                }
            }

            if (Check(TokenType::LBrace) || Check(TokenType::Colon) || Check(TokenType::Newline))
            {
                Statement *body = ParseBlockStmt();
                delete body;
            }
            else
            {
                while (!Check(TokenType::Semicolon) && !Check(TokenType::Newline) && !Check(TokenType::End))
                {
                    Advance();
                }
                ConsumeStatementTerminator();
            }

            return new PassStmt();
        }

        return ParseExprStmt();
    }

    Statement *Parser::ParseExprStmt()
    {
        Expression *expr = ParseExpression();
        ConsumeStatementTerminator();
        return new ExprStmt(expr);
    }

    Statement *Parser::ParseBlockStmt()
    {
        if (Match(TokenType::LBrace))
        {
            std::vector<Statement *> stmts = ParseBlockBody();
            Consume(TokenType::RBrace, "Expected '}' after block");
            return new BlockStmt(std::move(stmts));
        }

        if (Match(TokenType::Colon))
        {
            if (Match(TokenType::Newline) && Match(TokenType::Indent))
            {
                std::vector<Statement *> stmts = ParseBlockBody();
                Consume(TokenType::Dedent, "Expected dedent after block");
                return new BlockStmt(std::move(stmts));
            }

            Statement *single = ParseStatement();
            std::vector<Statement *> stmts;
            if (single != nullptr)
            {
                stmts.push_back(single);
            }
            return new BlockStmt(std::move(stmts));
        }

        if (Match(TokenType::Newline) && Match(TokenType::Indent))
        {
            std::vector<Statement *> stmts = ParseBlockBody();
            Consume(TokenType::Dedent, "Expected dedent after block");
            return new BlockStmt(std::move(stmts));
        }

        Statement *single = ParseStatement();
        std::vector<Statement *> stmts;
        if (single != nullptr)
        {
            stmts.push_back(single);
        }
        return new BlockStmt(std::move(stmts));
    }

    Statement *Parser::ParseIfStmt()
    {
        Expression *condition = ParseExpression();
        auto *trueBlock = static_cast<BlockStmt *>(ParseBlockStmt());

        BlockStmt *falseBlock = nullptr;
        if (Match(TokenType::Else))
        {
            falseBlock = static_cast<BlockStmt *>(ParseBlockStmt());
        }

        return new IfStmt(condition, trueBlock, falseBlock);
    }

    Statement *Parser::ParseForStmt()
    {
        Consume(TokenType::LParen, "Expected '(' after for");

        int depth = 1;
        bool hasSemicolon = false;
        bool hasIn = false;
        for (std::size_t i = m_Current; i < m_Tokens.size() && depth > 0; ++i)
        {
            TokenType type = m_Tokens[i].GetTokenType();
            if (type == TokenType::LParen)
            {
                ++depth;
            }
            else if (type == TokenType::RParen)
            {
                --depth;
            }
            else if (depth == 1)
            {
                if (type == TokenType::Semicolon)
                {
                    hasSemicolon = true;
                }
                if (type == TokenType::In)
                {
                    hasIn = true;
                }
            }
        }

        if (hasIn && !hasSemicolon)
        {
            while (!Check(TokenType::RParen) && !Check(TokenType::End))
            {
                Advance();
            }
            Consume(TokenType::RParen, "Expected ')' after for-range");
            Statement *body = ParseBlockStmt();
            delete body;
            return new PassStmt();
        }

        Statement *initializer = nullptr;
        if (!Check(TokenType::Semicolon))
        {
            if (Match(TokenType::Let) || Match(TokenType::Const))
            {
                const Token &nameToken = Consume(TokenType::Identifier, "Expected loop variable name");
                if (Match(TokenType::Colon))
                {
                    Consume(TokenType::Identifier, "Expected type name after ':'");
                    while (Match(TokenType::LBracket))
                    {
                        Consume(TokenType::RBracket, "Expected ']' in type annotation");
                    }
                }
                Consume(TokenType::Assign, "Expected '=' in variable declaration");
                initializer = new VarDeclStmt(new IdentifierExpr(nameToken.GetText()), new ExprStmt(ParseExpression()));
            }
            else
            {
                initializer = ParseExprStmt();
            }
        }
        Consume(TokenType::Semicolon, "Expected ';' after for initializer");

        Expression *condition = nullptr;
        if (!Check(TokenType::Semicolon))
        {
            condition = ParseExpression();
        }
        else
        {
            condition = new BoolExpr(true);
        }
        Consume(TokenType::Semicolon, "Expected ';' after for condition");

        Statement *update = nullptr;
        if (!Check(TokenType::RParen))
        {
            update = new ExprStmt(ParseExpression());
        }
        Consume(TokenType::RParen, "Expected ')' after for clauses");

        auto *body = static_cast<BlockStmt *>(ParseBlockStmt());
        return new ForStmt(initializer, condition, update, body);
    }

    Statement *Parser::ParseWhileStmt()
    {
        Expression *condition = ParseExpression();
        auto *body = static_cast<BlockStmt *>(ParseBlockStmt());
        return new WhileStmt(condition, body);
    }

    Statement *Parser::ParseSwitchStmt()
    {
        Statement *block = ParseBlockStmt();
        delete block;
        return new PassStmt();
    }

    Statement *Parser::ParseMatchStmt()
    {
        Statement *block = ParseBlockStmt();
        delete block;
        return new PassStmt();
    }

    Statement *Parser::ParseFuncDeclStmt()
    {
        const Token &nameToken = Consume(TokenType::Identifier, "Expected function name");

        Consume(TokenType::LParen, "Expected '(' after function name");
        std::vector<ParamExpr *> params;
        if (!Check(TokenType::RParen))
        {
            do
            {
                const Token &paramName = Consume(TokenType::Identifier, "Expected parameter name");
                Expression *typeExpr = new IdentifierExpr("any");
                if (Match(TokenType::Colon))
                {
                    const Token &typeName = Consume(TokenType::Identifier, "Expected parameter type");
                    while (Match(TokenType::LBracket))
                    {
                        Consume(TokenType::RBracket, "Expected ']' in parameter type");
                    }
                    delete typeExpr;
                    typeExpr = new IdentifierExpr(typeName.GetText());
                }
                params.push_back(new ParamExpr(new IdentifierExpr(paramName.GetText()), typeExpr));
            } while (Match(TokenType::Comma));
        }
        Consume(TokenType::RParen, "Expected ')' after parameter list");

        IdentifierExpr *returnType = new IdentifierExpr("void");
        if (Match(TokenType::Minus))
        {
            Consume(TokenType::Greater, "Expected '>' after '-' in return type");
            const Token &typeName = Consume(TokenType::Identifier, "Expected return type");
            while (Match(TokenType::LBracket))
            {
                Consume(TokenType::RBracket, "Expected ']' in return type");
            }
            delete returnType;
            returnType = new IdentifierExpr(typeName.GetText());
        }

        auto *body = static_cast<BlockStmt *>(ParseBlockStmt());
        return new FuncDeclStmt(new IdentifierExpr(nameToken.GetText()), std::move(params), body, returnType);
    }

    Statement *Parser::ParseVarDeclStmt()
    {
        const Token &nameToken = Consume(TokenType::Identifier, "Expected variable name");
        if (Match(TokenType::Colon))
        {
            Consume(TokenType::Identifier, "Expected type annotation");
            while (Match(TokenType::LBracket))
            {
                Consume(TokenType::RBracket, "Expected ']' in type annotation");
            }
        }

        Expression *initializer = new NullExpr();
        if (Match(TokenType::Assign))
        {
            delete initializer;
            initializer = ParseExpression();
        }

        ConsumeStatementTerminator();
        return new VarDeclStmt(new IdentifierExpr(nameToken.GetText()), new ExprStmt(initializer));
    }

    Statement *Parser::ParsePassStmt()
    {
        ConsumeStatementTerminator();
        return new PassStmt();
    }

    Statement *Parser::ParseBreakStmt()
    {
        ConsumeStatementTerminator();
        return new BreakStmt();
    }

    Statement *Parser::ParseContinueStmt()
    {
        ConsumeStatementTerminator();
        return new ContinueStmt();
    }

    Statement *Parser::ParseThrowStmt()
    {
        Expression *value = ParseExpression();
        ConsumeStatementTerminator();
        return new ThrowStmt(value);
    }

    Statement *Parser::ParseClassDeclStmt()
    {
        Statement *block = ParseBlockStmt();
        delete block;
        return new PassStmt();
    }

    Statement *Parser::ParseImportStmt()
    {
        Consume(TokenType::Identifier, "Expected module name after import");
        while (Match(TokenType::Dot))
        {
            Consume(TokenType::Identifier, "Expected identifier after '.' in import path");
        }
        ConsumeStatementTerminator();
        return new PassStmt();
    }

    Statement *Parser::ParseTryCatch()
    {
        Statement *tryBlock = ParseBlockStmt();
        delete tryBlock;

        while (Match(TokenType::Catch))
        {
            if (Match(TokenType::LParen))
            {
                while (!Check(TokenType::RParen) && !Check(TokenType::End))
                {
                    Advance();
                }
                Consume(TokenType::RParen, "Expected ')' after catch clause");
            }
            Statement *catchBlock = ParseBlockStmt();
            delete catchBlock;
        }

        return new PassStmt();
    }

    Expression *Parser::ParseExpression()
    {
        return AssignExpr();
    }

    Expression *Parser::ParseGroupExpr()
    {
        return ParseExpression();
    }

    Expression *Parser::ParseArrayExpr()
    {
        if (Match(TokenType::LBracket))
        {
            while (!Check(TokenType::RBracket) && !Check(TokenType::End))
            {
                Expression *value = ParseExpression();
                delete value;
                if (!Match(TokenType::Comma))
                {
                    break;
                }
            }
            Consume(TokenType::RBracket, "Expected ']' after array literal");
        }
        return new NullExpr();
    }

    Expression *Parser::ParseArrayIdExpr()
    {
        return ParsePrimary();
    }

    Expression *Parser::ParseParamExpr()
    {
        return ParsePrimary();
    }

    Expression *Parser::ParseUnaryExpr()
    {
        return ParseUnary();
    }

    Expression *Parser::PrefixUnaryExpr()
    {
        return ParseUnary();
    }

    Expression *Parser::PostfixUnaryExpr()
    {
        return ParseUnary();
    }

    Expression *Parser::ParseBinaryExpr()
    {
        return ParseOr();
    }

    Expression *Parser::AssignExpr()
    {
        Expression *expr = ParseOr();
        if (Match(TokenType::Assign))
        {
            Expression *rhs = AssignExpr();
            auto *identifier = dynamic_cast<IdentifierExpr *>(expr);
            if (identifier == nullptr)
            {
                delete rhs;
                return expr;
            }
            auto *lhs = new IdentifierExpr(identifier->name);
            delete expr;
            return new ast::AssignExpr(lhs, rhs);
        }
        return expr;
    }

    Expression *Parser::ParseFuncCallExpr()
    {
        Expression *expr = ParsePrimary();

        while (Match(TokenType::LParen))
        {
            std::vector<Statement *> args = ParseArguments();
            Consume(TokenType::RParen, "Expected ')' after function arguments");

            auto *name = dynamic_cast<IdentifierExpr *>(expr);
            if (name == nullptr)
            {
                for (Statement *arg : args)
                {
                    delete arg;
                }
                delete expr;
                expr = new NullExpr();
                continue;
            }

            IdentifierExpr *callName = new IdentifierExpr(name->name);
            delete expr;
            expr = new FuncCallExpr(callName, std::move(args));
        }

        return expr;
    }

    Expression *Parser::ParseOr()
    {
        Expression *expr = ParseAnd();
        while (Match(TokenType::Or))
        {
            TokenType op = Previous().GetTokenType();
            Expression *rhs = ParseAnd();
            expr = new BinaryExpr(expr, op, rhs);
        }
        return expr;
    }

    Expression *Parser::ParseAnd()
    {
        Expression *expr = ParseEquality();
        while (Match(TokenType::And))
        {
            TokenType op = Previous().GetTokenType();
            Expression *rhs = ParseEquality();
            expr = new BinaryExpr(expr, op, rhs);
        }
        return expr;
    }

    Expression *Parser::ParseEquality()
    {
        Expression *expr = ParseComparison();
        while (Match(TokenType::Equal) || Match(TokenType::NotEqual))
        {
            TokenType op = Previous().GetTokenType();
            Expression *rhs = ParseComparison();
            expr = new BinaryExpr(expr, op, rhs);
        }
        return expr;
    }

    Expression *Parser::ParseComparison()
    {
        Expression *expr = ParseTerm();
        while (Match(TokenType::Less) || Match(TokenType::LessEqual) || Match(TokenType::Greater) || Match(TokenType::GreaterEqual))
        {
            TokenType op = Previous().GetTokenType();
            Expression *rhs = ParseTerm();
            expr = new BinaryExpr(expr, op, rhs);
        }
        return expr;
    }

    Expression *Parser::ParseTerm()
    {
        Expression *expr = ParseFactor();
        while (Match(TokenType::Plus) || Match(TokenType::Minus))
        {
            TokenType op = Previous().GetTokenType();
            Expression *rhs = ParseFactor();
            expr = new BinaryExpr(expr, op, rhs);
        }
        return expr;
    }

    Expression *Parser::ParseFactor()
    {
        Expression *expr = ParseUnary();
        while (Match(TokenType::Star) || Match(TokenType::Slash) || Match(TokenType::Percent))
        {
            TokenType op = Previous().GetTokenType();
            Expression *rhs = ParseUnary();
            expr = new BinaryExpr(expr, op, rhs);
        }
        return expr;
    }

    Expression *Parser::ParseUnary()
    {
        if (Match(TokenType::Not) || Match(TokenType::Minus) || Match(TokenType::Plus))
        {
            TokenType op = Previous().GetTokenType();
            Expression *rhs = ParseUnary();
            return new UnaryExpr(op, rhs);
        }
        return ParseFuncCallExpr();
    }

    Expression *Parser::ParsePrimary()
    {
        if (Match(TokenType::Integer))
        {
            return new IntegerExpr(std::stoi(Previous().GetText()));
        }
        if (Match(TokenType::Float))
        {
            return new FloatExpr(std::stod(Previous().GetText()));
        }
        if (Match(TokenType::String))
        {
            return new StringExpr(Previous().GetText());
        }
        if (Match(TokenType::Null))
        {
            return new NullExpr();
        }
        if (Match(TokenType::True))
        {
            return new BoolExpr(true);
        }
        if (Match(TokenType::False))
        {
            return new BoolExpr(false);
        }
        if (Match(TokenType::LBracket))
        {
            while (!Check(TokenType::RBracket) && !Check(TokenType::End))
            {
                Expression *item = ParseExpression();
                delete item;
                if (!Match(TokenType::Comma))
                {
                    break;
                }
            }
            Consume(TokenType::RBracket, "Expected ']' after array literal");
            return new NullExpr();
        }
        if (Match(TokenType::Identifier) || Match(TokenType::This))
        {
            std::string name = Previous().GetText();
            while (Match(TokenType::Dot))
            {
                name = Consume(TokenType::Identifier, "Expected identifier after '.'").GetText();
            }
            return new IdentifierExpr(name);
        }
        if (Match(TokenType::LParen))
        {
            Expression *expr = ParseExpression();
            Consume(TokenType::RParen, "Expected ')' after expression");
            return expr;
        }

        const Token &token = Peek();
        RaiseParseError("Unexpected token '" + token.GetText() + "'", token);
    }

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

    bool Parser::CheckNext(TokenType type) const
    {
        if (m_Current + 1 >= m_Tokens.size())
        {
            return false;
        }
        return m_Tokens[m_Current + 1].GetTokenType() == type;
    }

    TokenType Parser::Type() const
    {
        return Peek().GetTokenType();
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
            RaiseParseError(message, Peek());
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

        RaiseParseError("Expected statement terminator", Peek());
    }

    void Parser::SkipNewlines()
    {
        while (Match(TokenType::Newline))
        {
        }
    }

    bool Parser::IsFunctionDeclAhead() const
    {
        if (!Check(TokenType::Identifier) || !CheckNext(TokenType::LParen))
        {
            return false;
        }

        std::size_t index = m_Current + 1;
        int depth = 0;
        while (index < m_Tokens.size())
        {
            TokenType type = m_Tokens[index].GetTokenType();
            if (type == TokenType::LParen)
            {
                ++depth;
            }
            else if (type == TokenType::RParen)
            {
                --depth;
                if (depth == 0)
                {
                    ++index;
                    break;
                }
            }
            ++index;
        }

        if (index >= m_Tokens.size())
        {
            return false;
        }

        if (index + 2 < m_Tokens.size() &&
            m_Tokens[index].GetTokenType() == TokenType::Minus &&
            m_Tokens[index + 1].GetTokenType() == TokenType::Greater &&
            m_Tokens[index + 2].GetTokenType() == TokenType::Identifier)
        {
            index += 3;
        }

        if (index >= m_Tokens.size())
        {
            return false;
        }

        TokenType tail = m_Tokens[index].GetTokenType();
        return tail == TokenType::Colon || tail == TokenType::LBrace;
    }

    bool Parser::IsNomalAssignmentAhead() const
    {
        if (!Check(TokenType::Identifier) && !Check(TokenType::This))
        {
            return false;
        }

        std::size_t index = m_Current + 1;
        while (index + 1 < m_Tokens.size() &&
               m_Tokens[index].GetTokenType() == TokenType::Dot &&
               m_Tokens[index + 1].GetTokenType() == TokenType::Identifier)
        {
            index += 2;
        }

        return index < m_Tokens.size() && m_Tokens[index].GetTokenType() == TokenType::Assign;
    }

    bool Parser::IsMemberAssignmentAhead() const
    {
        return IsNomalAssignmentAhead();
    }

    Expression *Parser::ParseAssignmentTarget()
    {
        const Token &base = Consume(Check(TokenType::This) ? TokenType::This : TokenType::Identifier, "Expected assignment target");
        std::string name = base.GetText();
        while (Match(TokenType::Dot))
        {
            name = Consume(TokenType::Identifier, "Expected member name").GetText();
        }
        return new IdentifierExpr(name);
    }

    error::DiagnosticContext Parser::MakeContext(const Token &token) const
    {
        error::DiagnosticContext context;
        context.fileName = m_FileName;
        context.moduleName = m_ModuleName;
        context.namespaceName = CurrentNamespacePath();
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

    std::string Parser::CurrentNamespacePath() const
    {
        std::string result;
        for (const auto &ns : m_NamespaceStack)
        {
            if (!result.empty())
            {
                result += "::";
            }
            result += ns;
        }
        return result;
    }

    constexpr int Parser::OperatorPriority(TokenType op)
    {
        switch (op)
        {
        case TokenType::Or:
            return 1;
        case TokenType::And:
            return 2;
        case TokenType::Equal:
        case TokenType::NotEqual:
            return 3;
        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual:
            return 4;
        case TokenType::Plus:
        case TokenType::Minus:
            return 5;
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
            return 6;
        default:
            return 0;
        }
    }

} // namespace cora::parser
