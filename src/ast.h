#ifndef AST_H
#define AST_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "token.h"

typedef struct expression_list expression_list;
typedef struct expression expression;

// just the index into the arena
typedef size_t expression_reference;

struct expression_list{
    expression_reference head;
    expression_reference tail;
    size_t size;
};

typedef enum {
    EXP_TYPE_PROGRAM,

    EXP_TYPE_IDENTIFIER,
    EXP_TYPE_INT_LITERAL,
    EXP_TYPE_FUNCTION_LITERAL,
    EXP_TYPE_LIST_LITERAL,

    EXP_TYPE_BLOCK_EXPRESSION,
    EXP_TYPE_PREFIX_EXPRESSION,
    EXP_TYPE_ASSIGN_EXPRESSION,
    EXP_TYPE_INFIX_EXPRESSION,
    EXP_TYPE_TERNARY_EXPRESSION,
    EXP_TYPE_CALL_EXPRESSION,
    EXP_TYPE_INDEX_EXPRESSION,

    EXP_ENUM_LENGTH,
} expression_type;

typedef struct {
    expression_list expressions;
} program;

typedef struct {
    char *value;
    size_t length;
} identifier;

typedef struct {
    int64_t value;
} int_literal;

typedef struct {
    // must be identifiers
    expression_list arguments;
    expression_reference body;
} function_literal;

typedef struct {
    expression_list values;
} list_literal;

typedef struct {
    expression_list expressions;
} block_expression;

typedef struct {
    token op;
    expression_reference right;
} prefix_expression;

typedef struct {
    expression_reference left;
    expression_reference right;
    bool constant;
} assign_expression;

typedef struct {
    token op;
    expression_reference left;
    expression_reference right;
} infix_expression;

typedef struct {
    expression_reference condition;
    expression_reference consequence;
    expression_reference alternative;
} ternary_expression;

typedef struct {
    expression_reference function;
    expression_list arguments;
} call_expression;

typedef struct {
    expression_reference list;
    expression_reference index;
} index_expression;

struct expression {
    expression_reference next;  // only used in expression list
    expression_type type;
    union {
        program             program;

        identifier          identifier;
        int_literal         int_literal;
        function_literal    function_literal;
        list_literal        list_literal;

        block_expression    block_expression;
        prefix_expression   prefix_expression;
        assign_expression   assign_expression;
        infix_expression    infix_expression;
        ternary_expression  ternary_expression;
        call_expression     call_expression;
        index_expression    index_expression;
    };
};

static inline expression new_expression(token_type tt) {
    return (expression) {
        .next = 0,
        .type = tt,
    };
}

#endif // AST_H
