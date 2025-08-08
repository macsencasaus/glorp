#include "error.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "hashtable.h"
#include "token.h"

typedef struct expr expr;

typedef struct {
    const token *start;
    const token *end;
} expr_range;

typedef expr_range expr_range_fn(const expr *e);

#define BOLD_START "\033[1m"
#define RED_START "\033[1;31m"
#define RESET_FMT "\033[0m"
#define RED_ERROR RED_START "error:"

#define ERR_LINE_FMT BOLD_START "%s:%u:%u: %s " RESET_FMT
#define ERR_LINE file_name, line_number, col_number, RED_ERROR

#define LEADING_CHAR '^'
#define TRAILING_CHAR '~'

static void unexpected_token_error(const char *file_name, const token *tok);
static void expected_token_error(const char *file_name, const token *tok,
                                 token_type tt);
static void print_error_line(const token *tok);
static void print_error_line_range(const token *start, const token *end);

void inspect_parser_error(const char *file_name, const parser_error *error) {
    switch (error->type) {
        case PARSER_ERROR_UNEXPECTED: {
            unexpected_token_error(file_name, &error->tok);
        } break;
        case PARSER_ERROR_EXPECTED: {
            expected_token_error(file_name, &error->tok, error->expected);
        } break;
    }
}

void inspect_eval_error(const char *file_name, const eval_error *error) {
    if (error->e == NULL)
        return;
    const token *start = &error->e->start_tok;
    const token *end = &error->e->end_tok;

    uint32_t line_number = start->line_number;
    uint32_t col_number = start->col_number;

    fprintf(stderr, ERR_LINE_FMT "%s\n", ERR_LINE, error->msg);

    print_error_line_range(start, end);
}

static void unexpected_token_error(const char *file_name, const token *tok) {
    uint32_t line_number = tok->line_number;
    uint32_t col_number = tok->col_number;
    fprintf(stderr, ERR_LINE_FMT "unexpected token %s\n", ERR_LINE,
            token_type_literals[tok->type]);

    print_error_line(tok);
}

static void expected_token_error(const char *file_name, const token *tok,
                                 token_type tt) {
    uint32_t line_number = tok->line_number;
    uint32_t col_number = tok->col_number;
    fprintf(stderr, ERR_LINE_FMT "expected %s\n", ERR_LINE,
            token_type_literals[tt]);

    print_error_line(tok);
}

static void print_error_line(const token *tok) {
    const char *line = tok->literal - tok->col_number + 1;

    size_t line_length = 0;
    for (const char *p = line; *p != '\n' && *p != 0; ++p)
        ++line_length;

    int length_before_err = tok->col_number - 1;
    int length_after_err = line_length - (tok->col_number + tok->length - 1);

    char trailings[VARIABLE_MAX_LENGTH];
    memset(trailings, TRAILING_CHAR, VARIABLE_MAX_LENGTH);

    fprintf(stderr, "%4u | %.*s" RED_START "%.*s" RESET_FMT "%.*s\n", tok->line_number,
            length_before_err, line, (int)tok->length, tok->literal,
            length_after_err, tok->literal + tok->length);
    fprintf(stderr, "     | " RED_START "%*c%.*s" RESET_FMT "\n", (int)tok->col_number,
            LEADING_CHAR, (int)(tok->length - 1), trailings);
}

static void print_error_line_range(const token *start, const token *end) {
    const char *line = start->literal - start->col_number + 1;

    int line_length = 0;
    for (const char *p = line; *p != '\n' && *p != 0; ++p)
        ++line_length;

    int length_before_err = start->col_number - 1;
    int length_after_err = line_length - (end->col_number + end->length - 1);
    int expr_length = (end->literal + end->length) - start->literal;

    char markers[expr_length];
    memset(markers, LEADING_CHAR, expr_length);

    fprintf(stderr, "%4u | %.*s" RED_START "%.*s" RESET_FMT "%.*s\n",
            start->line_number, length_before_err, line, expr_length,
            start->literal, length_after_err, end->literal + end->length);
    fprintf(stderr, "     | " RED_START "%*c%.*s" RESET_FMT "\n", (int)start->col_number,
            LEADING_CHAR, expr_length - 1, markers);
}
