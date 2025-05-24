#ifndef __VECTOR_H__
#define __VECTOR_H__

#include "global.h"

typedef struct Vector Vector;

Vector *Vector_create(size_t elementSize);
void Vector_destroy(Vector *v);
void Vector_push(Vector *v, void *element);
void Vector_pop(Vector *v);
void Vector_remove(Vector *v, size_t index);
size_t Vector_count(Vector *v);
void Vector_resize(Vector *v, size_t newSize);
void *Vector_start(Vector *v);
void *Vector_end(Vector *v);
void Vector_set(Vector *v, size_t index, void *element);
void *Vector_get(Vector *v, size_t index);
bool Vector_has(Vector *v, void *element);
void Vector_clear(Vector *v);

#endif
