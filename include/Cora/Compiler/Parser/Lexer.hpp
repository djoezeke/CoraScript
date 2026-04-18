#ifndef CORA_COMPILER_PARSER_LEXER_H
#define CORA_COMPILER_PARSER_LEXER_H

#include "Cora/Compiler/Parser/Token.hpp"

#include <deque>
#include <string>

namespace cora::compiler
{
    namespace parser
    {
        class Lexer
        {
        public:
            Lexer();
            explicit Lexer(std::string source);

            std::deque<Token> Lex(const std::string &source);
            std::deque<Token> Tokenize();

            Token NextToken();
            Token PrevToken() const;

            ~Lexer();

        private:
            static std::string Trim(const std::string &line);
            static int CountIndent(const std::string &line);

            void BuildTokens();
            void PushToken(TokenType type, const std::string &text, unsigned int line, unsigned int column);

        private:
            std::string m_Source;
            std::deque<Token> m_Tokens;
            std::size_t m_Position{0};
            Token m_Prev;
        };

    } // namespace parser

} // namespace cora::compiler

#endif // CORA_COMPILER_PARSER_LEXER_H
