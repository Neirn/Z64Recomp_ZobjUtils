#include "zobjutils.h"
#include "rt64_extended_gbi.h"
#include "libc/string.h"
#include "PR/gbi.h"
#include "recomputils.h"
#include "z64animation.h"
#include "stdbool.h"

#define LOCAL_ARRAY_BYTE_SIZE(a) (sizeof(a) / sizeof(a[0]))

u32 readU32(u8 array[], u32 offset) {
    return *(u32 *)(&array[offset]);
}

void writeU32(u8 array[], u32 offset, u32 value) {
    *(u32 *)(&array[offset]) = value;
}

void repointF3DCommand(u8 zobj[], u32 commandOffset, u8 targetSegment) {

    GfxCommand *command = (GfxCommand *)(&zobj[commandOffset]);

    u32 opcode = zobj[commandOffset];

    u8 segment = zobj[commandOffset + 4];

    u32 segmentedDataOffset = command->values.word1;

    u32 dataOffset = SEGMENT_OFFSET(segmentedDataOffset);

    u32 repointedOffset;

    switch (opcode) {
    case G_DL:
        if (segment == targetSegment) {
            repointDisplayList(zobj, dataOffset, targetSegment);
        }
    case G_VTX:
    case G_MTX:
    case G_SETTIMG:
        if (segment == targetSegment) {
            repointedOffset = (u32)&zobj[0] + dataOffset;
            command->values.word1 = repointedOffset;
            recomp_printf("Repointing 0x0%x -> 0x%x\n", segmentedDataOffset, repointedOffset);
        }
        break;

    default:
        break;
    }
}

void repointDisplayList(u8 zobj[], u32 displayListStartOffset, u8 targetSegment) {
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
                repointF3DCommand(zobj, offset, targetSegment);
            }
            break;

        default:
            break;
        }

        offset += 8;
    }
}

void repointFlexSkeleton(u8 zobj[], u32 skeletonHeaderOffset) {

    // repoint only if segmented
    if (zobj[skeletonHeaderOffset] == 0x06) {

        u32 firstLimbOffset = SEGMENT_OFFSET(readU32(zobj, skeletonHeaderOffset));

        FlexSkeletonHeader *flexHeader = (FlexSkeletonHeader *)(&zobj[skeletonHeaderOffset]);

        writeU32(zobj, skeletonHeaderOffset, firstLimbOffset + (u32)&zobj[0]);

        Gfx *repointedDisplayList;

        LodLimb **limbs = flexHeader->sh.segment;

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

bool isBytesEqual(const void *ptr1, const void *ptr2, size_t num) {
    u8 *a = ptr1;
    u8 *b = ptr2;

    for (int i = 0; i < num; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

s32 getFlexSkeletonHeaderOffset(const u8 zobj[], size_t zobjSize) {
    // Link should always have 0x15 limbs where 0x12 have display lists
    // so, if a hierarchy exists, then this string must appear at least once
    u8 lowerHeaderBytes[] = {0x15, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00};

    const LOWER_HEADER_SIZE = LOCAL_ARRAY_BYTE_SIZE(lowerHeaderBytes);

    const FLEX_HEADER_SIZE = 0xC;

    int index = FLEX_HEADER_SIZE - LOWER_HEADER_SIZE;

    int endIndex = zobjSize - FLEX_HEADER_SIZE;

    int flexSkelHeaderOffset = -1;

    while (index < endIndex) {
        if (isBytesEqual(&zobj[index], &lowerHeaderBytes[0], LOWER_HEADER_SIZE)) {
            // account for first four bytes of header
            return index - 4;
        }

        // header must be aligned
        index += 4;
    }

    return -1;
}
