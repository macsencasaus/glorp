#include "arena.h"

extern arena a;
//
// clang-format off
static const char *const expression_type_literals[EXPR_ENUM_LENGTH] = {
    [EXPR_TYPE_PROGRAM]            = "PROGRAM",           
    [EXPR_TYPE_UNIT]               = "UNIT",
    [EXPR_TYPE_IDENTIFIER]         = "IDENTIFIER",        
    [EXPR_TYPE_CHAR_LITERAL]       = "CHAR LITERAL",
    [EXPR_TYPE_INT_LITERAL]        = "INT LITERAL",
    [EXPR_TYPE_FLOAT_LITERAL]      = "FLOAT LITERAL",
    [EXPR_TYPE_STRING_LITERAL]     = "STRING LITERAL",
    [EXPR_TYPE_LIST_LITERAL]       = "LIST LITERAL",      
    [EXPR_TYPE_BLOCK_EXPRESSION]   = "BLOCK EXPRESSION",
    [EXPR_TYPE_PREFIX_EXPRESSION]  = "PREFIX EXPRESSION", 
    [EXPR_TYPE_INFIX_EXPRESSION]   = "INFIX EXPRESSION",  
    [EXPR_TYPE_TERNARY_EXPRESSION] = "TERNARY EXPRESSION",
    [EXPR_TYPE_CALL_EXPRESSION]    = "CALL EXPRESSION",   
    [EXPR_TYPE_INDEX_EXPRESSION]   = "INDEX EXPRESSION",
    [EXPR_TYPE_CASE_EXPRESSION]    = "CASE EXPRESSION",
    [EXPR_TYPE_IMPORT_EXPRESSION]  = "IMPORT EXPRESSION",
};
// clang-format on

// clang-format off
static const char *const object_type_literals[OBJECT_TYPE_ENUM_LENGTH] = {
    [OBJECT_TYPE_NULL]        = "NULL", 
    [OBJECT_TYPE_UNIT]        = "UNIT",
    [OBJECT_TYPE_CHAR]        = "CHAR",
    [OBJECT_TYPE_INT]         = "INT", 
    [OBJECT_TYPE_FLOAT]       = "FLOAT",
    [OBJECT_TYPE_FUNCTION]    = "FUNCTION", 
    [OBJECT_TYPE_LIST]        = "LIST", 
    [OBJECT_TYPE_LIST_NODE]   = "LIST NODE",
    [OBJECT_TYPE_ENVIRONMENT] = "ENVIRONMENT",
};
// clang-format on

void arena_init(arena *a) {
    ba_init(&a->expr_alloc);
    fa_init(&a->obj_alloc, sizeof(object));
}

void arena_destroy(arena *a) {
    ba_destroy(&a->expr_alloc);
    fa_destroy(&a->obj_alloc);
}

expr *new_expr(expr_type type, const token *tok) {
    expr *e = (expr *)ba_malloc(&a.expr_alloc, sizeof(expr));

    *e = (expr){
        .start_tok = tok ? *tok : (token){0},
        .type = type,
    };

    return e;
}

expr *new_expr2(expr_type type, const token *tok) {
    expr *e = (expr *)ba_malloc(&a.expr_alloc, sizeof(expr));

    *e = (expr){
        .start_tok = *tok,
        .end_tok = *tok,
        .type = type,
    };

    return e;
}

expr *new_expr3(expr_type type, const token *start, const token *end) {
    expr *e = (expr *)ba_malloc(&a.expr_alloc, sizeof(expr));

    *e = (expr){
        .start_tok = *start,
        .end_tok = *end,
        .type = type,
    };

    return e;
}

void free_last_expr(void) {
    ba_free(&a.expr_alloc, sizeof(expr));
}

object *new_obj(object_type type, size_t rc) {
    object *obj = (object *)fa_malloc(&a.obj_alloc);
    *obj = (object){
        .rc = rc,
        .type = type,
    };

    return obj;
}

object *new_empty_obj(void) {
    return (object *)fa_malloc(&a.obj_alloc);
}

object *new_copied_obj(const object *o) {
    object *r = (object *)fa_malloc(&a.obj_alloc);
    *r = *o;
    return r;
}

void free_obj(object *obj) { fa_free(&a.obj_alloc, obj); }

void cleanup(object *o) {
    switch (o->type) {
        case OBJECT_TYPE_LIST: {
            rc_dec(o->values.head);
        } break;
        case OBJECT_TYPE_LIST_NODE: {
            rc_dec(o->next);
            rc_dec(o->value);
        } break;
        case OBJECT_TYPE_ENVIRONMENT: {
            env_destroy(&o->env);
        } break;
        case OBJECT_TYPE_FUNCTION: {
            if (o->outer_env->obj != NULL) {
                rc_dec(o->outer_env->obj);
            }
        } break;
        default: {
        }
    }
    fa_free(&a.obj_alloc, o);
}

void temp_cleanup(object *o) {
    if (fa_valid_ptr(&a.obj_alloc, o))
        return;

    switch (o->type) {
        case OBJECT_TYPE_LIST: {
            rc_dec(o->values.head);
        } break;
        case OBJECT_TYPE_FUNCTION: {
            if (o->outer_env->obj != NULL)
                rc_dec(o->outer_env->obj);
        } break;
        default: {
        }
    }
}

void rc_dec(object *o) {
    if (o == NULL)
        return;

    assert(o->rc > 0);

    if (--o->rc == 0) {
        cleanup(o);
    }
}

void print_debug_info(void) {
    printf("\n---DEBUG---\n\n");
    /* arena_print_exprs(); */
    /* printf("\n------\n"); */
    arena_print_objects();
}

void arena_print_exprs(void) {
    size_t page_size = sysconf(_SC_PAGESIZE);

    size_t size = a.expr_alloc.size;

    size_t exprs = size / sizeof(expr);

    printf("EXPRESSIONS ARENA\nEXPRS: %zu\nSIZE: %zu\nCAPACITY: %zu\n",
           exprs, a.expr_alloc.size, a.expr_alloc.pages * page_size);

    printf("\nEXPRESSIONS:\n");
    expr *e;
    for (size_t i = 0; i < exprs; ++i) {
        e = (expr *)(a.expr_alloc.store + (i * sizeof(expr)));
        printf("%3zu: %s\n", i, expression_type_literals[e->type]);
    }
}

void arena_print_objects(void) {
    size_t page_size = sysconf(_SC_PAGESIZE);
    size_t capacity = a.obj_alloc.pages * page_size;

    printf("OBJECTS ARENA\nSIZE: %zu\nCAPACITY: %zu\nOBJECTS: %zu\n",
           a.obj_alloc.size, capacity, a.obj_alloc.size / a.obj_alloc.ty_size);

    size_t ty_size = a.obj_alloc.ty_size;

    for (size_t i = 0; i < capacity; i += ty_size) {
        object *o = (object *)(a.obj_alloc.store + i);
        if (!fa_valid_ptr(&a.obj_alloc, o))
            continue;

        printf("%p: type: %-11s, rc: %zu\n", (void *)o, object_type_literals[o->type], o->rc);
    }
}
