#include "zobjutils.h"
#include "rt64_extended_gbi.h"
#include "libc/string.h"
#include "PR/gbi.h"
#include "recomputils.h"

u32 readU32(u8 array[], u32 offset) {
    return ((u32)(array[offset]) << 24) |
           ((u32)(array[offset + 1]) << 16) |
           ((u32)(array[offset + 2]) << 8) |
           ((u32)(array[offset + 3]));
}

void writeU32(u8 array[], u32 offset, u32 value) {
    array[offset] = value >> 24;
    array[offset + 1] = value >> 16;
    array[offset + 2] = value >> 8;
    array[offset + 3] = value;
}

void repointF3DCommand(u8 zobj[], u32 newBase, u32 commandOffset, u8 targetSegment) {

    GfxCommand *command = (GfxCommand *)(&zobj[commandOffset]);

    u32 opcode = zobj[commandOffset];

    u8 segment = zobj[commandOffset + 4];

    u32 segmentedDataOffset = command->values.word1;

    u32 dataOffset = SEGMENT_OFFSET(segmentedDataOffset);

    u32 repointedOffset;

    switch (opcode) {
    case G_DL:
        if (segment == targetSegment) {
            repointDisplayList(zobj, newBase, dataOffset, targetSegment);
        }
    case G_VTX:
    case G_MTX:
    case G_SETTIMG:
        if (segment == targetSegment) {
            repointedOffset = newBase + dataOffset;
            command->values.word1 = repointedOffset;
            recomp_printf("Repointing 0x0%x -> 0x%x\n", segmentedDataOffset, repointedOffset);
        }
        break;

    default:
        break;
    }
}

void repointDisplayList(u8 zobj[], u32 newBase, u32 displayListStartOffset, u8 targetSegment) {
    u32 offset = displayListStartOffset;
    u8 segment;

    u8 opcode;
    bool isEndDl = false;

    while (!isEndDl) {
        opcode = zobj[offset];

        switch (opcode) {
        case G_ENDDL:
            isEndDl = true;
            break;

        case G_DL:
            if (zobj[offset + 1] == G_DL_NOPUSH) {
                isEndDl = true;
            }
        case G_VTX:
        case G_MTX:
        case G_SETTIMG:
            segment = zobj[offset + 4];
            if (segment == targetSegment) {
                repointF3DCommand(zobj, newBase, offset, targetSegment);
            }
            break;

        default:
            break;
        }

        offset += 8;
    }
}

void repointSkeleton(u8 zobj[], u32 skeletonHeaderOffset, u32 newBase) {

    // repoint only if segmented
    if (zobj[skeletonHeaderOffset] == 0x06) {

        u32 firstLimbOffset = SEGMENT_OFFSET(readU32(zobj, skeletonHeaderOffset));

        writeU32(zobj, skeletonHeaderOffset, firstLimbOffset + newBase);

        FlexSkeletonHeader *flexHeader = (FlexSkeletonHeader *)(&zobj[skeletonHeaderOffset]);

        Gfx *repointedDisplayList;

        LodLimb **limbs = (LodLimb **)(flexHeader->sh.segment);

        recomp_printf("Limb count: %d\n", flexHeader->sh.limbCount);
        recomp_printf("First limb entry location: 0x%x\n", limbs);

        for (u8 i = 0; i < flexHeader->sh.limbCount; i++) {
            limbs[i] = (LodLimb *)&zobj[SEGMENT_OFFSET(limbs[i])];
            if (limbs[i]->dLists[0]) {
                repointedDisplayList = (Gfx *)(&zobj[SEGMENT_OFFSET(limbs[i]->dLists[0])]);
                limbs[i]->dLists[0] = repointedDisplayList;
                limbs[i]->dLists[1] = repointedDisplayList;
            }
        }
    }
}

s32 getSkeletonHeaderOffset(u8 zobj[], int zobjSize) {
    recomp_printf("getSkeletonHeaderOffset function is not yet implemented.");
    return -1;
}
