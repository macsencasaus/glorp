#ifndef PARSER_H
#define PARSER_H

#include "arena.h"
#include "ast.h"
#include "lexer.h"
#include "token.h"

typedef struct {
    arena a;
    lexer *l;

    token cur_token;
    token peek_token;
} parser;

typedef expression_reference prefix_parse_fn(parser *p);
typedef expression_reference infix_parse_fn(parser *p,
                                            expression_reference left);

typedef enum {
    PRECEDENCE_LOWEST,
    PRECEDENCE_TERNARY,
    PRECEDENCE_ASSIGN,
    PRECEDENCE_EQUALS,
    PRECEDENCE_APPEND,
    PRECEDENCE_SUM,
    PRECEDENCE_PRODUCT,
    PRECEDENCE_PREFIX,
    PRECEDENCE_INDEX,
    PRECEDENCE_COMPOSE,
    PRECEDENCE_CALL,
} expression_precedence;

parser new_parser(lexer *l);

// for repl (reuses same arena given a different lexer)
void reset_parser(parser *p, lexer *l);

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

    PRECEDENCE_LOWEST,   // COMMA
    PRECEDENCE_LOWEST,   // COLON
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

#endif  // PARSER_H
