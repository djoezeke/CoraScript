#include "Cora/Compiler/Parser/Token.hpp"

#include <unordered_map>

namespace cora::compiler
{
    namespace parser
    {

        Token::Token()
            : m_Type(TokenType::End), m_Range(SourceRange()), m_TokenText() {}

        Token::Token(TokenType tokentype, unsigned int line, unsigned int column)
            : m_Type(tokentype),
              m_Range(SourceRange(SourceLocation(line, column))),
              m_TokenText() {}

        Token::Token(TokenType tokentype, std::string tokentext, unsigned int line, unsigned int column)
            : m_Type(tokentype),
              m_Range(SourceRange(SourceLocation(line, column), SourceLocation(line, column + static_cast<unsigned int>(tokentext.size())))),
              m_TokenText(std::move(tokentext)) {}

        std::string Token::Repr() { return GetText(); }

        TokenType Token::GetTokenType() const { return m_Type; }

        std::string Token::GetTokenTypeString() const
        {
            static const std::unordered_map<TokenType, std::string> names = {
                {TokenType::End, "End"},
                {TokenType::Newline, "Newline"},
                {TokenType::Indent, "Indent"},
                {TokenType::Dedent, "Dedent"},
                {TokenType::Identifier, "Identifier"},
                {TokenType::Number, "Number"},
                {TokenType::String, "String"},
                {TokenType::Null, "Null"},
                {TokenType::True, "True"},
                {TokenType::False, "False"},
                {TokenType::Let, "Let"},
                {TokenType::Int, "Int"},
                {TokenType::Float, "Float"},
                {TokenType::Bool, "Bool"},
                {TokenType::StringType, "StringType"},
                {TokenType::If, "If"},
                {TokenType::Elif, "Elif"},
                {TokenType::Else, "Else"},
                {TokenType::While, "While"},
                {TokenType::For, "For"},
                {TokenType::Import, "Import"},
                {TokenType::In, "In"},
                {TokenType::Range, "Range"},
                {TokenType::Break, "Break"},
                {TokenType::Continue, "Continue"},
                {TokenType::Pass, "Pass"},
                {TokenType::Print, "Print"},
                {TokenType::And, "And"},
                {TokenType::Or, "Or"},
                {TokenType::Not, "Not"},
                {TokenType::LParen, "LParen"},
                {TokenType::RParen, "RParen"},
                {TokenType::LBrace, "LBrace"},
                {TokenType::RBrace, "RBrace"},
                {TokenType::Colon, "Colon"},
                {TokenType::Comma, "Comma"},
                {TokenType::Semicolon, "Semicolon"},
                {TokenType::Assign, "Assign"},
                {TokenType::Plus, "Plus"},
                {TokenType::Minus, "Minus"},
                {TokenType::Star, "Star"},
                {TokenType::Slash, "Slash"},
                {TokenType::Percent, "Percent"},
                {TokenType::Dot, "Dot"},
                {TokenType::Equal, "Equal"},
                {TokenType::NotEqual, "NotEqual"},
                {TokenType::Less, "Less"},
                {TokenType::LessEqual, "LessEqual"},
                {TokenType::Greater, "Greater"},
                {TokenType::GreaterEqual, "GreaterEqual"},
                {TokenType::T_PUBLIC, "Public"},
                {TokenType::T_PRIVATE, "Private"},
                {TokenType::T_NAMESPACE, "Namespace"},
            };

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

        SourceLocation Token::GetStartPosition() const noexcept { return m_Range.GetStart(); }

        SourceLocation Token::GetEndPosition() const noexcept { return m_Range.GetEnd(); }

        SourceRange Token::GetSourceRange() const { return m_Range; }

        void Token::SetText(std::string text) { m_TokenText = std::move(text); }

        void Token::SetTokenType(TokenType tokentype) noexcept { m_Type = tokentype; }

        void Token::SetStartPosition(SourceLocation start) noexcept { m_Range.SetStart(start); }

        void Token::SetEndPosition(SourceLocation end) noexcept { m_Range.SetEnd(end); }

        void Token::SetSourceRange(SourceRange range) noexcept { m_Range = std::move(range); }

        std::ostream &operator<<(std::ostream &ostream, Token token)
        {
            return ostream << token.GetText();
        }

    } // namespace parser

} // namespace cora::compiler
