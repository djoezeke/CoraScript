#ifndef CORA_COMPILER_PARSER_TOKEN_H
#define CORA_COMPILER_PARSER_TOKEN_H

#include "Cora/Compiler/Basic/SourceLocation.h"

#include <ostream>
#include <string>

namespace cora::compiler
{
	using namespace basic;

	namespace parser
	{

		enum class TokenType
		{

			End,
			Newline,
			Indent,
			Dedent,
			Identifier,
			Number,
			String,
			Null,
			True,
			False,
			Let,
			Int,
			Float,
			Bool,
			StringType,
			If,
			Elif,
			Else,
			While,
			For,
			In,
			Range,
			Break,
			Continue,
			Pass,
			Print,
			And,
			Or,
			Not,
			LParen,
			RParen,
			LBrace,
			RBrace,
			Colon,
			Comma,
			Semicolon,
			Assign,
			Plus,
			Minus,
			Star,
			Slash,
			Percent,
			Dot,
			Equal,
			NotEqual,
			Less,
			LessEqual,
			Greater,
			GreaterEqual,

			T_UNKNOW,

			// others
			T_NEWLINE,
			T_INDENT,
			T_DEDENT,
			T_IGNORE,
			T_COMMENT,

			// DATA TYPES
			T_IDENTIFIER,
			T_INTEGER,
			T_NUMBER,
			T_CHARATER,
			T_BOOLEAN,
			T_BYTE,
			T_DOC,
			T_STRING,
			T_RAWSTRING,
			T_BYTESTRING,
			T_BYTERAWSTRING,

			// symbols
			T_LPAR,
			T_RPAR,
			T_LSQB,
			T_RSQB,
			T_COLON,
			T_COMMA,
			T_SEMI,
			T_UNDERSCORE,

			// operators
			T_PLUS,
			T_MINUS,
			T_STAR,
			T_SLASH,
			T_VBAR,
			T_AMPER,
			T_LESS,
			T_GREATER,
			T_EQUAL,
			T_DOT,
			T_PERCENT,
			T_LBRACE,
			T_RBRACE,
			T_EQEQUAL,
			T_NOTEQUAL,
			T_LESSEQUAL,
			T_GREATEREQUAL,
			T_TILDE,
			T_LEFTSHIFT,
			T_RIGHTSHIFT,
			T_DOUBLESTAR,
			T_PLUSEQUAL,
			T_MINEQUAL,
			T_STAREQUAL,
			T_SLASHEQUAL,
			T_PERCENTEQUAL,
			T_AMPEREQUAL,
			T_VBAREQUAL,
			T_CIRCUMFLEXEQUAL,
			T_LEFTSHIFTEQUAL,
			T_RIGHTSHIFTEQUAL,
			T_DOUBLESTAREQUAL,
			T_DOUBLESLASH,
			T_DOUBLESLASHEQUAL,
			T_AT,
			T_ATEQUAL,
			T_RARROW,
			T_ELLIPSIS,
			T_COLONEQUAL,
			T_EXCLAMATION,

			// keywords
			T_INT,
			T_FLOAT,
			T_DOUBLE,
			T_CHAR,
			T_BOOL,
			T_VOID,

			// MODIFIERS
			T_SIGNED,
			T_UNSIGNED,
			T_SHORT,
			T_LONG,

			// CONTROL FLOW
			T_IF,
			T_ELSE,
			T_SWITCH,
			T_CASE,
			T_DEFAULT,
			T_FOR,
			T_WHILE,
			T_GOTO,
			T_DO,
			T_BREAK,
			T_CONTINUE,

			//  CLASS
			T_CLASS,
			T_STRUCT,
			T_UNION,
			T_PRIVATE,
			T_PROTECTED,
			T_PUBLIC,
			T_SELF,

			// EXCEPTIONS
			T_TRY,
			T_CATCH,
			T_THROW,

			// OPERATORS
			T_SIZEOF,
			T_TYPEID,
			T_TYPEOF,
			T_OFFSETOF,

			// MEMORY
			T_NEW,
			T_DELETE,

			// FUNCTION
			T_FUN,
			T_RETURN,

			// STORAGE CLASSES
			T_STATIC,
			T_EXTERN,
			T_CONST,
			T_THIS,

			T_EOF,

		};

		class Token
		{
		public:
			Token();

			Token(TokenType tokentype, unsigned int line, unsigned int column);

			Token(TokenType tokentype, std::string tokentext, unsigned int line, unsigned int column);

			/**
			 * @brief Compute a representation of the token.
			 * @return String representation of the token
			 */
			std::string Repr();

			/**
			 * @brief Return the token kind
			 *
			 * @return TokenType
			 */
			TokenType GetTokenType() const;

			/**
			 * @brief Return the token kind string
			 *
			 * @return const std::string&
			 */
			std::string GetTokenTypeString() const;

			bool Is(TokenType tokentype) const;

			bool IsNot(TokenType tokentype) const;

			bool IsAny(TokenType tokentype) const;

			template <typename... T>
			bool IsAny(TokenType kind1, TokenType kind2, T... tokentypes) const;

			template <typename... T>
			bool IsNotAny(TokenType tokentype, T... tokentypes) const;

			std::string GetText() const;

			unsigned int GetLenght() const;

			/**
			 * @brief Get the start position of the node
			 *
			 * @return Position
			 */
			SourceLocation GetStartPosition() const noexcept;

			/**
			 * @brief Get the end position of the node
			 *
			 * @return Position
			 */
			SourceLocation GetEndPosition() const noexcept;

			/**
			 * @brief Get the source range of the node
			 *
			 * @return SourceRange
			 */
			SourceRange GetSourceRange() const;

			void SetText(std::string text);

			/**
			 * @brief Set the token kind object
			 *
			 * @param tokentype
			 */
			void SetTokenType(TokenType tokentype) noexcept;

			/**
			 * @brief Set the Node start position
			 *
			 * @param start
			 */
			void SetStartPosition(SourceLocation start) noexcept;

			/**
			 * @brief Set the Node end position
			 *
			 * @param end
			 */
			void SetEndPosition(SourceLocation end) noexcept;

			/**
			 * @brief Set the Node source range.
			 *
			 * @param range
			 */
			void SetSourceRange(SourceRange range) noexcept;

		private:
			TokenType m_Type;
			SourceRange m_Range;
			std::string m_TokenText;
		};

		std::ostream &operator<<(std::ostream &ostream, Token token);

	} // namespace parser

} // namespace cora::compiler

#endif // CORA_COMPILER_PARSER_TOKEN_H
