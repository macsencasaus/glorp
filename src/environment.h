#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdbool.h>

#include "glorpoptions.h"
#include "hashtable.h"

typedef struct environment environment;

struct environment {
    environment *outer;
    hash_table *ht;

    size_t scope;

    object *obj;

    const glorp_options *selected_options;
};

void environment_init(environment *e, environment *outer, hash_table *ht, size_t scope);
bool env_set(environment *env, const char *key, size_t key_length, object *value, bool is_const);
bool env_get(environment *env, const char *key, size_t key_length, object **value, bool *is_const);
bool env_contains_local_scope(environment *env, const char *key, size_t key_length);
void env_destroy(environment *env);

#endif  // ENVIRONMENT_H
