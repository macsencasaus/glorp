#include "evaluator.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"
#include "error.h"
#include "interpreter.h"
#include "sb.h"
#include "utils.h"

#define ARGS_VAR_NAME "args"

#define object_init(objp, _type) \
    *objp = (object) { .type = _type }

#define CHECK_EVAL(eval_res) \
    if (!(eval_res)) return false

#define MOD(l, r) ((((l) % (r)) + (r)) % (r))

static size_t scope_counter = 1;
eval_error eval_err;

typedef bool eval_fn(const expr *, environment *env, object *result);
typedef bool assign_fn(const expr *lhs,
                       const object *rhs,
                       const expr *parent,  // parent assigment expression
                       environment *env,
                       bool is_const,
                       object **new_obj);
//
// clang-format off
static const char *const object_type_literals[OBJECT_TYPE_ENUM_LENGTH] = {
    [OBJECT_TYPE_NULL]        = "null", 
    [OBJECT_TYPE_UNIT]        = "unit",
    [OBJECT_TYPE_CHAR]        = "char",
    [OBJECT_TYPE_INT]         = "int", 
    [OBJECT_TYPE_FLOAT]       = "float",
    [OBJECT_TYPE_FUNCTION]    = "function", 
    [OBJECT_TYPE_LIST]        = "list", 
};
// clang-format on

static eval_fn eval_no_l;

static eval_fn eval_program;
static eval_fn eval_unit;
static eval_fn eval_identifier;
static eval_fn eval_char_literal;
static eval_fn eval_int_literal;
static eval_fn eval_float_literal;
static eval_fn eval_string_literal;
static eval_fn eval_list_literal;
static eval_fn eval_block_expression;
static eval_fn eval_prefix_expression;
static eval_fn eval_assign_expression;
static eval_fn eval_infix_expression;
static eval_fn eval_ternary_expression;
static eval_fn eval_call_expression;
static eval_fn eval_index_expression;
static eval_fn eval_function_literal;
static eval_fn eval_compose_expression;
static eval_fn eval_pipe_expression;
static eval_fn eval_case_expression;
static eval_fn eval_import_expression;

static assign_fn assign_lhs;
static assign_fn assign_ident;
static assign_fn assign_list;
static assign_fn assign_index;
static assign_fn assign_tuple;
static assign_fn assign_prepend;

static object *resolve_assign_rhs(const object *rhs, bool *is_const);

WARN_UNUSED_RESULT
static inline bool get_ie_it(const expr *index_expression, environment *env, ol_iterator *oli, object *list);

WARN_UNUSED_RESULT
static inline bool eval_func_params(expr *params, environment *env, expr_list *parameters);

static inline bool valid_infix_num_types(const object *left, const object *right);
static inline bool is_truthy(const object *obj);
static inline bool is_num_type(const object *obj);
static inline bool is_tuple_exp(const expr *e);
static inline bool copy_by_value(object_type ot);
static inline void undefined_var_error(const expr *e);
static inline void generic_error(const expr *e, const char *msg, ...);
static inline size_t fn_param_count(const object *fn);

typedef struct {
    char *name;
    builtin_fn_t *fn;
    size_t param_count;
} builtin_entry;

// clang-format off
eval_fn *eval_fns[EXPR_ENUM_LENGTH] = {
    [EXPR_TYPE_PROGRAM]            = eval_program,
    [EXPR_TYPE_UNIT]               = eval_unit,
    [EXPR_TYPE_IDENTIFIER]         = eval_identifier,
    [EXPR_TYPE_CHAR_LITERAL]       = eval_char_literal,
    [EXPR_TYPE_INT_LITERAL]        = eval_int_literal,
    [EXPR_TYPE_FLOAT_LITERAL]      = eval_float_literal,
    [EXPR_TYPE_STRING_LITERAL]     = eval_string_literal,
    [EXPR_TYPE_LIST_LITERAL]       = eval_list_literal,
    [EXPR_TYPE_BLOCK_EXPRESSION]   = eval_block_expression,
    [EXPR_TYPE_PREFIX_EXPRESSION]  = eval_prefix_expression,
    [EXPR_TYPE_INFIX_EXPRESSION]   = eval_infix_expression,
    [EXPR_TYPE_TERNARY_EXPRESSION] = eval_ternary_expression,
    [EXPR_TYPE_CALL_EXPRESSION]    = eval_call_expression,
    [EXPR_TYPE_INDEX_EXPRESSION]   = eval_index_expression,
    [EXPR_TYPE_CASE_EXPRESSION]    = eval_case_expression,
    [EXPR_TYPE_IMPORT_EXPRESSION]  = eval_import_expression,
};
// clang-format on

bool eval(const expr *e, environment *env, object *result) {
    eval_fn *eval_fn = eval_fns[e->type];
    return eval_fn(e, env, result);
}

// normal eval but does not result in lvalues, only their underlying value
// which is then not tracked by reference
bool eval_no_l(const expr *e, environment *env, object *result) {
    CHECK_EVAL(eval(e, env, result));
    if (result->type == OBJECT_TYPE_LVALUE) {
        *result = *result->ref;
    }
    return true;
}

static bool eval_program(const expr *program, environment *env, object *result) {
    const expr_list *expressions = &program->expressions;

    const expr *exp = expressions->head;
    for (size_t i = 0; i < expressions->size; ++i, exp = exp->next) {
        CHECK_EVAL(eval(exp, env, result));
    }
    return true;
}

static bool eval_unit(const expr *unit, environment *env, object *result) {
    (void)unit;
    (void)env;
    object_init(result, OBJECT_TYPE_UNIT);
    return true;
}

static bool eval_identifier(const expr *ident, environment *env, object *result) {
    const char *key = ident->literal;
    size_t key_length = ident->length;

    object_init(result, OBJECT_TYPE_LVALUE);

    if (!env_get(env, key, key_length, &result->ref, &result->is_const)) {
        undefined_var_error(ident);
        return false;
    }

    return true;
}

static bool eval_char_literal(const expr *char_literal, environment *env, object *result) {
    (void)env;
    object_init(result, OBJECT_TYPE_CHAR);
    result->char_value = char_literal->char_value;
    return true;
}

static bool eval_int_literal(const expr *int_literal, environment *env, object *result) {
    (void)env;
    object_init(result, OBJECT_TYPE_INT);
    result->int_value = int_literal->int_value;
    return true;
}

static bool eval_float_literal(const expr *float_literal, environment *env, object *result) {
    (void)env;
    object_init(result, OBJECT_TYPE_FLOAT);
    result->float_value = float_literal->float_value;
    return true;
}

static bool eval_string_literal(const expr *string_literal, environment *env, object *result) {
    (void)env;
    const char *literal = string_literal->literal;
    size_t length = string_literal->length;

    object_init(result, OBJECT_TYPE_LIST);

    object_list *obj_values = &result->values;

    object *char_obj;
    for (size_t i = 0; i < length; ++i) {
        char_obj = new_obj(OBJECT_TYPE_CHAR, 1);
        char_obj->char_value = literal[i];
        ol_append(obj_values, char_obj);
    }

    return true;
}

static bool eval_list_literal(const expr *list_literal, environment *env, object *result) {
    object_init(result, OBJECT_TYPE_LIST);

    object_list *obj_values = &result->values;

    const expr_list *exprs = &list_literal->expressions;

    const expr *cur_expr = exprs->head;
    object cur_obj;
    for (; cur_expr; cur_expr = cur_expr->next) {
        CHECK_EVAL(eval(cur_expr, env, &cur_obj));
        object *new_obj = resolve_assign_rhs(&cur_obj, NULL);
        ol_append(obj_values, new_obj);
    }

    return true;
}

static bool eval_block_expression(const expr *block_expr, environment *env, object *result) {
    object_init(result, OBJECT_TYPE_UNIT);

    const expr *cur_expr = block_expr->expressions.head;
    for (; cur_expr; cur_expr = cur_expr->next) {
        CHECK_EVAL(eval(cur_expr, env, result));
    }
    return true;
}

#define eval_prefix_num_vals(obj, og_obj, obj_field, op) \
    switch (op) {                                        \
        case TOKEN_TYPE_MINUS: {                         \
            obj->obj_field = -obj->obj_field;            \
        } break;                                         \
        case TOKEN_TYPE_BANG: {                          \
            obj->obj_field = !obj->obj_field;            \
        } break;                                         \
        case TOKEN_TYPE_NOT: {                           \
            obj->obj_field = ~(int64_t)obj->obj_field;   \
        } break;                                         \
        case TOKEN_TYPE_PLUS_PLUS: {                     \
            ++obj->obj_field;                            \
            *og_obj = *obj;                              \
        } break;                                         \
        case TOKEN_TYPE_MINUS_MINUS: {                   \
            --obj->obj_field;                            \
            *og_obj = *obj;                              \
        } break;                                         \
        default: {                                       \
        }                                                \
    }

static bool eval_prefix_expression(const expr *prefix_expr, environment *env, object *result) {
    const token *op = &prefix_expr->op;
    object *og_result;

    switch (op->type) {
        case TOKEN_TYPE_COLON_COLON: {
            generic_error(prefix_expr, "Const declaration can only be used in parameters for function declarations");
            return false;
        } break;
        case TOKEN_TYPE_MINUS:
        case TOKEN_TYPE_BANG:
        case TOKEN_TYPE_NOT: {
            CHECK_EVAL(eval_no_l(prefix_expr->right, env, result));
        } break;
        case TOKEN_TYPE_PLUS_PLUS:
        case TOKEN_TYPE_MINUS_MINUS: {
            CHECK_EVAL(eval(prefix_expr->right, env, result));
            og_result = result;
            if (result->type != OBJECT_TYPE_LVALUE) {
                generic_error(prefix_expr, "Expression is not assignable");
                return false;
            }
            result = result->ref;
        } break;
        default: {
        }
    }

    switch (result->type) {
        case OBJECT_TYPE_INT: {
            eval_prefix_num_vals(result, og_result, int_value, op->type);
        } break;
        case OBJECT_TYPE_FLOAT: {
            eval_prefix_num_vals(result, og_result, float_value, op->type);
        } break;
        default: {
            generic_error(prefix_expr, "Invalid prefix expression");
            return false;
        }
    }

    return true;
}

static bool eval_assign_expression(const expr *assign_expr, environment *env, object *result) {
    bool is_const = assign_expr->op.type == TOKEN_TYPE_COLON_COLON;
    object right;
    CHECK_EVAL(eval(assign_expr->right, env, &right));
    object *heap_obj;
    CHECK_EVAL(assign_lhs(assign_expr->left, &right, assign_expr, env, is_const, &heap_obj));
    if (heap_obj) {
        *result = (object){
            .type = OBJECT_TYPE_LVALUE,
            .ref = heap_obj,
            .is_const = is_const,
        };
    } else {
        *result = right;
    }
    return true;
}

#define eval_infix_num_vals(obj, obj_ty, obj_field, op, lval, rval) \
    switch (op) {                                                   \
        case TOKEN_TYPE_PLUS: {                                     \
            object_init(obj, (obj_ty));                             \
            obj->obj_field = lval + rval;                           \
        } break;                                                    \
        case TOKEN_TYPE_MINUS: {                                    \
            object_init(obj, (obj_ty));                             \
            obj->obj_field = lval - rval;                           \
        } break;                                                    \
        case TOKEN_TYPE_ASTERISK: {                                 \
            object_init(obj, (obj_ty));                             \
            obj->obj_field = lval * rval;                           \
        } break;                                                    \
        case TOKEN_TYPE_SLASH: {                                    \
            object_init(obj, (obj_ty));                             \
            obj->obj_field = lval / rval;                           \
        } break;                                                    \
        case TOKEN_TYPE_PERCENT: {                                  \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = MOD((uint64_t)lval, (uint64_t)rval);   \
        } break;                                                    \
        case TOKEN_TYPE_LT: {                                       \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval < rval;                           \
        } break;                                                    \
        case TOKEN_TYPE_GT: {                                       \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval > rval;                           \
        } break;                                                    \
        case TOKEN_TYPE_GT_EQ: {                                    \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval >= rval;                          \
        } break;                                                    \
        case TOKEN_TYPE_LT_EQ: {                                    \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval <= rval;                          \
        } break;                                                    \
        case TOKEN_TYPE_EQ: {                                       \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval == rval;                          \
        } break;                                                    \
        case TOKEN_TYPE_NOT_EQ: {                                   \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval != rval;                          \
        } break;                                                    \
        case TOKEN_TYPE_LAND: {                                     \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval && rval;                          \
        } break;                                                    \
        case TOKEN_TYPE_LOR: {                                      \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = lval || rval;                          \
        } break;                                                    \
        case TOKEN_TYPE_BAND: {                                     \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = (int64_t)lval & (int64_t)rval;         \
        } break;                                                    \
        case TOKEN_TYPE_BOR: {                                      \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = (int64_t)lval | (int64_t)rval;         \
        } break;                                                    \
        case TOKEN_TYPE_XOR: {                                      \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = (int64_t)lval ^ (int64_t)rval;         \
        } break;                                                    \
        case TOKEN_TYPE_LEFT_SHIFT: {                               \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = (int64_t)lval << (int64_t)rval;        \
        } break;                                                    \
        case TOKEN_TYPE_RIGHT_SHIFT: {                              \
            object_init(obj, OBJECT_TYPE_INT);                      \
            obj->int_value = (int64_t)lval >> (int64_t)rval;        \
        } break;                                                    \
        default: {                                                  \
        }                                                           \
    }

static bool concat_lists(object *left_obj, object *right_obj, object *result) {
    object_init(result, OBJECT_TYPE_LIST);

    object_list *obj_values = &result->values;

    ol_iterator it = ol_start(&left_obj->values);

    for (; !oli_is_end(&it); oli_next(&it)) {
        if (copy_by_value(it.obj->type)) {
            object *new_obj = new_empty_obj();
            *new_obj = *it.obj;
            new_obj->rc = 1;
            ol_append(obj_values, new_obj);
        } else {
            ++it.obj->rc;
            ol_append(obj_values, it.obj);
        }
    }

    it = ol_start(&right_obj->values);

    for (; !oli_is_end(&it); oli_next(&it)) {
        if (copy_by_value(it.obj->type)) {
            object *new_obj = new_empty_obj();
            *new_obj = *it.obj;
            new_obj->rc = 1;
            ol_append(obj_values, new_obj);
        } else {
            ++it.obj->rc;
            ol_append(obj_values, it.obj);
        }
    }

    temp_cleanup(left_obj);
    temp_cleanup(right_obj);
    return true;
}

static bool eval_infix_expression(const expr *infix_expr, environment *env, object *result) {
    token_type op_type = infix_expr->op.type;

    switch (op_type) {
        case TOKEN_TYPE_RIGHT_ARROW:
            return eval_function_literal(infix_expr, env, result);
        case TOKEN_TYPE_ASSIGN:
        case TOKEN_TYPE_COLON_COLON:
            return eval_assign_expression(infix_expr, env, result);
        case TOKEN_TYPE_LEFT_COMPOSE:
        case TOKEN_TYPE_RIGHT_COMPOSE:
            return eval_compose_expression(infix_expr, env, result);
        case TOKEN_TYPE_DOT:
        case TOKEN_TYPE_LEFT_PIPE:
        case TOKEN_TYPE_RIGHT_PIPE:
            return eval_pipe_expression(infix_expr, env, result);
        default: {
        }
    }

    const expr *left = infix_expr->left;
    const expr *right = infix_expr->right;

    object left_obj, right_obj;
    CHECK_EVAL(eval_no_l(left, env, &left_obj));
    CHECK_EVAL(eval_no_l(right, env, &right_obj));

    switch (op_type) {
        case TOKEN_TYPE_PLUS: {
            if (left_obj.type == OBJECT_TYPE_LIST &&
                right_obj.type == OBJECT_TYPE_LIST) {
                CHECK_EVAL(concat_lists(&left_obj, &right_obj, result));
                return true;
            }
        }
        case TOKEN_TYPE_MINUS:
        case TOKEN_TYPE_ASTERISK:
        case TOKEN_TYPE_SLASH:
        case TOKEN_TYPE_PERCENT:
        case TOKEN_TYPE_LT:
        case TOKEN_TYPE_GT:
        case TOKEN_TYPE_LT_EQ:
        case TOKEN_TYPE_GT_EQ:
        case TOKEN_TYPE_EQ:
        case TOKEN_TYPE_NOT_EQ:
        case TOKEN_TYPE_LAND:
        case TOKEN_TYPE_LOR:
        case TOKEN_TYPE_BAND:
        case TOKEN_TYPE_BOR:
        case TOKEN_TYPE_XOR:
        case TOKEN_TYPE_LEFT_SHIFT:
        case TOKEN_TYPE_RIGHT_SHIFT: {
            if (!valid_infix_num_types(&left_obj, &right_obj)) {
                // TODO: specify expected types
                generic_error(infix_expr, "Invalid operands to arithmetic operation");
                return false;
            }
        } break;
        default: {
            generic_error(infix_expr, "Unimplemented infix expression - oops :(");
            return false;
        }
    }

    if (left_obj.type == OBJECT_TYPE_FLOAT ||
        right_obj.type == OBJECT_TYPE_FLOAT) {
        double left_val;
        double right_val;

        if (left_obj.type == OBJECT_TYPE_INT) {
            left_val = (double)left_obj.int_value;
        } else {
            left_val = left_obj.float_value;
        }

        if (right_obj.type == OBJECT_TYPE_INT) {
            right_val = (double)right_obj.int_value;
        } else {
            right_val = right_obj.float_value;
        }

        eval_infix_num_vals(result, OBJECT_TYPE_FLOAT, float_value, op_type, left_val, right_val);
    } else {
        int64_t left_val = left_obj.int_value;
        int64_t right_val = right_obj.int_value;

        eval_infix_num_vals(result, OBJECT_TYPE_INT, int_value, op_type, left_val, right_val);
    }

    return true;
}

static bool eval_ternary_expression(const expr *ternary_expr, environment *env, object *result) {
    object condition_obj;
    // TODO: eval assign pattern match differently
    CHECK_EVAL(eval_no_l(ternary_expr->condition, env, &condition_obj));

    if (is_truthy(&condition_obj)) {
        CHECK_EVAL(eval(ternary_expr->consequence, env, result));
    } else {
        CHECK_EVAL(eval(ternary_expr->alternative, env, result));
    }

    return true;
}

static bool eval_call_expression(const expr *call_expr, environment *env, object *result) {
    object func;
    CHECK_EVAL(eval_no_l(call_expr->function, env, &func));

    if (func.type != OBJECT_TYPE_FUNCTION) {
        generic_error(call_expr, "'%s' object is not callable, expected function",
                      object_type_literals[func.type]);
        return false;
    }

    size_t expected_params = fn_param_count(&func);
    size_t actual_params = call_expr->params.size;

    if (expected_params > actual_params) {
        generic_error(call_expr,
                      "Too few arguments to function call (expected %zu, got %zu)",
                      expected_params, actual_params);
        return false;
    }

    if (expected_params < actual_params) {
        generic_error(call_expr,
                      "Too many arguments to function call (expected %zu, got %zu)",
                      expected_params, actual_params);
        return false;
    }

    if (func.builtin) {
        return func.builtin_fn(&call_expr->params, call_expr, env, result);
    }

    size_t parameter_count = expected_params;

    object *func_env_obj = new_obj(OBJECT_TYPE_ENVIRONMENT, 1);

    environment *func_env = &func_env_obj->env;
    environment_init(func_env, func.outer_env, env->ht, scope_counter++);
    func_env->obj = func_env_obj;

    const expr *func_param = func.params.head;
    const expr *call_param = call_expr->params.head;

    bool is_const;
    const expr *ident;

    object param_obj;
    for (size_t i = 0; i < parameter_count; ++i) {
        CHECK_EVAL(eval(call_param, env, &param_obj));

        if (func_param->type == EXPR_TYPE_PREFIX_EXPRESSION) {
            is_const = true;
            ident = func_param->right;
        } else {
            is_const = false;
            ident = func_param;
        }

        CHECK_EVAL(assign_lhs(ident, &param_obj, call_expr, func_env, is_const, NULL));

        func_param = func_param->next;
        call_param = call_param->next;
    }

    CHECK_EVAL(eval(func.body, func_env, result));

    rc_dec(func_env_obj);

    return true;
}

static bool eval_index_expression(const expr *index_expr, environment *env, object *result) {
    ol_iterator it;
    object list;
    CHECK_EVAL(get_ie_it(index_expr, env, &it, &list));

    if (list.type == OBJECT_TYPE_LVALUE) {
        object_init(result, OBJECT_TYPE_LVALUE);
        result->ref = it.obj;
        result->is_const = list.is_const;
    } else {
        *result = *it.obj;
        temp_cleanup(&list);
    }

    return true;
}

static bool eval_function_literal(const expr *function_literal, environment *env, object *result) {
    expr *params = function_literal->left;
    const expr *body = function_literal->right;

    object_init(result, OBJECT_TYPE_FUNCTION);

    CHECK_EVAL(eval_func_params(params, env, &result->params));

    result->body = body;
    result->outer_env = env;

    if (env->obj != NULL)
        ++env->obj->rc;

    return true;
}

static bool eval_compose_expression(const expr *compose_expr, environment *env, object *result) {
    expr *outer, *inner;
    const token *start_tok, *end_tok;

    switch (compose_expr->op.type) {
        case TOKEN_TYPE_LEFT_COMPOSE: {
            outer = compose_expr->left;
            inner = compose_expr->right;

            start_tok = &outer->start_tok;
            end_tok = &inner->end_tok;
        } break;
        case TOKEN_TYPE_RIGHT_COMPOSE: {
            outer = compose_expr->right;
            inner = compose_expr->left;

            start_tok = &inner->start_tok;
            end_tok = &outer->end_tok;
        } break;
        default: {
            generic_error(compose_expr, "How did we get here?");
            return false;
        }
    }

    object outer_fn, inner_fn;

    CHECK_EVAL(eval_no_l(outer, env, &outer_fn));
    CHECK_EVAL(eval_no_l(inner, env, &inner_fn));

    if (outer_fn.type != OBJECT_TYPE_FUNCTION) {
        generic_error(outer, "'%s' object is composable, expected function",
                      object_type_literals[outer_fn.type]);
        return false;
    }

    if (inner_fn.type != OBJECT_TYPE_FUNCTION) {
        generic_error(inner, "'%s' object is not composable, expected function",
                      object_type_literals[inner_fn.type]);
        return false;
    }

    size_t outer_param_count = fn_param_count(&outer_fn);

    if (outer_param_count != 1) {
        generic_error(outer, "Outer function in composition must have 1 parameter, got %zu",
                      outer_param_count);
        return false;
    }

    object_init(result, OBJECT_TYPE_FUNCTION);
    result->params = inner_fn.params;
    result->outer_env = env;

    expr *inner_call = new_expr3(EXPR_TYPE_CALL_EXPRESSION, start_tok, end_tok);
    expr *outer_call = new_expr3(EXPR_TYPE_CALL_EXPRESSION, start_tok, end_tok);

    inner_call->function = inner;
    inner_call->params = inner_fn.params;

    outer_call->function = outer;
    outer_call->params = (expr_list){
        .head = inner_call,
        .tail = inner_call,
        .size = 1,
    };

    result->body = outer_call;

    return true;
}

static bool eval_pipe_expression(const expr *pipe_expr, environment *env, object *result) {
    expr *fn, *param;
    const token *start_tok, *end_tok;

    switch (pipe_expr->op.type) {
        case TOKEN_TYPE_LEFT_PIPE: {
            fn = pipe_expr->left;
            param = pipe_expr->right;

            start_tok = &fn->start_tok;
            end_tok = &param->end_tok;
        } break;
        case TOKEN_TYPE_DOT:
        case TOKEN_TYPE_RIGHT_PIPE: {
            fn = pipe_expr->right;
            param = pipe_expr->left;

            start_tok = &param->start_tok;
            end_tok = &fn->end_tok;
        } break;
        default: {
            generic_error(pipe_expr, "How did we get here?");
            return false;
        }
    }

    object fn_obj;

    CHECK_EVAL(eval_no_l(fn, env, &fn_obj));

    if (fn_obj.type != OBJECT_TYPE_FUNCTION) {
        generic_error(fn, "'%s' object cannot be piped into, expected function",
                      object_type_literals[fn_obj.type]);
        return false;
    }

    size_t param_count = fn_param_count(&fn_obj);

    if (param_count < 1) {
        generic_error(pipe_expr, "Cannot pipe into function with 0 arguments");
        return false;
    }

    object_init(result, OBJECT_TYPE_FUNCTION);
    result->params = (expr_list){
        .head = fn_obj.params.head->next,
        .tail = fn_obj.params.tail,
        .size = param_count - 1,
    };

    param->next = fn_obj.params.head->next;

    expr *call = new_expr3(EXPR_TYPE_CALL_EXPRESSION, start_tok, end_tok);
    call->function = fn;
    call->params = (expr_list){
        .head = param,
        .tail = fn_obj.params.tail,
        .size = param_count,
    };

    result->body = call;

    result->outer_env = env;

    return true;
}

static bool eval_case_expression(const expr *case_expr, environment *env, object *result) {
    object_init(result, OBJECT_TYPE_UNIT);

    size_t cases = case_expr->conditions.size;

    expr *condition_expr = case_expr->conditions.head;
    expr *result_expr = case_expr->results.head;

    object condition;
    for (size_t i = 0; i < cases; ++i) {
        CHECK_EVAL(eval(condition_expr, env, &condition));

        if (is_truthy(&condition)) {
            CHECK_EVAL(eval(result_expr, env, result));
            break;
        }

        condition_expr = condition_expr->next;
        result_expr = result_expr->next;
    }

    return true;
}

static bool eval_import_expression(const expr *import_expr, environment *env, object *result) {
    char *file_name = (char *)malloc(import_expr->length + 1);
    if (file_name == NULL) {
        generic_error(import_expr, "Failed to import file");
        return false;
    }
    memcpy(file_name, import_expr->literal, import_expr->length);
    file_name[import_expr->length] = 0;

    bool is_so = import_expr->length > 3 &&
                 strncmp(".so", import_expr->literal + import_expr->length - 3, 3) == 0;

    if (is_so) {
        void *so_handle = dlopen(file_name, RTLD_LAZY);
        if (so_handle == NULL) {
            generic_error(import_expr, "Error loading shared object file %s",
                          dlerror());
            return false;
        }

        dlerror();

        const builtin_entry *exported_functions = (builtin_entry *)dlsym(so_handle, "exported_functions");
        if (exported_functions == NULL) {
            generic_error(import_expr, "Error loading exported functions from %s",
                          dlerror());
            return false;
        }

        dlerror();

        size_t *exported_function_countp = (size_t *)dlsym(so_handle, "exported_functions_count");
        if (exported_function_countp == NULL) {
            generic_error(import_expr, "Error loading exported function count from %s",
                          dlerror());
            return false;
        }

        size_t count = *exported_function_countp;

        object *fn;
        const builtin_entry *entry;
        for (size_t i = 0; i < count; ++i) {
            entry = exported_functions + i;

            fn = new_obj(OBJECT_TYPE_FUNCTION, 1);

            fn->builtin = true;
            fn->builtin_param_count = entry->param_count;
            fn->builtin_fn = entry->fn;

            env_set(env, entry->name, strlen(entry->name), fn, true);
        }
    } else {
        char *file_contents = read_file(file_name);

        glorp_options new_options = *env->selected_options;

        new_options.file = file_name;

        CHECK_EVAL(interpret_with_env(file_contents, &new_options, env));

        free(file_name);
        free(file_contents);
    }

    object_init(result, OBJECT_TYPE_UNIT);

    return true;
}

static bool assign_lhs(const expr *lhs, const object *rhs, const expr *parent,
                       environment *env, bool is_const, object **new_obj) {
    switch (lhs->type) {
        case EXPR_TYPE_IDENTIFIER: {
            CHECK_EVAL(assign_ident(lhs, rhs, parent, env, is_const, new_obj));
        } break;
        case EXPR_TYPE_LIST_LITERAL: {
            CHECK_EVAL(assign_list(lhs, rhs, parent, env, is_const, new_obj));
        } break;
        case EXPR_TYPE_INDEX_EXPRESSION: {
            if (is_const) {
                generic_error(lhs, "Cannot assign index expression const");
                return false;
            }
            CHECK_EVAL(assign_index(lhs, rhs, parent, env, is_const, new_obj));
        } break;
        case EXPR_TYPE_INFIX_EXPRESSION: {
            switch (lhs->op.type) {
                case TOKEN_TYPE_COMMA: {
                    CHECK_EVAL(assign_tuple(lhs, rhs, parent, env, is_const, new_obj));
                } break;
                case TOKEN_TYPE_COLON: {
                    CHECK_EVAL(assign_prepend(lhs, rhs, parent, env, is_const, new_obj));
                } break;
                default: {
                    generic_error(lhs, "Expression is not assignable");
                    return false;
                }
            }
        } break;
        default: {
            generic_error(lhs, "Expression is not assignable");
            return false;
        }
    }
    return true;
}

static object *resolve_assign_rhs(const object *rhs, bool *is_const) {
    object *new_obj;

    if (rhs->type == OBJECT_TYPE_LVALUE) {
        if (copy_by_value(rhs->ref->type)) {
            new_obj = new_copied_obj(rhs->ref);
            new_obj->rc = 1;
            if (is_const)
                *is_const = false;
        } else {
            new_obj = rhs->ref;
            ++new_obj->rc;
            if (is_const)
                *is_const = rhs->is_const;
        }
    } else {
        new_obj = new_copied_obj(rhs);
        new_obj->rc = 1;
        if (is_const)
            *is_const = false;
    }

    return new_obj;
}

static bool assign_ident(const expr *ident, const object *rhs, const expr *parent,
                         environment *env, bool is_const, object **n_obj) {
    (void)parent;

    const char *key = ident->literal;
    size_t key_length = ident->length;

    if (env_contains_local_scope(env, key, key_length)) {
        object *old_obj;
        bool obj_is_const;
        CHECK_EVAL(env_get(env, key, key_length, &old_obj, &obj_is_const));
        if (obj_is_const) {
            generic_error(parent, "Assigning const expression");
            return false;
        }
        if (is_const) {
            generic_error(parent, "Assign mutable expression as const");
            return false;
        }
        rc_dec(old_obj);
    }

    bool is_new_const;
    object *new_obj = resolve_assign_rhs(rhs, &is_new_const);

    if (is_new_const && !is_const) {
        generic_error(parent, "Assigning const expression to mutable variable");
        return false;
    }

    if (n_obj) {
        *n_obj = new_obj;
    }

    env_set(env, key, key_length, new_obj, is_const);
    return true;
}

static bool assign_list(const expr *list, const object *rhs, const expr *parent,
                        environment *env, bool is_const, object **new_obj) {
    if (rhs->type == OBJECT_TYPE_LVALUE) {
        rhs = rhs->ref;
    }

    if (rhs->type != OBJECT_TYPE_LIST) {
        generic_error(parent, "'%s' object is not list unpackable, expected list",
                      object_type_literals[rhs->type]);
        return false;
    }

    const expr_list *lhs_values = &list->expressions;
    const object_list *rhs_values = &rhs->values;

    size_t actual = lhs_values->size;
    size_t expected = rhs_values->size;

    if (expected < actual) {
        generic_error(parent,
                      "Too few values to unpack (expected %zu, got %zu)",
                      expected, actual);
        return false;
    }

    if (expected > actual) {
        generic_error(parent,
                      "Too many values to unpack (expected %zu, got %zu)",
                      expected, actual);
        return false;
    }

    size_t value_count = lhs_values->size;

    const expr *cur_lhs_expr = lhs_values->head;
    ol_iterator rhs_it = ol_start(rhs_values);

    object lval;
    object_init(&lval, OBJECT_TYPE_LVALUE);
    for (size_t i = 0; i < value_count; ++i) {
        lval.ref = rhs_it.obj;
        CHECK_EVAL(assign_lhs(cur_lhs_expr, &lval, parent, env, is_const, NULL));

        cur_lhs_expr = cur_lhs_expr->next;
        oli_next(&rhs_it);
    }

    if (new_obj) {
        *new_obj = NULL;
    }

    return true;
}

static bool assign_index(const expr *index_expr, const object *rhs, const expr *parent,
                         environment *env, bool is_const, object **n_obj) {
    if (is_const) {
        generic_error(parent, "Unable to const assign index expressions");
        return false;
    }

    ol_iterator it;
    object list;
    CHECK_EVAL(get_ie_it(index_expr, env, &it, &list));

    if (list.type != OBJECT_TYPE_LVALUE) {
        generic_error(index_expr, "Expression is not assignable");
        return false;
    }

    if (list.is_const) {
        generic_error(parent, "Assigning const expression");
        return false;
    }

    object *old_obj = it.obj;
    rc_dec(old_obj);

    bool is_new_const;
    object *new_obj = resolve_assign_rhs(rhs, &is_new_const);

    if (is_new_const && !is_const) {
        generic_error(parent, "Assigning const expression to mutable variable");
        return false;
    }

    it.ln->value = new_obj;

    if (n_obj)
        *n_obj = new_obj;

    return true;
}

static bool assign_tuple_list(const expr *lhs, const object_list *rhs, const expr *parent,
                              environment *env, bool is_const);

static bool assign_tuple(const expr *lhs, const object *rhs, const expr *parent,
                         environment *env, bool is_const, object **new_obj) {
    switch (rhs->type) {
        case OBJECT_TYPE_LIST: {
            CHECK_EVAL(assign_tuple_list(lhs, &rhs->values, parent, env, is_const));
        } break;
        default: {
            generic_error(parent, "'%s' object is not tuple unpackable, expected list",
                          object_type_literals[rhs->type]);
            return false;
        }
    }
    if (new_obj)
        *new_obj = NULL;
    return true;
}

static bool assign_prepend(const expr *lhs, const object *rhs, const expr *parent,
                           environment *env, bool is_const, object **n_obj) {
    if (rhs->type == OBJECT_TYPE_LVALUE) {
        rhs = rhs->ref;
    }

    if (rhs->type != OBJECT_TYPE_LIST) {
        generic_error(parent, "Cannot unpack %s into prepend assignment",
                      object_type_literals[rhs->type]);
        return false;
    }

    /* arena *a = env->a; */
    expr *left = lhs->left;
    expr *right = lhs->right;

    const object_list *rhs_values = &rhs->values;
    if (rhs_values->size == 0) {
        generic_error(parent, "Cannot prepend unpack list of size 0");
        return false;
    }
    ol_iterator rhs_it = ol_start(rhs_values);
    object lval = {
        .type = OBJECT_TYPE_LVALUE,
        .ref = rhs_it.obj,
    };
    CHECK_EVAL(assign_lhs(left, &lval, parent, env, is_const, NULL));

    oli_next(&rhs_it);
    if (!oli_is_end(&rhs_it))
        ++rhs_it.ln->rc;

    object *rest = new_obj(OBJECT_TYPE_LIST, 1);

    rest->values = (object_list){
        .head = rhs_it.ln,
        .tail = rhs_values->tail,
        .size = rhs_values->size - 1,
    };

    lval = (object){
        .type = OBJECT_TYPE_LVALUE,
        .ref = rest,
    };

    CHECK_EVAL(assign_lhs(right, &lval, parent, env, is_const, NULL));

    if (n_obj)
        *n_obj = NULL;

    return true;
}

static bool assign_tuple_list(const expr *tuple_expr, const object_list *rhs, const expr *parent,
                              environment *env, bool is_const) {
    size_t size = rhs->size;
    ol_iterator rhs_it = ol_start(rhs);

    const expr *left = tuple_expr->left;
    const expr *right = tuple_expr->right;

    object lval;
    object_init(&lval, OBJECT_TYPE_LVALUE);

    size_t i = 0;
    for (; !oli_is_end(&rhs_it); oli_next(&rhs_it), ++i) {
        lval.ref = rhs_it.obj;
        CHECK_EVAL(assign_lhs(left, &lval, parent, env, is_const, NULL));

        if (!is_tuple_exp(right)) {
            oli_next(&rhs_it);
            break;
        }

        left = right->left;
        right = right->right;
    }

    if (i > size - 2) {
        generic_error(parent,
                      "Not enough values to unpack (expected %zu, got %zu)",
                      i + 2, size);
        return false;
    }

    if (i < size - 2) {
        generic_error(parent,
                      "Too many values to unpack (expected %zu, got %zu)",
                      i + 2, size);
        return false;
    }

    lval.ref = rhs_it.obj;
    CHECK_EVAL(assign_lhs(right, &lval, parent, env, is_const, NULL));
    return true;
}

static inline bool get_ie_it(const expr *index_expression, environment *env, ol_iterator *oli,
                             object *list) {
    CHECK_EVAL(eval(index_expression->list, env, list));

    object *l;
    if (list->type == OBJECT_TYPE_LVALUE) {
        l = list->ref;
    } else {
        l = list;
    }

    if (l->type != OBJECT_TYPE_LIST) {
        generic_error(index_expression, "'%s' object is not subscriptable, expected list",
                      object_type_literals[l->type]);
        return false;
    }

    object index_obj;
    CHECK_EVAL(eval_no_l(index_expression->index, env, &index_obj));

    if (index_obj.type != OBJECT_TYPE_INT) {
        generic_error(index_expression->index, "Invalid index type, expected int");
        return false;
    }

    object_list *values = &l->values;

    int64_t index_value = index_obj.int_value;

    if (index_value < 0) {
        generic_error(index_expression->index, "Negative index value: %lld", index_value);
        return false;
    }

    if ((size_t)index_value >= values->size) {
        generic_error(index_expression->index, "Index %lld out of bounds for list of size %zu",
                      index_value, values->size);
        return false;
    }

    *oli = ol_start(values);

    for (int64_t i = 0; i < index_value; ++i, oli_next(oli));

    return true;
}

static inline bool eval_func_params(expr *params, environment *env,
                                    expr_list *parameters) {
    switch (params->type) {
        case EXPR_TYPE_UNIT:
            break;
        case EXPR_TYPE_IDENTIFIER: {
            el_append(parameters, params);
        } break;
        case EXPR_TYPE_PREFIX_EXPRESSION: {
            if (params->op.type != TOKEN_TYPE_COLON_COLON ||
                params->right->type != EXPR_TYPE_IDENTIFIER) {
                generic_error(params, "Invalid parameter for function declaration");
                return false;
            }
            el_append(parameters, params);
        } break;
        case EXPR_TYPE_INFIX_EXPRESSION: {
            while (params->type == EXPR_TYPE_INFIX_EXPRESSION &&
                   params->op.type == TOKEN_TYPE_COMMA) {
                CHECK_EVAL(eval_func_params(params->left, env, parameters));

                params = params->right;
            }
            CHECK_EVAL(eval_func_params(params, env, parameters));
        } break;
        default: {
            generic_error(params, "Invalid parameter for function declaration");
            return false;
        }
    }

    return true;
}

static inline bool valid_infix_num_types(const object *left, const object *right) {
    return is_num_type(left) && is_num_type(right);
}

static inline bool is_truthy(const object *obj) {
    switch (obj->type) {
        case OBJECT_TYPE_INT:
            return obj->int_value != 0;
        case OBJECT_TYPE_FLOAT:
            return obj->float_value != 0.0;
        case OBJECT_TYPE_LIST:
            return obj->values.size != 0;
        case OBJECT_TYPE_UNIT:
            return false;
        default: {
            return true;
        }
    }
}

static inline bool is_num_type(const object *obj) {
    return obj->type == OBJECT_TYPE_INT || obj->type == OBJECT_TYPE_FLOAT;
}

static inline bool is_tuple_exp(const expr *e) {
    if (e->type != EXPR_TYPE_INFIX_EXPRESSION) {
        return false;
    }
    return e->op.type == TOKEN_TYPE_COMMA;
}

static inline bool copy_by_value(object_type ot) {
    return ot == OBJECT_TYPE_INT || ot == OBJECT_TYPE_FLOAT || ot == OBJECT_TYPE_CHAR;
}

static inline void undefined_var_error(const expr *e) {
    const token *tok = &e->start_tok;
    generic_error(e, "Variable not in scope: %.*s", (int)tok->length, tok->literal);
}

static inline void generic_error(const expr *e, const char *msg, ...) {
    va_list vargs;
    va_start(vargs, msg);
    eval_err = (eval_error){
        .e = e,
    };
    vsnprintf(eval_err.msg, ERROR_MSG_LENGTH, msg, vargs);
    va_end(vargs);
}

static inline size_t fn_param_count(const object *fn) {
    return fn->builtin ? fn->builtin_param_count : fn->params.size;
}

void add_cmdline_args(char **args, size_t argc, environment *env) {
    object *arg_list = new_obj(OBJECT_TYPE_LIST, 1);

    object_list *arg_obj_list = &arg_list->values;

    for (size_t i = 0; i < argc; ++i) {
        object *arg_obj = new_obj(OBJECT_TYPE_LIST, 1);

        char *arg_literal = args[i];
        size_t arg_length = strlen(arg_literal);

        object *char_obj;
        for (size_t i = 0; i < arg_length; ++i) {
            char_obj = new_obj(OBJECT_TYPE_CHAR, 1);
            char_obj->char_value = arg_literal[i];
            ol_append(&arg_obj->values, char_obj);
        }

        ol_append(arg_obj_list, arg_obj);
    }

    env_set(env, ARGS_VAR_NAME, sizeof(ARGS_VAR_NAME) - 1, arg_list, true);
}

static bool builtin_println(const expr_list *params, const expr *call, environment *env, object *result) {
    (void)call;

    const expr *e = params->head;
    object o;
    CHECK_EVAL(eval(e, env, &o));

    String_Builder sb = {0};
    inspect(&o, &sb, true);

    printf("%.*s\n", (int)sb.size, sb.store);

    object_init(result, OBJECT_TYPE_UNIT);
    return true;
}

static bool builtin_len(const expr_list *params, const expr *call, environment *env, object *result) {
    const expr *list_expr = params->head;

    object list;
    CHECK_EVAL(eval_no_l(list_expr, env, &list));

    if (list.type != OBJECT_TYPE_LIST) {
        generic_error(call, "len function expected list, got %s",
                      object_type_literals[list.type]);
        return false;
    }

    object_init(result, OBJECT_TYPE_INT);
    result->int_value = (int64_t)list.values.size;

    return true;
}

static bool builtin_head(const expr_list *params, const expr *call, environment *env, object *result) {
    const expr *list_expr = params->head;

    object list;
    CHECK_EVAL(eval_no_l(list_expr, env, &list));

    if (list.type != OBJECT_TYPE_LIST) {
        generic_error(call, "head function expected list, got %s",
                      object_type_literals[list.type]);
        return false;
    }

    if (list.values.size < 1) {
        generic_error(call, "Cannot evaluate head of an empty list");
        return false;
    }

    object *head_value = list.values.head->value;

    *result = (object){
        .type = OBJECT_TYPE_LVALUE,
        .ref = head_value,
    };

    return true;
}

static bool builtin_tail(const expr_list *params, const expr *call, environment *env, object *result) {
    const expr *list_expr = params->head;

    object list;
    CHECK_EVAL(eval_no_l(list_expr, env, &list));

    if (list.type != OBJECT_TYPE_LIST) {
        generic_error(call, "head function expected list, got %s",
                      object_type_literals[list.type]);
        return false;
    }

    if (list.values.size < 1) {
        generic_error(call, "Cannot evaluate tail of an empty list");
        return false;
    }

    object_init(result, OBJECT_TYPE_LIST);
    result->values = (object_list){
        .head = list.values.head->next,
        .tail = list.values.tail,
        .size = list.values.size - 1,
    };

    return true;
}

static bool builtin_copy(const expr_list *params, const expr *call, environment *env, object *result) {
    (void)call;

    const expr *list_expr = params->head;

    object obj;
    CHECK_EVAL(eval_no_l(list_expr, env, &obj));

    if (obj.type == OBJECT_TYPE_LIST) {
        object_init(result, OBJECT_TYPE_LIST);

        ol_iterator it = ol_start(&obj.values);
        for (; !oli_is_end(&it); oli_next(&it)) {
            object *to_append;
            if (copy_by_value(it.obj->type)) {
                to_append = new_empty_obj();
                *to_append = *it.obj;
            } else {
                to_append = it.obj;
                ++to_append->rc;
            }
            ol_append(&result->values, to_append);
        }
    } else {
        *result = obj;
    }

    return true;
}

static bool builtin_foreach(const expr_list *params, const expr *call, environment *env, object *result) {
    const expr *list_expr = params->head;
    const expr *func_expr = list_expr->next;

    object list_maybe_l, func, *list;
    CHECK_EVAL(eval(list_expr, env, &list_maybe_l));

    if (list_maybe_l.type == OBJECT_TYPE_LVALUE) {
        list = list_maybe_l.ref;
    } else {
        list = &list_maybe_l;
    }

    if (list->type != OBJECT_TYPE_LIST) {
        generic_error(call, "foreach expected first argument to be list, got %s",
                      object_type_literals[list->type]);
        return false;
    }

    CHECK_EVAL(eval_no_l(func_expr, env, &func));

    if (func.type != OBJECT_TYPE_FUNCTION) {
        generic_error(call, "foreach expected second argument to be function, got %s",
                      object_type_literals[func.type]);
        return false;
    }

    if (func.builtin) {
        generic_error(call,
                      "foreach does not support builtin functions as argument,"
                      "use its normal variant instead");
        return false;
    }

    size_t param_count = fn_param_count(&func);
    if (param_count != 1) {
        generic_error(call, "foreach expected function with one argument, got %zu",
                      param_count);
        return false;
    }

    const expr *func_param = func.params.head;

    object param_obj, *new_entry;
    ol_iterator it = ol_start(&list->values);
    for (; !oli_is_end(&it); oli_next(&it)) {
        object *func_env_obj = new_obj(OBJECT_TYPE_ENVIRONMENT, 1);

        environment *func_env = &func_env_obj->env;
        environment_init(func_env, func.outer_env, env->ht, scope_counter++);
        func_env->obj = func_env_obj;

        param_obj = (object){
            .type = OBJECT_TYPE_LVALUE,
            .ref = it.obj,
        };
        CHECK_EVAL(assign_lhs(func_param, &param_obj, call, func_env, false, NULL));

        new_entry = new_empty_obj();
        CHECK_EVAL(eval(func.body, func_env, new_entry));
        ++new_entry->rc;

        rc_dec(func_env_obj);

        rc_dec(it.obj);
        it.ln->value = new_entry;
    }

    *result = list_maybe_l;

    return true;
}

static bool builtin_append(const expr_list *params, const expr *call, environment *env, object *result) {
    const expr *list_expr = params->head;
    const expr *new_item_expr = list_expr->next;

    object list_maybe_l, *list;
    CHECK_EVAL(eval(list_expr, env, &list_maybe_l));
    if (list_maybe_l.type == OBJECT_TYPE_LVALUE) {
        list = list_maybe_l.ref;
    } else {
        list = &list_maybe_l;
    }

    if (list->type != OBJECT_TYPE_LIST) {
        generic_error(call, "append expected first argument to be list, got %s",
                      object_type_literals[list->type]);
        return false;
    }

    object new_item;
    CHECK_EVAL(eval(new_item_expr, env, &new_item));

    ol_append(&list->values, resolve_assign_rhs(&new_item, NULL));

    *result = list_maybe_l;

    return true;
}

static bool builtin_remove(const expr_list *params, const expr *call, environment *env, object *result) {
    const expr *list_expr = params->head;
    const expr *idx_expr = list_expr->next;

    object list_maybe_l, *list;
    CHECK_EVAL(eval(list_expr, env, &list_maybe_l));
    if (list_maybe_l.type == OBJECT_TYPE_LVALUE) {
        list = list_maybe_l.ref;
    } else {
        list = &list_maybe_l;
    }

    if (list->type != OBJECT_TYPE_LIST) {
        generic_error(call, "remove expected first argument to be list, got %s",
                      object_type_literals[list->type]);
        return false;
    }

    object idx;
    CHECK_EVAL(eval_no_l(idx_expr, env, &idx));
    if (idx.type != OBJECT_TYPE_INT) {
        generic_error(call, "remove expected second argument to an index (iint), got %s",
                      object_type_literals[list->type]);
        return false;
    }

    size_t size = list->values.size;

    if (idx.int_value < 0) {
        generic_error(call, "Negative index value");
        return false;
    }

    size_t idx_value = (size_t)idx.int_value;

    if (idx_value >= size) {
        generic_error(call, "Index %zu out of bounds for list of size %zu", idx_value, size);
        return false;
    }

    object *ln_to_remove;
    if (idx_value == 0) {
        ln_to_remove = list->values.head;

        list->values.head = list->values.head->next;
    } else {
        ol_iterator it = ol_start(&list->values);

        for (size_t i = 0; i < idx_value - 1; ++i, oli_next(&it));

        ln_to_remove = it.ln->next;
        it.ln->next = ln_to_remove->next;

        if (list->values.tail == ln_to_remove) {
            list->values.tail = it.ln;
        }
    }
    --list->values.size;

    ln_to_remove->next = NULL;
    rc_dec(ln_to_remove);

    *result = list_maybe_l;

    return true;
}

static const builtin_entry builtin_fns[] = {
    {"__builtin_println", builtin_println, 1},
    {"__builtin_len", builtin_len, 1},
    {"__builtin_head", builtin_head, 1},
    {"__builtin_tail", builtin_tail, 1},
    {"__builtin_copy", builtin_copy, 1},
    {"__builtin_foreach", builtin_foreach, 2},
    {"__builtin_append", builtin_append, 2},
    {"__builtin_remove", builtin_remove, 2},
};

static const size_t builtin_count = sizeof(builtin_fns) / sizeof(builtin_entry);

void add_builtins(environment *env) {
    object *fn;
    const builtin_entry *entry;
    for (size_t i = 0; i < builtin_count; ++i) {
        entry = builtin_fns + i;

        fn = new_obj(OBJECT_TYPE_FUNCTION, 1);

        fn->builtin = true;
        fn->builtin_param_count = entry->param_count;
        fn->builtin_fn = entry->fn;

        env_set(env, entry->name, strlen(entry->name), fn, true);
    }
}
