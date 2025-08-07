#ifndef ERROR_H
#define ERROR_H

#include "token.h"

#ifndef ERROR_MSG_LENGTH
#define ERROR_MSG_LENGTH 1023
#endif

typedef struct expr expr;

typedef enum {
    PARSER_ERROR_UNEXPECTED,
    PARSER_ERROR_EXPECTED,
} parser_error_type;

typedef struct {
    token tok;

    parser_error_type type;
    token_type expected;
} parser_error;

typedef struct {
    const expr *e;
    char msg[ERROR_MSG_LENGTH + 1];
} eval_error;

void inspect_parser_error(const char *file_name, const parser_error *error);

void inspect_eval_error(const char *file_name, const eval_error *error);

#endif  // ERROR_H
