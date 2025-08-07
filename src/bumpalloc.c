#include "bumpalloc.h"

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

typedef unsigned char byte;

static size_t page_size;

void ba_init(bump_alloc *ba) {
    page_size = sysconf(_SC_PAGESIZE);

    ba->store = mmap(NULL, BUMP_ALLOC_RESERVE_SIZE, PROT_NONE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ba->store == MAP_FAILED) {
        perror("mmap failed");
        exit(1);
    }

    ba->next_page = ba->store;

    ba->pages = 0;
    ba->size = 0;
}

void *ba_malloc(bump_alloc *ba, size_t size) {
    ba_reserve(ba, size);

    void *ret = (byte *)ba->store + ba->size;
    ba->size += size;
    return ret;
}

void ba_reserve(bump_alloc *ba, size_t size) {
    if ((byte *)ba->store + ba->size + size >= (byte *)ba->next_page) {
        size_t additional_size =
            (byte *)ba->store + ba->size + size - (byte *)ba->next_page;
        size_t pages_to_commit = additional_size / page_size + 1;

        ba->pages += pages_to_commit;

        size_t bytes_to_commit = pages_to_commit * page_size;

        if (mprotect(ba->next_page, bytes_to_commit, PROT_READ | PROT_WRITE) !=
            0) {
            perror("mprotect failed");
            exit(1);
        }

        ba->next_page = (byte *)ba->next_page + bytes_to_commit;
    }
}

void ba_reset(bump_alloc *ba) {
    size_t bytes_to_uncommit = ba->pages * page_size;
    if (madvise(ba->store, bytes_to_uncommit, MADV_DONTNEED) != 0) {
        perror("madvise failed");
        exit(1);
    }
    ba->size = 0;
    ba->pages = 0;
    ba->next_page = ba->store;
}

void ba_destroy(bump_alloc *ba) {
    if (ba->store == NULL) return;
    if (munmap(ba->store, BUMP_ALLOC_RESERVE_SIZE) != 0) {
        perror("munmap failed");
        exit(1);
    }
    ba->size = ba->pages = 0;
    ba->next_page = ba->store = NULL;
}

void ba_free(bump_alloc *ba, size_t size) {
    if (ba->store == NULL) return;
    if (ba->size <= size) {
        ba->size = 0;
    } else {
        ba->size -= size;
    }

    // TODO: madvise pages that are no longer required
}
