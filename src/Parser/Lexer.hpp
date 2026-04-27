#ifndef CORA_PARSER_LEXER_H
#define CORA_PARSER_LEXER_H

#include "Token.hpp"

#include <vector>
#include <string>

namespace cora::parser
{
    using namespace cora;

    class Lexer
    {
    public:
        Lexer();
        explicit Lexer(std::string source);

        std::vector<Token> Lex(const std::string &source);
        std::vector<Token> Tokenize();

        void SetFileName(std::string fileName);
        void SetModuleName(std::string moduleName);

        Token NextToken();
        Token PrevToken() const;

        ~Lexer();

    private:
        static std::string Trim(const std::string &line);
        static int CountIndent(const std::string &line);

        void BuildTokens();
        void PushToken(TokenType type, const std::string &text, unsigned int line, unsigned int column);
        [[noreturn]] void RaiseLexError(const std::string &message, unsigned int line, unsigned int column) const;

    private:
        std::string m_Source;
        std::vector<Token> m_Tokens;
        std::size_t m_Position{0};
        Token m_Prev;
        std::string m_FileName{"<memory>"};
        std::string m_ModuleName;
    };

} // namespace cora::parser

#endif // CORA_PARSER_LEXER_H
