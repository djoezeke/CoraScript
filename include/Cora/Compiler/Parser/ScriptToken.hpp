#ifndef CORASCRIPT_SCRIPT_TOKEN_HPP
#define CORASCRIPT_SCRIPT_TOKEN_HPP

#include <string>

namespace cora
{
    namespace script
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
            Equal,
            NotEqual,
            Less,
            LessEqual,
            Greater,
            GreaterEqual,
        };

        struct Token
        {
            TokenType type;
            std::string text;
            int line;
            int column;
        };
    }
}

#endif
