#include "global.h"
#include "libc/string.h"
#include "modding.h"
#include "recomputils.h"
#include "rt64_extended_gbi.h"
#include "libc/stdbool.h"
#include "z64animation.h"
#include "helpers.h"
#include "recompdata.h"
#include "vector.h"

typedef struct ZobjUtils_DisplayListExtractionResult ZobjUtils_DisplayListExtractionResult;

void ZobjUtils_repointGfxCommand(u8 zobj[], u32 commandOffset, u8 targetSegment, const void *newBase);
void ZobjUtils_repointDisplayList(u8 zobj[], u32 displayListStartOffset, u8 targetSegment, const void *newBase);

RECOMP_EXPORT void ZobjUtils_repointGfxCommand(u8 zobj[], u32 commandOffset, u8 targetSegment, const void *newBase) {
    u32 newBaseAddress = (u32)newBase;

    Gfx *command = (Gfx *)(&zobj[commandOffset]);

    u32 opcode = zobj[commandOffset];

    u8 segment = zobj[commandOffset + 4];

    u32 segmentedDataOffset = command->words.w1;

    u32 dataOffset = SEGMENT_OFFSET(segmentedDataOffset);

    u32 repointedOffset;

    switch (opcode) {
    case G_DL:
    case G_VTX:
    case G_MTX:
    case G_SETTIMG:
        if (segment == targetSegment) {
            repointedOffset = newBaseAddress + dataOffset;
            command->words.w1 = repointedOffset;
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
                segment = zobj[offset + 4];
                if (segment == targetSegment) {
                    ZobjUtils_repointDisplayList(zobj, SEGMENT_OFFSET(readU32(zobj, offset + 4)), targetSegment, newBase);
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

RECOMP_EXPORT s32 ZobjUtils_getFlexSkeletonHeaderOffset(const u8 zobj[], u32 zobjSize) {
    // Link should always have 0x15 limbs where 0x12 have display lists
    // so, if a hierarchy exists, then this string must appear at least once
    u8 lowerHeaderBytes[] = {0x15, 0x00, 0x00, 0x00, 0x12, 0x00, 0x00, 0x00};

    const u8 LOWER_HEADER_SIZE = ARRAY_COUNT(lowerHeaderBytes);

    const u8 FLEX_HEADER_SIZE = 0xC;

    u32 index = FLEX_HEADER_SIZE - LOWER_HEADER_SIZE;

    u32 endIndex = zobjSize - FLEX_HEADER_SIZE;

    while (index < endIndex) {
        if (isMemEqual(&zobj[index], &lowerHeaderBytes[0], LOWER_HEADER_SIZE)) {
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

typedef struct {
    u8 *buffer;
    size_t size;
    u32 *dependencies;
} DisplayListData;

void insertOrReplaceIfBigger(U32ValueHashmapHandle handle, u32 key, u32 value) {
    if (recomputil_u32_value_hashmap_contains(handle, key)) {
        u32 currentVal;
        recomputil_u32_value_hashmap_get(handle, key, currentVal);
        if (value > currentVal) {
            recomputil_u32_value_hashmap_erase(handle, key);
            recomputil_u32_value_hashmap_insert(handle, key, value);
        }

    } else {
        recomputil_u32_value_hashmap_insert(handle, key, value);
    }
}

u32 findTexSize(u8* buffer, size_t startIndex) {
    
}

RECOMP_EXPORT ZobjUtils_DisplayListExtractionResult *ZobjUtils_extractDisplayLists(void *zobj, void *newBase, u8 targetSegment, Gfx *displayLists[], size_t numDisplayLists) {
    u8* buffer = (u8 *)zobj;

    U32HashsetHandle visitedDls = recomputil_create_u32_hashset();

    // map offset from start of zobj to size of these elements
    U32ValueHashmapHandle vertices = recomputil_create_u32_value_hashmap();
    U32ValueHashmapHandle textures = recomputil_create_u32_value_hashmap();

    Vector* dlsToVisit = Vector_create(sizeof(u32));

    for (size_t i = 0; i < numDisplayLists; ++i) {
        Gfx* curr = displayLists[i];
        u8 seg = SEGMENT_NUMBER(curr);
        u32 offset = SEGMENT_OFFSET(curr);
        if (seg == targetSegment && !recomputil_u32_hashset_contains(visitedDls, offset)) {
            Vector_push(dlsToVisit, &offset);
            recomputil_u32_hashset_insert(visitedDls, offset);
        }
    }

    // first pass: gather all relevant offsets for display lists, textures, palettes, and vertex data
    u32 *dlOffsets = (u32 *)Vector_start(dlsToVisit);

    Vector* dlDependencies = Vector_create(sizeof(u32));

    Vector* deReturnStack = Vector_create(sizeof(u32));

    for (size_t i = 0; i < Vector_count(dlsToVisit); ++i) {
        bool isEnd = false;

        u32 startOffset = dlOffsets[i];

        size_t i = startOffset;

        DisplayListData dlDat;

        dlDat.buffer = &buffer[startOffset];

        Vector_clear(dlDependencies);

        while (!isEnd) {

            u8 opcode = buffer[i];

            u32 upper = readU32(buffer, i);
            u32 lower = readU32(buffer, i + 4);

            u8 seg = SEGMENT_OFFSET(upper);

            u32 segAddr = SEGMENT_OFFSET(lower);

            u32 newDlOffset;

            u32 len;
            u32 len2;

            u32 texAddr = SEGMENT_ADDR(targetSegment, 0);

            if (seg == targetSegment) {
                switch (opcode) {
                    case G_ENDDL:
                        isEnd = true;
                        break;

                    case G_DL:

                        if (seg == targetSegment && !recomputil_u32_hashset_contains(visitedDls, segAddr)) {
                            Vector_push(dlsToVisit, &segAddr);
                            recomputil_u32_hashset_insert(visitedDls, segAddr);
                        }

                        if (buffer[i + 1] == G_DL_NOPUSH) {
                            isEnd = true;
                        }
                        break;
                    
                    case G_SETTIMG:
                        len = (upper & 0x0FF000) >> 12;
                        insertOrReplaceIfBigger(vertices, lower, len);
                        break;
                    
                    case G_OBJ_LOADTXTR:

                    default:
                        break;
                }
            }

            i += 8;
        }
    }

    Vector_destroy(dlDependencies);
    Vector_destroy(dlsToVisit);
    recomputil_destroy_u32_hashset(visitedDls);
    recomputil_destroy_u32_memory_hashmap(vertices);
    recomputil_destroy_u32_memory_hashmap(textures);
}
