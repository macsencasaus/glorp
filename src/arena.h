#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

#include "ast.h"
#include "object.h"

typedef struct {
    size_t size;
    size_t capacity;

    size_t start_idx;
    size_t end_idx;
    expression_reference *available;

    expression *expressions;
    object *objects;

    void *mem;
} arena;

arena new_arena();

// expressions

expression_reference arena_alloc(arena *a, expression exp);

// unsafe (don't use expression after another alloc)
expression *get_expression(arena *a, expression_reference ref);

static inline expression_list new_expression_list() {
    return (expression_list){0};
}

static inline void el_append(arena *a, expression_list *el,
                             expression_reference val) {
    if (el->size == 0) {
        el->head = val;
    } else {
        a->expressions[el->tail].next = val;
    }
    el->tail = val;
    ++el->size;
}

void print_ast(arena *a, expression_reference program);

// objects

#endif  // ARENA_H
