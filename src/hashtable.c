#include "hashtable.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_LOAD_FACTOR 0.7

static bool find_avail(hash_table *ht, const char *key, size_t key_length,
                 size_t scope, size_t *idx);
static bool find(hash_table *ht, const char *key, size_t key_length,
                 size_t scope, size_t *idx);
static void ensure_load_factor(hash_table *ht);
static void resize(hash_table *ht);
inline static bool is_avail_item(const table_item *item);
inline static bool is_null_item(const table_item *item);
static size_t djb2_hash(const char *key, size_t key_length);
static size_t hash_combine(size_t h1, size_t h2);

static const size_t primes[] = {
    53, 97, 193, 389, 769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317, 196613, 393241,
    786433, 1572869, 3145739, 6291469, 12582917,
    25165843, 50331653, 100663319, 201326611,
    402653189, 805306457, 1610612741};

static size_t cur_prime = 0;

static size_t next_prime(void) {
    return primes[cur_prime++];
}

void ht_init(hash_table *ht) {
    size_t capacity = next_prime();
    table_item *values =
        (table_item *)calloc(capacity, sizeof(table_item));

    if (values == NULL) {
        fprintf(stderr, "Error malloc hash_table");
        exit(1);
    }

    *ht = (hash_table){
        .size = 0,
        .capacity = capacity,
        .values = values,
    };
}

bool ht_set(hash_table *ht, const table_item *pair) {
    size_t idx;
    bool replace_existing = find_avail(ht, pair->key, pair->key_length, pair->scope, &idx);

    if (!replace_existing) {
        ht->values[idx] = *pair;
        ++ht->size;
    } else {
        if (ht->values[idx].is_const) {
            return false;
        }
        if (pair->is_const) {
            return false;
        }
        ht->values[idx].value = pair->value;
    }

    ensure_load_factor(ht);
    return true;
}

bool ht_get(hash_table *ht, const char *key, size_t key_length, size_t scope,
            object **ref, bool *is_const) {
    size_t idx;
    if (!find(ht, key, key_length, scope, &idx)) return false;

    if (ref != NULL) {
        *ref = ht->values[idx].value;
    }
    if (is_const != NULL) {
        *is_const = ht->values[idx].is_const;
    }
    return true;
}

bool ht_remove(hash_table *ht, const char *key, size_t key_length,
               size_t scope) {
    size_t idx;
    if (!find(ht, key, key_length, scope, &idx)) return false;

    hti_set_avail(ht->values + idx);
    --ht->size;
    return true;
}

void ht_destroy(hash_table *ht) { free(ht->values); }

void hti_set_avail(table_item *ht) {
    ht->key[0] = 1;
}

static bool find(hash_table *ht, const char *key, size_t key_length,
                 size_t scope, size_t *idx) {
    size_t h1 = djb2_hash(key, key_length);
    size_t h2 = scope * 11400714819323198485llu;
    size_t hash = hash_combine(h1, h2);

    table_item *cur;
    for (size_t i = 0;; ++i) {
        *idx = (hash + i * i) % ht->capacity;
        cur = ht->values + *idx;

        if (is_null_item(cur)) {
            return false;
        }

        if (strncmp(ht->values[*idx].key, key, key_length) == 0 &&
            ht->values[*idx].scope == scope) {
            break;
        }
    }
    return true;
}

static bool find_avail(hash_table *ht, const char *key, size_t key_length,
                       size_t scope, size_t *idx) {
    size_t h1 = djb2_hash(key, key_length);
    size_t h2 = scope * 11400714819323198485llu;
    size_t hash = hash_combine(h1, h2);

    table_item *cur;
    for (size_t i = 0;; ++i) {
        *idx = (hash + i * i) % ht->capacity;
        cur = ht->values + *idx;

        if (is_null_item(cur) || is_avail_item(cur)) {
            return false;
        }

        if (strncmp(ht->values[*idx].key, key, key_length) == 0 &&
            ht->values[*idx].scope == scope) {
            break;
        }
    }
    return true;
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
    size_t new_capacity = next_prime();

    ht->values = (table_item *)calloc(new_capacity, sizeof(table_item));
    ht->capacity = new_capacity;
    ht->size = 0;

    table_item *cur;
    for (size_t i = 0; i < old_capacity; ++i) {
        cur = temp + i;
        if (is_null_item(cur)) {
            continue;
        }
        ht_set(ht, &temp[i]);
    }

    free(temp);
}

inline static bool is_avail_item(const table_item *item) {
    return *item->key == 1;
}

inline static bool is_null_item(const table_item *item) {
    return *item->key == 0;
}

#define DJB2_PRIME 5381

static size_t djb2_hash(const char *key, size_t key_length) {
    size_t hash = DJB2_PRIME;
    for (size_t i = 0; i < key_length; ++i) {
        hash = ((hash << 5) + hash) ^ ((size_t)key[i]);
    }
    return hash;
}

static size_t hash_combine(size_t h1, size_t h2) {
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

void print_ht_info(const hash_table *ht) {
    printf("\n------\n");

    printf("HASH TABLE\nSIZE: %zu\nCAPACITY: %lu\n", ht->size, ht->capacity);

    if (ht->size > 0)
        printf("\nVALUES:\n");

    for (size_t i = 0; i < ht->capacity; ++i) {
        const table_item *item = ht->values + i;
        if (item->value == NULL)
            continue;
        printf("%3zu: value: %3p, const: %d, scope: %zu, key: %.*s\n", i,
               (void *)item->value, item->is_const, item->scope, (int)item->key_length,
               item->key);
    }
}
