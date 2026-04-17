#include "Cora/Compiler/Parser/Token.hpp"

namespace cora::compiler
{
    namespace parser
    {

        Token::Token() = default;

        Token::Token(TokenType tokentype, std::string tokentext, unsigned int line, unsigned int column)
        {
            m_Type = tokentype, m_TokenText = tokentext;
            SourceLocation start(line, column);
            SourceLocation end(line, column + tokentext.size());
            m_Range = SourceRange(start, end);
        };

        Token::Token(TokenType tokentype, unsigned int line, unsigned int column)
        {
            m_Type = tokentype;
            SourceLocation start(line, column);
            m_Range = SourceRange(start);
        };

        Token::Token(TokenType tokentype, std::string kinsdtring, std::string tokentext, unsigned int line, unsigned int column)
        {
            m_Type = tokentype;
            m_TypeString = kinsdtring;
            m_TokenText = tokentext;
            SourceLocation start(line, column);
            SourceLocation end(line, column + tokentext.size());
            m_Range = SourceRange(start, end);
        };

        std::string Token::Repr() { return GetText(); };

        TokenType Token::GetTokenType() const { return m_Type; };

        std::string Token::GetTokenTypeString() const { return m_TypeString; };

        bool Token::Is(TokenType tokentype) const { return m_Type == tokentype; };
        bool Token::IsNot(TokenType tokentype) const { return m_Type != tokentype; };

        bool Token::IsAny(TokenType tokentype) const
        {
            return Is(tokentype);
        }

        template <typename... T>
        bool Token::IsAny(TokenType kind1, TokenType kind2, T... tokentypes) const
        {
            if (Is(kind1))
                return true;
            return IsAny(kind2, tokentypes...);
        }

        template <typename... T>
        bool Token::IsNotAny(TokenType tokentype, T... tokentypes) const
        {
            return !IsAny(tokentype, tokentypes...);
        }

        std::string Token::GetText() const { return m_TokenText; };

        unsigned int Token::GetLenght() const { return m_TokenText.size(); };

        SourceLocation Token::GetStartPosition() const noexcept { return m_Range.GetStart(); };

        SourceLocation Token::GetEndPosition() const noexcept { return m_Range.GetEnd(); };

        SourceRange Token::GetSourceRange() const { return m_Range; };

        void Token::SetText(std::string text) { m_TokenText = text; };

        void Token::SetTokenType(TokenType tokentype) noexcept { m_Type = tokentype; };

        void Token::SetTokenTypeString(std::string kindstring) noexcept { m_TypeString = kindstring; };

        void Token::SetStartPosition(SourceLocation start) noexcept { m_Range.SetStart(start); };

        void Token::SetEndPosition(SourceLocation end) noexcept { m_Range.SetEnd(end); };

        void Token::SetSourceRange(SourceRange range) noexcept { m_Range = range; };

        std::ostream &operator<<(std::ostream &ostream, Token token)
        {
            return ostream << token.GetText();
        };

    } // namespace parser

} // namespace cora::compiler
