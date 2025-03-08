#ifndef PARSER_H
#define PARSER_H

#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "token.h"

typedef uint8_t parser_flags;

typedef struct {
    arena *a;
    lexer *l;

    token cur_token;
    token peek_token;

    parser_flags flags;
} parser;

typedef expression_reference prefix_parse_fn(parser *p);
typedef expression_reference infix_parse_fn(parser *p,
                                            expression_reference left);

typedef enum {
    PRECEDENCE_STOP,
    PRECEDENCE_LOWEST,
    PRECEDENCE_ASSIGN,
    PRECEDENCE_TUPLE,
    PRECEDENCE_TERNARY,
    PRECEDENCE_EQUALS,
    PRECEDENCE_APPEND,
    PRECEDENCE_SUM,
    PRECEDENCE_PRODUCT,
    PRECEDENCE_PREFIX,
    PRECEDENCE_INDEX,
    PRECEDENCE_COMPOSE,
    PRECEDENCE_CALL,
} expression_precedence;

typedef enum {
    ASSOC_NONE,
    ASSOC_LEFT,
    ASSOC_RIGHT,
} expression_associativity;

void parser_init(parser *p, lexer *l, arena *a);

// for repl (reuses same arena given a different lexer)
void parser_reset_lexer(parser *p, lexer *l);

expression_reference parse_program(parser *p);

static const expression_precedence precedence_lookup[TOKEN_TYPE_ENUM_LENGTH] = {
    PRECEDENCE_LOWEST,  // ILLEGAL
    PRECEDENCE_LOWEST,  // EOF

    PRECEDENCE_LOWEST,  // IDENT
    PRECEDENCE_LOWEST,  // CHAR
    PRECEDENCE_LOWEST,  // INT
    PRECEDENCE_LOWEST,  // FLOAT
    PRECEDENCE_LOWEST,  // STRING

    PRECEDENCE_ASSIGN,   // ASSIGN
    PRECEDENCE_SUM,      // PLUS
    PRECEDENCE_SUM,      // MINUS
    PRECEDENCE_LOWEST,   // BANG
    PRECEDENCE_PRODUCT,  // ASTERISK
    PRECEDENCE_PRODUCT,  // SLASH
    PRECEDENCE_PRODUCT,  // PERCENT

    PRECEDENCE_APPEND,  // PLUS PLUS
    PRECEDENCE_LOWEST,  // MINUS MINUS

    PRECEDENCE_LOWEST,  // BACK SLASH

    PRECEDENCE_EQUALS,  // LT
    PRECEDENCE_EQUALS,  // GT
    PRECEDENCE_EQUALS,  // LT EQ
    PRECEDENCE_EQUALS,  // GT EQ
    PRECEDENCE_EQUALS,  // EQ
    PRECEDENCE_EQUALS,  // NOT EQ

    PRECEDENCE_TUPLE,    // COMMA
    PRECEDENCE_APPEND,   // COLON
    PRECEDENCE_ASSIGN,   // COLON COLON
    PRECEDENCE_LOWEST,   // SEMICOLON
    PRECEDENCE_COMPOSE,  // DOT

    PRECEDENCE_CALL,    // LPAREN
    PRECEDENCE_LOWEST,  // RPAREN
    PRECEDENCE_LOWEST,  // LBRACE
    PRECEDENCE_LOWEST,  // RBRACE
    PRECEDENCE_INDEX,   // LBRACKET
    PRECEDENCE_LOWEST,  // RBRACKET

    PRECEDENCE_TERNARY,  // QUESTION
    PRECEDENCE_LOWEST,   // RIGHT ARROW
};

static const expression_associativity assoc_lookup[TOKEN_TYPE_ENUM_LENGTH] = {
    ASSOC_NONE,  // ILLEGAL
    ASSOC_NONE,  // EOF

    ASSOC_NONE,  // IDENT
    ASSOC_NONE,  // CHAR
    ASSOC_NONE,  // INT
    ASSOC_NONE,  // FLOAT
    ASSOC_NONE,  // STRING

    ASSOC_RIGHT,  // ASSIGN
    ASSOC_LEFT,   // PLUS
    ASSOC_LEFT,   // MINUS
    ASSOC_NONE,   // BANG
    ASSOC_LEFT,   // ASTERISK
    ASSOC_LEFT,   // SLASH
    ASSOC_LEFT,   // PERCENT

    ASSOC_LEFT,  // PLUS PLUS
    ASSOC_NONE,  // MINUS MINUS

    ASSOC_NONE,  // BACK SLASH

    ASSOC_LEFT,  // LT
    ASSOC_LEFT,  // GT
    ASSOC_LEFT,  // LT EQ
    ASSOC_LEFT,  // GT EQ
    ASSOC_LEFT,  // EQ
    ASSOC_LEFT,  // NOT EQ

    ASSOC_RIGHT,  // COMMA
    ASSOC_RIGHT,  // COLON
    ASSOC_RIGHT,  // COLON COLON
    ASSOC_NONE,   // SEMICOLON
    ASSOC_LEFT,   // DOT

    ASSOC_LEFT,  // LPAREN
    ASSOC_NONE,  // RPAREN
    ASSOC_NONE,  // LBRACE
    ASSOC_NONE,  // RBRACE
    ASSOC_LEFT,  // LBRACKET
    ASSOC_NONE,  // RBRACKET

    ASSOC_LEFT,  // QUESTION
    ASSOC_NONE,  // RIGHT ARROW
};

#endif  // PARSER_H
