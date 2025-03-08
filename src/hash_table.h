#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include <stdlib.h>

typedef size_t object_reference;

#define VARIABLE_MAX_LENGTH 128

typedef struct {
    char key[VARIABLE_MAX_LENGTH];
    size_t key_length;

    object_reference value;
    bool is_const;
} table_item;

typedef struct {
    size_t size;
    size_t capacity;

    table_item *values;
} hash_table;

void ht_init(hash_table *ht);
void ht_set(hash_table *ht, table_item pair, bool *ok);
object_reference ht_get(hash_table *ht, const char *key, size_t key_length, bool *ok);

#endif  // HASH_TABLE_H
