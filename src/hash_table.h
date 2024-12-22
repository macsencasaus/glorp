#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>

#include "object.h"

#define INITIAL_TABLE_CAPACITY 128;
#define VARIABLE_MAX_LENGTH 128

typedef struct {
    char key[VARIABLE_MAX_LENGTH];
    size_t key_length;

    object_reference value;
} pair;

typedef struct {
    size_t size;
    size_t capacity;

    pair *values;
} hash_table;

void table_set(hash_table *ht, pair pair);

object_reference table_get(hash_table *ht, char *key, size_t key_length);

#endif // HASH_TABLE_H
