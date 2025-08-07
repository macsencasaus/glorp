#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "error.h"
#include "token.h"

#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))

typedef struct expr_list expr_list;
typedef struct expr expr;

struct expr_list {
    expr *head;
    expr *tail;
    size_t size;
};

typedef enum {
    EXPR_TYPE_NONE,

    EXPR_TYPE_PROGRAM,

    EXPR_TYPE_UNIT,

    EXPR_TYPE_IDENTIFIER,
    EXPR_TYPE_CHAR_LITERAL,
    EXPR_TYPE_INT_LITERAL,
    EXPR_TYPE_FLOAT_LITERAL,
    EXPR_TYPE_STRING_LITERAL,
    EXPR_TYPE_LIST_LITERAL,

    EXPR_TYPE_BLOCK_EXPRESSION,
    EXPR_TYPE_PREFIX_EXPRESSION,
    EXPR_TYPE_INFIX_EXPRESSION,
    EXPR_TYPE_TERNARY_EXPRESSION,
    EXPR_TYPE_CALL_EXPRESSION,
    EXPR_TYPE_INDEX_EXPRESSION,
    EXPR_TYPE_CASE_EXPRESSION,

    EXPR_TYPE_IMPORT_EXPRESSION,

    EXPR_ENUM_LENGTH,
} expr_type;

struct expr {
    token start_tok;
    token end_tok;

    expr *next;  // only used in expression list

    expr_type type;
    union {
        // program
        // list literal
        // block
        expr_list expressions;

        // identifier
        // string literal
        // import
        struct {
            const char *literal;
            size_t length;
        };

        // char literal
        char char_value;

        // int literal
        int64_t int_value;

        // float literal
        double float_value;

        // prefix
        // assign
        // infix
        struct {
            token op;
            expr *right;
            expr *left;
        };

        // ternary
        struct {
            expr *condition;
            expr *consequence;
            expr *alternative;
        };

        // call
        struct {
            expr *function;
            expr_list params;
        };

        // index
        struct {
            expr *list;
            expr *index;
        };

        // case
        struct {
            expr_list conditions;
            expr_list results;
        };
    };
};

void print_ast(expr *program);

void el_append(expr_list *, expr *);

#endif  // AST_H
