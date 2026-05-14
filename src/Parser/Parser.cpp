#include "Parser.hpp"

#include <stdexcept>

namespace cora::parser
{

    Parser::Parser(SourceManager &sm, DiagnosticEngine &de)
        : m_SourceManager(sm), m_DiagnosticEngine(de), m_Tokens({}), m_Current(0) {};

    Parser::Parser(SourceManager &sm, DiagnosticEngine &de, const std::vector<Token> &tokens)
        : m_SourceManager(sm), m_DiagnosticEngine(de), m_Tokens(tokens), m_Current(0) {};

    void Parser::SetFileID(uint32_t fileID)
    {
        m_FileID = fileID;
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
        uint32_t fileID = m_SourceManager.addFile("<memory>", source);
        Lexer lexer(m_SourceManager, m_DiagnosticEngine, fileID);
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

        if (Match(TokenType::Import) || Match(TokenType::From))
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
            // Look ahead to distinguish between traditional for and for-in
            if (Check(TokenType::LParen))
            {
                std::size_t savedCurrent = m_Current;
                Advance(); // Consume TokenType::LParen
                bool isForIn = false;
                // Scan for 'in' keyword within the for-loop header
                while (!Check(TokenType::RParen) && !Check(TokenType::End))
                {
                    if (Check(TokenType::In))
                    {
                        isForIn = true;
                        break;
                    }
                    Advance();
                }
                m_Current = savedCurrent; // Reset position

                if (isForIn)
                {
                    return ParseForInStmt();
                }
            }
            // If not for-in, it's a regular for loop
            return ParseForStmt();
        }
        // if (Match(TokenType::Do))
        // {
        //     Statement *body = ParseBlockStmt();
        //     if (Match(TokenType::While))
        //     {
        //         Consume(TokenType::LParen, "Expected '(' after while in do-while");
        //         Expression *condition = ParseExpression(); // Store the condition
        //         Consume(TokenType::RParen, "Expected ')' after do-while condition");
        //         ConsumeStatementTerminator();            // Ensure terminator is consumed
        //         return new DoWhileStmt(body, condition); // Create and return DoWhileStmt
        //     }
        //     ConsumeStatementTerminator(); // Ensure terminator is consumed for simple do-block
        //     return body;
        // }
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
        if (Match(TokenType::Class)) // Handle Class declaration
        {
            return ParseClassDeclStmt();
        }
        if (Match(TokenType::Enum)) // Handle Enum declaration
        {
            return ParseEnumDeclStmt();
        }
        if (Match(TokenType::Struct)) // Handle Struct declaration
        {
            return ParseStructDeclStmt();
        }
        if (Match(TokenType::Switch)) // Handle Switch statement
        {
            return ParseSwitchStmt();
        }
        if (Match(TokenType::Try)) // Handle Try-Catch statement
        {
            return ParseTryCatch();
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
        SkipNewlines();
        if (Match(TokenType::Else))
        {
            if (Match(TokenType::If))
            {
                auto *nestedIf = ParseIfStmt();
                std::vector<Statement *> stmts;
                stmts.push_back(nestedIf);
                falseBlock = new BlockStmt(std::move(stmts));
            }
            else
            {
                falseBlock = static_cast<BlockStmt *>(ParseBlockStmt());
            }
        }

        return new IfStmt(condition, trueBlock, falseBlock);
    }

    Statement *Parser::ParseForStmt()
    {
        // Ensure '(' is consumed before parsing for-in clauses
        Consume(TokenType::LParen, "Expected '(' after for-in");
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
            // This entire block is now handled by ParseForInStmt() from ParseStatement()
            // This original logic for `for...in` needs to be removed as ParseForStmt
            // should only handle the traditional for loop if isForIn is false.
            // For now, I'll keep it as it is in the original file, as the ParseStatement()
            // will dispatch to ParseForInStmt() if 'in' is detected earlier.
            // If ParseForStmt is reached, it implies it's a traditional for loop.
        }

        Statement *initializer = nullptr;
        if (!Check(TokenType::Semicolon))
        {
            if (Match(TokenType::Let) || Match(TokenType::Const))
            {
                const Token &nameToken = Consume(TokenType::Identifier, "Expected loop variable name");
                IdentifierExpr *typeExpr = nullptr; // For plan
                if (Match(TokenType::Colon))
                {
                    auto readTypeName = [this]() -> std::string
                    {
                        std::string typeName;

                        if (Check(TokenType::Identifier) || Check(TokenType::Int) || Check(TokenType::Str) || Check(TokenType::Void) || Check(TokenType::This))
                        {
                            typeName = Advance().GetText();
                        }
                        else
                        {
                            RaiseParseError("Expected type name", Peek());
                        }

                        while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
                        {
                            typeName += Previous().GetTokenType() == TokenType::Dot ? "." : "::";
                            typeName += Consume(TokenType::Identifier, "Expected identifier after type separator").GetText();
                        }

                        while (Match(TokenType::LBracket))
                        {
                            if (Check(TokenType::Integer))
                            {
                                typeName += "[" + Advance().GetText() + "]";
                            }
                            else
                            {
                                typeName += "[]";
                            }
                            Consume(TokenType::RBracket, "Expected ']' in type annotation");
                        }

                        return typeName;
                    };

                    typeExpr = new IdentifierExpr(readTypeName());
                }
                Consume(TokenType::Assign, "Expected '=' in variable declaration");
                initializer = new VarDeclStmt(new IdentifierExpr(nameToken.GetText()), ParseExpression(), typeExpr); // Updated
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
        // 'switch' token is already consumed by ParseStatement()
        Consume(TokenType::LParen, "Expected '(' after switch");
        Expression *condition = ParseExpression();
        Consume(TokenType::RParen, "Expected ')' after switch condition");

        Consume(TokenType::LBrace, "Expected '{' after switch condition");

        auto parseCaseBody = [this]() -> BlockStmt *
        {
            if (Check(TokenType::LBrace) || (Check(TokenType::Newline) && m_Current + 1 < m_Tokens.size() && m_Tokens[m_Current + 1].GetTokenType() == TokenType::Indent))
            {
                return static_cast<BlockStmt *>(ParseBlockStmt());
            }

            std::vector<Statement *> stmts;
            while (!Check(TokenType::Match) && !Check(TokenType::Default) && !Check(TokenType::RBrace) && !Check(TokenType::End) && !Check(TokenType::Dedent))
            {
                SkipNewlines();
                if (Check(TokenType::Match) || Check(TokenType::Default) || Check(TokenType::RBrace) || Check(TokenType::End) || Check(TokenType::Dedent))
                {
                    break;
                }

                Statement *statement = ParseStatement();
                if (statement != nullptr)
                {
                    stmts.push_back(statement);
                }
                SkipNewlines();
            }

            return new BlockStmt(std::move(stmts));
        };

        std::vector<MatchStmt *> matches;
        while (!Check(TokenType::RBrace))
        {
            SkipNewlines();
            if (Match(TokenType::Match))
            {
                // Parse individual match cases
                Expression *matchCond = ParseExpression();
                Consume(TokenType::Colon, "Expected ':' after match condition");
                auto *matchBlock = parseCaseBody();
                matches.push_back(new MatchStmt(matchCond, matchBlock));
            }
            else if (Match(TokenType::Default))
            {
                Consume(TokenType::Colon, "Expected ':' after default");
                auto *defaultBlock = parseCaseBody();
                matches.push_back(new MatchStmt(new NullExpr(), defaultBlock)); // Default case with NullExpr condition
                break;                                                          // Default is usually the last case
            }
            else
            {
                RaiseParseError("Expected 'match' or 'default' in switch statement", Peek());
            }
            SkipNewlines();
        }

        Consume(TokenType::RBrace, "Expected '}' after switch body");
        ConsumeStatementTerminator();
        return new SwitchStmt(condition, std::move(matches));
    }

    Statement *Parser::ParseMatchStmt()
    {
        // This method will be integrated into ParseSwitchStmt().
        // Keeping it as a placeholder or removing it if no longer needed standalone.
        Statement *block = ParseBlockStmt();
        delete block;
        return new PassStmt();
    }

    Statement *Parser::ParseFuncDeclStmt()
    {
        const Token &nameToken = Consume(TokenType::Identifier, "Expected function name");

        auto readTypeName = [this]() -> std::string
        {
            std::string typeName;

            if (Check(TokenType::Identifier) || Check(TokenType::Int) || Check(TokenType::Str) || Check(TokenType::Void) || Check(TokenType::This))
            {
                typeName = Advance().GetText();
            }
            else
            {
                RaiseParseError("Expected type name", Peek());
            }

            while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
            {
                typeName += Previous().GetTokenType() == TokenType::Dot ? "." : "::";
                typeName += Consume(TokenType::Identifier, "Expected identifier after type separator").GetText();
            }

            while (Match(TokenType::LBracket))
            {
                if (Check(TokenType::Integer))
                {
                    typeName += "[" + Advance().GetText() + "]";
                }
                else
                {
                    typeName += "[]";
                }
                Consume(TokenType::RBracket, "Expected ']' in type annotation");
            }

            return typeName;
        };

        Consume(TokenType::LParen, "Expected '(' after function name");
        std::vector<ParamExpr *> params;
        if (!Check(TokenType::RParen))
        {
            do
            {
                const Token &paramName = Consume(TokenType::Identifier, "Expected parameter name");
                IdentifierExpr *typeExpr = new IdentifierExpr("any");
                if (Match(TokenType::Colon))
                {
                    delete typeExpr;
                    typeExpr = new IdentifierExpr(readTypeName());
                }
                params.push_back(new ParamExpr(new IdentifierExpr(paramName.GetText()), typeExpr));
            } while (Match(TokenType::Comma));
        }
        Consume(TokenType::RParen, "Expected ')' after parameter list");

        IdentifierExpr *returnType = new IdentifierExpr("void");
        if (Match(TokenType::Arrow)) // Changed from TokenType::Minus followed by TokenType::Greater
        {
            delete returnType;
            returnType = new IdentifierExpr(readTypeName());
        }

        auto *body = static_cast<BlockStmt *>(ParseBlockStmt());
        return new FuncDeclStmt(new IdentifierExpr(nameToken.GetText()), std::move(params), body, returnType);
    }

    Statement *Parser::ParseVarDeclStmt()
    {
        const Token &nameToken = Consume(TokenType::Identifier, "Expected variable name");
        IdentifierExpr *typeExpr = nullptr; // Initialize to nullptr
        if (Match(TokenType::Colon))        // Check for type annotation
        {
            auto readTypeName = [this]() -> std::string
            {
                std::string typeName;

                if (Check(TokenType::Identifier) || Check(TokenType::Int) || Check(TokenType::Str) || Check(TokenType::Void) || Check(TokenType::This))
                {
                    typeName = Advance().GetText();
                }
                else
                {
                    RaiseParseError("Expected type annotation", Peek());
                }

                while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
                {
                    typeName += Previous().GetTokenType() == TokenType::Dot ? "." : "::";
                    typeName += Consume(TokenType::Identifier, "Expected identifier after type separator").GetText();
                }

                while (Match(TokenType::LBracket))
                {
                    if (Check(TokenType::Integer))
                    {
                        typeName += "[" + Advance().GetText() + "]";
                    }
                    else
                    {
                        typeName += "[]";
                    }
                    Consume(TokenType::RBracket, "Expected ']' in type annotation");
                }

                return typeName;
            };

            typeExpr = new IdentifierExpr(readTypeName()); // Create IdentifierExpr for the type
        }

        Expression *initializer = nullptr; // Initialize to nullptr
        if (Match(TokenType::Assign))
        {
            initializer = ParseExpression();
        }
        else
        {
            initializer = new NullExpr(); // If no assignment, provide a default initializer
        }

        ConsumeStatementTerminator();
        return new VarDeclStmt(new IdentifierExpr(nameToken.GetText()), initializer, typeExpr); // Pass typeExpr
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
        // 'class' token is already consumed by ParseStatement()
        const Token &nameToken = Consume(TokenType::Identifier, "Expected class name");

        Consume(TokenType::LBrace, "Expected '{' after class name");

        std::vector<FuncDeclStmt *> methods;
        m_ClassStack.push_back(nameToken.GetText()); // For context in error messages

        while (!Check(TokenType::RBrace))
        {
            SkipNewlines();
            if (Match(TokenType::Func))
            {
                methods.push_back(static_cast<FuncDeclStmt *>(ParseFuncDeclStmt()));
            }
            else
            {
                RaiseParseError("Expected function declaration in class body", Peek());
            }
            SkipNewlines();
        }

        m_ClassStack.pop_back();
        Consume(TokenType::RBrace, "Expected '}' after class body");
        ConsumeStatementTerminator();
        return new ClassDecl(new IdentifierExpr(nameToken.GetText()), std::move(methods));
    }

    Statement *Parser::ParseImportStmt()
    {
        auto readQualifiedName = [this]() -> std::string
        {
            const Token &first = Consume(Check(TokenType::String) ? TokenType::String : TokenType::Identifier, "Expected module name");
            std::string name = first.GetText();
            while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
            {
                name += Previous().GetTokenType() == TokenType::Dot ? "." : "::";
                name += Consume(TokenType::Identifier, "Expected identifier after module separator").GetText();
            }
            return name;
        };

        std::string moduleName;

        if (Previous().GetTokenType() == TokenType::From)
        {
            moduleName = readQualifiedName();
            Consume(TokenType::Import, "Expected 'import' after module name");

            while (!Check(TokenType::Semicolon) && !Check(TokenType::Newline) && !Check(TokenType::End))
            {
                if (Match(TokenType::As))
                {
                    moduleName = Consume(TokenType::Identifier, "Expected alias after 'as'").GetText();
                    break;
                }

                if (Check(TokenType::Identifier))
                {
                    Advance();
                    continue;
                }

                if (Match(TokenType::Comma))
                {
                    continue;
                }

                break;
            }
        }
        else
        {
            moduleName = readQualifiedName();

            if (Match(TokenType::From))
            {
                moduleName = readQualifiedName();
            }
            else if (Match(TokenType::As))
            {
                moduleName = Consume(TokenType::Identifier, "Expected alias after 'as'").GetText();
            }
        }

        while (!Check(TokenType::Semicolon) && !Check(TokenType::Newline) && !Check(TokenType::End))
        {
            Advance();
        }

        ConsumeStatementTerminator();
        return new ImportStmt(new IdentifierExpr(moduleName));
    }

    Expression *Parser::ParseStructLiteralExpr()
    {
        Consume(TokenType::LBrace, "Expected '{' for struct literal");

        std::vector<std::pair<IdentifierExpr *, Expression *>> fields;
        if (!Check(TokenType::RBrace))
        {
            do
            {
                const Token &fieldName = Consume(TokenType::Identifier, "Expected field name in struct literal");
                Consume(TokenType::Colon, "Expected ':' after field name in struct literal");
                fields.emplace_back(new IdentifierExpr(fieldName.GetText()), ParseExpression());
            } while (Match(TokenType::Comma));
        }

        Consume(TokenType::RBrace, "Expected '}' after struct literal");
        return new StructLiteralExpr(std::move(fields));
    }

    Statement *Parser::ParseTryCatch()
    {
        // 'try' token is already consumed by ParseStatement()
        auto *tryBlock = static_cast<BlockStmt *>(ParseBlockStmt());

        Consume(TokenType::Catch, "Expected 'catch' after 'try' block");

        IdentifierExpr *catchVar = nullptr;
        if (Match(TokenType::LParen))
        {
            const Token &varToken = Consume(TokenType::Identifier, "Expected identifier for catch variable");
            catchVar = new IdentifierExpr(varToken.GetText());
            Consume(TokenType::RParen, "Expected ')' after catch variable");
        }

        auto *catchBlock = static_cast<BlockStmt *>(ParseBlockStmt());

        return new TryCatchStmt(tryBlock, catchVar, catchBlock);
    }

    Statement *Parser::ParseStructDeclStmt()
    {
        // 'struct' token is already consumed by ParseStatement()
        const Token &nameToken = Consume(TokenType::Identifier, "Expected struct name");

        auto readTypeName = [this]() -> std::string
        {
            std::string typeName;

            if (Check(TokenType::Identifier) || Check(TokenType::Int) || Check(TokenType::Str) || Check(TokenType::Void) || Check(TokenType::This))
            {
                typeName = Advance().GetText();
            }
            else
            {
                RaiseParseError("Expected field type", Peek());
            }

            while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
            {
                typeName += Previous().GetTokenType() == TokenType::Dot ? "." : "::";
                typeName += Consume(TokenType::Identifier, "Expected identifier after type separator").GetText();
            }

            while (Match(TokenType::LBracket))
            {
                if (Check(TokenType::Integer))
                {
                    typeName += "[" + Advance().GetText() + "]";
                }
                else
                {
                    typeName += "[]";
                }
                Consume(TokenType::RBracket, "Expected ']' in field type");
            }

            return typeName;
        };

        Consume(TokenType::LBrace, "Expected '{' after struct name");

        std::vector<VarDeclStmt *> fields;
        while (!Check(TokenType::RBrace))
        {
            SkipNewlines();
            if (Check(TokenType::RBrace))
            {
                break;
            }

            const Token &fieldNameToken = Consume(TokenType::Identifier, "Expected field name");
            Consume(TokenType::Colon, "Expected ':' after field name");
            const std::string fieldTypeName = readTypeName();
            ConsumeStatementTerminator();

            fields.push_back(new VarDeclStmt(new IdentifierExpr(fieldNameToken.GetText()),
                                             new NullExpr(), // No initializer for struct fields
                                             new IdentifierExpr(fieldTypeName)));
        }

        Consume(TokenType::RBrace, "Expected '}' after struct fields");
        ConsumeStatementTerminator();
        return new StructDecl(new IdentifierExpr(nameToken.GetText()), std::move(fields));
    }

    Statement *Parser::ParseEnumDeclStmt()
    {
        // 'enum' token is already consumed by ParseStatement()
        const Token &nameToken = Consume(TokenType::Identifier, "Expected enum name");

        if (Match(TokenType::Colon))
        {
            if (Check(TokenType::Identifier) || Check(TokenType::Int) || Check(TokenType::Str) || Check(TokenType::Void))
            {
                Advance();
                while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
                {
                    Consume(TokenType::Identifier, "Expected identifier after enum base type separator");
                }

                while (Match(TokenType::LBracket))
                {
                    if (Check(TokenType::Integer))
                    {
                        Advance();
                    }
                    Consume(TokenType::RBracket, "Expected ']' in enum base type");
                }
            }
            else
            {
                RaiseParseError("Expected enum base type after ':'", Peek());
            }
        }

        Consume(TokenType::LBrace, "Expected '{' after enum name");

        std::vector<IdentifierExpr *> variants;
        if (!Check(TokenType::RBrace))
        {
            do
            {
                SkipNewlines();
                if (Check(TokenType::RBrace))
                {
                    break;
                }

                const Token &variantToken = Consume(TokenType::Identifier, "Expected enum variant");
                if (Match(TokenType::Assign))
                {
                    Expression *ignoredValue = ParseExpression();
                    delete ignoredValue;
                }
                variants.push_back(new IdentifierExpr(variantToken.GetText()));
            } while (Match(TokenType::Comma));
        }

        Consume(TokenType::RBrace, "Expected '}' after enum variants");
        ConsumeStatementTerminator();
        return new EnumDecl(new IdentifierExpr(nameToken.GetText()), std::move(variants));
    }

    Statement *Parser::ParseForInStmt()
    {
        // Consume '(' since lookahead in ParseStatement resets m_Current to point at '('
        Consume(TokenType::LParen, "Expected '(' after for-in");

        VarDeclStmt *variable = nullptr;
        auto readTypeName = [this]() -> std::string
        {
            std::string typeName;

            if (Check(TokenType::Identifier) || Check(TokenType::Int) || Check(TokenType::Str) || Check(TokenType::Void) || Check(TokenType::This))
            {
                typeName = Advance().GetText();
            }
            else
            {
                RaiseParseError("Expected type annotation for loop variable", Peek());
            }

            while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
            {
                typeName += Previous().GetTokenType() == TokenType::Dot ? "." : "::";
                typeName += Consume(TokenType::Identifier, "Expected identifier after type separator").GetText();
            }

            while (Match(TokenType::LBracket))
            {
                if (Check(TokenType::Integer))
                {
                    typeName += "[" + Advance().GetText() + "]";
                }
                else
                {
                    typeName += "[]";
                }
                Consume(TokenType::RBracket, "Expected ']' in type annotation");
            }

            return typeName;
        };

        if (Match(TokenType::Let) || Match(TokenType::Const))
        {
            const Token &nameToken = Consume(TokenType::Identifier, "Expected loop variable name");
            IdentifierExpr *typeExpr = nullptr;
            if (Match(TokenType::Colon))
            {
                typeExpr = new IdentifierExpr(readTypeName());
            }
            variable = new VarDeclStmt(new IdentifierExpr(nameToken.GetText()), new NullExpr(), typeExpr);
        }
        else if (Check(TokenType::Identifier))
        {
            const Token &nameToken = Consume(TokenType::Identifier, "Expected loop variable name");
            IdentifierExpr *typeExpr = nullptr;
            if (Match(TokenType::Colon))
            {
                typeExpr = new IdentifierExpr(readTypeName());
            }
            variable = new VarDeclStmt(new IdentifierExpr(nameToken.GetText()), new NullExpr(), typeExpr);
        }
        else
        {
            RaiseParseError("Expected loop variable in 'for-in' statement", Peek());
        }

        Consume(TokenType::In, "Expected 'in' keyword in 'for-in' statement");

        Expression *iterable = ParseExpression();

        Consume(TokenType::RParen, "Expected ')' after 'for-in' clauses");

        auto *body = static_cast<BlockStmt *>(ParseBlockStmt());
        return new ForInStmt(variable, iterable, body);
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
            std::vector<Expression *> elements; // Changed to Expression*
            if (!Check(TokenType::RBracket))
            {
                do
                {
                    elements.push_back(ParseExpression()); // Directly push Expression*
                } while (Match(TokenType::Comma));
            }
            Consume(TokenType::RBracket, "Expected ']' after array literal");
            return new ArrayExpr(std::move(elements)); // Assuming ArrayExpr constructor takes std::vector<Expression*>
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
        Expression *expr = ParseTernaryExpr();
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

    Expression *Parser::ParseTernaryExpr()
    {
        Expression *expr = ParseOr(); // Start with the lowest precedence binary operator

        if (Match(TokenType::Question))
        {
            Expression *thenExpr = ParseExpression(); // Parse the expression after '?'
            Consume(TokenType::Colon, "Expected ':' in ternary expression");
            Expression *elseExpr = ParseTernaryExpr(); // Recursively parse the else part
            return new ast::TernaryExpr(expr, thenExpr, elseExpr);
        }

        return expr;
    }

    Expression *Parser::ParseFuncCallExpr()
    {
        Expression *expr = ParsePrimary();

        while (Match(TokenType::LParen))
        {
            std::vector<Statement *> args; // Changed to Statement*
            if (!Check(TokenType::RParen))
            {
                do
                {
                    args.push_back(new ExprStmt(ParseExpression())); // Wrap expressions in ExprStmt
                } while (Match(TokenType::Comma));
            }
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

        // Handle postfix ++ and -- (tokenized as two Plus/Minus tokens)
        if (Check(TokenType::Plus) && CheckNext(TokenType::Plus))
        {
            Advance(); // consume first '+'
            Advance(); // consume second '+'
            expr = new ast::PostfixUnaryExpr(parser::TokenType::Plus, expr);
        }
        else if (Check(TokenType::Minus) && CheckNext(TokenType::Minus))
        {
            Advance(); // consume first '-'
            Advance(); // consume second '-'
            expr = new ast::PostfixUnaryExpr(parser::TokenType::Minus, expr);
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
        if (Check(TokenType::LBrace)) // Check for struct literal before other LBrace handling if any
        {
            return ParseStructLiteralExpr();
        }
        if (Match(TokenType::LBracket))
        {
            // This part should eventually return a proper ArrayExpr
            // For now, assuming it handles the tokens.
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
            Expression *currentExpr = new IdentifierExpr(name); // Start with IdentifierExpr

            while (Match(TokenType::Dot) || Match(TokenType::ScopeResolution))
            {
                TokenType opType = Previous().GetTokenType(); // Get the operator type
                const Token &nextTok = Peek();                // Peek at the next token

                if (opType == TokenType::Dot)
                {
                    // Handle struct member access
                    Consume(TokenType::Identifier, "Expected identifier after '.'");
                    // currentExpr = new StructAccess(currentExpr, new IdentifierExpr(nextTok.GetText()));
                }
                else // ScopeResolution
                {
                    // Handle scope resolution (e.g., MyNamespace::MyClass)
                    Consume(TokenType::Identifier, "Expected identifier after '::'");
                    name += "::" + nextTok.GetText();       // Append to the name string for this case
                    currentExpr = new IdentifierExpr(name); // Update to a new IdentifierExpr for scope resolution
                }
            }
            return currentExpr;
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
            name += "." + Consume(TokenType::Identifier, "Expected member name").GetText();
        }
        return new IdentifierExpr(name);
    }

    void Parser::RaiseParseError(const std::string &message, const Token &token) const
    {
        m_DiagnosticEngine.report(ErrorDiagnostic(ErrorCode::E0004, message, token.GetStartPosition()));
        throw std::runtime_error(message);
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