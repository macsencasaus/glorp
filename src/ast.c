#include "ast.h"

#include <string.h>

#include "arena.h"

#define INDENT ((int)(4 * indent)), ""
#define INDENT_FMT "%*s"

typedef void print_expression_fn(arena *a, expression_reference ref,
                                 size_t indent);

static print_expression_fn print_expression;

static print_expression_fn print_program;

static print_expression_fn print_identifier;
static print_expression_fn print_int_literal;
static print_expression_fn print_list_literal;

static print_expression_fn print_block_expression;
static print_expression_fn print_prefix_expression;
static print_expression_fn print_assign_expression;
static print_expression_fn print_infix_expression;
static print_expression_fn print_ternary_expression;
static print_expression_fn print_call_expression;
static print_expression_fn print_index_expression;

static void print_expression_list(arena *a, expression_list el, size_t indent);

// clang-format off
static print_expression_fn *print_expression_fns[EXP_ENUM_LENGTH] = {
    print_program,

    print_identifier,
    print_int_literal,
    print_list_literal,

    print_block_expression,
    print_prefix_expression,
    print_assign_expression,
    print_infix_expression,
    print_ternary_expression,
    print_call_expression,
    print_index_expression,
};
// clang-format on

expression new_expression(expression_type et, token token) {
    return (expression){
        .token = token,
        .next = 0,
        .type = et,
    };
}

void print_ast(arena *a, expression_reference program) {
    print_expression(a, program, 0);
}

static void print_expression(arena *a, expression_reference ref,
                             size_t indent) {
    expression *e = get_expression(a, ref);
    print_expression_fn *print = print_expression_fns[e->type];
    print(a, ref, indent);
}

static void print_program(arena *a, expression_reference ref, size_t indent) {
    expression *program = get_expression(a, ref);
    expression_list expressions = program->program.expressions;

    printf(INDENT_FMT "%s(%ld)\n", INDENT, "PROGRAM", expressions.size);
    ++indent;

    print_expression_list(a, expressions, indent);
}

static void print_identifier(arena *a, expression_reference ref,
                             size_t indent) {
    expression *identifier = get_expression(a, ref);

    size_t ident_length = identifier->identifier.length;
    char buf[ident_length + 1];
    buf[ident_length] = 0;
    strncpy(buf, identifier->identifier.value, ident_length);

    printf(INDENT_FMT "%s %s\n", INDENT, "IDENTIFIER", buf);
}

static void print_int_literal(arena *a, expression_reference ref,
                              size_t indent) {
    expression *int_literal = get_expression(a, ref);
    int64_t value = int_literal->int_literal.value;

    printf(INDENT_FMT "%s %ld\n", INDENT, "INT LITERAL", value);
}

static void print_list_literal(arena *a, expression_reference ref,
                               size_t indent) {
    expression *list_literal = get_expression(a, ref);

    printf(INDENT_FMT "%s\n", INDENT, "LIST LITERAL");
    ++indent;

    expression_list expressions = list_literal->list_literal.values;

    printf(INDENT_FMT "%s(%ld):\n", INDENT, "expressions", expressions.size);
    ++indent;

    print_expression_list(a, expressions, indent);
}

static void print_block_expression(arena *a, expression_reference ref,
                                   size_t indent) {
    expression *block_expression = get_expression(a, ref);
    expression_list expressions =
        block_expression->block_expression.expressions;

    printf(INDENT_FMT "%s(%ld):\n", INDENT, "BLOCK", expressions.size);
    ++indent;

    print_expression_list(a, expressions, indent);
}

static void print_prefix_expression(arena *a, expression_reference ref,
                                    size_t indent) {
    expression *prefix_expression = get_expression(a, ref);

    printf(INDENT_FMT "%s:\n", INDENT, "PREFIX EXPRESSION");
    ++indent;

    token op = prefix_expression->prefix_expression.op;
    expression_reference right = prefix_expression->prefix_expression.right;

    printf(INDENT_FMT "%s: %s\n", INDENT, "OP", token_type_literals[op.type]);

    printf(INDENT_FMT "%s:\n", INDENT, "RIGHT");
    ++indent;

    print_expression(a, right, indent);
}

static void print_assign_expression(arena *a, expression_reference ref,
                                    size_t indent) {
    expression *assign_expression = get_expression(a, ref);

    expression_reference left = assign_expression->assign_expression.left;
    expression_reference right = assign_expression->assign_expression.right;
    bool constant = assign_expression->assign_expression.constant;

    printf(INDENT_FMT "%s%s\n", INDENT, constant ? "CONST " : "",
           "ASSIGN EXPRESSION");
    ++indent;

    printf(INDENT_FMT "%s:\n", INDENT, "LEFT");
    ++indent;

    print_expression(a, left, indent);
    --indent;

    printf(INDENT_FMT "%s:\n", INDENT, "RIGHT");
    ++indent;

    print_expression(a, right, indent);
}

static void print_infix_expression(arena *a, expression_reference ref,
                                   size_t indent) {
    expression *infix_expression = get_expression(a, ref);

    token op = infix_expression->infix_expression.op;
    expression_reference left = infix_expression->infix_expression.left;
    expression_reference right = infix_expression->infix_expression.right;

    printf(INDENT_FMT "%s\n", INDENT, "INFIX EXPRESSION");
    ++indent;

    printf(INDENT_FMT "%s: %s\n", INDENT, "OP", token_type_literals[op.type]);

    printf(INDENT_FMT "%s:\n", INDENT, "LEFT");
    ++indent;

    print_expression(a, left, indent);
    --indent;

    printf(INDENT_FMT "%s:\n", INDENT, "RIGHT");
    ++indent;

    print_expression(a, right, indent);
}

static void print_ternary_expression(arena *a, expression_reference ref,
                                     size_t indent) {
    expression *ternary_expression = get_expression(a, ref);

    expression_reference condition =
        ternary_expression->ternary_expression.condition;
    expression_reference consequence =
        ternary_expression->ternary_expression.consequence;
    expression_reference alternative =
        ternary_expression->ternary_expression.alternative;

    printf(INDENT_FMT "%s\n", INDENT, "TERNARY EXPRESSION");
    ++indent;

    printf(INDENT_FMT "%s\n", INDENT, "CONDITION:");
    ++indent;

    print_expression(a, condition, indent);
    --indent;

    printf(INDENT_FMT "%s\n", INDENT, "CONSEQUENCE:");
    ++indent;

    print_expression(a, consequence, indent);
    --indent;

    printf(INDENT_FMT "%s\n", INDENT, "ALTERNATIVE:");
    ++indent;

    print_expression(a, alternative, indent);
}

static void print_call_expression(arena *a, expression_reference ref,
                                  size_t indent) {
    expression *call_expression = get_expression(a, ref);

    expression_reference function = call_expression->call_expression.function;

    expression_list arguments = call_expression->call_expression.arguments;

    printf(INDENT_FMT "%s\n", INDENT, "CALL EXPRESSION");
    ++indent;

    printf(INDENT_FMT "%s\n", INDENT, "FUNCTION:");
    ++indent;

    print_expression(a, function, indent);
    --indent;

    printf(INDENT_FMT "%s(%ld):\n", INDENT, "ARGUMENTS", arguments.size);
    ++indent;

    print_expression_list(a, arguments, indent);
}

static void print_index_expression(arena *a, expression_reference ref,
                                   size_t indent) {
    expression *index_expression = get_expression(a, ref);

    expression_reference list = index_expression->index_expression.list;
    expression_reference index = index_expression->index_expression.index;

    printf(INDENT_FMT "%s\n", INDENT, "INDEX EXPRESSION");
    ++indent;

    printf(INDENT_FMT "%s\n", INDENT, "LIST:");
    ++indent;

    print_expression(a, list, indent);
    --indent;

    printf(INDENT_FMT "%s\n", INDENT, "INDEX:");
    ++indent;

    print_expression(a, index, indent);
}

static void print_expression_list(arena *a, expression_list el, size_t indent) {
    if (el.size == 0) return;
    expression_reference ref = el.head;
    expression *exp = get_expression(a, ref);

    do {
        print_expression(a, ref, indent);
        ref = exp->next;
        exp = get_expression(a, ref);
    } while (ref != 0);
}

expression_list new_expression_list(arena *a) {
    return (expression_list){
        .a = a,
    };
}

void el_append(expression_list *el, expression_reference ref) {
    if (el->size == 0) {
        el->head = ref;
    } else {
        el->a->expressions[el->tail].next = ref;
    }
    el->tail = ref;
    ++el->size;
}

el_iterator el_start(expression_list *el) {
    return (el_iterator){
        .ref = el->head,
        .exp = get_expression(el->a, el->head),
        .el = el,
    };
}

el_iterator el_end(expression_list *el) {
    return (el_iterator){
        .ref = 0,
        .exp = NULL,
        .el = el,
    };
}

void eli_next(el_iterator *eli) {
    eli->ref = eli->exp->next;
    eli->exp = get_expression(eli->el->a, eli->exp->next);
}

bool eli_eq(el_iterator *a, el_iterator *b) { return a->ref == b->ref; }
