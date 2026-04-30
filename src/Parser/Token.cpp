#include "Token.hpp"

#include <unordered_map>

namespace cora::parser
{
    Token::Token()
        : m_Type(TokenType::End), m_Range(basic::SourceRange()), m_TokenText() {}

    Token::Token(TokenType tokentype, unsigned int line, unsigned int column)
        : m_Type(tokentype),
          m_Range(basic::SourceRange(basic::SourceLocation(line, column))),
          m_TokenText() {}

    Token::Token(TokenType tokentype, std::string tokentext, unsigned int line, unsigned int column)
        : m_Type(tokentype),
          m_Range(basic::SourceRange(basic::SourceLocation(line, column), basic::SourceLocation(line, column + static_cast<unsigned int>(tokentext.size())))),
          m_TokenText(std::move(tokentext)) {}

    std::string Token::Repr() { return GetText(); }

    TokenType Token::GetTokenType() const { return m_Type; }

    std::string Token::GetTokenTypeString() const
    {
        static const std::unordered_map<TokenType, std::string> names = {};

        auto it = names.find(m_Type);
        if (it != names.end())
        {
            return it->second;
        }

        return "TokenType(" + std::to_string(static_cast<int>(m_Type)) + ")";
    }

    bool Token::Is(TokenType tokentype) const { return m_Type == tokentype; }
    bool Token::IsNot(TokenType tokentype) const { return m_Type != tokentype; }

    bool Token::IsAny(TokenType tokentype) const
    {
        return Is(tokentype);
    }

    std::string Token::GetText() const { return m_TokenText; }

    unsigned int Token::GetLenght() const { return static_cast<unsigned int>(m_TokenText.size()); }

    basic::SourceLocation Token::GetStartPosition() const noexcept { return m_Range.GetStart(); }

    basic::SourceLocation Token::GetEndPosition() const noexcept { return m_Range.GetEnd(); }

    basic::SourceRange Token::GetSourceRange() const { return m_Range; }

    void Token::SetText(std::string text) { m_TokenText = std::move(text); }

    void Token::SetTokenType(TokenType tokentype) noexcept { m_Type = tokentype; }

    void Token::SetStartPosition(basic::SourceLocation start) noexcept { m_Range.SetStart(start); }

    void Token::SetEndPosition(basic::SourceLocation end) noexcept { m_Range.SetEnd(end); }

    void Token::SetSourceRange(basic::SourceRange range) noexcept { m_Range = std::move(range); }

    std::ostream &operator<<(std::ostream &ostream, Token token)
    {
        // (Type=type : Text=test )
        ostream << "( Type=" << token.GetTokenTypeString();
        ostream << " : Text=" << token.GetText();
        ostream << " )";
        return ostream;
    };

} // namespace cora::parser
