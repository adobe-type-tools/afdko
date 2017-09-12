/* Copyright 2017 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Dictionary support.
 */


#include "ctutil.h"
#include "cffwrite_varstore.h"
#include "cffwrite_dict.h"
#include <stdio.h>


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

unsigned long getRegionListSize(variationRegionList* regionList)
{
    unsigned int size = 4 + regionList->regionCount*regionList->axisCount*6;
    
    return size;
}

unsigned long getDataItemSize(itemVariationDataSubtable* ds)
{
    unsigned int size = 6 + ds->regionCount*2 + ds->itemCount*2;
    return size;
}

void writeRegionList(DICT *dst, variationRegionList* regionList)
{
    unsigned short i, j;
    variationRegion* vr = &(regionList->regions.array[0]);
    cfwDictSaveShortInt(dst, regionList->axisCount);
    cfwDictSaveShortInt(dst, regionList->regionCount);
    
    
    for (i = 0; i < regionList->regionCount; i++)
    {
        for (j = 0; j < regionList->axisCount; j++)
        {
            cfwDictSaveShortInt(dst, FIXED_TO_F2DOT14(vr->startCoord));
            cfwDictSaveShortInt(dst, FIXED_TO_F2DOT14(vr->peakCoord));
            cfwDictSaveShortInt(dst, FIXED_TO_F2DOT14(vr->endCoord));
            vr++;
        }
    }
}

void writeDataItemList(DICT *dst, itemVariationDataSubtable* ivd)
{
    unsigned short i, cnt;
    cfwDictSaveShortInt(dst, ivd->itemCount);
    cfwDictSaveShortInt(dst, 0);
    cfwDictSaveShortInt(dst, ivd->regionCount);
    
    cnt = ivd->regionCount;
    for (i = 0; i < cnt; i++)
    {
        cfwDictSaveShortInt(dst, ivd->regionIndices.array[i]);
    }
}


void cfwDictFillVarStore(cfwCtx g, DICT *dst, abfTopDict *top)
{
    int i;
    var_itemVariationStore  ivs = top->varStore;
    unsigned long offset;
    
    // Write place holder for length.
    cfwDictSaveShortInt(dst, 0);
   
    // write format
    cfwDictSaveShortInt(dst, 1);
    
    // write offset to RegionList.
    // The Region list immediately follows the IVS header and offset to the dataItems.
    offset = 8 + ivs->dataList.ivdSubtables.cnt*4;
    cfwDictSaveLongInt(dst, offset);

    // write ItemVariationData count
    cfwDictSaveShortInt(dst, ivs->dataList.ivdSubtables.cnt);

    // write the offsets to the ItemVariationData items.
    offset += getRegionListSize(&(ivs->regionList));
    for (i = 0; i < ivs->dataList.ivdSubtables.cnt; i++)
    {
        cfwDictSaveLongInt(dst, offset);
        offset += getDataItemSize(&(ivs->dataList.ivdSubtables.array[i]));
    }
    
    writeRegionList(dst, &(ivs->regionList));
    
    // write the ItemVariationData subtables
    for (i = 0; i < ivs->dataList.ivdSubtables.cnt; i++)
    {
        writeDataItemList(dst, &(ivs->dataList.ivdSubtables.array[i]));
    }
    
    //Write length
    {
        char *arg = &(dst->array[0]);
        arg[0] = (unsigned char)(offset >> 8);
        arg[1] = (unsigned char)offset;
    }
}

