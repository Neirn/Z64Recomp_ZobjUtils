#ifndef __HELPERS_H__
#define __HELPERS_H__

#include "global.h"

bool isMemEqual(const void *a, const void *b, size_t n);
u32 readU32(const u8 array[], u32 offset);
void writeU32(u8 array[], u32 offset, u32 value);

#endif
