#include "Cora/Compiler/Parser/Parser.hpp"

#include <stdexcept>

namespace cora::compiler
{
    namespace parser
    {

        Parser::Parser() = default;

        Parser::Parser(const std::deque<Token> &tokens)
            : m_Tokens(tokens), m_Current(0) {}

        std::deque<ast::Statement *> Parser::ParseProgram(const std::string &source)
        {
            m_Tokens = m_Lexer.Lex(source);
            m_Current = 0;
            return ParseProgram();
        }

        std::deque<ast::Statement *> Parser::ParseProgram()
        {
            std::deque<Statement *> program;
            SkipNewlines();
            while (!Check(TokenType::End))
            {
                program.push_back(ParseStatement());
                SkipNewlines();
            }
            return program;
        }

        std::deque<Statement *> Parser::ParseBlockBody(TokenType blockEnd, bool useIndent)
        {
            std::deque<Statement *> statements;
            SkipNewlines();
            while (!Check(blockEnd) && !Check(TokenType::End))
            {
                statements.push_back(ParseStatement());
                SkipNewlines();
            }

            if (useIndent)
            {
                Consume(TokenType::Dedent, "Expected dedent to close block");
            }
            else
            {
                Consume(blockEnd, "Expected '}' to close block");
            }
            return statements;
        }

        Statement *Parser::ParseBlock()
        {
            auto *block = new BlockStmt();

            if (Match(TokenType::LBrace))
            {
                block->statements = ParseBlockBody(TokenType::RBrace, false);
                return block;
            }

            if (Match(TokenType::Colon))
            {
                Consume(TokenType::Newline, "Expected newline after ':'");
                Consume(TokenType::Indent, "Expected indented block");
                block->statements = ParseBlockBody(TokenType::Dedent, true);
                return block;
            }

            block->statements.push_back(ParseStatement());
            return block;
        }

        Statement *Parser::ParseStatement()
        {
            if (Match(TokenType::If))
            {
                return ParseIf();
            }
            if (Match(TokenType::While))
            {
                return ParseWhile();
            }
            if (Match(TokenType::For))
            {
                return ParseFor();
            }
            if (Match(TokenType::T_CLASS))
            {
                return ParseClassDecl();
            }
            if (Match(TokenType::T_FUN))
            {
                return ParseFunctionDecl(true);
            }
            if (Match(TokenType::T_RETURN))
            {
                return ParseReturn();
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
            if (Match(TokenType::Print))
            {
                return ParsePrint();
            }
            if (Match(TokenType::T_DELETE))
            {
                return ParseDelete();
            }
            if (Match(TokenType::Let))
            {
                return ParseVarDecl(std::nullopt);
            }
            if (Match(TokenType::Int))
            {
                return ParseVarDecl(std::string("int"));
            }
            if (Match(TokenType::Float))
            {
                return ParseVarDecl(std::string("float"));
            }
            if (Match(TokenType::Bool))
            {
                return ParseVarDecl(std::string("bool"));
            }
            if (Match(TokenType::StringType))
            {
                return ParseVarDecl(std::string("string"));
            }
            return ParseAssignOrExpr();
        }

        Statement *Parser::ParseIf()
        {
            auto *ifStmt = new IfStmt();
            Expression *condition = ParseExpression();
            BlockStmt *body = static_cast<BlockStmt *>(ParseBlock());
            ifStmt->branches.emplace_back(condition, body);

            while (Match(TokenType::Elif))
            {
                Expression *elifCondition = ParseExpression();
                BlockStmt *elifBody = static_cast<BlockStmt *>(ParseBlock());
                ifStmt->branches.emplace_back(elifCondition, elifBody);
            }

            if (Match(TokenType::Else))
            {
                ifStmt->elseBlock = static_cast<BlockStmt *>(ParseBlock());
            }

            return ifStmt;
        }

        Statement *Parser::ParseWhile()
        {
            Expression *condition = ParseExpression();
            BlockStmt *body = static_cast<BlockStmt *>(ParseBlock());
            return new WhileStmt(condition, body);
        }

        Statement *Parser::ParseFor()
        {
            if (Match(TokenType::LParen))
            {
                Statement *initializer = nullptr;
                if (!Check(TokenType::Semicolon))
                {
                    if (Match(TokenType::Let))
                    {
                        initializer = ParseVarDecl(std::nullopt, false);
                    }
                    else if (Match(TokenType::Int))
                    {
                        initializer = ParseVarDecl(std::string("int"), false);
                    }
                    else if (Match(TokenType::Float))
                    {
                        initializer = ParseVarDecl(std::string("float"), false);
                    }
                    else if (Match(TokenType::Bool))
                    {
                        initializer = ParseVarDecl(std::string("bool"), false);
                    }
                    else if (Match(TokenType::StringType))
                    {
                        initializer = ParseVarDecl(std::string("string"), false);
                    }
                    else
                    {
                        initializer = ParseAssignOrExpr(false);
                    }
                }
                Consume(TokenType::Semicolon, "Expected ';' after for initializer");

                Expression *condition = nullptr;
                if (!Check(TokenType::Semicolon))
                {
                    condition = ParseExpression();
                }
                Consume(TokenType::Semicolon, "Expected ';' after for condition");

                Statement *update = nullptr;
                if (!Check(TokenType::RParen))
                {
                    update = ParseAssignOrExpr(false);
                }
                Consume(TokenType::RParen, "Expected ')' after for clauses");

                BlockStmt *body = static_cast<BlockStmt *>(ParseBlock());
                return new ForCStyleStmt(initializer, condition, update, body);
            }

            const Token &nameToken = Consume(TokenType::Identifier, "Expected loop variable name");
            Consume(TokenType::In, "Expected 'in' in for-range loop");
            Consume(TokenType::Range, "Expected 'range' in for-range loop");
            Consume(TokenType::LParen, "Expected '(' after range");

            Expression *start = new LiteralExpr(0.0);
            Expression *end = nullptr;
            Expression *step = new LiteralExpr(1.0);

            Expression *first = ParseExpression();
            if (Match(TokenType::Comma))
            {
                delete start;
                start = first;
                end = ParseExpression();

                if (Match(TokenType::Comma))
                {
                    delete step;
                    step = ParseExpression();
                }
            }
            else
            {
                end = first;
            }

            Consume(TokenType::RParen, "Expected ')' after range arguments");
            BlockStmt *body = static_cast<BlockStmt *>(ParseBlock());
            return new ForRangeStmt(nameToken.GetText(), start, end, step, body);
        }

        Statement *Parser::ParseClassDecl()
        {
            const Token &nameToken = Consume(TokenType::Identifier, "Expected class name");
            auto *classBody = static_cast<BlockStmt *>(ParseBlock());

            std::deque<FunctionDeclStmt *> methods;
            for (Statement *statement : classBody->statements)
            {
                auto *method = dynamic_cast<FunctionDeclStmt *>(statement);
                if (method == nullptr)
                {
                    delete classBody;
                    for (FunctionDeclStmt *existing : methods)
                    {
                        delete existing;
                    }
                    throw std::runtime_error("Only method declarations are allowed inside class body");
                }
                methods.push_back(method);
            }

            classBody->statements.clear();
            delete classBody;
            return new ClassDeclStmt(nameToken.GetText(), std::move(methods));
        }

        Statement *Parser::ParseFunctionDecl(bool requireName)
        {
            std::string functionName;
            if (requireName)
            {
                functionName = Consume(TokenType::Identifier, "Expected function name").GetText();
            }
            else
            {
                functionName = "<anonymous>";
            }

            Consume(TokenType::LParen, "Expected '(' after function name");
            std::deque<std::string> parameters;
            if (!Check(TokenType::RParen))
            {
                do
                {
                    parameters.push_back(Consume(TokenType::Identifier, "Expected parameter name").GetText());
                } while (Match(TokenType::Comma));
            }
            Consume(TokenType::RParen, "Expected ')' after parameter list");

            auto *body = static_cast<BlockStmt *>(ParseBlock());
            return new FunctionDeclStmt(functionName, std::move(parameters), body);
        }

        Statement *Parser::ParseReturn()
        {
            Expression *value = nullptr;
            if (!Check(TokenType::Semicolon) && !Check(TokenType::Newline) && !Check(TokenType::Dedent) && !Check(TokenType::RBrace) && !Check(TokenType::End))
            {
                value = ParseExpression();
            }
            ConsumeStatementTerminator();
            return new ReturnStmt(value);
        }

        Statement *Parser::ParseVarDecl(std::optional<std::string> explicitType, bool consumeTerminator)
        {
            const Token &nameToken = Consume(TokenType::Identifier, "Expected variable name");
            Consume(TokenType::Assign, "Expected '=' in variable declaration");
            Expression *initializer = ParseExpression();
            if (consumeTerminator)
            {
                ConsumeStatementTerminator();
            }
            return new VarDeclStmt(nameToken.GetText(), explicitType, initializer);
        }

        Statement *Parser::ParseAssignOrExpr(bool consumeTerminator)
        {
            if (Check(TokenType::Identifier) && CheckNext(TokenType::Assign))
            {
                const Token &nameToken = Advance();
                Advance();
                Expression *expr = ParseExpression();
                if (consumeTerminator)
                {
                    ConsumeStatementTerminator();
                }
                return new AssignStmt(nameToken.GetText(), expr);
            }

            Expression *expr = ParseExpression();
            if (consumeTerminator)
            {
                ConsumeStatementTerminator();
            }
            return new ExprStmt(expr);
        }

        Statement *Parser::ParsePrint()
        {
            Consume(TokenType::LParen, "Expected '(' after print");
            auto *printStmt = new PrintStmt();

            if (!Check(TokenType::RParen))
            {
                do
                {
                    printStmt->expressions.push_back(ParseExpression());
                } while (Match(TokenType::Comma));
            }

            Consume(TokenType::RParen, "Expected ')' after print expression");
            ConsumeStatementTerminator();
            return printStmt;
        }

        Statement *Parser::ParseDelete()
        {
            Expression *target = ParseCall();
            if (dynamic_cast<VariableExpr *>(target) == nullptr && dynamic_cast<MemberExpr *>(target) == nullptr)
            {
                delete target;
                throw std::runtime_error("delete target must be a variable or member access");
            }
            ConsumeStatementTerminator();
            return new DeleteStmt(target);
        }

        Expression *Parser::ParseExpression()
        {
            return ParseOr();
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
            return ParseCall();
        }

        Expression *Parser::ParseCall()
        {
            Expression *expr = ParsePrimary();
            while (true)
            {
                if (Match(TokenType::LParen))
                {
                    std::deque<Expression *> arguments;
                    if (!Check(TokenType::RParen))
                    {
                        do
                        {
                            arguments.push_back(ParseExpression());
                        } while (Match(TokenType::Comma));
                    }
                    Consume(TokenType::RParen, "Expected ')' after arguments");
                    expr = new CallExpr(expr, std::move(arguments));
                    continue;
                }

                if (Match(TokenType::Dot))
                {
                    const Token &member = Consume(TokenType::Identifier, "Expected member name after '.'");
                    expr = new MemberExpr(expr, member.GetText());
                    continue;
                }

                break;
            }
            return expr;
        }

        Expression *Parser::ParseMember()
        {
            return ParsePrimary();
        }

        Expression *Parser::ParsePrimary()
        {
            if (Match(TokenType::Number))
            {
                return new LiteralExpr(std::stod(Previous().GetText()));
            }
            if (Match(TokenType::String))
            {
                return new LiteralExpr(Previous().GetText());
            }
            if (Match(TokenType::Null))
            {
                return new LiteralExpr(std::monostate{});
            }
            if (Match(TokenType::True))
            {
                return new LiteralExpr(true);
            }
            if (Match(TokenType::False))
            {
                return new LiteralExpr(false);
            }
            if (Match(TokenType::T_NEW))
            {
                const Token &className = Consume(TokenType::Identifier, "Expected class name after 'new'");
                Consume(TokenType::LParen, "Expected '(' after class name");
                std::deque<Expression *> args;
                if (!Check(TokenType::RParen))
                {
                    do
                    {
                        args.push_back(ParseExpression());
                    } while (Match(TokenType::Comma));
                }
                Consume(TokenType::RParen, "Expected ')' after constructor arguments");
                return new NewExpr(className.GetText(), std::move(args));
            }
            if (Match(TokenType::Identifier) || Match(TokenType::Int) || Match(TokenType::Float) || Match(TokenType::Bool) || Match(TokenType::StringType) || Match(TokenType::T_THIS))
            {
                return new VariableExpr(Previous().GetText());
            }
            if (Match(TokenType::LParen))
            {
                Expression *expr = ParseExpression();
                Consume(TokenType::RParen, "Expected ')' after expression");
                return expr;
            }

            const Token &token = Peek();
            throw std::runtime_error("Unexpected token '" + token.GetText() + "' at line " + std::to_string(token.GetStartPosition().Line()));
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
                throw std::runtime_error(message + " at line " + std::to_string(token.GetStartPosition().Line()) + ", col " + std::to_string(token.GetStartPosition().Column()));
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
            throw std::runtime_error("Expected statement terminator at line " + std::to_string(token.GetStartPosition().Line()));
        }

        void Parser::SkipNewlines()
        {
            while (Match(TokenType::Newline))
            {
            }
        }

    } // namespace parser

} // namespace cora::compiler
