#ifndef LEXER_H
#define LEXER_H

#include "stdbool.h"
#include "stdlib.h"
#include "token.h"

typedef struct {
    const char *filename;

    const char *input;
    size_t input_length;

    size_t position;
    size_t read_position;

    size_t line_number;
    size_t col_number;

    char ch;
} lexer;

void lexer_init(lexer *l, const char *filename, const char *input, size_t n);
token lexer_next_token(lexer *l);
void print_lexer_output(const char *filename, const char *input, size_t n);

// Determines whether repl should read line again before parsing.
// Handles open parens/brackets/braces and guards
bool wait_for_more(const char *input, size_t n);

#endif  // LEXER_H
