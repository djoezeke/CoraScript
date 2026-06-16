#ifndef JUNE_JUNE_TOKEN_H
#define JUNE_JUNE_TOKEN_H

#include <stdint.h>

typedef enum
{
    TOK_NONE,

    TOK_INTNUMBER,
    TOK_STRLITERAL,
    TOK_IDENTIFIER,
    TOK_FLOATNUMBER,

    // Keywords
    TOK_BREAK,
    TOK_CONST,
    TOK_CONTINUE,
    TOK_ELSE,
    TOK_FUNC,
    TOK_FOR,
    TOK_IMPORT,
    TOK_IF,
    TOK_RETURN,
    TOK_STRUCT,
    TOK_TYPE,
    TOK_VAR,

    // Operators
    TOK_PLUS,
    TOK_MINUS,
    TOK_MUL,
    TOK_DIV,
    TOK_MOD,
    TOK_AND,
    TOK_OR,
    TOK_XOR,
    TOK_SHL,
    TOK_SHR,
    TOK_PLUSEQ, // `+=` Add and assign
    TOK_MINUSEQ,
    TOK_MULEQ,
    TOK_DIVEQ,
    TOK_MODEQ,
    TOK_ANDEQ,
    TOK_OREQ,
    TOK_XOREQ,
    TOK_SHLEQ,
    TOK_SHREQ,
    TOK_ANDAND,
    TOK_OROR,
    TOK_PLUSPLUS,
    TOK_MINUSMINUS,
    TOK_EQEQ,
    TOK_LESS,
    TOK_GREATER,
    TOK_EQ,
    TOK_QUESTION,
    TOK_NOT,
    TOK_NOTEQ,
    TOK_LESSEQ,
    TOK_GREATEREQ,
    TOK_COLONEQ,
    TOK_LPAR,
    TOK_RPAR,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_CARET,
    TOK_COMMA,
    TOK_COLON,
    TOK_PERIOD,
    TOK_ELLIPSIS,
    TOK_SEMICOLON,
    TOK_COLONCOLON,

    // Other tokens
    TOK_EOF,
    TOK_EOLN,
} TokenKind;

typedef struct
{
    TokenKind type; // enum based token type

    const char *value; // token value (not null terminated)
    uint32_t bytes;    // token length in bytes
    uint32_t length;   // token length (UTF-8)

    uint32_t lineno;   // token line number (1-based)
    uint32_t colno;    // token column number (0-based) at the end of the token
    uint32_t position; // offset of the first character of the token
} Token;

#endif // JUNE_JUNE_TOKEN_H
