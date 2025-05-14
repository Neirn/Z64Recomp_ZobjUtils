#include "global.h"
#include "libc/string.h"
#include "modding.h"
#include "recomputils.h"
#include "rt64_extended_gbi.h"
#include "stdbool.h"
#include "z64animation.h"

#define LOCAL_ARRAY_BYTE_SIZE(a) (sizeof(a) / sizeof(a[0]))

void ZobjUtils_repointGfxCommand(u8 zobj[], u32 commandOffset, u8 targetSegment, const void *newBase);
void ZobjUtils_repointDisplayList(u8 zobj[], u32 displayListStartOffset, u8 targetSegment, const void *newBase);

u32 readU32(const u8 array[], u32 offset) {
    return (u32)(array[offset + 0]) << 24 |
           (u32)(array[offset + 1]) << 16 |
           (u32)(array[offset + 2]) << 8 |
           (u32)(array[offset + 3]);
}

void writeU32(u8 array[], u32 offset, u32 value) {
    array[offset + 0] = (value & 0xFF000000) >> 24;
    array[offset + 1] = (value & 0x00FF0000) >> 16;
    array[offset + 2] = (value & 0x0000FF00) >> 8;
    array[offset + 3] = (value & 0x000000FF);
}

RECOMP_EXPORT void ZobjUtils_repointGfxCommand(u8 zobj[], u32 commandOffset, u8 targetSegment, const void *newBase) {
    u32 newBaseAddress = (u32)newBase;

    GfxCommand *command = (GfxCommand *)(&zobj[commandOffset]);

    u32 opcode = zobj[commandOffset];

    u8 segment = zobj[commandOffset + 4];

    u32 segmentedDataOffset = command->values.word1;

    u32 dataOffset = SEGMENT_OFFSET(segmentedDataOffset);

    u32 repointedOffset;

    switch (opcode) {
    case G_DL:
    case G_VTX:
    case G_MTX:
    case G_SETTIMG:
        if (segment == targetSegment) {
            repointedOffset = newBaseAddress + dataOffset;
            command->values.word1 = repointedOffset;
            recomp_printf("Repointing 0x0%x -> 0x%x\n", segmentedDataOffset, repointedOffset);
        }
        break;

    default:
        break;
    }
}

RECOMP_EXPORT void ZobjUtils_repointDisplayList(u8 zobj[], u32 displayListStartOffset, u8 targetSegment, const void *newBase) {

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
            if (segment == targetSegment) {
                ZobjUtils_repointDisplayList(zobj, offset, targetSegment, newBase);
            }
            
            if (zobj[offset + 1] == G_DL_NOPUSH) {
                isEndDl = true;
            }
        case G_VTX:
        case G_MTX:
        case G_SETTIMG:
            segment = zobj[offset + 4];
            if (segment == targetSegment) {
                ZobjUtils_repointGfxCommand(zobj, offset, targetSegment, newBase);
            }
            break;

        default:
            break;
        }

        offset += 8;
    }
}

RECOMP_EXPORT void ZobjUtils_repointFlexSkeleton(u8 zobj[], u32 skeletonHeaderOffset, u8 targetSegment, const void *newBase) {
    u32 newBaseAddress = (u32)newBase;

    // repoint only if segmented
    if (zobj[skeletonHeaderOffset] == targetSegment) {

        u32 firstLimbOffset = SEGMENT_OFFSET(readU32(zobj, skeletonHeaderOffset));

        FlexSkeletonHeader *flexHeader = (FlexSkeletonHeader *)(&zobj[skeletonHeaderOffset]);

        writeU32(zobj, skeletonHeaderOffset, firstLimbOffset + newBaseAddress);

        Gfx *repointedDisplayList;

        LodLimb **limbs = (LodLimb **)(&zobj[firstLimbOffset]);

        recomp_printf("Limb count: %d\n", flexHeader->sh.limbCount);
        recomp_printf("First limb entry location: 0x%x\n", limbs);

        LodLimb *limb;
        for (u8 i = 0; i < flexHeader->sh.limbCount; i++) {
            limb = (LodLimb *)&zobj[SEGMENT_OFFSET(limbs[i])];
            if (limb->dLists[0]) { // do not repoint limbs without display lists
                limb->dLists[0] = (Gfx *)(SEGMENT_OFFSET(limb->dLists[0]) + newBaseAddress);
                limb->dLists[1] = (Gfx *)(SEGMENT_OFFSET(limb->dLists[1]) + newBaseAddress);
            }
            limbs[i] = (LodLimb *)(SEGMENT_OFFSET(limbs[i]) + newBaseAddress);
        }
    }
}

bool isBytesEqual(const void *ptr1, const void *ptr2, size_t num) {
    const u8 *a = ptr1;
    const u8 *b = ptr2;

    for (size_t i = 0; i < num; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}

RECOMP_EXPORT s32 ZobjUtils_getFlexSkeletonHeaderOffset(const u8 zobj[], u32 zobjSize) {
    // Link should always have 0x15 limbs where 0x12 have display lists
    // so, if a hierarchy exists, then this string must appear at least once
    u8 lowerHeaderBytes[] = {0x15, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00};

    const u8 LOWER_HEADER_SIZE = LOCAL_ARRAY_BYTE_SIZE(lowerHeaderBytes);

    const u8 FLEX_HEADER_SIZE = 0xC;

    u32 index = FLEX_HEADER_SIZE - LOWER_HEADER_SIZE;

    u32 endIndex = zobjSize - FLEX_HEADER_SIZE;

    while (index < endIndex) {
        if (isBytesEqual(&zobj[index], &lowerHeaderBytes[0], LOWER_HEADER_SIZE)) {
            // account for first four bytes of header
            return index - 4;
        }

        // header must be aligned
        index += 4;
    }

    // Returning a signed value here isn't ideal
    // but in practice there should never be a zobj passed in that's >2 GB in size
    return -1;
}
