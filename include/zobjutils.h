#ifndef __ZOBJUTILS_H__
#define __ZOBJUTILS_H__

#include "global.h"

// Repoints F3DZEX2 command to be relative to object's location in RAM
void repointF3DCommand(u8 zobj[], u32 newBase, u32 commandOffset, u8 targetSegment);

void repointDisplayList(u8 zobj[], u32 newBase, u32 displayListOffset, u8 targetSegment);

void repointSkeleton(u8 zobj[], u32 skeletonHeaderOffset, u32 newBase);

// Returns -1 if no skeleton header can be found
s32 getSkeletonHeaderOffset(u8 zobj[], int zobjSize);

#endif
