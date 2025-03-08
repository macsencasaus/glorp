#include "evaluator.h"

#include <string.h>

#include "arena.h"

typedef object eval_fn(expression_reference ref, environment *env);

static eval_fn eval_program;
static eval_fn eval_identifier;
static eval_fn eval_int_literal;
static eval_fn eval_function_literal;
static eval_fn eval_list_literal;
static eval_fn eval_block_expression;
static eval_fn eval_prefix_expression;
static eval_fn eval_assign_expression;
static eval_fn eval_infix_expression;
static eval_fn eval_ternary_expression;
static eval_fn eval_call_expression;
static eval_fn eval_index_expression;

static void assign_lhs(expression_reference lhs, object rhs, environment *env);
static void assign_lhs_identifier(expression *lhs, object rhs,
                                  environment *env);
static void assign_lhs_list(expression *lhs, object rhs, environment *env);
static void assign_lhs_index_expression(expression *lhs, object rhs,
                                        environment *env);
static inline bool copy_by_value(object_type ot);

// clang-format off
eval_fn *eval_fns[EXP_ENUM_LENGTH] = {
    eval_program,
    eval_identifier,
    eval_int_literal,
    eval_function_literal,
    eval_list_literal,
    eval_block_expression,
    eval_prefix_expression,
    eval_assign_expression,
    /* eval_infix_expression, */
    /* eval_ternary_expression, */
    /* eval_call_expression, */
    /* eval_index_expression, */
};
// clang-format on

object eval(expression_reference ref, environment *env) {
    arena *a = env->a;
    expression *expression = get_expression(a, ref);
    eval_fn *eval_fn = eval_fns[expression->type];
    return eval_fn(ref, env);
}

static object eval_program(expression_reference ref, environment *env) {
    arena *a = env->a;
    expression *program = get_expression(a, ref);
    expression_list expressions = program->program.expressions;

    expression *exp;

    object obj;
    ref = expressions.head;
    for (size_t i = 0; i < expressions.size; ++i) {
        obj = eval(ref, env);

        exp = get_expression(a, ref);
        ref = exp->next;
    }

    return obj;
}

static object eval_identifier(expression_reference ref, environment *env) {
    arena *a = env->a;
    expression *exp = get_expression(a, ref);

    const char *key = exp->identifier.value;
    size_t key_length = exp->identifier.length;

    bool ok;
    object_reference obj_ref = env_get(env, key, key_length, &ok);
    if (!ok) {
        fprintf(stderr, "%.*s undefined\n", (int)key_length, key);
        exit(1);
    }

    return *get_object(a, obj_ref);
}

static object eval_int_literal(expression_reference ref, environment *env) {
    arena *a = env->a;
    expression *exp = get_expression(a, ref);

    object obj = new_object(OBJECT_TYPE_INT);
    obj.int_object.value = exp->int_literal.value;

    return obj;
}

static object eval_function_literal(expression_reference ref,
                                    environment *env) {
    arena *a = env->a;
    expression *exp = get_expression(a, ref);
    object obj = new_object(OBJECT_TYPE_FUNCTION);

    obj.function_object.arguments = exp->function_literal.arguments;
    obj.function_object.body = exp->function_literal.body;
    /* obj.function_object.env = new_environment(a, env); */

    return obj;
}

static object eval_list_literal(expression_reference ref, environment *env) {
    arena *a = env->a;
    expression *exp = get_expression(a, ref);
    object obj = new_object(OBJECT_TYPE_LIST);
    object_list *obj_values = &obj.list_object.values;
    *obj_values = new_object_list(env->a);

    expression_list_iterator it = el_start(&exp->list_literal.values);
    expression_list_iterator end = el_end(&exp->list_literal.values);
    for (; !eli_eq(&it, &end); eli_next(&it)) {
        ol_append(obj_values, arena_alloc_object(a, eval(it.ref, env)));
    }

    return obj;
}

static object eval_block_expression(expression_reference ref,
                                    environment *env) {
    arena *a = env->a;
    expression *exp = get_expression(a, ref);

    object obj;
    expression_list_iterator it = el_start(&exp->block_expression.expressions);
    expression_list_iterator end = el_end(&exp->block_expression.expressions);
    for (; !eli_eq(&it, &end); eli_next(&it)) {
        obj = eval(it.ref, env);
    }

    return obj;
}

static object eval_prefix_expression(expression_reference ref,
                                     environment *env) {
    arena *a = env->a;
    expression *exp = get_expression(a, ref);
    token op = exp->prefix_expression.op;

    object right = eval(exp->prefix_expression.right, env);

    if (right.type != OBJECT_TYPE_INT) {
        // TODO:
        fprintf(stderr, "TODO: invalid prefix expression error message");
        exit(1);
    }

    if (strncmp(op.literal, "-", op.length) == 0) {
        right.int_object.value = -right.int_object.value;
    } else if (strncmp(op.literal, "!", op.length) == 0) {
        right.int_object.value = !right.int_object.value;
    }

    return right;
}

static object eval_assign_expression(expression_reference ref,
                                     environment *env) {
    arena *a = env->a;
    expression *exp = get_expression(a, ref);
    object right = eval(exp->assign_expression.right, env);
    expression_reference left = exp->assign_expression.left;
    assign_lhs(left, right, env);
    return right;
}

static void assign_lhs(expression_reference lhs, object rhs, environment *env) {
    arena *a = env->a;
    expression *lhs_exp = get_expression(a, lhs);
    switch (lhs_exp->type) {
        case EXP_TYPE_IDENTIFIER: {
            assign_lhs_identifier(lhs_exp, rhs, env);
        } break;
        case EXP_TYPE_LIST_LITERAL: {
            assign_lhs_list(lhs_exp, rhs, env);
        } break;
        case EXP_TYPE_INDEX_EXPRESSION: {
            assign_lhs_index_expression(lhs_exp, rhs, env);
        } break;
        default: {
            fprintf(stderr, "invalid LHS\n");
            exit(1);
        }
    }
}

static void assign_lhs_identifier(expression *identifier, object rhs,
                                  environment *env) {
    arena *a = env->a;
    const char *key = identifier->identifier.value;
    size_t key_length = identifier->identifier.length;

    object_reference ref;
    if (rhs.ref == NULL_OBJECT_REFERENCE || copy_by_value(rhs.type)) {
        ref = arena_alloc_object(a, rhs);
    } else {
        ref = rhs.ref;
        // TODO: increment rc
    }

    object_reference old_ref;
    /* if (env_contains_local_scope(env, key, key_length)) { */
        /* old_ref = env_get(env, key, key_length); */
        // TODO: decrement rc old_ref
    /* } */
    
    bool ok;
    env_set(env, key, key_length, ref, &ok);
}

static void assign_lhs_list(expression *list, object rhs, environment *env) {
    if (rhs.type != OBJECT_TYPE_LIST) {
        fprintf(stderr, "cannot pattern match list with \n");
        exit(1);
    }

    expression_list lhs_values = list->list_literal.values;
    object_list rhs_values = rhs.list_object.values;
    if (lhs_values.size != rhs_values.size) {
        fprintf(stderr, "cannot pattern match lists of different sizes \n");
        exit(1);
    }

    expression_list_iterator lhs_it = el_start(&lhs_values);
    object_list_iterator rhs_it = ol_start(&rhs_values);

    expression_list_iterator end = el_end(&lhs_values);
    for (; !eli_eq(&lhs_it, &end); eli_next(&lhs_it), oli_next(&rhs_it)) {
        assign_lhs(lhs_it.ref, *rhs_it.obj, env);
    }
}

static void assign_lhs_index_expression(expression *lhs, object rhs,
                                        environment *env) {
    object index = eval(lhs->index_expression.index, env);
    if (index.type != OBJECT_TYPE_INT) {
        fprintf(stderr, "invalid index type\n");
        exit(1);
    }
    int64_t index_value = index.int_object.value;

    object list = eval(lhs->index_expression.list, env);
    if (list.type != OBJECT_TYPE_LIST) {
        fprintf(stderr, "invalid list type\n");
        exit(1);
    }

    object_list list_values = list.list_object.values;
    if (index_value < 0 || (size_t)index_value >= list_values.size) {
        fprintf(stderr, "index out of bounds\n");
        exit(1);
    }

    object_reference ref;

    // get ref
}

static inline bool copy_by_value(object_type ot) {
    return ot == OBJECT_TYPE_INT;
}
