#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stdio.h>

#include "ast.h"
#include "environment.h"

typedef size_t object_reference;
typedef struct object_list object_list;

#define NULL_OBJECT_REFERENCE ((object_reference)0)

struct object_list {
    object_reference head;
    object_reference tail;
    size_t size;

    arena *a;
};

typedef enum {
    OBJECT_TYPE_NULL,
    OBJECT_TYPE_INT,
    // OBJECT_TYPE_FLOAT,
    // OBJECT_TYPE_CHAR,
    OBJECT_TYPE_FUNCTION,
    OBJECT_TYPE_LIST,
    OBJECT_TYPE_LIST_NODE,

    OBJECT_TYPE_ENUM_LENGTH,
} object_type;

typedef struct {
    int64_t value;
} int_object;

typedef struct {
    double value;
} float_object;

typedef struct {
    char value;
} char_object;

typedef struct {
    expression_list arguments;

    expression_reference body;
    environment env;
} function_object;

typedef struct {
    object_list values;
} list_object;

typedef struct {
    object_reference value;
    object_reference next;
} list_node;

typedef struct {
    size_t rc;

    // reference to itself if stored in the arena
    object_reference ref;

    object_type type;
    union {
        int_object int_object;
        // float_object float_object;
        // char_object char_object;
        function_object function_object;
        list_object list_object;
        list_node list_node;
    };
} object;

typedef struct {
    object_reference ref;
    object *obj;
    object_reference ln_ref;
    object_list *ol;
} object_list_iterator;

object new_object(object_type ot);

void object_list_init(object_list *ol, arena *a);
void ol_append(object_list *ol, object_reference ref);
object_list_iterator ol_start(object_list *ol);
object_list_iterator ol_end(object_list *ol);

void oli_next(object_list_iterator *oli);
bool oli_eq(object_list_iterator *a, object_list_iterator *b);

void inspect(object *obj);

#endif  // OBJECT_H
