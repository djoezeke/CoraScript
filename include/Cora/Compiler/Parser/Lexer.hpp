#ifndef CORA_COMPILER_PARSER_LEXER_H
#define CORA_COMPILER_PARSER_LEXER_H

#include "Cora/Compiler/Parser/Token.hpp"

#include <deque>

namespace cora::compiler
{
    namespace parser
    {
        class Lexer
        {
        public:
            Lexer();
            std::deque<Token> Lex(const std::string &source) const;
            ~Lexer();

        private:
            std::string Trim(const std::string &line) const;
            int CountIndent(const std::string &line) const;

        private:
            /* data */
        };

    } // namespace parser

} // namespace cora::compiler

#endif // CORA_COMPILER_PARSER_LEXER_H
