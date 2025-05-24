#ifndef __VECTORU32_H__
#define __VECTORU32_H__

#include "global.h"

typedef u32 * VectorU32;

VectorU32 VectorU32_create(size_t initialSize);
void VectorU32_destroy(VectorU32 *v);
void VectorU32_push(VectorU32 *v, u32 element);
void VectorU32_pop(VectorU32 *v);
void VectorU32_remove(VectorU32 *v, size_t index);
size_t VectorU32_count(const VectorU32 *v);
void VectorU32_resize(VectorU32 *v, size_t newSize);
bool VectorU32_has(VectorU32 *v, u32 element);
void VectorU32_clear(VectorU32 *v);

#endif
