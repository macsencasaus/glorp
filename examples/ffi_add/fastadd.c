#include "../../lib/glorp.h"

#define CHECK_EVAL(eval_res) \
    if (!(eval_res)) return false

builtin_fn_t builtin_add;

bool builtin_fast_add(const expr_list *params, const expr *call, environment *env, object *result) {
    (void)call;

    const expr *addend1_expr = params->head;
    const expr *addend2_expr = addend1_expr->next;

    object addend1, addend2;
    CHECK_EVAL(eval(addend1_expr, env, &addend1));
    CHECK_EVAL(eval(addend2_expr, env, &addend2));

    *result = (object){
        .type = OBJECT_TYPE_INT,
        .int_value = addend1.int_value + addend2.int_value, 
    };

    return true;
}

builtin_entry exported_functions[] = {
    {"__builtin_fast_add", builtin_fast_add, 2},
};

size_t exported_functions_count = 1;
