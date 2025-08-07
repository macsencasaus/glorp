#ifndef FIXED_ALLOC_H
#define FIXED_ALLOC_H

#include <assert.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdbool.h>

// must be multiple of page size
#ifndef FIXED_ALLOC_RESERVE_SIZE
#define FIXED_ALLOC_RESERVE_SIZE 5llu * 1024llu * 1024llu * 1024llu  // 5 gigabytes
#endif

typedef unsigned char byte;

typedef struct {
    byte *store;
    byte *mask;
    byte *next_page;

    size_t size;
    size_t ty_size;
    size_t pages;
} fixed_alloc;

int fa_init(fixed_alloc *fa, size_t ty_size);

void *fa_malloc(fixed_alloc *fa);

void fa_free(fixed_alloc *fa, void *ptr);

int fa_destroy(fixed_alloc *fa);

bool fa_valid_ptr(fixed_alloc *fa, void *ptr);

#endif  // FIXED_ALLOC_H
