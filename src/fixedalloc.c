#include "fixedalloc.h"

static size_t page_size;

int fa_init(fixed_alloc *fa, size_t ty_size) {
    page_size = sysconf(_SC_PAGESIZE);

    void *store = mmap(NULL, FIXED_ALLOC_RESERVE_SIZE, PROT_NONE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (store == MAP_FAILED)
        return -1;

    void *mask = mmap(NULL, FIXED_ALLOC_RESERVE_SIZE >> 3, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mask == MAP_FAILED)
        return -1;

    *fa = (fixed_alloc){
        .store = store,
        .mask = mask,
        .next_page = store,

        .ty_size = ty_size,
    };

    return 0;
}

void *fa_malloc(fixed_alloc *fa) {
    char *mask = (char *)fa->mask;

    size_t capacity = page_size * fa->pages;

    if (fa->size + fa->ty_size > capacity) {
        size_t additional_bytes = fa->size + fa->ty_size - capacity;
        size_t pages_to_commit = additional_bytes / page_size + 1;
        size_t bytes_to_commit = pages_to_commit * page_size;

        if (mprotect(fa->next_page, bytes_to_commit, PROT_READ | PROT_WRITE) != 0) {
            return NULL;
        }

        fa->pages += pages_to_commit;
        fa->next_page += bytes_to_commit;

        capacity += bytes_to_commit;
    }

    for (size_t byte_offset = 0; byte_offset < (capacity >> 3); ++byte_offset) {
        int bit_offset = __builtin_ffs(~mask[byte_offset]);
        if (bit_offset == 0 || bit_offset > 8)
            continue;

        bit_offset = (bit_offset - 1) & 7;
        mask[byte_offset] |= 1 << bit_offset;

        size_t offset = ((byte_offset << 3) + (bit_offset & 7)) * fa->ty_size;
        fa->size += fa->ty_size;
        return fa->store + offset;
    }

    return NULL;
}

void fa_free(fixed_alloc *fa, void *ptr) {
    byte *mask = (byte *)fa->mask;

    size_t offset = (byte *)ptr - fa->store;

    assert(offset % fa->ty_size == 0);

    offset /= fa->ty_size;

    size_t byte_offset = offset >> 3;
    size_t bit_offset = offset & 7;

    mask[byte_offset] &= ~(1 << bit_offset);

    fa->size -= fa->ty_size;
}

int fa_destroy(fixed_alloc *fa) {
    if (munmap(fa->store, FIXED_ALLOC_RESERVE_SIZE))
        return -1;

    if (munmap(fa->mask, FIXED_ALLOC_RESERVE_SIZE >> 3))
        return -1;

    return 0;
}

bool fa_valid_ptr(fixed_alloc *fa, void *ptr) {
    byte *mask = (byte *)fa->mask;

    size_t offset = (byte *)ptr - fa->store;

    size_t capacity = fa->pages * page_size;

    if (offset > capacity) {
        return false;
    }

    if (offset % fa->ty_size != 0) {
        return false;
    }

    offset /= fa->ty_size;

    size_t byte_offset = offset >> 3;
    size_t bit_offset = offset & 7;

    return mask[byte_offset] & (1 << bit_offset);
}
