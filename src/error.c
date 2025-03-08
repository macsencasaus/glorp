#include "error.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "hash_table.h"
#include "token.h"

// TODO: don't just exit on error

#define BOLD_START "\033[1m"
#define RED_START "\033[1;31m"
#define RESET_FMT "\033[0m"
#define RED_ERROR RED_START "error:"

#define ERR_LINE_FMT BOLD_START "%s:%lu:%lu: %s " RESET_FMT
#define ERR_LINE filename, line_number, col_number, RED_ERROR

#define LEADING_CHAR '^'
#define TRAILING_CHAR '~'

void generic_error(const char *filename, token tok, const char *msg) {
    size_t line_number = tok.line_number;
    size_t col_number = tok.col_number;
    fprintf(stderr, ERR_LINE_FMT "%s\n", ERR_LINE, msg);

    print_error_line(tok);
    exit(1);
}

void match_token_type_error(const char *filename, token tok, token_type tt) {
    if (tok.type == tt) return;

    size_t line_number = tok.line_number;
    size_t col_number = tok.col_number;
    fprintf(stderr, ERR_LINE_FMT "expected %s\n", ERR_LINE,
            token_type_literals[tt]);

    print_error_line(tok);
    exit(1);
}

void unexpected_token_error(const char *filename, token tok) {
    size_t line_number = tok.line_number;
    size_t col_number = tok.col_number;
    fprintf(stderr, ERR_LINE_FMT "unexpected token %s\n", ERR_LINE,
            token_type_literals[tok.type]);

    print_error_line(tok);
    exit(1);
}

void print_error_line(token tok) {
    const char *line = tok.literal - tok.col_number + 1;

    size_t line_length = 0;
    for (const char *p = line; *p != '\n' && *p != 0 && *p != EOF; ++p)
        ++line_length;

    int length_before_err = tok.col_number - 1;
    int length_after_err = line_length - (tok.col_number + tok.length - 1);

    char trailings[VARIABLE_MAX_LENGTH];
    memset(trailings, TRAILING_CHAR, VARIABLE_MAX_LENGTH);

    printf("%4lu | %.*s" RED_START "%.*s" RESET_FMT "%.*s\n", tok.line_number,
           length_before_err, line, (int)tok.length, tok.literal,
           length_after_err, tok.literal + tok.length);
    printf("     | " RED_START "%*c%.*s" RESET_FMT "\n", (int)tok.col_number,
           LEADING_CHAR, (int)(tok.length - 1), trailings);
}
