#include "hash_table.h"

#include <stdbool.h>

#define MAX_LOAD_FACTOR 0.7

static void ensure_load_factor(hash_table *ht);
static void resize(hash_table *ht);
static size_t djb2_hash(char *key, size_t key_length);

void table_set(hash_table *ht, pair pair) {
    size_t hash = djb2_hash(pair.key, pair.key_length);
    
    bool replace_existing = false;
    size_t idx;
    for (size_t i = 0;; ++i) {
        idx = (hash + i * i) % ht->capacity;
        if (ht->values + idx == NULL) {
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
        ht->values[idx].value = pair.value;
    }

    ensure_load_factor(ht);
}

object_reference table_get(hash_table *ht, char *key, size_t key_length) {
    size_t hash = djb2_hash(key, key_length);
    
    size_t idx;
    for (size_t i = 0;; ++i) {
        idx = (hash + i * i) % ht->capacity;
        if (ht->values + idx == NULL) {
            break;
        }

        if (strncmp(ht->values[idx].key, key, key_length) == 0) {
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

static size_t djb2_hash(char *key, size_t key_length) {
    size_t hash = 5381;
    for (size_t i = 0; i < key_length; ++i) {
        hash = ((hash << 5) + hash) ^ ((size_t)key[i]);
    }
    return hash;
}
