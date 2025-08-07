#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdbool.h>
#include <stdlib.h>

#define VARIABLE_MAX_LENGTH 128

typedef struct object object;

typedef struct {
    char key[VARIABLE_MAX_LENGTH];
    size_t key_length;

    size_t scope;

    object *value;
    bool is_const;
} table_item;

typedef struct {
    size_t size;
    size_t capacity;

    table_item *values;
} hash_table;

void ht_init(hash_table *ht);
bool ht_set(hash_table *ht, const table_item *pair);
bool ht_get(hash_table *ht, const char *key, size_t key_length, size_t scope, object **ref, bool *is_const);
bool ht_remove(hash_table *ht, const char *key, size_t key_length, size_t scope);
void ht_destroy(hash_table *ht);
void hti_set_avail(table_item *hti);

void print_ht_info(const hash_table *ht);

#endif  // HASH_TABLE_H
