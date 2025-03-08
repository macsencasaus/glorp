#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <stdbool.h>

#include "hash_table.h"

typedef struct environment environment;
typedef struct arena arena;

struct environment {
    arena *a;

    environment *outer;
    hash_table ht;
};

void environment_init(environment *e, arena *a, environment *outer);
void env_set(environment *env, const char *key, size_t key_length,
             object_reference value, bool *ok);
object_reference env_get(environment *env, const char *key, size_t key_length, bool *ok);
bool env_contains_local_scope(environment *env, const char *key, size_t key_length);

#endif  // ENVIRONMENT_H
