#ifndef __ZOBJUTILS_H__
#define __ZOBJUTILS_H__

#include "global.h"

// Repoints F3DZEX2 command to be relative to binary's location in RAM.
//
// Assumes zobj points to is the start of the binary.
void repointF3DCommand(u8 zobj[], u32 commandOffset, u8 targetSegment);

// Repoints F3DZEX2 display list to be relative to object's location in RAM.
//
// Assumes zobj is to the start of the binary.
void repointDisplayList(u8 zobj[], u32 displayListOffset, u8 targetSegment);

// Repoints FlexSkeleton to be relative to binary's location in RAM
// assumes zobj is to the start of the binary.
void repointFlexSkeleton(u8 zobj[], u32 skeletonHeaderOffset);

// Finds index of FlexSkeletonHeader in binary.
// Returns -1 if no skeleton header can be found
s32 getFlexSkeletonHeaderOffset(u8 zobj[], int zobjSize);

#endif
