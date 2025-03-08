#ifndef ARENA_H
#define ARENA_H

#include <stdlib.h>

#include "ast.h"
#include "object.h"

struct arena {
    size_t expressions_size;
    size_t expressions_capacity;

    size_t expressions_start_idx;
    size_t expressions_end_idx;
    expression_reference *available_expressions;
    expression *expressions;

    size_t objects_size;
    size_t objects_capacity;

    size_t objects_start_idx;
    size_t objects_end_idx;
    object_reference *available_objects;
    object *objects;

    void *expressions_mem;
    void *objects_mem;
};

typedef struct arena arena;

void arena_init(arena *a);
void arena_clear_expressions(arena *a);
bool is_null_expression(arena *a, expression_reference ref);
bool is_null_object(arena *a, object_reference ref);

// expressions

expression_reference arena_alloc_expression(arena *a, expression exp);
expression *get_expression(arena *a, expression_reference ref);

// objects

object_reference arena_alloc_object(arena *a, object obj);
object *get_object(arena *a, object_reference ref);
void inspect_object(arena *a, object_reference ref);

#ifdef DEBUG

void print_debug_info(arena *a);

#endif  // DEBUG

#endif  // ARENA_H
