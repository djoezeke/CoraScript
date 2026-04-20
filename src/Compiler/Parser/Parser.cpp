#include "Cora/Compiler/Parser/Parser.hpp"

#include <stdexcept>

namespace cora::compiler
{
    namespace parser
    {

        Parser::Parser() = default;

        Parser::Parser(const std::deque<Token> &tokens)
            : m_Tokens(tokens), m_Current(0) {}

        void Parser::SetFileName(std::string fileName)
        {
            m_FileName = fileName.empty() ? "<memory>" : std::move(fileName);
            m_Lexer.SetFileName(m_FileName);
        }

        void Parser::SetModuleName(std::string moduleName)
        {
            m_ModuleName = std::move(moduleName);
            m_Lexer.SetModuleName(m_ModuleName);
        }

        std::deque<ast::Statement *> Parser::ParseProgram(const std::string &source)
        {
            m_Lexer.SetFileName(m_FileName);
            m_Lexer.SetModuleName(m_ModuleName);
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
                const Token start = Peek();
                Statement *statement = ParseStatement();
                statement->SetStartPosition(start.GetStartPosition());
                program.push_back(statement);
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
                const Token start = Peek();
                Statement *statement = ParseStatement();
                statement->SetStartPosition(start.GetStartPosition());
                statements.push_back(statement);
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

            SkipNewlines();

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
            std::optional<ast::AccessModifier> accessModifier = ParseOptionalAccessModifier();
            if (accessModifier.has_value())
            {
                if (Match(TokenType::T_FUN))
                {
                    return ParseFunctionDecl(true, accessModifier.value());
                }
                if (Check(TokenType::Identifier) && CheckNext(TokenType::LParen) && IsFunctionDeclAhead())
                {
                    return ParseFunctionDecl(true, accessModifier.value());
                }
                if (Match(TokenType::Let))
                {
                    return ParseVarDeclaration(true);
                }
                RaiseParseError("Access modifier must precede a function or variable declaration", Peek());
            }

            if (Match(TokenType::T_NAMESPACE))
            {
                return ParseNamespaceDecl();
            }
            if (Match(TokenType::Import))
            {
                return ParseImport();
            }
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
            if (Check(TokenType::Identifier) && CheckNext(TokenType::LParen) && IsFunctionDeclAhead())
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
            if (Match(TokenType::T_DELETE))
            {
                return ParseDelete();
            }
            if (Match(TokenType::Let))
            {
                return ParseVarDeclaration();
            }
            if (Match(TokenType::Const))
            {
                return ParseVarDeclaration();
            }
            return ParseAssignment();
        }

        Statement *Parser::ParseNamespaceDecl()
        {
            std::string namespaceName = Consume(TokenType::Identifier, "Expected namespace name").GetText();
            while (Match(TokenType::Dot))
            {
                namespaceName += "." + Consume(TokenType::Identifier, "Expected identifier after '.' in namespace name").GetText();
            }

            m_NamespaceStack.push_back(namespaceName);
            BlockStmt *body = nullptr;
            try
            {
                body = static_cast<BlockStmt *>(ParseBlock());
            }
            catch (...)
            {
                m_NamespaceStack.pop_back();
                throw;
            }
            m_NamespaceStack.pop_back();
            return new NamespaceDeclStmt(namespaceName, body);
        }

        Statement *Parser::ParseImport()
        {
            std::string moduleName = Consume(TokenType::Identifier, "Expected module name after 'import'").GetText();
            while (Match(TokenType::Dot))
            {
                moduleName += "." + Consume(TokenType::Identifier, "Expected identifier after '.' in import path").GetText();
            }
            ConsumeStatementTerminator();
            return new ImportStmt(moduleName);
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
                        initializer = ParseVarDeclaration(false);
                    }
                    else if (Match(TokenType::Const))
                    {
                        initializer = ParseVarDeclaration(false);
                    }
                    else
                    {
                        initializer = ParseAssignment(false);
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
                    update = ParseAssignment(false);
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
            m_ClassStack.push_back(nameToken.GetText());
            BlockStmt *classBody = nullptr;
            try
            {
                classBody = static_cast<BlockStmt *>(ParseBlock());
            }
            catch (...)
            {
                m_ClassStack.pop_back();
                throw;
            }
            m_ClassStack.pop_back();

            std::deque<VarDeclStmt *> fields;
            std::deque<FunctionDeclStmt *> methods;
            for (Statement *statement : classBody->statements)
            {
                auto *method = dynamic_cast<FunctionDeclStmt *>(statement);
                if (method != nullptr)
                {
                    methods.push_back(method);
                    continue;
                }

                auto *field = dynamic_cast<VarDeclStmt *>(statement);
                if (field != nullptr)
                {
                    fields.push_back(field);
                    continue;
                }

                delete classBody;
                for (VarDeclStmt *existing : fields)
                {
                    delete existing;
                }
                for (FunctionDeclStmt *existing : methods)
                {
                    delete existing;
                }
                RaiseParseError("Only member fields and methods are allowed inside class body", nameToken);
            }

            classBody->statements.clear();
            delete classBody;
            return new ClassDeclStmt(nameToken.GetText(), std::move(fields), std::move(methods));
        }

        Statement *Parser::ParseFunctionDecl(bool requireName, ast::AccessModifier access)
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

            std::optional<std::string> returnType;
            if (Match(TokenType::Minus))
            {
                Consume(TokenType::Greater, "Expected '>' after '-' in return type annotation");
                returnType = Consume(TokenType::Identifier, "Expected return type after '->'").GetText();
            }

            m_FunctionStack.push_back(functionName);
            BlockStmt *body = nullptr;
            try
            {
                body = static_cast<BlockStmt *>(ParseBlock());
            }
            catch (...)
            {
                m_FunctionStack.pop_back();
                throw;
            }
            m_FunctionStack.pop_back();
            return new FunctionDeclStmt(functionName, std::move(parameters), body, access, returnType);
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

        Statement *Parser::ParseVarDecl(std::optional<std::string> explicitType, bool consumeTerminator, ast::AccessModifier access)
        {
            const Token &nameToken = Consume(TokenType::Identifier, "Expected variable name");
            Consume(TokenType::Assign, "Expected '=' in variable declaration");
            Expression *initializer = ParseExpression();
            if (consumeTerminator)
            {
                ConsumeStatementTerminator();
            }
            return new VarDeclStmt(nameToken.GetText(), explicitType, initializer, access);
        }

        Statement *Parser::ParseAssignment(bool consumeTerminator)
        {
            if (IsMemberAssignmentAhead())
            {
                Expression *target = ParseAssignmentTarget();

                if (Match(TokenType::Colon))
                {
                    (void)Consume(TokenType::Identifier, "Expected type name after ':'");
                }

                Consume(TokenType::Assign, "Expected '=' in assignment");
                Expression *expr = ParseExpression();
                if (consumeTerminator)
                {
                    ConsumeStatementTerminator();
                }
                return new Assignment(target, expr);
            }

            if (IsNomalAssignmentAhead())
            {
                Expression *target = ParseAssignmentTarget();

                if (Match(TokenType::Colon))
                {
                    (void)Consume(TokenType::Identifier, "Expected type name after ':'");
                }

                Consume(TokenType::Assign, "Expected '=' in assignment");
                Expression *expr = ParseExpression();
                if (consumeTerminator)
                {
                    ConsumeStatementTerminator();
                }
                return new Assignment(target, expr);
            }

            Expression *expr = ParseExpression();
            if (consumeTerminator)
            {
                ConsumeStatementTerminator();
            }
            return new ExprStmt(expr);
        }

        Statement *Parser::ParseVarDeclaration(bool consumeTerminator)
        {
            const Token &name = Consume(TokenType::Identifier, "Expected variable name");
            Consume(TokenType::Colon, "Expected ':' in variable declaration");
            const Token &type = Consume(TokenType::Identifier, "Expected type name in variable declaration");
            Consume(TokenType::Assign, "Expected '=' in variable declaration");
            Expression *initializer = ParseExpression();
            if (consumeTerminator)
            {
                ConsumeStatementTerminator();
            }
            return new VarDeclaration(name.GetText(), type.GetText(), initializer);
        }

        Statement *Parser::ParseDelete()
        {
            Expression *target = ParseCall();
            if (dynamic_cast<VariableExpr *>(target) == nullptr && dynamic_cast<MemberExpr *>(target) == nullptr)
            {
                delete target;
                RaiseParseError("delete target must be a variable or member access", Peek());
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
                std::string className = Consume(TokenType::Identifier, "Expected class name after 'new'").GetText();
                while (Match(TokenType::Dot))
                {
                    className += "." + Consume(TokenType::Identifier, "Expected identifier after '.' in class name").GetText();
                }
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
                return new NewExpr(std::move(className), std::move(args));
            }
            if (Match(TokenType::Identifier) || Match(TokenType::T_THIS))
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

            const TokenType afterParams = m_Tokens[index].GetTokenType();
            return afterParams == TokenType::Colon || afterParams == TokenType::LBrace;
        }

        bool Parser::IsMemberAssignmentAhead() const
        {
            if (!Check(TokenType::Identifier) && !Check(TokenType::T_THIS))
            {
                return false;
            }

            std::size_t index = m_Current;
            if (index >= m_Tokens.size())
            {
                return false;
            }

            ++index;
            while (index + 1 < m_Tokens.size() && m_Tokens[index].GetTokenType() == TokenType::Dot && m_Tokens[index + 1].GetTokenType() == TokenType::Identifier)
            {
                index += 2;
            }

            if (index + 2 < m_Tokens.size() && m_Tokens[index].GetTokenType() == TokenType::Colon && m_Tokens[index + 1].GetTokenType() == TokenType::Identifier)
            {
                index += 2;
            }

            return index < m_Tokens.size() && m_Tokens[index].GetTokenType() == TokenType::Assign;
        }

        bool Parser::IsNomalAssignmentAhead() const
        {
            if (!Check(TokenType::Identifier) && !Check(TokenType::T_THIS))
            {
                return false;
            }

            std::size_t index = m_Current;
            if (index >= m_Tokens.size())
            {
                return false;
            }

            ++index;
            if (index + 2 < m_Tokens.size() && m_Tokens[index].GetTokenType() == TokenType::Colon && m_Tokens[index + 1].GetTokenType() == TokenType::Identifier)
            {
                index += 2;
            }

            if (index + 1 < m_Tokens.size() && m_Tokens[index].GetTokenType() == TokenType::Assign)
            {
                return true;
            }

            return false;
        }

        Expression *Parser::ParseAssignmentTarget()
        {
            const Token &base = Consume(Check(TokenType::T_THIS) ? TokenType::T_THIS : TokenType::Identifier, "Expected assignment target");
            Expression *target = new VariableExpr(base.GetText());

            while (Match(TokenType::Dot))
            {
                const Token &member = Consume(TokenType::Identifier, "Expected member name after '.'");
                target = new MemberExpr(target, member.GetText());
            }

            return target;
        }

        std::optional<ast::AccessModifier> Parser::ParseOptionalAccessModifier()
        {
            if (Match(TokenType::T_PUBLIC))
            {
                return ast::AccessModifier::Public;
            }
            if (Match(TokenType::T_PRIVATE))
            {
                return ast::AccessModifier::Private;
            }
            return std::nullopt;
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

    } // namespace parser

} // namespace cora::compiler
