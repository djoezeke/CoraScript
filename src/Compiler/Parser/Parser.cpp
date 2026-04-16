#include "Cora/Compiler/Runtime/Interpreter.hpp"

#include <stdexcept>

namespace cora
{
	namespace script
	{
		std::deque<Stmt *> Interpreter::Parse(const std::deque<Token> &tokens)
		{
			Parser parser(tokens);
			return parser.ParseProgram();
		}

		std::deque<Stmt *> Interpreter::Parser::ParseProgram()
		{
			std::deque<Stmt *> program;
			SkipNewlines();
			while (!Check(TokenType::End))
			{
				program.push_back(ParseStatement());
				SkipNewlines();
			}
			return std::move(program);
		}

		std::deque<Stmt *> Interpreter::Parser::ParseBlockBody(TokenType blockEnd, bool useIndent)
		{
			std::deque<Stmt *> statements;
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
			return std::move(statements);
		}

		BlockStmt *Interpreter::Parser::ParseBlock()
		{
			auto *block = new BlockStmt();

			if (Match(TokenType::LBrace))
			{
				block->statements = std::move(ParseBlockBody(TokenType::RBrace, false));
				return block;
			}

			if (Match(TokenType::Colon))
			{
				Consume(TokenType::Newline, "Expected newline after ':'");
				Consume(TokenType::Indent, "Expected indented block");
				block->statements = std::move(ParseBlockBody(TokenType::Dedent, true));
				return block;
			}

			block->statements.push_back(ParseStatement());
			return block;
		}

		Stmt *Interpreter::Parser::ParseStatement()
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

		Stmt *Interpreter::Parser::ParseIf()
		{
			auto *ifStmt = new IfStmt();

			Expr *condition = ParseExpression();
			BlockStmt *body = ParseBlock();
			ifStmt->branches.emplace_back(condition, body);

			while (Match(TokenType::Elif))
			{
				Expr *elifCondition = ParseExpression();
				BlockStmt *elifBody = ParseBlock();
				ifStmt->branches.emplace_back(elifCondition, elifBody);
			}

			while (Match(TokenType::Else))
			{
				if (Match(TokenType::If))
				{
					Expr *elseIfCondition = ParseExpression();
					BlockStmt *elseIfBody = ParseBlock();
					ifStmt->branches.emplace_back(elseIfCondition, elseIfBody);
					continue;
				}

				ifStmt->elseBlock = ParseBlock();
				break;
			}

			return ifStmt;
		}

		Stmt *Interpreter::Parser::ParseWhile()
		{
			Expr *condition = ParseExpression();
			BlockStmt *body = ParseBlock();
			return new WhileStmt(condition, body);
		}

		Stmt *Interpreter::Parser::ParseFor()
		{
			if (Match(TokenType::LParen))
			{
				Stmt *initializer = nullptr;
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

				Expr *condition = nullptr;
				if (!Check(TokenType::Semicolon))
				{
					condition = ParseExpression();
				}
				Consume(TokenType::Semicolon, "Expected ';' after for condition");

				Stmt *update = nullptr;
				if (!Check(TokenType::RParen))
				{
					update = ParseAssignOrExpr(false);
				}
				Consume(TokenType::RParen, "Expected ')' after for clauses");

				BlockStmt *body = ParseBlock();
				return new ForCStyleStmt(initializer, condition, update, body);
			}

			const Token &nameToken = Consume(TokenType::Identifier, "Expected loop variable name");
			Consume(TokenType::In, "Expected 'in' in for-range loop");
			Consume(TokenType::Range, "Expected 'range' in for-range loop");
			Consume(TokenType::LParen, "Expected '(' after range");

			Expr *start = new LiteralExpr(0.0);
			Expr *end = nullptr;
			Expr *step = new LiteralExpr(1.0);

			Expr *first = ParseExpression();
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

			BlockStmt *body = ParseBlock();
			return new ForRangeStmt(nameToken.text, start, end, step, body);
		}

		Stmt *Interpreter::Parser::ParseVarDecl(std::optional<std::string> explicitType, bool consumeTerminator)
		{
			const Token &nameToken = Consume(TokenType::Identifier, "Expected variable name");
			Consume(TokenType::Assign, "Expected '=' in variable declaration");
			Expr *initializer = ParseExpression();
			if (consumeTerminator)
			{
				ConsumeStatementTerminator();
			}
			return new VarDeclStmt(nameToken.text, explicitType, initializer);
		}

		Stmt *Interpreter::Parser::ParseAssignOrExpr(bool consumeTerminator)
		{
			if (Check(TokenType::Identifier) && CheckNext(TokenType::Assign))
			{
				const Token &nameToken = Advance();
				Advance();
				Expr *expr = ParseExpression();
				if (consumeTerminator)
				{
					ConsumeStatementTerminator();
				}
				return new AssignStmt(nameToken.text, expr);
			}

			Expr *expr = ParseExpression();
			if (consumeTerminator)
			{
				ConsumeStatementTerminator();
			}
			return new ExprStmt(expr);
		}

		Stmt *Interpreter::Parser::ParsePrint()
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

		Expr *Interpreter::Parser::ParseExpression()
		{
			return ParseOr();
		}

		Expr *Interpreter::Parser::ParseOr()
		{
			Expr *expr = ParseAnd();
			while (Match(TokenType::Or))
			{
				TokenType op = Previous().type;
				Expr *rhs = ParseAnd();
				expr = new BinaryExpr(expr, op, rhs);
			}
			return expr;
		}

		Expr *Interpreter::Parser::ParseAnd()
		{
			Expr *expr = ParseEquality();
			while (Match(TokenType::And))
			{
				TokenType op = Previous().type;
				Expr *rhs = ParseEquality();
				expr = new BinaryExpr(expr, op, rhs);
			}
			return expr;
		}

		Expr *Interpreter::Parser::ParseEquality()
		{
			Expr *expr = ParseComparison();
			while (Match(TokenType::Equal) || Match(TokenType::NotEqual))
			{
				TokenType op = Previous().type;
				Expr *rhs = ParseComparison();
				expr = new BinaryExpr(expr, op, rhs);
			}
			return expr;
		}

		Expr *Interpreter::Parser::ParseComparison()
		{
			Expr *expr = ParseTerm();
			while (Match(TokenType::Less) || Match(TokenType::LessEqual) || Match(TokenType::Greater) || Match(TokenType::GreaterEqual))
			{
				TokenType op = Previous().type;
				Expr *rhs = ParseTerm();
				expr = new BinaryExpr(expr, op, rhs);
			}
			return expr;
		}

		Expr *Interpreter::Parser::ParseTerm()
		{
			Expr *expr = ParseFactor();
			while (Match(TokenType::Plus) || Match(TokenType::Minus))
			{
				TokenType op = Previous().type;
				Expr *rhs = ParseFactor();
				expr = new BinaryExpr(expr, op, rhs);
			}
			return expr;
		}

		Expr *Interpreter::Parser::ParseFactor()
		{
			Expr *expr = ParseUnary();
			while (Match(TokenType::Star) || Match(TokenType::Slash) || Match(TokenType::Percent))
			{
				TokenType op = Previous().type;
				Expr *rhs = ParseUnary();
				expr = new BinaryExpr(expr, op, rhs);
			}
			return expr;
		}

		Expr *Interpreter::Parser::ParseUnary()
		{
			if (Match(TokenType::Not) || Match(TokenType::Minus) || Match(TokenType::Plus))
			{
				TokenType op = Previous().type;
				Expr *rhs = ParseUnary();
				return new UnaryExpr(op, rhs);
			}
			return ParsePrimary();
		}

		Expr *Interpreter::Parser::ParsePrimary()
		{
			if (Match(TokenType::Number))
			{
				return new LiteralExpr(std::stod(Previous().text));
			}
			if (Match(TokenType::String))
			{
				return new LiteralExpr(Previous().text);
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
			if (Match(TokenType::Identifier))
			{
				return new VariableExpr(Previous().text);
			}
			if (Match(TokenType::LParen))
			{
				Expr *expr = ParseExpression();
				Consume(TokenType::RParen, "Expected ')' after expression");
				return expr;
			}

			const Token &token = Peek();
			throw std::runtime_error("Unexpected token '" + token.text + "' at line " + std::to_string(token.line));
		}

		bool Interpreter::Parser::Match(TokenType type)
		{
			if (Check(type))
			{
				Advance();
				return true;
			}
			return false;
		}

		bool Interpreter::Parser::Check(TokenType type) const
		{
			return Peek().type == type;
		}

		bool Interpreter::Parser::CheckNext(TokenType type) const
		{
			if (m_Current + 1 >= m_Tokens.size())
			{
				return false;
			}
			return m_Tokens[m_Current + 1].type == type;
		}

		const Token &Interpreter::Parser::Advance()
		{
			if (!Check(TokenType::End))
			{
				++m_Current;
			}
			return Previous();
		}

		const Token &Interpreter::Parser::Peek() const
		{
			return m_Tokens[m_Current];
		}

		const Token &Interpreter::Parser::Previous() const
		{
			return m_Tokens[m_Current - 1];
		}

		const Token &Interpreter::Parser::Consume(TokenType type, const std::string &message)
		{
			if (!Check(type))
			{
				const Token &token = Peek();
				throw std::runtime_error(message + " at line " + std::to_string(token.line) + ", col " + std::to_string(token.column));
			}
			return Advance();
		}

		void Interpreter::Parser::ConsumeStatementTerminator()
		{
			if (Match(TokenType::Semicolon))
			{
				return;
			}
			if (Check(TokenType::Newline) || Check(TokenType::Dedent) || Check(TokenType::RBrace) || Check(TokenType::End))
			{
				Match(TokenType::Newline);
				return;
			}

			const Token &token = Peek();
			throw std::runtime_error("Expected statement terminator at line " + std::to_string(token.line));
		}

		void Interpreter::Parser::SkipNewlines()
		{
			while (Match(TokenType::Newline))
			{
			}
		}
	}
}
