#ifndef CORA_PARSER_TOKEN_H
#define CORA_PARSER_TOKEN_H

#include "Cora/Basic/Location.hpp"

#include <ostream>
#include <string>

namespace cora::parser
{
	using namespace cora;

	enum class TokenType
	{
		Identifier,
		Integer,
		String,
		Float,
		Bool,
		Int,
		Str,
		Void,
		ScopeResolution,

		End,
		Newline,
		Indent,
		Dedent,
		Null,
		True,
		False,
		Let,
		Const,
		If,
		Elif,
		Else,
		While,
		For,
		Do,
		Func,
		Return,
		Class,
		Enum,
		Struct,
		Switch,
		Match,
		Default,
		Try,
		Catch,
		This,
		Import,
		In,
		Range,
		Break,
		Continue,
		Pass,
		From,
		As,
		Print,
		And,
		Or,
		Not,
		LParen,
		RParen,
		LBracket,
		RBracket,
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
		Arrow,
		Question,
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
		basic::SourceLocation GetStartPosition() const noexcept;

		/**
		 * @brief Get the end position of the node
		 *
		 * @return Position
		 */
		basic::SourceLocation GetEndPosition() const noexcept;

		/**
		 * @brief Get the source range of the node
		 *
		 * @return basic::SourceRange
		 */
		basic::SourceRange GetSourceRange() const;

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
		void SetStartPosition(basic::SourceLocation start) noexcept;

		/**
		 * @brief Set the Node end position
		 *
		 * @param end
		 */
		void SetEndPosition(basic::SourceLocation end) noexcept;

		/**
		 * @brief Set the Node source range.
		 *
		 * @param range
		 */
		void SetSourceRange(basic::SourceRange range) noexcept;

	private:
		TokenType m_Type;
		basic::SourceRange m_Range;
		std::string m_TokenText;
	};

	std::ostream &operator<<(std::ostream &ostream, Token token);

} // namespace cora::parser

#endif // CORA_PARSER_TOKEN_H
