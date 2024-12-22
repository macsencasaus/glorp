#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_ARENA_CAPACITY 1024
#define INDENT ((int)(4 * indent)), ""

static void arena_free(arena *a, expression *exp);
static void ensure_capacity(arena *a);

arena new_arena() {
    void *mem = malloc(sizeof(size_t) * INITIAL_ARENA_CAPACITY +
                       sizeof(expression) * INITIAL_ARENA_CAPACITY +
                       sizeof(object) * INITIAL_ARENA_CAPACITY);

    expression_reference *available = (expression_reference *)mem;
    expression *expressions =
        (expression *)(available + INITIAL_ARENA_CAPACITY);
    object *objects = (object *)(expressions + INITIAL_ARENA_CAPACITY);

    for (size_t i = 0; i < INITIAL_ARENA_CAPACITY; ++i) {
        available[i] = i;
    }

    return (arena){
        .size = 0,
        .capacity = INITIAL_ARENA_CAPACITY,

        .start_idx = 0,
        .end_idx = 0,
        .available = available,

        .expressions = expressions,
        .objects = objects,

        .mem = mem,
    };
}

expression_reference arena_alloc(arena *a, expression exp) {
    ensure_capacity(a);

    expression_reference ref = a->available[a->start_idx];
    a->start_idx = (a->start_idx + 1) % a->capacity;

    a->expressions[ref] = exp;
    ++a->size;

    return ref;
}

expression *get_expression(arena *a, expression_reference ref) {
    return a->expressions + ref;
}

static void arena_free(arena *a, expression *exp) {
    size_t idx = exp - a->expressions;
    a->available[a->end_idx] = idx;
    a->end_idx = (a->end_idx + 1) % a->capacity;
    --a->size;
}

// TODO:
static void ensure_capacity(arena *a) {
    if (a->size == a->capacity) {
        assert(a->start_idx == a->end_idx);
        fprintf(stderr, "TODO: ensure_capacity, size == capacity\n");
        exit(1);
    }
}

// print utilities

typedef void print_expression_fn(arena *a, expression_reference ref,
                                 size_t indent);

static print_expression_fn print_expression;

static print_expression_fn print_program;

static print_expression_fn print_identifier;
static print_expression_fn print_int_literal;
static print_expression_fn print_function_literal;
static print_expression_fn print_list_literal;

static print_expression_fn print_block_expression;
static print_expression_fn print_prefix_expression;
static print_expression_fn print_assign_expression;
static print_expression_fn print_infix_expression;
static print_expression_fn print_ternary_expression;
static print_expression_fn print_call_expression;
static print_expression_fn print_index_expression;

static void print_expression_list(arena *a, expression_list el, size_t indent);

static print_expression_fn *print_expression_fns[EXP_ENUM_LENGTH] = {
    print_program,

    print_identifier,       print_int_literal,        print_function_literal,
    print_list_literal,

    print_block_expression, print_prefix_expression,  print_assign_expression,
    print_infix_expression, print_ternary_expression, print_call_expression,
    print_index_expression,
};

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

    printf("%*s%s(%ld)\n", INDENT, "PROGRAM", expressions.size);
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

    printf("%*s%s %s\n", INDENT, "IDENTIFIER", buf);
}

static void print_int_literal(arena *a, expression_reference ref,
                              size_t indent) {
    expression *int_literal = get_expression(a, ref);
    int64_t value = int_literal->int_literal.value;

    printf("%*s%s %ld\n", INDENT, "INT LITERAL", value);
}

static void print_function_literal(arena *a, expression_reference ref,
                                   size_t indent) {
    expression *function_literal = get_expression(a, ref);

    printf("%*s%s\n", INDENT, "FUNCTION LITERAL");
    ++indent;

    expression_list arguments = function_literal->function_literal.arguments;

    printf("%*s%s(%ld):\n", INDENT, "ARGUMENTS", arguments.size);
    ++indent;

    print_expression_list(a, arguments, indent);

    --indent;

    printf("%*s%s:\n", INDENT, "BODY");
    ++indent;

    expression_reference body = function_literal->function_literal.body;

    print_expression(a, body, indent);
}

static void print_list_literal(arena *a, expression_reference ref,
                               size_t indent) {
    expression *list_literal = get_expression(a, ref);

    printf("%*s%s\n", INDENT, "LIST LITERAL");
    ++indent;

    expression_list expressions = list_literal->list_literal.values;

    printf("%*s%s(%ld):\n", INDENT, "expressions", expressions.size);
    ++indent;

    print_expression_list(a, expressions, indent);
}

static void print_block_expression(arena *a, expression_reference ref,
                                   size_t indent) {
    expression *block_expression = get_expression(a, ref);
    expression_list expressions =
        block_expression->block_expression.expressions;

    printf("%*s%s(%ld):\n", INDENT, "BLOCK", expressions.size);
    ++indent;

    print_expression_list(a, expressions, indent);
}

static void print_prefix_expression(arena *a, expression_reference ref,
                                    size_t indent) {
    expression *prefix_expression = get_expression(a, ref);

    printf("%*s%s:\n", INDENT, "PREFIX EXPRESSION");
    ++indent;

    token op = prefix_expression->prefix_expression.op;
    expression_reference right = prefix_expression->prefix_expression.right;

    printf("%*s%s: %s\n", INDENT, "OP", token_type_literals[op.type]);

    printf("%*s%s:\n", INDENT, "RIGHT");
    ++indent;

    print_expression(a, right, indent);
}

static void print_assign_expression(arena *a, expression_reference ref,
                                    size_t indent) {
    expression *assign_expression = get_expression(a, ref);

    expression_reference left = assign_expression->assign_expression.left;
    expression_reference right = assign_expression->assign_expression.right;
    bool constant = assign_expression->assign_expression.constant;

    printf("%*s%s%s\n", INDENT, constant ? "CONST " : "", "ASSIGN EXPRESSION");
    ++indent;

    printf("%*s%s:\n", INDENT, "LEFT");
    ++indent;

    print_expression(a, left, indent);
    --indent;

    printf("%*s%s:\n", INDENT, "RIGHT");
    ++indent;

    print_expression(a, right, indent);
}

static void print_infix_expression(arena *a, expression_reference ref,
                                   size_t indent) {
    expression *infix_expression = get_expression(a, ref);

    token op = infix_expression->infix_expression.op;
    expression_reference left = infix_expression->infix_expression.left;
    expression_reference right = infix_expression->infix_expression.right;

    printf("%*s%s\n", INDENT, "INFIX EXPRESSION");
    ++indent;

    printf("%*s%s: %s\n", INDENT, "OP", token_type_literals[op.type]);

    printf("%*s%s:\n", INDENT, "LEFT");
    ++indent;

    print_expression(a, left, indent);
    --indent;

    printf("%*s%s:\n", INDENT, "RIGHT");
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

    printf("%*s%s\n", INDENT, "TERNARY EXPRESSION");
    ++indent;

    printf("%*s%s\n", INDENT, "CONDITION:");
    ++indent;

    print_expression(a, condition, indent);
    --indent;

    printf("%*s%s\n", INDENT, "CONSEQUENCE:");
    ++indent;

    print_expression(a, consequence, indent);
    --indent;

    printf("%*s%s\n", INDENT, "ALTERNATIVE:");
    ++indent;

    print_expression(a, alternative, indent);
}

static void print_call_expression(arena *a, expression_reference ref,
                                  size_t indent) {
    expression *call_expression = get_expression(a, ref);

    expression_reference function = call_expression->call_expression.function;

    expression_list arguments = call_expression->call_expression.arguments;

    printf("%*s%s\n", INDENT, "CALL EXPRESSION");
    ++indent;

    printf("%*s%s\n", INDENT, "FUNCTION:");
    ++indent;

    print_expression(a, function, indent);
    --indent;

    printf("%*s%s(%ld):\n", INDENT, "ARGUMENTS", arguments.size);
    ++indent;

    print_expression_list(a, arguments, indent);
}

static void print_index_expression(arena *a, expression_reference ref,
                                   size_t indent) {
    expression *index_expression = get_expression(a, ref);

    expression_reference list = index_expression->index_expression.list;
    expression_reference index = index_expression->index_expression.index;

    printf("%*s%s\n", INDENT, "INDEX EXPRESSION");
    ++indent;

    printf("%*s%s\n", INDENT, "LIST:");
    ++indent;

    print_expression(a, list, indent);
    --indent;

    printf("%*s%s\n", INDENT, "INDEX:");
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
