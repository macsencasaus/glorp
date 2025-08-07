#include "ast.h"

#include <stdio.h>
#include <string.h>

#define INDENT ((int)(4 * indent)), ""
#define INDENT_FMT "%*s"

typedef void print_expression_fn(const expr *e, size_t indent);

static print_expression_fn print_expression;

static print_expression_fn print_program;

static print_expression_fn print_unit;

static print_expression_fn print_identifier;
static print_expression_fn print_char_literal;
static print_expression_fn print_int_literal;
static print_expression_fn print_float_literal;
static print_expression_fn print_string_literal;
static print_expression_fn print_list_literal;

static print_expression_fn print_block_expression;
static print_expression_fn print_prefix_expression;
static print_expression_fn print_infix_expression;
static print_expression_fn print_ternary_expression;
static print_expression_fn print_call_expression;
static print_expression_fn print_index_expression;
static print_expression_fn print_case_expression;

static print_expression_fn print_import_expression;

static void print_expression_list(const expr_list *el, size_t indent);

// clang-format off
static print_expression_fn *print_expression_fns[EXPR_ENUM_LENGTH] = {
    [EXPR_TYPE_PROGRAM]            = print_program,

    [EXPR_TYPE_UNIT]               = print_unit,

    [EXPR_TYPE_IDENTIFIER]         = print_identifier,
    [EXPR_TYPE_CHAR_LITERAL]       = print_char_literal,
    [EXPR_TYPE_INT_LITERAL]        = print_int_literal,
    [EXPR_TYPE_FLOAT_LITERAL]      = print_float_literal,
    [EXPR_TYPE_STRING_LITERAL]     = print_string_literal,
    [EXPR_TYPE_LIST_LITERAL]       = print_list_literal,

    [EXPR_TYPE_BLOCK_EXPRESSION]   = print_block_expression,
    [EXPR_TYPE_PREFIX_EXPRESSION]  = print_prefix_expression,
    [EXPR_TYPE_INFIX_EXPRESSION]   = print_infix_expression,
    [EXPR_TYPE_TERNARY_EXPRESSION] = print_ternary_expression,
    [EXPR_TYPE_CALL_EXPRESSION]    = print_call_expression,
    [EXPR_TYPE_INDEX_EXPRESSION]   = print_index_expression,
    [EXPR_TYPE_CASE_EXPRESSION]    = print_case_expression,

    [EXPR_TYPE_IMPORT_EXPRESSION]  = print_import_expression,
};
// clang-format on

void print_ast(expr *program) { print_expression(program, 0); }

static void print_expression(const expr *e, size_t indent) {
    print_expression_fn *print = print_expression_fns[e->type];
    print(e, indent);
}

static void print_program(const expr *program, size_t indent) {
    const expr_list *expressions = &program->expressions;

    printf(INDENT_FMT "PROGRAM(%ld)\n", INDENT, expressions->size);
    ++indent;

    print_expression_list(expressions, indent);
}

static void print_unit(const expr *e, size_t indent) {
    (void)e;
    printf(INDENT_FMT "UNIT\n", INDENT);
}

static void print_identifier(const expr *ident, size_t indent) {
    printf(INDENT_FMT "IDENTIFIER %.*s\n", INDENT, (int)ident->length, ident->literal);
}

static void print_char_literal(const expr *char_literal, size_t indent) {
    printf(INDENT_FMT "CHAR LITERAL %c\n", INDENT, char_literal->char_value);
}

static void print_int_literal(const expr *int_literal, size_t indent) {
    printf(INDENT_FMT "INT LITERAL %ld\n", INDENT, int_literal->int_value);
}

static void print_float_literal(const expr *float_literal, size_t indent) {
    printf(INDENT_FMT "FLOAT LITERAL %lf\n", INDENT, float_literal->float_value);
}

static void print_string_literal(const expr *string_literal, size_t indent) {
    printf(INDENT_FMT "STRING LITERAL \"%.*s\"\n", INDENT, 
           (int)string_literal->length, string_literal->literal);
}

static void print_list_literal(const expr *list_literal, size_t indent) {
    printf(INDENT_FMT "%s\n", INDENT, "LIST LITERAL");
    ++indent;

    const expr_list *expressions = &list_literal->expressions;

    printf(INDENT_FMT "expressions(%ld):\n", INDENT, expressions->size);
    ++indent;

    print_expression_list(expressions, indent);
}

static void print_block_expression(const expr *block_expression, size_t indent) {
    const expr_list *expressions = &block_expression->expressions;

    printf(INDENT_FMT "BLOCK(%ld):\n", INDENT, expressions->size);
    ++indent;

    print_expression_list(expressions, indent);
}

static void print_prefix_expression(const expr *prefix_expr, size_t indent) {
    printf(INDENT_FMT "PREFIX_EXPERSSION:\n", INDENT);
    ++indent;

    const token *op = &prefix_expr->op;
    expr *right = prefix_expr->right;

    printf(INDENT_FMT "OP: %s\n", INDENT, token_type_literals[op->type]);

    printf(INDENT_FMT "RIGHT:\n", INDENT);
    ++indent;

    print_expression(right, indent);
}

static void print_infix_expression(const expr *infix_expr, size_t indent) {
    const token *op = &infix_expr->op;
    expr *left = infix_expr->left;
    expr *right = infix_expr->right;

    printf(INDENT_FMT "INFIX EXPRESSION\n", INDENT);
    ++indent;

    printf(INDENT_FMT "OP: %s\n", INDENT, token_type_literals[op->type]);

    printf(INDENT_FMT "LEFT:\n", INDENT);
    ++indent;

    print_expression(left, indent);
    --indent;

    printf(INDENT_FMT "RIGHT:\n", INDENT);
    ++indent;

    print_expression(right, indent);
}

static void print_ternary_expression(const expr *ternary_expr, size_t indent) {
    expr *condition = ternary_expr->condition;
    expr *consequence = ternary_expr->consequence;
    expr *alternative = ternary_expr->alternative;

    printf(INDENT_FMT "TERNARY EXPRESSION\n", INDENT);
    ++indent;

    printf(INDENT_FMT "CONDITION:\n", INDENT);
    ++indent;

    print_expression(condition, indent);
    --indent;

    printf(INDENT_FMT "CONSEQUENCE:\n", INDENT);
    ++indent;

    print_expression(consequence, indent);
    --indent;

    printf(INDENT_FMT "ALTERNATIVE:\n", INDENT);
    ++indent;

    print_expression(alternative, indent);
}

static void print_call_expression(const expr *call_expr, size_t indent) {
    expr *function = call_expr->function;
    const expr_list *params = &call_expr->params;

    printf(INDENT_FMT "CALL EXPRESSION\n", INDENT);
    ++indent;

    printf(INDENT_FMT "FUNCTION:\n", INDENT);
    ++indent;

    print_expression(function, indent);
    --indent;

    printf(INDENT_FMT "ARGUMENTS(%ld):\n", INDENT, params->size);
    ++indent;

    print_expression_list(params, indent);
}

static void print_index_expression(const expr *index_expr, size_t indent) {
    expr *list = index_expr->list;
    expr *index = index_expr->index;

    printf(INDENT_FMT "INDEX EXPRESSION\n", INDENT);
    ++indent;

    printf(INDENT_FMT "LIST:\n", INDENT);
    ++indent;

    print_expression(list, indent);
    --indent;

    printf(INDENT_FMT "INDEX:\n", INDENT);
    ++indent;

    print_expression(index, indent);
}

static void print_case_expression(const expr *case_expr, size_t indent) {
    printf(INDENT_FMT "CASE EXPRESSION\n", INDENT);

    size_t cases = case_expr->conditions.size;

    printf(INDENT_FMT "cases(%zu):\n", INDENT, cases);
    ++indent;

    const expr *condition = case_expr->conditions.head;
    const expr *result = case_expr->results.head;

    for (size_t i = 0; i < cases; ++i) {
        printf(INDENT_FMT "CONDITION:\n", INDENT);
        ++indent;
        print_expression(condition, indent);
        --indent;
        printf(INDENT_FMT "RESULT:\n", INDENT);
        ++indent;
        print_expression(result, indent);
        --indent;

        condition = condition->next;
        result = result->next;
    }
}

static void print_import_expression(const expr *import_expr, size_t indent) {
    printf(INDENT_FMT "IMPORT EXPRESSION %.*s\n", INDENT,
           (int)import_expr->length, import_expr->literal);
}

static void print_expression_list(const expr_list *el, size_t indent) {
    if (el->size == 0) return;
    const expr *e = el->head;

    while (e) {
        print_expression(e, indent);
        e = e->next;
    }
}

void el_append(expr_list *el, expr *e) {
    if (el->size == 0) {
        el->head = e;
    } else {
        el->tail->next = e;
    }
    el->tail = e;
    ++el->size;
}
