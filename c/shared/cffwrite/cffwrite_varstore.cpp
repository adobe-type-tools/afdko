/* Copyright 2017 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Dictionary support.
 */

#include "cffwrite_varstore.h"

#include <stdio.h>
#include <vector>

#include "ctutil.h"
#include "cffwrite_dict.h"

/* Save byte int  in DICT. */
void cfwDictSaveByteInt(DICT *dict, long i) {
    char *arg = dnaEXTEND(*dict, 1);
    arg[1] = (unsigned char)(i);
}

/* Save short int  in DICT. */
void cfwDictSaveShortInt(DICT *dict, long i) {
    char *arg = dnaEXTEND(*dict, 2);
    arg[0] = (unsigned char)(i >> 8);
    arg[1] = (unsigned char)i;
}

/* Save long int  in DICT. */
void cfwDictSaveLongInt(DICT *dict, long i) {
    char *arg = dnaEXTEND(*dict, 4);
    arg[0] = (unsigned char)(i >> 24);
    arg[1] = (unsigned char)(i >> 16);
    arg[2] = (unsigned char)(i >> 8);
    arg[3] = (unsigned char)i;
}

void writeRegionList(DICT *dst, uint16_t axisCount,
                     std::vector<std::vector<itemVariationStore::variationRegion>> &regions) {
    uint16_t i;
    cfwDictSaveShortInt(dst, axisCount);
    cfwDictSaveShortInt(dst, (int16_t) regions.size());

    for (auto &r : regions) {
        for (auto &vr: r) {
            cfwDictSaveShortInt(dst, FIXED_TO_F2DOT14(vr.startCoord));
            cfwDictSaveShortInt(dst, FIXED_TO_F2DOT14(vr.peakCoord));
            cfwDictSaveShortInt(dst, FIXED_TO_F2DOT14(vr.endCoord));
        }
    }
}

void writeDataItemList(DICT *dst,
                       itemVariationStore::itemVariationDataSubtable &ivd) {
    uint16_t i, cnt = (uint16_t) ivd.regionIndices.size();
    cfwDictSaveShortInt(dst, (uint16_t) ivd.deltaValues.size());
    cfwDictSaveShortInt(dst, 0);
    cfwDictSaveShortInt(dst, cnt);

    for (i = 0; i < cnt; i++)
        cfwDictSaveShortInt(dst, ivd.regionIndices[i]);
}

void cfwDictFillVarStore(cfwCtx g, DICT *dst, abfTopDict *top) {
    int i;
    itemVariationStore *ivs = top->varStore;
    unsigned long offset;

    // Write place holder for length.
    cfwDictSaveShortInt(dst, 0);

    // write format
    cfwDictSaveShortInt(dst, 1);

    // write offset to RegionList.
    // The Region list immediately follows the IVS header and offset to the dataItems.
    offset = 8 + ivs->subtables.size() * 4;
    cfwDictSaveLongInt(dst, offset);

    // write ItemVariationData count
    cfwDictSaveShortInt(dst, ivs->subtables.size());

    // write the offsets to the ItemVariationData items.
    offset += ivs->getRegionListSize();
    for (i = 0; i < ivs->subtables.size(); i++) {
        cfwDictSaveLongInt(dst, offset);
        offset += ivs->getDataItemSize(i);
    }

    writeRegionList(dst, ivs->axisCount, ivs->regions);

    // write the ItemVariationData subtables
    for (i = 0; i < ivs->subtables.size(); i++) {
        writeDataItemList(dst, ivs->subtables[i]);
    }

    // Write length
    {
        char *arg = &(dst->array[0]);
        arg[0] = (unsigned char)(offset >> 8);
        arg[1] = (unsigned char)offset;
    }
}
