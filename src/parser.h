#ifndef PARSER_H
#define PARSER_H

#include "token.h"
#include "lexer.h"
#include "ast.h"
#include "arena.h"

typedef struct {
    arena a;
    lexer *l;

    token cur_token;
    token peek_token;
} parser;

parser new_parser(lexer *l);

// for repl (reuses same arena given a different lexer)
void reset_parser(parser *p, lexer *l);

expression_reference parse_program(parser *p);

#endif // PARSER_H
