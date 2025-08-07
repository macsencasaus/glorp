// Bump allocator

#ifndef BUMP_ALLOC_H
#define BUMP_ALLOC_H

#include <stdlib.h>

#ifndef BUMP_ALLOC_RESERVE_SIZE
#define BUMP_ALLOC_RESERVE_SIZE 5llu * 1024llu * 1024llu * 1024llu  // 5 gigabytes
#endif

typedef unsigned char byte;

typedef struct {
    size_t size;
    size_t pages;
    byte *store;
    byte *next_page;
} bump_alloc;

void ba_init(bump_alloc *ba);

void *ba_malloc(bump_alloc *ba, size_t size);

void ba_reserve(bump_alloc *ba, size_t size);

void ba_reset(bump_alloc *ba);

void ba_destroy(bump_alloc *ba);

void ba_free(bump_alloc *ba, size_t size);

#endif
