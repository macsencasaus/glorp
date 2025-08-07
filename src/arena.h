#ifndef ARENA_H
#define ARENA_H

#include "ast.h"
#include "bumpalloc.h"
#include "fixedalloc.h"
#include "object.h"

typedef struct {
    bump_alloc expr_alloc;
    fixed_alloc obj_alloc;
} arena;

void arena_init(arena *);
void arena_destroy(arena *);

expr *new_expr(expr_type, const token *);
expr *new_expr2(expr_type, const token *);
expr *new_expr3(expr_type, const token *start, const token *end);

object *new_obj(object_type, size_t rc);
object *new_empty_obj(void);
object *new_copied_obj(const object *);
void free_obj(object *);

void cleanup(object *);
void temp_cleanup(object *);
void rc_dec(object *);

void print_debug_info(void);
void arena_print_exprs(void);
void arena_print_objects(void);

#endif  // ARENA_H
