#ifndef __ZOBJUTILS_H__
#define __ZOBJUTILS_H__

#include "global.h"

// Repoints F3DZEX2 command to be relative to binary's location in RAM.
//
// assumes zobj points to the start of the binary and that segmented addresses are relative to it.
void repointGfxCommand(u8 zobj[], u32 commandOffset, u8 targetSegment, u32 newBaseAddress);

// Repoints F3DZEX2 display list to be relative to object's location in RAM.
//
// Assumes zobj is to the start of the binary.
void repointDisplayList(u8 zobj[], u32 displayListOffset, u8 targetSegment, u32 newBaseAddress);

// Repoints FlexSkeleton to be relative to binary's location in RAM
// assumes zobj points to the start of the binary and that segmented addresses are relative to it.
void repointFlexSkeleton(u8 zobj[], u32 skeletonHeaderOffset, u8 targetSegment, u32 newBaseAddress);

// Finds index of FlexSkeletonHeader in binary.
// Returns -1 if no skeleton header can be found
s32 getFlexSkeletonHeaderOffset(u8 zobj[], size_t zobjSize);

#endif
