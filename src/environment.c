#include "environment.h"

#include <assert.h>
#include <string.h>

#include "arena.h"

void environment_init(environment *env, arena *a, environment *outer) {
    env->a = a;
    env->outer = outer;
    ht_init(&env->ht);
}

void env_set(environment *env, const char *key, size_t key_length,
             object_reference value, bool *ok) {
    assert(key_length <= VARIABLE_MAX_LENGTH);
    table_item item;

    memcpy(item.key, key, key_length);
    item.key_length = key_length;
    item.value = value;

    ht_set(&env->ht, item, ok);
}

object_reference env_get(environment *env, const char *key, size_t key_length, bool *ok) {
    object_reference ref = ht_get(&env->ht, key, key_length, ok);
    if (!is_null_object(env->a, ref)) {
        return ref;
    }
    if (env->outer != NULL) {
        return env_get(env->outer, key, key_length, ok);
    }
    return ref;
}

bool env_contains_local_scope(environment *env, const char *key, size_t key_length) {
    bool ok;
    ht_get(&env->ht, key, key_length, &ok);
    return ok;
}
/**/
/* bool env_contains_global(environment *env, char *key, size_t key_length) { */
/*     return !is_null_object(env->a, env_get(env, key, key_length)); */
/* } */
