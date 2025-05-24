#include "recomputils.h"
#include "vectorU32.h"
#include "helpers.h"

#define MIN_CAPACITY 1
#define GROWTH_FACTOR 2

// spells out "VECTOR32" in ASCII
#define HEADER_MAGIC 0x564543544F523332ULL

typedef struct {
    u64 identifier;
    size_t capacity;
    size_t count;
} VectorU32Header;

VectorU32 VectorU32_create(size_t initialSize) {
    if (initialSize < MIN_CAPACITY) {
        initialSize = MIN_CAPACITY;
    }

    VectorU32Header* h = recomp_alloc(sizeof(VectorU32Header) + initialSize * sizeof(u32));

    h->capacity = initialSize;
    h->count = 0;

    uintptr_t hInt = (uintptr_t)h;

    hInt += sizeof(VectorU32Header);

    return (u32 *)hInt;
}

bool isVectorU32(VectorU32 v) {
    return *(u64 *)((uintptr_t)v - sizeof(VectorU32Header)) == HEADER_MAGIC;
}

VectorU32Header *getVector32UHeader(VectorU32 v) {
    if (v && isVectorU32(v)) {
        return (VectorU32Header *)((uintptr_t)v - sizeof(VectorU32Header));
    }

    return NULL;
}

void VectorU32_destroy(VectorU32 *v) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (h) {
        h->identifier = 0;
        *v = NULL;
        recomp_free(h);
    }
}

void VectorU32_resize(VectorU32 *v, size_t newCapacity) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (!h) {
        return;
    }

    if (newCapacity < MIN_CAPACITY) {
        newCapacity = MIN_CAPACITY;
    }

    if (newCapacity != h->capacity) {
        size_t numElementsToCopy = h->count;
        if (numElementsToCopy < newCapacity) {
            numElementsToCopy = newCapacity;
        }

        void *newVec = recomp_alloc(sizeof(VectorU32Header) + newCapacity * sizeof(u32));

        u32 *newContainer = (u32 *)((uintptr_t)newVec + sizeof(VectorU32Header));

        for (size_t i = 0; i < numElementsToCopy; ++i) {
            newContainer[i] = (*v)[i];
        }

        recomp_free(h);

        *v = newContainer;
    }
}

void VectorU32_push(VectorU32 *v, u32 element) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (!h) {
        return;
    }

    size_t newCount = h->count + 1;

    if (newCount > h->capacity) {
        size_t newCapacity = h->capacity;

        while (newCount > newCapacity) {
            newCapacity *= GROWTH_FACTOR;
        }

        VectorU32_resize(v, newCapacity);

        // resizing changes header location
        h = getVector32UHeader(*v);
    }
    
    (*v)[h->count] = element;

    h->count++;
}

void VectorU32_pop(VectorU32 *v) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (!h) {
        return;
    }

    if (h->count > 0) {
        h->count--;
    }
}

size_t VectorU32_count(VectorU32 *v) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (!h) {
        return 0;
    }

    return h->count;
}

void VectorU32_remove(VectorU32 *v, size_t index) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (!h) {
        return;
    }

    if (index >= h->count) {
        return 0;
    }

    u32 result = (*v)[index];
    for (size_t i = index; i < h->count - 1; ++i) {
        (*v)[i] = (*v)[i + 1];
    }

    h->count--;
}

bool VectorU32_has(VectorU32 *v, u32 val) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (!h) {
        return false;
    }

    for (size_t i = 0; i < h->count; ++i) {
        if ((*v)[i] == val) {
            return true;
        }
    }

    return false;
}

void VectorU32_clear(VectorU32 *v) {
    VectorU32Header *h = getVector32UHeader(*v);

    if (!h) {
        return;
    }

    h->count = 0;
}
