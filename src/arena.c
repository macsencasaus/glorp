#include "arena.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_ARENA_CAPACITY 1024

static void ensure_expression_capacity(arena *a);
static void ensure_object_capacity(arena *a);

void arena_init(arena *a) {
    a->expressions_mem =
        malloc(sizeof(expression_reference) * INITIAL_ARENA_CAPACITY +
               sizeof(expression) * INITIAL_ARENA_CAPACITY);

    a->objects_mem = malloc(sizeof(object_reference) * INITIAL_ARENA_CAPACITY +
                            sizeof(expression) * INITIAL_ARENA_CAPACITY);

    a->available_expressions = (expression_reference *)a->expressions_mem;
    a->expressions =
        (expression *)(a->available_expressions + INITIAL_ARENA_CAPACITY);

    a->available_objects = (object_reference *)a->objects_mem;
    a->objects = (object *)(a->available_objects + INITIAL_ARENA_CAPACITY);

    // null
    a->objects[NULL_OBJECT_REFERENCE] = (object){0};

    for (size_t i = 0; i < INITIAL_ARENA_CAPACITY; ++i) {
        a->available_expressions[i] = i;
    }

    for (size_t i = 1; i < INITIAL_ARENA_CAPACITY; ++i) {
        a->available_objects[i] = i;
    }

    a->expressions_size = 0;
    a->expressions_capacity = INITIAL_ARENA_CAPACITY;

    a->expressions_start_idx = 0;
    a->expressions_end_idx = 0;

    a->objects_size = 1;
    a->objects_capacity = INITIAL_ARENA_CAPACITY;

    a->objects_start_idx = 1;
    a->objects_end_idx = 1;
}

void arena_clear_expressions(arena *a) {
    a->expressions_size = 0;
    a->expressions_start_idx = 0;
    a->expressions_end_idx = 0;

    for (size_t i = 0; i < INITIAL_ARENA_CAPACITY; ++i) {
        a->available_expressions[i] = i;
    }
}

bool is_null_expression(arena *a, expression_reference ref) {
    return get_expression(a, ref) == NULL;
}

bool is_null_object(arena *a, expression_reference ref) {
    return get_object(a, ref) == NULL;
}

expression_reference arena_alloc_expression(arena *a, expression exp) {
    ensure_expression_capacity(a);

    expression_reference ref =
        a->available_expressions[a->expressions_start_idx];
    a->expressions_start_idx =
        (a->expressions_start_idx + 1) % a->expressions_capacity;

    a->expressions[ref] = exp;
    ++a->expressions_size;

    return ref;
}

expression *get_expression(arena *a, expression_reference ref) {
    return a->expressions + ref;
}

static void ensure_expression_capacity(arena *a) {
    if (a->expressions_size == a->expressions_capacity) {
        assert(a->expressions_start_idx == a->expressions_end_idx);

        size_t new_capacity = 2 * a->expressions_capacity;
        void *temp_mem = a->expressions_mem;
        void *temp_expressions = a->expressions;

        a->expressions_mem = malloc(
            new_capacity * (sizeof(expression) + sizeof(expression_reference)));

        a->available_expressions = (expression_reference *)a->expressions_mem;
        a->expressions =
            (expression *)(a->available_expressions + new_capacity);

        memcpy(a->expressions, temp_expressions,
               sizeof(expression) * a->expressions_capacity);

        a->expressions_start_idx = a->expressions_capacity;
        a->expressions_end_idx = 0;
        a->expressions_capacity = new_capacity;

        for (size_t i = a->expressions_start_idx; i < a->expressions_capacity;
             ++i) {
            a->available_expressions[i] = i;
        }

        free(temp_mem);
    }
}

static void ensure_object_capacity(arena *a) {
    if (a->objects_size == a->objects_capacity) {
        assert(a->objects_start_idx == a->objects_end_idx);

        size_t new_capacity = 2 * a->objects_capacity;
        void *temp_mem = a->objects_mem;
        void *temp_objects = a->objects;

        a->objects_mem =
            malloc(new_capacity * (sizeof(object) + sizeof(object_reference)));

        a->available_objects = (object_reference *)a->expressions_mem;
        a->objects = (object *)(a->available_objects + new_capacity);

        memcpy(a->objects, temp_objects, sizeof(object) * a->objects_capacity);

        a->objects_start_idx = a->objects_capacity;
        a->objects_end_idx = 0;
        a->objects_capacity = new_capacity;

        for (size_t i = a->objects_start_idx; i < a->objects_capacity; ++i) {
            a->available_objects[i] = i;
        }

        free(temp_mem);
    }
}

// object

object_reference arena_alloc_object(arena *a, object obj) {
    ensure_object_capacity(a);
    obj.rc = 1;

    object_reference ref = a->available_objects[a->objects_start_idx];
    a->objects_start_idx = (a->objects_start_idx + 1) % a->objects_capacity;

    obj.ref = ref;
    a->objects[ref] = obj;
    ++a->objects_size;

    return ref;
}

object *get_object(arena *a, object_reference ref) { return a->objects + ref; }

#ifdef DEBUG

static const char *const expression_type_literals[EXP_ENUM_LENGTH] = {
    "PROGRAM",           "IDENTIFIER",
    "INT LITERAL",       "FUNCTION LITERAL",
    "LIST LITERAL",      "BLOCK EXPRESSION",
    "PREFIX EXPRESSION", "ASSIGN EXPRESSION",
    "INFIX EXPRESSION",  "TERNARY EXPRESSION",
    "CALL EXPRESSION",   "INDEX EXPRESSION",
};

static const char *const object_type_literals[OBJECT_TYPE_ENUM_LENGTH] = {
    "NULL", "INT", "FUNCTION", "LIST", "LIST NODE",
};

static void print_expression_arena(arena *a);
static void print_object_arena(arena *a);

void print_debug_info(arena *a) {
    printf("\n---DEBUG---\n\n");
    print_expression_arena(a);
    printf("\n------\n");
    print_object_arena(a);
}

static void print_expression_arena(arena *a) {
    printf("EXPRESSIONS ARENA\nSIZE: %lu\nCAPACITY: %lu\n", a->expressions_size,
           a->expressions_capacity);

    printf("\nEXPRESSIONS:\n");
    expression *expression;
    for (size_t i = 0; i < a->expressions_size; ++i) {
        expression = a->expressions + i;
        printf("%s\n", expression_type_literals[expression->type]);
    }
}

static void print_object_arena(arena *a) {
    printf("OBJECTS ARENA\nSIZE: %lu\nCAPACITY: %lu\n", a->objects_size,
           a->objects_capacity);

    printf("\nOBJECTS:\n");
    object *object;
    for (size_t i = 0; i < a->objects_size; ++i) {
        object = a->objects + i;
        printf("%s\n", object_type_literals[object->type]);
    }
}
#endif  // DEBUG
