#include "recomputils.h"
#include "vector.h"
#include "libc/string.h"
#include "helpers.h"

struct Vector {
    size_t elementSize;
    size_t capacity;
    size_t count;
    void *data;
};

Vector *Vector_create(size_t elementSize) {
    Vector *v = recomp_alloc(sizeof(Vector));
    v->elementSize = elementSize;
    v->capacity = 0;
    v->count = 0;
    v->data = NULL;
    return v;
}

void Vector_destroy(Vector *v) {
    if (v->data) {
        recomp_free(v->data);
    }
    recomp_free(v);
}

void *Vector_get(Vector *v, size_t index) {
    if (index > v->count) {
        return NULL;
    }

    char *data = (char *)v->data;

    return &data[v->elementSize * index];
}

void Vector_set(Vector *v, size_t index, void *element) {
    if (index > v->count) {
        return;
    }

    char *data = (char *)v->data;

    memcpy(&data[v->elementSize * index], v->data, v->elementSize);
}

void Vector_resize(Vector *v, size_t newCapacity) {
    if (!v->data) {
        v->capacity = newCapacity;
        v = recomp_alloc(newCapacity * v->elementSize);
    } else if (newCapacity != v->capacity) {
        size_t numElementsToCopy = v->count;
        if (numElementsToCopy < newCapacity) {
            numElementsToCopy = newCapacity;
        }

        size_t numBytesToCopy = numElementsToCopy * v->elementSize;

        void *newContainer = recomp_alloc(newCapacity * v->elementSize);

        memcpy(newContainer, v->data, numBytesToCopy);

        recomp_free(v->data);

        v->data = newContainer;
    }
}

void Vector_push(Vector *v, void *newElement) {
    size_t newCount = v->count + 1;

    if (newCount >= v->capacity) {
        size_t newCapacity = v->capacity;

        if (!newCapacity) {
            newCapacity++;
        }

        while (newCount > newCapacity) {
            newCapacity *= 2;
        }

        Vector_resize(v, newCapacity);
    }

    char *data = (char *)v->data;

    memcpy(&data[v->elementSize * v->count], newElement, v->elementSize);

    v->count++;
}

void Vector_pop(Vector *v) {
    if (v->count > 0) {
        v->count--;
    }
}

size_t Vector_count(Vector *v) {
    return v->count;
}

void *Vector_start(Vector *v) {
    return v->data;
}

void *Vector_end(Vector *v) {
    if (v->count == 0) {
        return NULL;
    }

    char *data = (char *)v->data;

    return &data[v->count * v->elementSize];
}

void Vector_remove(Vector *v, size_t index) {
    if (index >= v->count) {
        return;
    }

    for (size_t i = index; i < v->count - 1; ++i) {
        Vector_set(v, i, Vector_get(v, i + 1));
    }
}

bool Vector_has(Vector *v, void *element) {
    for (size_t i = 0; i < v->count; ++i) {
        if (isMemEqual(element, Vector_get(v, i), v->elementSize)) {
            return true;
        }
    }

    return false;
}
