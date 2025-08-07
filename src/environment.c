#include "environment.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "arena.h"

void environment_init(environment *env, environment *outer, hash_table *ht,
                      size_t scope) {
    *env = (environment){
        .outer = outer,
        .ht = ht,
        .scope = scope,

        .selected_options = outer ? outer->selected_options : NULL,
    };
}

bool env_set(environment *env, const char *key, size_t key_length,
             object *value, bool is_const) {
    assert(key_length <= VARIABLE_MAX_LENGTH);
    table_item item = {
        .key_length = key_length,
        .scope = env->scope,
        .value = value,
        .is_const = is_const,
    };

    memcpy(item.key, key, key_length);

    return ht_set(env->ht, &item);
}

bool env_get(environment *env, const char *key, size_t key_length, object **value,
             bool *is_const) {
    bool ok = ht_get(env->ht, key, key_length, env->scope, value, is_const);
    if (ok) return ok;
    if (env->outer != NULL) {
        return env_get(env->outer, key, key_length, value, is_const);
    }
    return ok;
}

bool env_contains_local_scope(environment *env, const char *key,
                              size_t key_length) {
    return ht_get(env->ht, key, key_length, env->scope, NULL, NULL);
}

void env_destroy(environment *env) {
    // TODO: implement list of allocated objects to destroy more efficiently

    size_t scope = env->scope;

    table_item *cur;
    for (size_t i = 0; i < env->ht->capacity; ++i) {
        cur = env->ht->values + i;
        if (cur->scope == scope) {
            rc_dec(cur->value);
            hti_set_avail(cur);
            --env->ht->size;
        }
    }
}
