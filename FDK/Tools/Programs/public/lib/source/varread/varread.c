/* Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
#include "varread.h"
#include "ctlshare.h"
#include "cffread.h"
#include "dynarr.h"
#include "supportfp.h"
#include "txops.h"

#include <string.h>

/* ----------------------------- fixed number constants, types, macros ---------------------------- */

#define FIXED_ZERO                  0
#define FIXED_ONE                   0x00010000
#define F2DOT14_ZERO                0
#define F2DOT14_ONE                 (1<<14)
#define F2DOT14_MINUS_ONE           (-F2DOT14_ONE)

#define F2DOT14_TO_FIXED(v)         (((Fixed)(v))<<2)
#define FIXED_TO_F2DOT14(v)         ((var_F2dot14)(((Fixed)(v)+0x00000002)>>2))

/* ----------------------------- variation font table constants ---------------------------- */

#define HHEA_TABLE_TAG              CTL_TAG('h','h','e','a')
#define VHEA_TABLE_TAG              CTL_TAG('v','h','e','a')
#define HMTX_TABLE_TAG              CTL_TAG('h','m','t','x')
#define VMTX_TABLE_TAG              CTL_TAG('v','m','t','x')
#define VORG_TABLE_TAG              CTL_TAG('V','O','R','G')
#define AVAR_TABLE_TAG              CTL_TAG('a','v','a','r')
#define FVAR_TABLE_TAG              CTL_TAG('f','v','a','r')
#define HVAR_TABLE_TAG              CTL_TAG('H','V','A','R')
#define VVAR_TABLE_TAG              CTL_TAG('V','V','A','R')
#define MVAR_TABLE_TAG              CTL_TAG('M','V','A','R')

#define HHEA_TABLE_VERSION          0x00010000
#define VHEA_TABLE_VERSION          0x00010000
#define VHEA_TABLE_VERSION_1_1      0x00011000
#define VORG_TABLE_VERSION          0x00010000
#define AVAR_TABLE_VERSION          0x00010000
#define FVAR_TABLE_VERSION          0x00010000
#define HVAR_TABLE_VERSION          0x00010000
#define VVAR_TABLE_VERSION          0x00010000
#define MVAR_TABLE_VERSION          0x00010000

#define HHEA_TABLE_HEADER_SIZE      36
#define VHEA_TABLE_HEADER_SIZE      36
#define VORG_TABLE_HEADER_SIZE      8
#define AVAR_TABLE_HEADER_SIZE      6
#define FVAR_TABLE_HEADER_SIZE      16
#define HVAR_TABLE_HEADER_SIZE      20
#define VVAR_TABLE_HEADER_SIZE      24
#define MVAR_TABLE_HEADER_SIZE      12
#define MVAR_TABLE_RECORD_SIZE      8

#define AVAR_SEGMENT_MAP_SIZE       (2+4*3)
#define AVAR_AXIS_VALUE_MAP_SIZE    4

#define FVAR_OFFSET_TO_AXES_ARRAY   16
#define FVAR_COUNT_SIZE_PAIRS       2
#define FVAR_AXIS_SIZE              20
#define FVAR_INSTANCE_SIZE          4
#define FVAR_INSTANCE_WITH_NAME_SIZE    6

#define ITEM_VARIATION_STORE_TABLE_FORMAT       1
#define IVS_SUBTABLE_HEADER_SIZE                12
#define IVS_VARIATION_REGION_LIST_HEADER_SIZE   4

#define REGION_AXIS_COORDINATES_SIZE            (2*3)
#define ITEM_VARIATION_DATA_HEADER_SIZE         (2*3)

#define DELTA_SET_INDEX_MAP_HEADER_SIZE         (2*2)

#define INNER_INDEX_BIT_COUNT_MASK              0x000F
#define MAP_ENTRY_SIZE_MASK                     0x0030
#define MAP_ENTRY_SIZE_SHIFT                    4

/* ----------------------------- variation font table data ---------------------------- */

/* avar table */
struct var_avar_;
typedef struct var_avar_ *var_avar;

/* fvar table */
struct var_fvar_;
typedef struct var_fvar_ *var_fvar;

/* avar table */

typedef struct axisValueMap_ {
    Fixed  fromCoord;
    Fixed  toCoord;
} axisValueMap;

typedef dnaDCL(axisValueMap, axisValueMapArray);

typedef struct segmentMap_ {
    unsigned short  positionMapCount;
    axisValueMapArray   valueMaps;
} segmentMap;

typedef dnaDCL(segmentMap, segmentMapArray);

struct var_avar_ {
    unsigned short          axisCount;
    segmentMapArray         segmentMaps;
};

/* fvar table */

typedef struct variationAxis_ {
    ctlTag          tag;
    Fixed           minValue;
    Fixed           defaultValue;
    Fixed           maxValue;
    unsigned short  flags;
    unsigned short  nameID;
} variationAxis;

typedef dnaDCL(variationAxis, variationAxesArray);

typedef dnaDCL(float, coordinatesArray);

typedef struct variationInstance_ {
    unsigned short  subfamilyNameID;
    unsigned short  flags;
    coordinatesArray    coordinates;
    unsigned short  postScriptNameID;
} variationInstance;

typedef dnaDCL(variationInstance, variationInstancesArray);

struct var_fvar_ {
    unsigned short          axisCount;
    unsigned short          instanceCount;
    variationAxesArray      axes;
    variationInstancesArray instances;
};

/* variable font axis tables */

struct var_axes_ {
    var_avar    avar;
    var_fvar    fvar;
};

typedef dnaDCL(var_glyphMetrics, var_glyphMetricsArray);

/* index pair */

typedef struct indexPair_ {
    unsigned short  outerIndex;
    unsigned short  innerIndex;
} indexPair;

typedef dnaDCL(indexPair, indexMap);

/* horizontal metrics tables: hhea, hmtx, HVAR */

typedef struct var_hhea_ {
    Fixed   version;
    short   ascender;
    short   descender;
    short   lineGap;
    unsigned short  advanceWidthMax;
    short   minLeftSideBearing;
    short   minRightSideBearing;
    short   xMaxExtent;
    short   caretSlopeRise;
    short   caretSlopeRun;
    short   caretOffset;
    short   reserved[4];
    short   metricDataFormat;
    unsigned short  numberOfHMetrics;
} var_hhea;

struct var_hmtx_ {
    var_hhea        header;
    var_glyphMetricsArray   defaultMetrics;

    /* HHVAR */
    var_itemVariationStore  ivs;
    indexMap   widthMap;
    indexMap   lsbMap;
    indexMap   rsbMap;
};
typedef struct var_hmtx_ *var_hmtx;

/* horizontal metrics tables: hhea, hmtx, HVAR */

typedef struct var_vhea_ {
    Fixed   version;
    short   vertTypoAscender;
    short   vertTypoDescender;
    short   vertTypoLineGap;
    short   advanceHeightMax;
    short   minTop;
    short   minBottom;
    short   yMaxExtent;
    short   caretSlopeRise;
    short   caretSlopeRun;
    short   caretOffset;
    short   reserved[4];
    short   metricDataFormat;
    unsigned short  numOfLongVertMetrics;
} var_vhea;

/* vertical metrics tables: vhea, vmtx, VORG, VVAR */

struct var_vmtx_ {
    var_vhea        header;
    var_itemVariationStore  ivs;
    var_glyphMetricsArray   defaultMetrics;
    dnaDCL(short, vertOriginY);
    indexMap   widthMap;
    indexMap   tsbMap;
    indexMap   bsbMap;
    indexMap   vorgMap;
};
typedef struct var_vmtx_ *var_vmtx;

/* MVAR table */

typedef struct mvarValueRecord_ {
    ctlTag          valueTag;
    indexPair       pair;
} mvarValueRecord;

typedef dnaDCL(mvarValueRecord, mvarValueArray);

struct var_MVAR_ {
    var_itemVariationStore  ivs;
    unsigned short  axisCount;
    unsigned short  valueRecordCount;
    mvarValueArray  values;
};

/* ----------------------------- math utility functions ---------------------------- */

float var_F2dot14ToFloat(var_F2dot14 v)
{
    return (float)v / (1 << 14);
}

/* ----------------------------- variation font table loading functions ---------------------------- */

/* avar table */

static void var_freeavar(ctlSharedStmCallbacks *sscb, var_avar avar)
{
    if (avar) {
        unsigned short  i;
        
        for (i = 0; i < avar->axisCount; i++) {
            dnaFREE(avar->segmentMaps.array[i].valueMaps);
        }
        dnaFREE(avar->segmentMaps);

        sscb->memFree(sscb, avar);
    }
}

static var_avar var_loadavar(sfrCtx sfr, ctlSharedStmCallbacks *sscb)
{
    var_avar   avar = NULL;
    unsigned short  i;

	sfrTable *table = sfrGetTableByTag(sfr, AVAR_TABLE_TAG);
	if (table == NULL)
		return NULL;

	sscb->seek(sscb, table->offset);

	/* Read and validate table version */
	if (sscb->read4(sscb) != AVAR_TABLE_VERSION) {
		sscb->message(sscb, "invalid avar table version");
        return NULL;
    }

    if (table->length < AVAR_TABLE_HEADER_SIZE) {
		sscb->message(sscb, "invalid avar table size");
        return NULL;
    }
    
    avar = sscb->memNew(sscb, sizeof(*avar));
    memset(avar, 0, sizeof(*avar));

    avar->axisCount = sscb->read2(sscb);

    if (table->length < AVAR_TABLE_HEADER_SIZE + (unsigned long)AVAR_SEGMENT_MAP_SIZE * avar->axisCount) {
		sscb->message(sscb, "invalid avar table size or axis/instance count/size");
        goto cleanup;
    }

    dnaINIT(sscb->dna, avar->segmentMaps, 0, 1);
    if (dnaSetCnt(&avar->segmentMaps, DNA_ELEM_SIZE_(avar->segmentMaps), avar->axisCount) < 0)
        goto cleanup;

    for (i = 0; i < avar->axisCount; i++) {
        segmentMap  *seg = &avar->segmentMaps.array[i];
        unsigned short  j;
        int hasZeroMap = 0;
        seg->positionMapCount = sscb->read2(sscb);
        if (table->length < sscb->tell(sscb) - table->offset + AVAR_AXIS_VALUE_MAP_SIZE * seg->positionMapCount) {
            sscb->message(sscb, "avar axis value map out of bounds");
            goto cleanup;
        }

        dnaINIT(sscb->dna, seg->valueMaps, 0, 1);
        if (dnaSetCnt(&seg->valueMaps, DNA_ELEM_SIZE_(seg->valueMaps), seg->positionMapCount) < 0)
            goto cleanup;
        for (j = 0; j < seg->positionMapCount; j++) {
            Fixed fromCoord = F2DOT14_TO_FIXED((var_F2dot14)sscb->read2(sscb));
            Fixed toCoord = F2DOT14_TO_FIXED((var_F2dot14)sscb->read2(sscb));

            if (j > 0 && j < seg->positionMapCount - 1 && fromCoord == 0 && toCoord == 0) {
                hasZeroMap = 1;
            }
            seg->valueMaps.array[j].fromCoord = fromCoord;
            seg->valueMaps.array[j].toCoord = toCoord;
        }
        if (seg->positionMapCount < 3
            || seg->valueMaps.array[0].fromCoord != 0
            || seg->valueMaps.array[0].toCoord != 0
            || !hasZeroMap
            || seg->valueMaps.array[seg->positionMapCount-1].fromCoord != FIXED_ONE
            || seg->valueMaps.array[seg->positionMapCount-1].toCoord != FIXED_ONE) {
            seg->positionMapCount = 0;  /* incomplete value maps: invalidate the maps entirely for this axis */
        }
    }

    return avar;

cleanup:
    var_freeavar(sscb, avar);

    return NULL;
}

/* fvar table */

static void var_freefvar(ctlSharedStmCallbacks *sscb, var_fvar fvar)
{
    if (fvar) {
        unsigned short  i;
        
       for (i = 0; i < fvar->instanceCount; i++) {
            dnaFREE(fvar->instances.array[i].coordinates);
        }
        dnaFREE(fvar->instances);
        dnaFREE(fvar->axes);

        sscb->memFree(sscb, fvar);
    }
}

static var_fvar var_loadfvar(sfrCtx sfr, ctlSharedStmCallbacks *sscb)
{
    var_fvar   fvar = NULL;
    unsigned short  offsetToAxesArray;
    unsigned short  countSizePairs;
    unsigned short  axisSize;
    unsigned short  instanceSize;
    unsigned short  i;

	sfrTable *table = sfrGetTableByTag(sfr, FVAR_TABLE_TAG);
	if (table == NULL)
		return NULL;

	sscb->seek(sscb, table->offset);

	/* Read and validate table version */
	if (sscb->read4(sscb) != FVAR_TABLE_VERSION) {
		sscb->message(sscb, "invalid fvar table version");
        return NULL;
    }

    if (table->length < FVAR_TABLE_HEADER_SIZE) {
		sscb->message(sscb, "invalid fvar table size");
        return NULL;
    }
    
    fvar = sscb->memNew(sscb, sizeof(*fvar));
    memset(fvar, 0, sizeof(*fvar));

    offsetToAxesArray = sscb->read2(sscb);
    countSizePairs = sscb->read2(sscb);
    fvar->axisCount = sscb->read2(sscb);
    axisSize = sscb->read2(sscb);
    fvar->instanceCount = sscb->read2(sscb);
    instanceSize = sscb->read2(sscb);

    /* sanity check of values */
    if (offsetToAxesArray < FVAR_OFFSET_TO_AXES_ARRAY
        || countSizePairs < FVAR_COUNT_SIZE_PAIRS
        || axisSize < FVAR_AXIS_SIZE) {
		sscb->message(sscb, "invalid values in fvar table header");
        goto cleanup;
    }

    if (table->length < offsetToAxesArray + (unsigned long)axisSize * fvar->axisCount + (unsigned long)instanceSize * fvar->instanceCount
        || instanceSize < FVAR_INSTANCE_SIZE + 4 * fvar->axisCount) {
		sscb->message(sscb, "invalid fvar table size or axis/instance count/size");
        return NULL;
    }
    
    sscb->seek(sscb, table->offset + offsetToAxesArray);

    dnaINIT(sscb->dna, fvar->axes, 0, 1);
    dnaINIT(sscb->dna, fvar->instances, 0, 1);
    if (dnaSetCnt(&fvar->axes, DNA_ELEM_SIZE_(fvar->axes), fvar->axisCount) < 0)
        goto cleanup;
    if (dnaSetCnt(&fvar->instances, DNA_ELEM_SIZE_(fvar->instances), fvar->instanceCount) < 0)
        goto cleanup;

    for (i = 0; i < fvar->axisCount; i++) {
        fvar->axes.array[i].tag = sscb->read4(sscb);
        fvar->axes.array[i].minValue = (Fixed)sscb->read4(sscb);
        fvar->axes.array[i].defaultValue = (Fixed)sscb->read4(sscb);
        fvar->axes.array[i].maxValue = (Fixed)sscb->read4(sscb);
        fvar->axes.array[i].flags = sscb->read2(sscb);
        fvar->axes.array[i].nameID = sscb->read2(sscb);
    }
    
    for (i = 0; i < fvar->instanceCount; i++) {
        variationInstance   *inst = &fvar->instances.array[i];
        unsigned short  j;
        inst->subfamilyNameID = sscb->read2(sscb);
        inst->flags = sscb->read2(sscb);
        dnaINIT(sscb->dna, inst->coordinates, 0, 1);
        if (dnaSetCnt(&inst->coordinates, DNA_ELEM_SIZE_(inst->coordinates), fvar->axisCount) < 0)
            goto cleanup;
        for (j = 0; j < fvar->axisCount; j++) {
			fixtopflt((Fixed)sscb->read4(sscb), &inst->coordinates.array[j]);
        }
        if (instanceSize >= FVAR_INSTANCE_WITH_NAME_SIZE + 4 * fvar->axisCount)
            inst->postScriptNameID = sscb->read2(sscb);
        else
            inst->postScriptNameID = 0;    /* indicating an unspecified PostScript name ID */
    }

    return fvar;

cleanup:
    var_freefvar(sscb, fvar);

    return NULL;
}

/* load font axis tables */
var_axes var_loadaxes(sfrCtx sfr, ctlSharedStmCallbacks *sscb)
{
    var_axes    axis_tables = NULL;

    axis_tables = sscb->memNew(sscb, sizeof(*axis_tables));
    memset(axis_tables, 0, sizeof(*axis_tables));

    axis_tables->fvar = var_loadfvar(sfr, sscb);
    if (!axis_tables->fvar)
        goto cleanup;

    axis_tables->avar = var_loadavar(sfr, sscb);
    if (axis_tables->avar && axis_tables->fvar->axisCount != axis_tables->avar->axisCount) {
		sscb->message(sscb, "mismatching axis counts in fvar and avar");
        var_freeavar(sscb, axis_tables->avar);
        axis_tables->avar = NULL;
    }

    return axis_tables;

cleanup:
    var_freeaxes(sscb, axis_tables);
    return NULL;
}

/* free font axis tables */
void var_freeaxes(ctlSharedStmCallbacks *sscb, var_axes axes)
{
    if (axes) {
        if (axes->avar)
            var_freeavar(sscb, axes->avar);
        if (axes->fvar)
            var_freefvar(sscb, axes->fvar);
        sscb->memFree(sscb, axes);
    }
}

unsigned short  var_getAxisCount(var_axes axes)
{
    if (axes && axes->fvar) {
        return axes->fvar->axisCount;
    }
    else
        return 0;
}

int var_getAxis(var_axes axes, unsigned short index, unsigned long *tag, Fixed *minValue, Fixed *defaultValue, Fixed *maxValue, unsigned short *flags)
{
    if (axes && axes->fvar && index < axes->fvar->axisCount ) {
        variationAxis   *axis = &axes->fvar->axes.array[index];
        if (tag) *tag = axis->tag;
        if (minValue) *minValue = axis->minValue;
        if (defaultValue) *defaultValue = axis->defaultValue;
        if (maxValue) *maxValue = axis->maxValue;
        if (flags) *flags = axis->flags;
        return 0;
    }
    else
        return 1;
}

static var_F2dot14 var_defaultNormalizeAxis(var_fvar fvar, unsigned short axisIndex, Fixed userValue)
{
    variationAxis   *axis = &fvar->axes.array[axisIndex];

    if (userValue < axis->defaultValue) {
        if (userValue < axis->minValue)
            return F2DOT14_MINUS_ONE;

        return FIXED_TO_F2DOT14(fixdiv(-(axis->defaultValue - userValue), axis->defaultValue - axis->minValue));
    } else if (userValue > axis->defaultValue) {
        if (userValue > axis->maxValue)
            return F2DOT14_MINUS_ONE;

        return FIXED_TO_F2DOT14(fixdiv(userValue - axis->defaultValue, axis->maxValue - axis->defaultValue));
    }
    else
        return F2DOT14_ZERO;
}

static Fixed    applySegmentMap(segmentMap *seg, Fixed value)
{
    long  i;
    axisValueMap    *map;
    Fixed   endFromVal, endToVal, startFromVal, startToVal;

    if (seg->valueMaps.cnt <= 0)
        return value;

    for (i = 0; i < seg->valueMaps.cnt; i++) {
        map = &seg->valueMaps.array[i];
        if (value >= map->fromCoord)
            break;
    }
    if (i == 0)
        return map[0].toCoord;

    if (i >= seg->valueMaps.cnt)        /* shouldn't happen */
        return map[-1].toCoord;

    endFromVal = map[0].fromCoord;
    endToVal = map[0].toCoord;

    if (value == endFromVal)
        return endToVal;

    startFromVal = map[-1].fromCoord;
    startToVal = map[-1].toCoord;

    return startToVal + fixmul(endToVal - startToVal, fixdiv(value - startFromVal, endFromVal - startFromVal));
}

int var_normalizeCoords(ctlSharedStmCallbacks *sscb, var_axes axes, Fixed *userCoords, var_F2dot14 *normCoords)
{
    unsigned short  i;
    unsigned short axisCount;

    if (!axes || !axes->fvar) {
		sscb->message(sscb, "var_normalizeCoords: invalid axis table");
        return 1;
    }

    axisCount = axes->fvar->axisCount;

    /* default normalization */
    for (i = 0; i < axisCount; i++) {
        normCoords[i] = var_defaultNormalizeAxis(axes->fvar, i, userCoords[i]);
    }

    /* modify the default normalized Coords with the axis variations table */
    if (axes->avar) {
        for (i = 0; i < axisCount; i++) {
            segmentMap  *seg = &axes->avar->segmentMaps.array[i];
            if (seg->positionMapCount > 0)
                normCoords[i] = (var_F2dot14)applySegmentMap(seg, normCoords[i]);
        }
    }

    return 0;
}

/* get the number of named instances */
unsigned short var_getInstanceCount(var_axes axes)
{
    if (axes && axes->fvar)
        return axes->fvar->instanceCount;
    else
        return 0;
}

/* get a named instance */
int var_findInstance(var_axes axes, float *userCoords, unsigned short axisCount, unsigned short *subfamilyID, unsigned short *postscriptID)
{
    unsigned short  i;
    var_fvar        fvar;

    if (!axes || !axes->fvar)
        return -1;

    fvar = axes->fvar;
    if (axisCount != fvar->axisCount)
        return -1;

    for (i = 0; i < fvar->instanceCount; i++) {
        int match = 1;
        variationInstance   *instance = &fvar->instances.array[i];
        unsigned short  axis;

        for (axis = 0; axis < axisCount; axis++) {
             if (userCoords[axis] != instance->coordinates.array[axis]) {
                match = 0;
                break;
            }
        }
        if (match) {
            *subfamilyID = instance->subfamilyNameID;
            *postscriptID = instance->postScriptNameID;
            return (int)i;
        }
    }
    return -1;
}

/* item variation store sub-table */

var_itemVariationStore var_loadItemVariationStore(ctlSharedStmCallbacks *sscb, unsigned long tableOffset, unsigned long tableLength, unsigned long ivsOffset)
{
    unsigned long regionListOffset;
    unsigned short ivdSubtableCount;
    var_itemVariationStore  ivs = NULL;
    unsigned short  i, axis;
    dnaDCL(unsigned long, ivdSubtablesOffsets);
    variationRegion *region;

    if (ivsOffset + IVS_SUBTABLE_HEADER_SIZE > tableLength) {
		sscb->message(sscb, "item variation store offset not within table range");
        return NULL;
    }

	sscb->seek(sscb, tableOffset + ivsOffset);

    /* load table header */
    if (sscb->read2(sscb) != ITEM_VARIATION_STORE_TABLE_FORMAT) {
		sscb->message(sscb, "invalid item variation store table format");
        return NULL;
    }
    regionListOffset = sscb->read4(sscb);
    ivdSubtableCount = sscb->read2(sscb);

    /* allocate and initialize item variation data store structure */
    ivs = sscb->memNew(sscb, sizeof(*ivs));
    if (ivs == NULL)
        return NULL;
    memset(ivs, 0, sizeof(*ivs));
    dnaINIT(sscb->dna, ivs->regionList.regions, 0, 1);
    dnaINIT(sscb->dna, ivs->dataList.ivdSubtables, 0, 1);

    /* load item variation data offsets into a temporary array */
    dnaINIT(sscb->dna, ivdSubtablesOffsets, ivdSubtableCount, 1);
    if ((dnaSetCnt(&ivs->dataList.ivdSubtables, DNA_ELEM_SIZE_(ivs->dataList.ivdSubtables), ivdSubtableCount) < 0)
        || (dnaSetCnt(&ivdSubtablesOffsets, DNA_ELEM_SIZE_(ivdSubtablesOffsets), ivdSubtableCount) < 0))
        goto cleanup;
    for (i = 0; i < ivdSubtableCount; i++) {
        ivdSubtablesOffsets.array[i] = sscb->read4(sscb);
    }

    /* load variation region list */
    if (ivsOffset + regionListOffset + IVS_VARIATION_REGION_LIST_HEADER_SIZE > tableLength) {
		sscb->message(sscb, "invalid item variation region offset");
        goto cleanup;
    }
    sscb->seek(sscb, tableOffset + ivsOffset + regionListOffset);
    ivs->regionList.axisCount = sscb->read2(sscb);
    ivs->regionList.regionCount = sscb->read2(sscb);
    if (dnaSetCnt(&ivs->regionList.regions, DNA_ELEM_SIZE_(ivs->regionList.regions), ivs->regionList.axisCount * ivs->regionList.regionCount) < 0)
        goto cleanup;

    if (ivsOffset + regionListOffset + IVS_VARIATION_REGION_LIST_HEADER_SIZE
        + REGION_AXIS_COORDINATES_SIZE * ivs->regionList.regions.cnt > tableLength) {
		sscb->message(sscb, "item variation region list out of bounds");
        goto cleanup;
    }

    region = &ivs->regionList.regions.array[0];
    for (i = 0; i < ivs->regionList.regionCount; i++) {
        for (axis = 0; axis < ivs->regionList.axisCount; axis++) {
            region->startCoord = (var_F2dot14)sscb->read2(sscb);
            region->peakCoord = (var_F2dot14)sscb->read2(sscb);
            region->endCoord = (var_F2dot14)sscb->read2(sscb);
            region++;
            
            /* TODO: coord range check? */
        }
    }

    /* load item variation data list */
    for (i = 0; i < ivdSubtableCount; i++) {
        unsigned long   ivdSubtablesOffset;
        unsigned short  shortDeltaCount;
        itemVariationDataSubtable  *ivd;
        unsigned short  r, t, j;

        ivdSubtablesOffset = ivdSubtablesOffsets.array[i];
        if (ivsOffset + ivdSubtablesOffset + ITEM_VARIATION_DATA_HEADER_SIZE > tableLength) {
            sscb->message(sscb, "item variation data offset out of bounds");
            goto cleanup;
        }

        /* load item variation data sub-table header */
        sscb->seek(sscb, tableOffset + ivsOffset + ivdSubtablesOffset);

        ivd = &ivs->dataList.ivdSubtables.array[i];
        ivd->itemCount = sscb->read2(sscb);
        shortDeltaCount = sscb->read2(sscb);
        ivd->regionCount = sscb->read2(sscb);

        dnaINIT(sscb->dna, ivd->regionIndices, ivd->regionCount, 1);
        dnaINIT(sscb->dna, ivd->deltaValues, ivd->itemCount * ivd->regionCount, 1);
        if ((dnaSetCnt(&ivd->regionIndices, DNA_ELEM_SIZE_(ivd->regionIndices), ivd->regionCount) < 0)
            || (dnaSetCnt(&ivd->deltaValues, DNA_ELEM_SIZE_(ivd->deltaValues), ivd->itemCount * ivd->regionCount) < 0))
            goto cleanup;

        /* load region indices */
        for (r = 0; r < ivd->regionCount; r++) {
            ivd->regionIndices.array[r] = (short)sscb->read2(sscb);
        }
        
        /* load two-dimensional delta values array */
        j = 0;
        for (t = 0; t < ivd->itemCount; t++) {
            for (r = 0; r < ivd->regionCount; r++) {
                ivd->deltaValues.array[j++] = (short)((r < shortDeltaCount)? sscb->read2(sscb): (char)sscb->read1(sscb));
            }
        }
    }

    dnaFREE(ivdSubtablesOffsets);

    return ivs;

cleanup:
    var_freeItemVariationStore(sscb, ivs);
    dnaFREE(ivdSubtablesOffsets);

    return NULL;
}

void var_freeItemVariationStore(ctlSharedStmCallbacks *sscb, var_itemVariationStore ivs)
{
    if (ivs) {
        short   i;
        
        for (i = 0; i < ivs->dataList.ivdSubtables.cnt; i++) {
            itemVariationDataSubtable  *ivd = &ivs->dataList.ivdSubtables.array[i];
            dnaFREE(ivd->regionIndices);
            dnaFREE(ivd->deltaValues);
        }
    
        dnaFREE(ivs->regionList.regions);
        dnaFREE(ivs->dataList.ivdSubtables);
        sscb->memFree(sscb, ivs);
    }
}

unsigned short var_getIVSRegionCount(var_itemVariationStore ivs)
{
    return (unsigned short)ivs->regionList.regionCount;
}

/* get the region count for a given variation store index */
unsigned short var_getIVSRegionCountForIndex(var_itemVariationStore ivs, unsigned short vsIndex)
{
    if (vsIndex >= ivs->dataList.ivdSubtables.cnt)
        return 0;

    return ivs->dataList.ivdSubtables.array[vsIndex].regionCount;
}

/* calculate scalars for all regions given a normalized design vector. */
void     var_calcRegionScalars(ctlSharedStmCallbacks *sscb, var_itemVariationStore ivs, unsigned short axisCount, var_F2dot14 *instCoords, float *scalars)
{
    variationRegionList *regionList = &ivs->regionList;
    long regionCount = regionList->regionCount;
    long i;
    unsigned short axis;

    if (axisCount != regionList->axisCount) {
        sscb->message(sscb, "invalid axis count in variation font region list");
        for (i = 0; i < regionCount; i++)
            scalars[i] = .0f;
        return;
    }

    for (i = 0; i < regionCount; i++) {
        float   scalar = 1.0f;

        for (axis = 0; axis < axisCount; axis++) {
            float   axisScalar;
            variationRegion *r = &regionList->regions.array[i * axisCount + axis];
        
            if (r->startCoord > r->peakCoord || r->peakCoord > r->endCoord)
                axisScalar = 1.0f;
            else if (r->startCoord < F2DOT14_ZERO && r->endCoord > F2DOT14_ZERO && r->peakCoord != F2DOT14_ZERO)
                axisScalar = 1.0f;
            else if (r->peakCoord == F2DOT14_ZERO)
                axisScalar = 1.0f;
            else if (instCoords[axis] < r->startCoord || instCoords[axis] > r->endCoord)
                axisScalar = .0f;
            else
            {
                if (instCoords[axis] == r->peakCoord)
                    axisScalar = 1.0f;
                else if (instCoords[axis] < r->peakCoord)
                    axisScalar = (float)(instCoords[axis] - r->startCoord) / (r->peakCoord - r->startCoord);
                else /* instCoords[axis] > r->peakCoord */
                    axisScalar = (float)(r->endCoord - instCoords[axis]) / (r->endCoord - r->peakCoord);
            }
            scalar = scalar * axisScalar;
        }

        scalars[i] = scalar;
    }
}

static int  loadIndexMap(ctlSharedStmCallbacks *sscb, sfrTable *table, unsigned long indexOffset, indexMap *ima)
{
    unsigned short  entryFormat;
    unsigned short  mapCount;
    unsigned short  entrySize;
    unsigned short  innerIndexEntryMask;
    unsigned short  outerIndexEntryShift;
    unsigned long   mapDataSize;
    unsigned short  i, j;

    if (indexOffset + DELTA_SET_INDEX_MAP_HEADER_SIZE > table->offset) {
		sscb->message(sscb, "invalid delta set index map table header");
        return 0;
    }

    sscb->seek(sscb, table->offset + indexOffset);
    entryFormat = sscb->read2(sscb);
    mapCount = sscb->read2(sscb);

    entrySize = ((entryFormat & MAP_ENTRY_SIZE_MASK) >> MAP_ENTRY_SIZE_SHIFT) + 1;
    innerIndexEntryMask = (1 << ((entryFormat & INNER_INDEX_BIT_COUNT_MASK) + 1)) - 1;
    outerIndexEntryShift = (entryFormat & INNER_INDEX_BIT_COUNT_MASK) + 1;

    mapDataSize = (unsigned long)mapCount * entrySize;

    if (mapCount == 0 || indexOffset + DELTA_SET_INDEX_MAP_HEADER_SIZE + mapDataSize > table->length) {
		sscb->message(sscb, "invalid delta set index map table size");
        return 0;
    }

    if (dnaSetCnt(ima, DNA_ELEM_SIZE_(*ima), mapCount) < 0)
        return 0;

    for (i = 0; i < mapCount; i++) {
        unsigned short  entry = 0;
        for (j = 0; j < entrySize; j++) {
            entry <<= 8;
            entry += sscb->read1(sscb);
        }
        ima->array[i].innerIndex = (entry & innerIndexEntryMask);
        ima->array[i].outerIndex = (entry >> outerIndexEntryShift);
    }

    return 1;
}

static void lookupIndexMap(indexMap *map, unsigned short gid, indexPair *index)
{
    if (map->cnt <= gid) {
        if (map->cnt == 0) {
            index->innerIndex = gid;
            index->outerIndex = 0;
        } else {
            *index = map->array[map->cnt-1];
        }
    } else {
        *index = map->array[gid];
    }
}

long var_getIVSRegionIndices(var_itemVariationStore ivs, unsigned short vsIndex, unsigned short *regionIndices, long regionCount)
{
    itemVariationDataSubtableList *dataList = &ivs->dataList;
    itemVariationDataSubtable   *subtable;
    long        i;

    if (vsIndex >= dataList->ivdSubtables.cnt)
        return 0;

    subtable = &dataList->ivdSubtables.array[vsIndex];

    if (regionCount < subtable->regionIndices.cnt)
        return 0;

    for (i = 0; i < subtable->regionIndices.cnt; i++) {
        regionIndices[i] = subtable->regionIndices.array[i];
    }

    return subtable->regionIndices.cnt;
}

static float var_applyDeltasForIndexPair(ctlSharedStmCallbacks *sscb, var_itemVariationStore ivs, indexPair *pair, float *scalars, long regionCount)
{
    itemVariationDataSubtableList *dataList = &ivs->dataList;
    float   netAdjustment = .0f;
    long        i, deltaSetIndex;
    itemVariationDataSubtable   *subtable;
    unsigned short  regionIndices[CFF2_MAX_MASTERS];
    long subRegionCount;

    if ((long)pair->outerIndex >= dataList->ivdSubtables.cnt) {
        sscb->message(sscb, "invalid outer index in index map");
        return netAdjustment;
    }

    subtable = &dataList->ivdSubtables.array[pair->outerIndex];
    if (subtable->regionCount > regionCount) {
        sscb->message(sscb, "out of range region count in item variation store subtable");
        return netAdjustment;
    }
    
    deltaSetIndex = subtable->regionCount * pair->innerIndex;
    
    if ((long)pair->innerIndex >= subtable->itemCount || deltaSetIndex + subtable->regionCount > subtable->deltaValues.cnt) {
        sscb->message(sscb, "invalid inner index in index map");
        return netAdjustment;
    }

    subRegionCount = var_getIVSRegionIndices(ivs, pair->outerIndex, regionIndices, subtable->regionCount);
    if (subRegionCount == 0) {
        sscb->message(sscb, "out of range region index found in item variation store subtable");
    }

    for (i = 0; i < subRegionCount; i++) {
        unsigned short  regionIndex = regionIndices[i];

        if (scalars[regionIndex])
            netAdjustment += scalars[regionIndex] * subtable->deltaValues.array[deltaSetIndex + regionIndex];
    }

    return netAdjustment;
}

static float   var_applyDeltasForGid(ctlSharedStmCallbacks *sscb, var_itemVariationStore ivs, indexMap *map, unsigned short gid, float *scalars, long regionCount)
{
    indexPair   pair;

    /* no adjustment if the index map table is missing */
    if (map->cnt == 0)
        return .0f;

    lookupIndexMap(map, gid, &pair);

    return var_applyDeltasForIndexPair(sscb, ivs, &pair, scalars, regionCount);
}

/* HVAR / vmtx tables */

var_hmtx   var_loadhmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb)
{
    var_hmtx   hmtx = NULL;
    sfrTable *table = NULL;
    unsigned long   ivsOffset;
    unsigned long   widthMapOffset;
    unsigned long   lsbMapOffset;
    unsigned long   rsbMapOffset;
    float  defaultWidth;
    unsigned short   i;
    long  numGlyphs;

    hmtx = sscb->memNew(sscb, sizeof(*hmtx));
    memset(hmtx, 0, sizeof(*hmtx));

    /* read hhea table */
	table = sfrGetTableByTag(sfr, HHEA_TABLE_TAG);
	if (table == NULL || table->length < HHEA_TABLE_HEADER_SIZE) {
		sscb->message(sscb, "invalid/missing hhea table");
		goto cleanup;
    }

	sscb->seek(sscb, table->offset);

    hmtx->header.version = (Fixed)sscb->read4(sscb);
    if (hmtx->header.version != HHEA_TABLE_VERSION) {
		sscb->message(sscb, "invalid hhea table version");
		goto cleanup;
    }

    hmtx->header.ascender = (short)sscb->read2(sscb);
    hmtx->header.descender = (short)sscb->read2(sscb);
    hmtx->header.lineGap = (short)sscb->read2(sscb);
    hmtx->header.advanceWidthMax = (unsigned short)sscb->read2(sscb);
    hmtx->header.minLeftSideBearing = (short)sscb->read2(sscb);
    hmtx->header.minRightSideBearing = (short)sscb->read2(sscb);
    hmtx->header.xMaxExtent = (short)sscb->read2(sscb);
    hmtx->header.caretSlopeRise = (short)sscb->read2(sscb);
    hmtx->header.caretSlopeRun = (short)sscb->read2(sscb);
    hmtx->header.caretOffset = (short)sscb->read2(sscb);
    for (i = 0; i < 4; i++) hmtx->header.reserved[i] = (short)sscb->read2(sscb);
    hmtx->header.metricDataFormat = (short)sscb->read2(sscb);
    hmtx->header.numberOfHMetrics = (unsigned short)sscb->read2(sscb);
    if (hmtx->header.numberOfHMetrics == 0) {
		sscb->message(sscb, "invalid numberOfHMetrics value in hhea table");
		goto cleanup;
    }

    /* read hmtx table */
	table = sfrGetTableByTag(sfr, HMTX_TABLE_TAG);
	if (table == NULL)
		goto cleanup;

    /* estimate the number of glphs from the table size instead of reading the head table */
    numGlyphs = (table->length / 2) - hmtx->header.numberOfHMetrics;
    if (numGlyphs < hmtx->header.numberOfHMetrics) {
		sscb->message(sscb, "invalid hmtx table size");
		goto cleanup;
    }

	sscb->seek(sscb, table->offset);

    dnaINIT(sscb->dna, hmtx->defaultMetrics, numGlyphs, 1);
    if (dnaSetCnt(&hmtx->defaultMetrics, DNA_ELEM_SIZE_(hmtx->defaultMetrics), numGlyphs) < 0)
        goto cleanup;
    for (i = 0; i < hmtx->header.numberOfHMetrics; i++) {
        hmtx->defaultMetrics.array[i].width = (unsigned short)sscb->read2(sscb);
        hmtx->defaultMetrics.array[i].sideBearing = (short)sscb->read2(sscb);
    }
    defaultWidth = hmtx->defaultMetrics.array[i-1].width;
    for (; i < numGlyphs; i++) {
        hmtx->defaultMetrics.array[i].width = defaultWidth;
        hmtx->defaultMetrics.array[i].sideBearing = (short)sscb->read2(sscb);
    }

	table = sfrGetTableByTag(sfr, HVAR_TABLE_TAG);
	if (table == NULL)  /* HVAR table is optional */
		return hmtx;

	sscb->seek(sscb, table->offset);

    if (table->length < HVAR_TABLE_HEADER_SIZE) {
		sscb->message(sscb, "invalid HVAR table size");
        goto cleanup;
    }

	/* Read and validate HVAR table version */
	if (sscb->read4(sscb) != HVAR_TABLE_VERSION) {
		sscb->message(sscb, "invalid HVAR table version");
        goto cleanup;
    }

    ivsOffset = sscb->read4(sscb);
    widthMapOffset = sscb->read4(sscb);
    lsbMapOffset = sscb->read4(sscb);
    rsbMapOffset = sscb->read4(sscb);

    if (ivsOffset == 0) {
		sscb->message(sscb, "item variation store offset in HVAR is NULL");
        goto cleanup;
    }

    hmtx->ivs = var_loadItemVariationStore(sscb, table->offset, table->length, ivsOffset);
    if (hmtx->ivs == NULL)
        goto cleanup;

    dnaINIT(sscb->dna, hmtx->widthMap, 0, 1);

    if (widthMapOffset) {
        if (!loadIndexMap(sscb, table, widthMapOffset, &hmtx->widthMap))
            goto cleanup;
    }

    if (lsbMapOffset) {
        if (!loadIndexMap(sscb, table, lsbMapOffset, &hmtx->lsbMap))
            goto cleanup;
    }

    if (rsbMapOffset) {
        if (!loadIndexMap(sscb, table, rsbMapOffset, &hmtx->rsbMap))
            goto cleanup;
    }

    return hmtx;

cleanup:
    var_freehmtx(sscb, hmtx);

    return NULL;
}

void var_freehmtx(ctlSharedStmCallbacks *sscb, var_hmtx hmtx)
{
    if (hmtx) {
        dnaFREE(hmtx->defaultMetrics);
        var_freeItemVariationStore(sscb, hmtx->ivs);
        dnaFREE(hmtx->widthMap);
        dnaFREE(hmtx->lsbMap);
        dnaFREE(hmtx->rsbMap);

        sscb->memFree(sscb, hmtx);
    }
}

int var_lookuphmtx(ctlSharedStmCallbacks *sscb, var_hmtx hmtx, unsigned short axisCount, float *scalars, unsigned short gid, var_glyphMetrics *metrics)
{
    if (!hmtx) {
		sscb->message(sscb, "invalid HVAR table data");
        return 1;
    }

    if (gid >= hmtx->defaultMetrics.cnt) {
		sscb->message(sscb, "var_lookuphmtx: invalid glyph ID");
        return 1;
    }

    *metrics = hmtx->defaultMetrics.array[gid];

    /* modify the default metrics if the font has variable font tables */
    if (hmtx->ivs && scalars && (axisCount > 0)) {
        long        regionCount = hmtx->ivs->regionList.regionCount;

        metrics->width += var_applyDeltasForGid(sscb, hmtx->ivs, &hmtx->widthMap, gid, scalars, regionCount);
        metrics->sideBearing += var_applyDeltasForGid(sscb, hmtx->ivs, &hmtx->lsbMap, gid, scalars, regionCount);
    }

    return 0;
}

var_vmtx   var_loadvmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb)
{
    var_vmtx   vmtx = NULL;
    sfrTable *table = NULL;
    unsigned long   ivsOffset;
    unsigned long   widthMapOffset;
    unsigned long   tsbMapOffset;
    unsigned long   bsbMapOffset;
    unsigned long   vorgMapOffset;
    float   defaultWidth;
    unsigned short  i;
    long    numGlyphs;

    vmtx = sscb->memNew(sscb, sizeof(*vmtx));
    memset(vmtx, 0, sizeof(*vmtx));

    /* read hhea table */
	table = sfrGetTableByTag(sfr, VHEA_TABLE_TAG);
	if (table == NULL || table->length < VHEA_TABLE_HEADER_SIZE) {
		sscb->message(sscb, "invalid/missing vhea table");
		goto cleanup;
    }

	sscb->seek(sscb, table->offset);

    vmtx->header.version = (Fixed)sscb->read4(sscb);
    if (vmtx->header.version != VHEA_TABLE_VERSION && vmtx->header.version != VHEA_TABLE_VERSION_1_1) {
		sscb->message(sscb, "invalid hhea table version");
		goto cleanup;
    }

    vmtx->header.vertTypoAscender = (short)sscb->read2(sscb);
    vmtx->header.vertTypoDescender = (short)sscb->read2(sscb);
    vmtx->header.vertTypoLineGap = (short)sscb->read2(sscb);
    vmtx->header.advanceHeightMax = (unsigned short)sscb->read2(sscb);
    vmtx->header.minTop = (short)sscb->read2(sscb);
    vmtx->header.minBottom = (short)sscb->read2(sscb);
    vmtx->header.caretSlopeRise = (short)sscb->read2(sscb);
    vmtx->header.caretSlopeRun = (short)sscb->read2(sscb);
    vmtx->header.caretOffset = (short)sscb->read2(sscb);
    for (i = 0; i < 4; i++) vmtx->header.reserved[i] = (short)sscb->read2(sscb);
    vmtx->header.metricDataFormat = (short)sscb->read2(sscb);
    vmtx->header.numOfLongVertMetrics = (unsigned short)sscb->read2(sscb);
    if (vmtx->header.numOfLongVertMetrics == 0) {
		sscb->message(sscb, "invalid numOfLongVertMetrics value in vhea table");
		goto cleanup;
    }

    /* read vmtx table */
	table = sfrGetTableByTag(sfr, VMTX_TABLE_TAG);
	if (table == NULL)
		goto cleanup;

    /* estimate the number of glphs from the table size instead of reading the head table */
    numGlyphs = (table->length / 2) - vmtx->header.numOfLongVertMetrics;
    if (numGlyphs < vmtx->header.numOfLongVertMetrics) {
		sscb->message(sscb, "invalid vmtx table size");
		goto cleanup;
    }

	sscb->seek(sscb, table->offset);

    if (dnaSetCnt(&vmtx->defaultMetrics, DNA_ELEM_SIZE_(vmtx->defaultMetrics), numGlyphs) < 0)
        goto cleanup;
    for (i = 0; i < vmtx->header.numOfLongVertMetrics; i++) {
        vmtx->defaultMetrics.array[i].width = (unsigned short)sscb->read2(sscb);
        vmtx->defaultMetrics.array[i].sideBearing = (short)sscb->read2(sscb);
    }
    defaultWidth = vmtx->defaultMetrics.array[i-1].width;
    for (; i < numGlyphs; i++) {
        vmtx->defaultMetrics.array[i].width = defaultWidth;
        vmtx->defaultMetrics.array[i].sideBearing = (short)sscb->read2(sscb);
    }

    /* read optional VORG table */
    dnaINIT(sscb->dna, vmtx->vertOriginY, 0, 1);

	table = sfrGetTableByTag(sfr, VORG_TABLE_TAG);
	if (table != NULL) {
        short   defaultVertOriginY;
        unsigned short  numVertOriginYMetrics;
        
        sscb->seek(sscb, table->offset);

        if (dnaSetCnt(&vmtx->vertOriginY, DNA_ELEM_SIZE_(vmtx->vertOriginY), numGlyphs) < 0)
            goto cleanup;

        if (table->length < VORG_TABLE_HEADER_SIZE) {
            sscb->message(sscb, "invalid VVAR table size");
            goto cleanup;
        }

        if (sscb->read4(sscb) != VORG_TABLE_VERSION) {
            sscb->message(sscb, "invalid VORG table version");
            goto cleanup;
        }

        defaultVertOriginY = (short)sscb->read2(sscb);
        numVertOriginYMetrics = (unsigned short)sscb->read2(sscb);
        if (table->length < (unsigned long)(VORG_TABLE_HEADER_SIZE + 4 * numVertOriginYMetrics)) {
            sscb->message(sscb, "invalid VORG table size");
            goto cleanup;
        }

        for (i = 0; i < numGlyphs; i++) {
            vmtx->vertOriginY.array[i] = defaultVertOriginY;
        }

        for (i = 0; i < numVertOriginYMetrics; i++) {
            unsigned short  glyphIndex = (unsigned short)sscb->read2(sscb);
            short   vertOriginY = (short)sscb->read2(sscb);

            if (glyphIndex >= numGlyphs) {
                sscb->message(sscb, "invalid glyph index in VORG table");
                goto cleanup;
            }
            vmtx->vertOriginY.array[glyphIndex] = vertOriginY;
        }
    }

	table = sfrGetTableByTag(sfr, VVAR_TABLE_TAG);
	if (table == NULL)  /* VVAR table is optional */
		return vmtx;

	sscb->seek(sscb, table->offset);

	/* Read and validate HVAR/VVAR table version */
    if (table->length < VVAR_TABLE_HEADER_SIZE) {
		sscb->message(sscb, "invalid VVAR table size");
        goto cleanup;
    }

	if (sscb->read4(sscb) != VVAR_TABLE_VERSION) {
		sscb->message(sscb, "invalid VVAR table version");
        goto cleanup;
    }

    ivsOffset = sscb->read4(sscb);
    widthMapOffset = sscb->read4(sscb);
    tsbMapOffset = sscb->read4(sscb);
    bsbMapOffset = sscb->read4(sscb);
    vorgMapOffset = sscb->read4(sscb);

    if (ivsOffset == 0) {
		sscb->message(sscb, "item variation store offset in VVAR is NULL");
        goto cleanup;
    }

    vmtx->ivs = var_loadItemVariationStore(sscb, table->offset, table->length, ivsOffset);
    if (vmtx->ivs == NULL)
        goto cleanup;

    dnaINIT(sscb->dna, vmtx->widthMap, 0, 1);
    dnaINIT(sscb->dna, vmtx->vorgMap, 0, 1);

    if (widthMapOffset) {
        if (!loadIndexMap(sscb, table, widthMapOffset, &vmtx->widthMap))
            goto cleanup;
    }

    if (tsbMapOffset) {
        if (!loadIndexMap(sscb, table, tsbMapOffset, &vmtx->tsbMap))
            goto cleanup;
    }

    if (bsbMapOffset) {
        if (!loadIndexMap(sscb, table, bsbMapOffset, &vmtx->bsbMap))
            goto cleanup;
    }

    if (vorgMapOffset) {
        if (!loadIndexMap(sscb, table, vorgMapOffset, &vmtx->vorgMap))
            goto cleanup;
    }

    return vmtx;

cleanup:
    var_freevmtx(sscb, vmtx);

    return NULL;
}

void var_freevmtx(ctlSharedStmCallbacks *sscb, var_vmtx vmtx)
{
    if (vmtx) {
        dnaFREE(vmtx->defaultMetrics);
        dnaFREE(vmtx->vertOriginY);
        var_freeItemVariationStore(sscb, vmtx->ivs);
        dnaFREE(vmtx->widthMap);
        dnaFREE(vmtx->tsbMap);
        dnaFREE(vmtx->bsbMap);
        dnaFREE(vmtx->vorgMap);

        sscb->memFree(sscb, vmtx);
    }
}

int var_lookupvmtx(ctlSharedStmCallbacks *sscb, var_vmtx vmtx, unsigned short axisCount, float *scalars, unsigned short gid, var_glyphMetrics *metrics)
{
    if (!vmtx) {
		sscb->message(sscb, "invalid VVAR table data");
        return 1;
    }

    if (gid >= vmtx->defaultMetrics.cnt) {
		sscb->message(sscb, "var_lookupvmtx: invalid glyph ID");
        return 1;
    }

    *metrics = vmtx->defaultMetrics.array[gid];

    /* modify the default metrics if the font has variable font tables */
    if (vmtx->ivs && scalars && (axisCount > 0)) {
        long        regionCount = vmtx->ivs->regionList.regionCount;

        metrics->width += var_applyDeltasForGid(sscb, vmtx->ivs, &vmtx->widthMap, gid, scalars, regionCount);
        metrics->sideBearing += var_applyDeltasForGid(sscb, vmtx->ivs, &vmtx->tsbMap, gid, scalars, regionCount);
    }

    return 0;
}

/* MVAR table */

var_MVAR   var_loadMVAR(sfrCtx sfr, ctlSharedStmCallbacks *sscb)
{
    var_MVAR   mvar = NULL;
    unsigned long   ivsOffset;
    unsigned short  valueRecordSize;
    unsigned short  i;

	sfrTable *table = sfrGetTableByTag(sfr, MVAR_TABLE_TAG);
	if (table == NULL)
		return NULL;

	sscb->seek(sscb, table->offset);

	/* Read and validate table version */
    if (table->length < MVAR_TABLE_HEADER_SIZE) {
		sscb->message(sscb, "invalid MVAR table size");
        return NULL;
    }
    
	if (sscb->read4(sscb) != MVAR_TABLE_VERSION) {
		sscb->message(sscb, "invalid MVAR table version");
        return NULL;
    }

    mvar = sscb->memNew(sscb, sizeof(*mvar));
    memset(mvar, 0, sizeof(*mvar));

    mvar->axisCount = sscb->read2(sscb);
    valueRecordSize	= sscb->read2(sscb);
    mvar->valueRecordCount = sscb->read2(sscb);
    ivsOffset = (unsigned long)sscb->read2(sscb);

    if (ivsOffset == 0) {
		sscb->message(sscb, "item variation store offset in MVAR is NULL");
        return NULL;
    }

    if (valueRecordSize < MVAR_TABLE_RECORD_SIZE
        || table->length < MVAR_TABLE_HEADER_SIZE + (unsigned long)valueRecordSize * mvar->valueRecordCount) {
		sscb->message(sscb, "invalid MVAR record size or table size");
        return NULL;
    }
    
    dnaINIT(sscb->dna, mvar->values, 0, 1);
    if (dnaSetCnt(&mvar->values, DNA_ELEM_SIZE_(mvar->values), mvar->valueRecordCount) < 0)
        goto cleanup;

    for (i = 0; i < mvar->valueRecordCount; i++) {
        unsigned short   j;

        mvar->values.array[i].valueTag = sscb->read4(sscb);
        mvar->values.array[i].pair.outerIndex = sscb->read2(sscb);
        mvar->values.array[i].pair.innerIndex = sscb->read2(sscb);
        for (j = MVAR_TABLE_RECORD_SIZE; j < valueRecordSize; j++)
            (void)sscb->read1(sscb);
    }

    mvar->ivs = var_loadItemVariationStore(sscb, table->offset, table->length, ivsOffset);
    if (mvar->ivs == NULL)
        goto cleanup;

    return mvar;

cleanup:
    var_freeMVAR(sscb, mvar);

    return NULL;
}

int var_lookupMVAR(ctlSharedStmCallbacks *sscb, var_MVAR mvar, unsigned short axisCount, float *scalars, ctlTag tag, float *value)
{
    long    top, bot, index;
    mvarValueRecord *rec = NULL;
    int found = 0;

    if (!mvar || !mvar->ivs) {
		sscb->message(sscb, "invalid MVAR table data");
        return 1;
    }

    if (scalars == 0 || axisCount == 0) {
		sscb->message(sscb, "zero scalars/axis count specified for MVAR");
        return 1;
    }

    bot = 0;
    top = (long)mvar->valueRecordCount - 1;
    while (bot <= top) {
        index = (top + bot) / 2;
        rec = &mvar->values.array[index];
        if (rec->valueTag == tag)
        {
            found = 1;
            break;
        }
        if (tag < rec->valueTag)
            top = index - 1;
        else
            bot = index + 1;
    }

    if (!found) {
        /* Specified tag was not found. */
        return 1;
    }

    /* Blend the metric value using the IVS table */
    *value = var_applyDeltasForIndexPair(sscb, mvar->ivs, &rec->pair, scalars, mvar->ivs->regionList.regionCount);

    return 0;
}

void var_freeMVAR(ctlSharedStmCallbacks *sscb, var_MVAR mvar)
{
    if (mvar) {
        var_freeItemVariationStore(sscb, mvar->ivs);
        dnaFREE(mvar->values);

        sscb->memFree(sscb, mvar);
    }
}

