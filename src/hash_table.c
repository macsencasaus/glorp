#include "hash_table.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define INITIAL_TABLE_CAPACITY 32
#define MAX_LOAD_FACTOR 0.7

static void ensure_load_factor(hash_table *ht);
static void resize(hash_table *ht);
inline static bool is_null_item(table_item *item);
static size_t djb2_hash(const char *key, size_t key_length);

void ht_init(hash_table *ht) {
    ht->size = 0;
    ht->capacity = INITIAL_TABLE_CAPACITY;
    ht->values = (table_item *)malloc(sizeof(table_item) * INITIAL_TABLE_CAPACITY);
}

void ht_set(hash_table *ht, table_item pair, bool *ok) {
    size_t hash = djb2_hash(pair.key, pair.key_length);

    bool replace_existing = false;
    size_t idx;
    table_item *cur;
    for (size_t i = 0;; ++i) {
        idx = (hash + i * i) % ht->capacity;
        cur = ht->values + idx;

        if (is_null_item(cur)) {
            break;
        }

        if (strncmp(ht->values[idx].key, pair.key, pair.key_length) == 0) {
            replace_existing = true;
            break;
        }
    }

    if (!replace_existing) {
        ht->values[idx] = pair;
        ++ht->size;
    } else {
        if (ht->values[idx].is_const) {
            // TODO: implement error values for hash table
            exit(1);
        }
        ht->values[idx].value = pair.value;
    }

    ensure_load_factor(ht);
}

object_reference ht_get(hash_table *ht, const char *key, size_t key_length,
                           bool *ok) {
    size_t hash = djb2_hash(key, key_length);

    size_t idx;
    table_item *cur;
    for (size_t i = 0;; ++i) {
        idx = (hash + i * i) % ht->capacity;
        cur = ht->values + idx;

        if (is_null_item(cur)) {
            *ok = false;
            break;
        }

        if (strncmp(ht->values[idx].key, key, key_length) == 0) {
            *ok = true;
            break;
        }
    }

    return ht->values[idx].value;
}

static void ensure_load_factor(hash_table *ht) {
    float size = (float)ht->size;
    float capacity = (float)ht->capacity;

    float load_factor = size / capacity;

    if (load_factor > MAX_LOAD_FACTOR) {
        resize(ht);
    }
}

static void resize(hash_table *ht) {
    table_item *temp = ht->values;
    size_t old_capacity = ht->capacity;

    size_t new_capacity = 2 * old_capacity;
    ht->values = (table_item *)malloc(sizeof(table_item) * new_capacity);
    ht->capacity = new_capacity;
    ht->size = 0;

    table_item *cur;
    bool ok;
    for (size_t i = 0; i < old_capacity; ++i) {
        cur = temp + i;
        if (is_null_item(cur)) {
            continue;
        }
        ht_set(ht, temp[i], &ok);
    }

    free(temp);
}

inline static bool is_null_item(table_item *item) { return *(char *)item == 0; }

#define DJB2_PRIME 5381

static size_t djb2_hash(const char *key, size_t key_length) {
    size_t hash = DJB2_PRIME;
    for (size_t i = 0; i < key_length; ++i) {
        hash = ((hash << 5) + hash) ^ ((size_t)key[i]);
    }
    return hash;
}
