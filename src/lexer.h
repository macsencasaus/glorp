#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include "stdlib.h"
#include "stdbool.h"
#include "ast.h"

typedef struct {
    char *filename;

    char *input;
    size_t input_length;

    size_t position;
    size_t read_position;

    size_t line_number;
    size_t col_number;
    
    char ch;
} lexer;

lexer new_lexer(char *filename, char *input, size_t n);

token lexer_next_token(lexer *l);

token peek_token(lexer *l);

#endif // LEXER_H
