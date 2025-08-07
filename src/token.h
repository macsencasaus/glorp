#ifndef TOKEN_H
#define TOKEN_H

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    TOKEN_TYPE_ILLEGAL,
    TOKEN_TYPE_EOF,

    TOKEN_TYPE_IDENT,
    TOKEN_TYPE_CHAR,
    TOKEN_TYPE_INT,
    TOKEN_TYPE_FLOAT,
    TOKEN_TYPE_STRING,

    TOKEN_TYPE_ASSIGN,    // =
    TOKEN_TYPE_PLUS,      // +
    TOKEN_TYPE_MINUS,     // -
    TOKEN_TYPE_BANG,      // !
    TOKEN_TYPE_ASTERISK,  // *
    TOKEN_TYPE_SLASH,     // /
    TOKEN_TYPE_PERCENT,   // %

    TOKEN_TYPE_PLUS_PLUS,    // ++
    TOKEN_TYPE_MINUS_MINUS,  // --

    TOKEN_TYPE_LT,      // <
    TOKEN_TYPE_GT,      // >
    TOKEN_TYPE_LT_EQ,   // <=
    TOKEN_TYPE_GT_EQ,   // >=
    TOKEN_TYPE_EQ,      // ==
    TOKEN_TYPE_NOT_EQ,  // !=

    TOKEN_TYPE_LAND,  // &&
    TOKEN_TYPE_LOR,   // ||
    TOKEN_TYPE_BAND,  // &
    TOKEN_TYPE_BOR,   // |
    TOKEN_TYPE_NOT,   // ~
    TOKEN_TYPE_XOR,   // ^

    TOKEN_TYPE_LEFT_SHIFT,   // <<
    TOKEN_TYPE_RIGHT_SHIFT,  // >>

    TOKEN_TYPE_LEFT_COMPOSE,   // <<<
    TOKEN_TYPE_RIGHT_COMPOSE,  // >>>
    TOKEN_TYPE_LEFT_PIPE,      // <|
    TOKEN_TYPE_RIGHT_PIPE,     // |>

    // Delimiters
    TOKEN_TYPE_COMMA,        // ,
    TOKEN_TYPE_COLON,        // :
    TOKEN_TYPE_COLON_COLON,  // ::
    TOKEN_TYPE_SEMICOLON,    // ;
    TOKEN_TYPE_DOT,          // .

    TOKEN_TYPE_LPAREN,    // (
    TOKEN_TYPE_RPAREN,    // )
    TOKEN_TYPE_LBRACE,    // {
    TOKEN_TYPE_RBRACE,    // }
    TOKEN_TYPE_LBRACKET,  // [
    TOKEN_TYPE_RBRACKET,  // ]

    TOKEN_TYPE_QUESTION,     // ?
    TOKEN_TYPE_RIGHT_ARROW,  // ->
    TOKEN_TYPE_LEFT_ARROW,   // <-

    TOKEN_TYPE_FAT_RIGHT_ARROW, // =>

    TOKEN_TYPE_ENUM_LENGTH,
} token_type;

typedef struct {
    token_type type;

    const char *literal;
    size_t length;

    uint32_t line_number;
    uint32_t col_number;
} token;

// clang-format off
static const char *const token_type_literals[TOKEN_TYPE_ENUM_LENGTH] = {
    [TOKEN_TYPE_ILLEGAL] = "ILLEGAL", 
    [TOKEN_TYPE_EOF] = "EOF",
    [TOKEN_TYPE_IDENT] = "identifier",
    [TOKEN_TYPE_CHAR] = "char",
    [TOKEN_TYPE_INT] = "int",
    [TOKEN_TYPE_FLOAT] = "float",
    [TOKEN_TYPE_STRING] = "string",
    [TOKEN_TYPE_ASSIGN] = "'='",
    [TOKEN_TYPE_PLUS] = "'+'",
    [TOKEN_TYPE_MINUS] = "'-'",
    [TOKEN_TYPE_BANG] = "'!'",
    [TOKEN_TYPE_ASTERISK] = "'*'",
    [TOKEN_TYPE_SLASH] = "'/'",
    [TOKEN_TYPE_PERCENT] = "'%'",
    [TOKEN_TYPE_PLUS_PLUS] = "'++'",
    [TOKEN_TYPE_MINUS_MINUS] = "'--'",
    [TOKEN_TYPE_LT] = "'<'",
    [TOKEN_TYPE_GT] = "'>'",
    [TOKEN_TYPE_LT_EQ] = "'<='",
    [TOKEN_TYPE_GT_EQ] = "'>='",
    [TOKEN_TYPE_EQ] = "'=='",
    [TOKEN_TYPE_NOT_EQ] = "'!='",
    [TOKEN_TYPE_LAND] = "'&&'",
    [TOKEN_TYPE_LOR] = "'||'",
    [TOKEN_TYPE_BAND] = "'&'",
    [TOKEN_TYPE_BOR] = "'|'",
    [TOKEN_TYPE_NOT] = "'~'",
    [TOKEN_TYPE_XOR] = "'^'",
    [TOKEN_TYPE_LEFT_SHIFT] = "'<<'",
    [TOKEN_TYPE_RIGHT_SHIFT] = "'>>'",
    [TOKEN_TYPE_LEFT_COMPOSE] = "'<<<'",
    [TOKEN_TYPE_RIGHT_COMPOSE] = "'>>>'",
    [TOKEN_TYPE_LEFT_PIPE] = "'<|'",
    [TOKEN_TYPE_RIGHT_PIPE] = "'|>'",
    [TOKEN_TYPE_COMMA] = "','",
    [TOKEN_TYPE_COLON] = "':'",
    [TOKEN_TYPE_COLON_COLON] = "'::'",
    [TOKEN_TYPE_SEMICOLON] = "';'",
    [TOKEN_TYPE_DOT] = "'.'",
    [TOKEN_TYPE_LPAREN] = "'('",
    [TOKEN_TYPE_RPAREN] = "')'",
    [TOKEN_TYPE_LBRACE] = "'{'",
    [TOKEN_TYPE_RBRACE] = "'}'",
    [TOKEN_TYPE_LBRACKET] = "'['",
    [TOKEN_TYPE_RBRACKET] = "']'",
    [TOKEN_TYPE_QUESTION] = "'?'",
    [TOKEN_TYPE_RIGHT_ARROW] = "'->'",
    [TOKEN_TYPE_LEFT_ARROW] = "'<-'",
    [TOKEN_TYPE_FAT_RIGHT_ARROW] = "'=>'",
};
// clang-format on 

#endif  // TOKEN_H
