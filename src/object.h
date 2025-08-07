#ifndef OBJECT_H
#define OBJECT_H

#include <stdint.h>
#include <stdio.h>

#include "ast.h"
#include "environment.h"
#include "sb.h"

typedef struct object_list object_list;

struct object_list {
    object *head;
    object *tail;
    size_t size;
};

typedef enum {
    OBJECT_TYPE_NULL,

    OBJECT_TYPE_UNIT,

    OBJECT_TYPE_CHAR,
    OBJECT_TYPE_INT,
    OBJECT_TYPE_FLOAT,

    OBJECT_TYPE_FUNCTION,
    OBJECT_TYPE_LIST,

    OBJECT_TYPE_LVALUE,

    // don't get evaluated
    OBJECT_TYPE_LIST_NODE,
    OBJECT_TYPE_ENVIRONMENT,

    OBJECT_TYPE_ENUM_LENGTH,
} object_type;

typedef bool builtin_fn_t(const expr_list *params, const expr *call, environment *env, object *result);

typedef struct object object;

struct object {
    size_t rc;

    object_type type;
    union {
        // char
        char char_value;

        // int
        int64_t int_value;

        // float
        double float_value;

        // function
        struct {
            bool builtin;
            union {
                // normal functions
                struct {
                    expr_list params;
                    const expr *body;
                };

                // builtin functions
                struct {
                    size_t builtin_param_count;
                    builtin_fn_t *builtin_fn;
                };
            };

            environment *outer_env;
        };

        // list
        object_list values;

        // lvalue
        struct {
            object *ref;  // reference to heap allocated object
            bool is_const;
        };

        // list node
        struct {
            object *value;
            object *next;
        };

        // environment
        environment env;
    };
};

typedef struct {
    object *obj;
    object *ln;
} ol_iterator;

void ol_append(object_list *ol, object *obj);
ol_iterator ol_start(const object_list *ol);

void oli_next(ol_iterator *oli);
bool oli_is_end(const ol_iterator *oli);

void inspect(const object *obj, String_Builder *sb, bool from_print);

#endif  // OBJECT_H
