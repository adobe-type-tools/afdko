/* Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "varsupport.h"

#include <string.h>

#include "ctlshare.h"
#include "ctutil.h"
#include "cffread_abs.h"
#include "dynarr.h"
#include "supportfp.h"
#include "txops.h"
#include "supportexcept.h"

/* ----------------------------- fixed number constants, types, macros ---------------------------- */

#define FIXED_ZERO 0
#define FIXED_ONE 0x00010000
#define FIXED_MINUS_ONE (-FIXED_ONE)

/* ----------------------------- variation font table constants ---------------------------- */

#define HHEA_TABLE_TAG CTL_TAG('h', 'h', 'e', 'a')
#define VHEA_TABLE_TAG CTL_TAG('v', 'h', 'e', 'a')
#define HMTX_TABLE_TAG CTL_TAG('h', 'm', 't', 'x')
#define VMTX_TABLE_TAG CTL_TAG('v', 'm', 't', 'x')
#define VORG_TABLE_TAG CTL_TAG('V', 'O', 'R', 'G')
#define AVAR_TABLE_TAG CTL_TAG('a', 'v', 'a', 'r')
#define FVAR_TABLE_TAG CTL_TAG('f', 'v', 'a', 'r')
#define HVAR_TABLE_TAG CTL_TAG('H', 'V', 'A', 'R')
#define VVAR_TABLE_TAG CTL_TAG('V', 'V', 'A', 'R')
#define MVAR_TABLE_TAG CTL_TAG('M', 'V', 'A', 'R')

#define HHEA_TABLE_VERSION 0x00010000
#define VHEA_TABLE_VERSION 0x00010000
#define VHEA_TABLE_VERSION_1_1 0x00011000
#define VORG_TABLE_VERSION 0x00010000
#define AVAR_TABLE_VERSION 0x00010000
#define FVAR_TABLE_VERSION 0x00010000
#define HVAR_TABLE_VERSION 0x00010000
#define VVAR_TABLE_VERSION 0x00010000
#define MVAR_TABLE_VERSION 0x00010000

#define HHEA_TABLE_HEADER_SIZE 36
#define VHEA_TABLE_HEADER_SIZE 36
#define VORG_TABLE_HEADER_SIZE 8
#define AVAR_TABLE_HEADER_SIZE 6
#define FVAR_TABLE_HEADER_SIZE 16
#define HVAR_TABLE_HEADER_SIZE 20
#define VVAR_TABLE_HEADER_SIZE 24
#define MVAR_TABLE_HEADER_SIZE 12
#define MVAR_TABLE_RECORD_SIZE 8

#define AVAR_SEGMENT_MAP_SIZE (2 + 4 * 3)
#define AVAR_AXIS_VALUE_MAP_SIZE 4

#define FVAR_OFFSET_TO_AXES_ARRAY 16
#define FVAR_COUNT_SIZE_PAIRS 2
#define FVAR_AXIS_SIZE 20
#define FVAR_INSTANCE_SIZE 4
#define FVAR_INSTANCE_WITH_NAME_SIZE 6

#define ITEM_VARIATION_STORE_TABLE_FORMAT 1
#define IVS_SUBTABLE_HEADER_SIZE 12
#define IVS_VARIATION_REGION_LIST_HEADER_SIZE 4

#define REGION_AXIS_COORDINATES_SIZE (2 * 3)
#define ITEM_VARIATION_DATA_HEADER_SIZE (2 * 3)

#define DELTA_SET_INDEX_MAP_HEADER_SIZE (2 * 2)

#define INNER_INDEX_BIT_COUNT_MASK 0x000F
#define MAP_ENTRY_SIZE_MASK 0x0030
#define MAP_ENTRY_SIZE_SHIFT 4

/* avar table */

bool var_axes::load_avar(sfrCtx sfr, ctlSharedStmCallbacks *sscb) {
    bool success = true;
    uint16_t i;

    sfrTable *table = sfrGetTableByTag(sfr, AVAR_TABLE_TAG);
    if (table == NULL)
        return true;

    sscb->seek(sscb, table->offset);

    /* Read and validate table version */
    if (sscb->read4(sscb) != AVAR_TABLE_VERSION) {
        sscb->message(sscb, "invalid avar table version");
        return false;
    }

    if (table->length < AVAR_TABLE_HEADER_SIZE) {
        sscb->message(sscb, "invalid avar table size");
        return false;
    }

    i = sscb->read2(sscb); /* skip reserved short */
    avarAxisCount = sscb->read2(sscb);

    if (table->length < AVAR_TABLE_HEADER_SIZE +
                        (uint32_t)AVAR_SEGMENT_MAP_SIZE * avarAxisCount) {
        sscb->message(sscb, "invalid avar table size or axis/instance count/size");
        return false;
    }

    for (i = 0; i < avarAxisCount; i++) {
        segmentMap seg;
        unsigned short j;
        bool hasZeroMap {false};
        uint16_t positionMapCount = sscb->read2(sscb);
        if (table->length < sscb->tell(sscb) - table->offset +
                            AVAR_AXIS_VALUE_MAP_SIZE * positionMapCount) {
            sscb->message(sscb, "avar axis value map out of bounds");
            success = false;
            break;
        }

        for (j = 0; j < positionMapCount; j++) {
            Fixed fromCoord = F2DOT14_TO_FIXED((var_F2dot14)sscb->read2(sscb));
            Fixed toCoord = F2DOT14_TO_FIXED((var_F2dot14)sscb->read2(sscb));

            if (j > 0 && j < positionMapCount - 1 && fromCoord == 0 && toCoord == 0) {
                hasZeroMap = 1;
            }
            seg.valueMaps.emplace_back(fromCoord, toCoord);
        }
        if (positionMapCount < 3
            || seg.valueMaps.front().fromCoord != FIXED_MINUS_ONE
            || seg.valueMaps.front().toCoord != FIXED_MINUS_ONE
            || !hasZeroMap
            || seg.valueMaps.back().fromCoord != FIXED_ONE
            || seg.valueMaps.back().toCoord != FIXED_ONE) {
            // incomplete value maps: invalidate the maps entirely for this axis
            seg.valueMaps.clear();
        }
        segmentMaps.push_back(std::move(seg));
    }
    if (!success)
        segmentMaps.clear();
    return success;
}

bool var_axes::load_fvar(sfrCtx sfr, ctlSharedStmCallbacks *sscb) {
    bool success = true;
    uint16_t offsetToAxesArray;
    uint16_t countSizePairs;
    uint16_t axisSize;
    uint16_t instanceSize;
    uint16_t axisCount;
    uint16_t instanceCount;
    uint16_t i;

    sfrTable *table = sfrGetTableByTag(sfr, FVAR_TABLE_TAG);
    if (table == NULL)
        return false;

    sscb->seek(sscb, table->offset);

    /* Read and validate table version */
    if (sscb->read4(sscb) != FVAR_TABLE_VERSION) {
        sscb->message(sscb, "invalid fvar table version");
        return false;
    }

    if (table->length < FVAR_TABLE_HEADER_SIZE) {
        sscb->message(sscb, "invalid fvar table size");
        return false;
    }

    offsetToAxesArray = sscb->read2(sscb);
    countSizePairs = sscb->read2(sscb);
    axisCount = sscb->read2(sscb);
    axisSize = sscb->read2(sscb);
    instanceCount = sscb->read2(sscb);
    instanceSize = sscb->read2(sscb);

    /* sanity check of values */
    if (offsetToAxesArray < FVAR_OFFSET_TO_AXES_ARRAY
        || countSizePairs < FVAR_COUNT_SIZE_PAIRS
        || axisSize < FVAR_AXIS_SIZE) {
        sscb->message(sscb, "invalid values in fvar table header");
        return false;
    }

    if (table->length < offsetToAxesArray + (unsigned long)axisSize * axisCount + (unsigned long)instanceSize * instanceCount
        || instanceSize < FVAR_INSTANCE_SIZE + 4 * axisCount) {
        sscb->message(sscb, "invalid fvar table size or axis/instance count/size");
        return false;
    }

    sscb->seek(sscb, table->offset + offsetToAxesArray);

    for (i = 0; i < axisCount; i++) {
        variationAxis axis;
        axis.tag = sscb->read4(sscb);
        axis.minValue = (Fixed)sscb->read4(sscb);
        axis.defaultValue = (Fixed)sscb->read4(sscb);
        axis.maxValue = (Fixed)sscb->read4(sscb);
        axis.flags = sscb->read2(sscb);
        axis.nameID = sscb->read2(sscb);
        axes.push_back(axis);
    }

    for (i = 0; i < instanceCount; i++) {
        variationInstance inst;
        inst.subfamilyNameID = sscb->read2(sscb);
        inst.flags = sscb->read2(sscb);
        for (uint16_t j = 0; j < axisCount; j++) {
            float t;
            fixtopflt((Fixed)sscb->read4(sscb), &t);
            inst.coordinates.push_back(t);
        }
        if (instanceSize >= FVAR_INSTANCE_WITH_NAME_SIZE + 4 * axisCount)
            inst.postScriptNameID = sscb->read2(sscb);
        else
            inst.postScriptNameID = 0; /* indicating an unspecified PostScript name ID */
        instances.push_back(std::move(inst));
    }

    if (!success) {
        axes.clear();
        instances.clear();
    }
    return success;
}

/* load font axis tables */
var_axes::var_axes(sfrCtx sfr, ctlSharedStmCallbacks *sscb) {
    if (!load_fvar(sfr, sscb))
        return;
    if (!load_avar(sfr, sscb)) {
        sscb->message(sscb, "Could not load avar table");
    } else if (segmentMaps.size() > 0 && avarAxisCount != axes.size()) {
        sscb->message(sscb, "mismatching axis counts in fvar and avar");
        segmentMaps.clear();
    }
}

bool var_axes::getAxis(uint16_t index, ctlTag *tag, Fixed *minValue,
                       Fixed *defaultValue, Fixed *maxValue, uint16_t *flags) {
    if (index < axes.size()) {
        variationAxis &axis = axes[index];
        if (tag)
            *tag = axis.tag;
        if (minValue)
            *minValue = axis.minValue;
        if (defaultValue)
            *defaultValue = axis.defaultValue;
        if (maxValue)
            *maxValue = axis.maxValue;
        if (flags)
            *flags = axis.flags;
        return true;
    } else
        return false;
}

int16_t var_axes::getAxisIndex(ctlTag tag) {
    int i = 0;
    for (auto &a : axes) {
        if (a.tag == tag)
            return i;
        i++;
    }
    return -1;
}

Fixed var_axes::defaultNormalizeAxis(const variationAxis &axis,
                                            Fixed userValue) {
    if (userValue < axis.defaultValue) {
        if (userValue < axis.minValue)
            return FIXED_MINUS_ONE;

        return fixdiv(-(axis.defaultValue - userValue),
                      axis.defaultValue - axis.minValue);
    } else if (userValue > axis.defaultValue) {
        if (userValue > axis.maxValue)
            return FIXED_ONE;

        return fixdiv(userValue - axis.defaultValue,
                       axis.maxValue - axis.defaultValue);
    } else
        return FIXED_ZERO;
}

Fixed var_axes::applySegmentMap(const segmentMap &seg, Fixed value) {
    long i;
    Fixed endFromVal, endToVal, startFromVal, startToVal;

    if (seg.valueMaps.size() <= 0)
        return value;

    for (i = 0; i < seg.valueMaps.size(); i++) {
        if (value < seg.valueMaps[i].fromCoord)
            break;
    }

    if (i <= 0) /* value is at min axis value */
        return seg.valueMaps.front().toCoord;

    if (i >= seg.valueMaps.size()) /* value is at max axis value */
        return seg.valueMaps.back().toCoord;

    endFromVal = seg.valueMaps[i].fromCoord;
    endToVal = seg.valueMaps[i].toCoord;

    if (value == endFromVal)
        return endToVal;

    startFromVal = seg.valueMaps[i-1].fromCoord;
    startToVal = seg.valueMaps[i-1].toCoord;

    return startToVal + fixmul(endToVal - startToVal,
                               fixdiv(value - startFromVal,
                                      endFromVal - startFromVal));
}

bool var_axes::normalizeCoord(uint16_t index, Fixed userCoord, Fixed &normCoord) {
    if (index >= axes.size())
        return false;

    normCoord = defaultNormalizeAxis(axes[index], userCoord);

    if (index >= segmentMaps.size())
        return true;

    segmentMap &seg = segmentMaps[index];
    if (seg.valueMaps.size() > 0)

        normCoord = applySegmentMap(seg, normCoord);
    return true;
}

bool var_axes::normalizeCoords(ctlSharedStmCallbacks *sscb, Fixed *userCoords, Fixed *normCoords) {
    if (axes.size() < 1) {
        sscb->message(sscb, "var_normalizeCoords: invalid axis table");
        return false;
    }

    for (size_t i = 0; i < axes.size(); i++) {
        if (!normalizeCoord(i, userCoords[i], normCoords[i])) {
            sscb->message(sscb, "var_normalizeCoords: invalid axis %d", i);
            return false;
        }
    }
    return true;
}

/* get a named instance */
int var_axes::findInstance(float *userCoords, uint16_t axisCount,
                           uint16_t &subfamilyID, uint16_t &postscriptID) {
    if (axisCount != axes.size())
        return -1;

    for (size_t i = 0; i < instances.size(); i++) {
        bool match = true;
        for (uint16_t axis = 0; axis < axisCount; axis++) {
            if (userCoords[axis] != instances[i].coordinates[axis]) {
                match = false;
                break;
            }
        }
        if (match) {
            subfamilyID = instances[i].subfamilyNameID;
            postscriptID = instances[i].postScriptNameID;
            return i;
        }
    }
    return -1;
}

/* item variation store sub-table */

itemVariationStore::itemVariationStore(ctlSharedStmCallbacks *sscb,
                                       uint32_t tableOffset,
                                       uint32_t tableLength,
                                       uint32_t ivsOffset) {
    uint32_t regionListOffset;
    uint16_t ivdSubtableCount;
    uint16_t i, axis;
    std::vector<uint32_t> ivdSubtablesOffsets;

    if (ivsOffset + IVS_SUBTABLE_HEADER_SIZE > tableLength) {
        sscb->message(sscb, "item variation store offset not within table range");
        return;
    }

    sscb->seek(sscb, tableOffset + ivsOffset);

    /* load table header */
    if (sscb->read2(sscb) != ITEM_VARIATION_STORE_TABLE_FORMAT) {
        sscb->message(sscb, "invalid item variation store table format");
        return;
    }
    regionListOffset = sscb->read4(sscb);
    ivdSubtableCount = sscb->read2(sscb);

    for (i = 0; i < ivdSubtableCount; i++)
        ivdSubtablesOffsets.push_back(sscb->read4(sscb));

    /* load variation region list */
    if (ivsOffset + regionListOffset + IVS_VARIATION_REGION_LIST_HEADER_SIZE > tableLength) {
        sscb->message(sscb, "invalid item variation region offset");
        return;
    }
    sscb->seek(sscb, tableOffset + ivsOffset + regionListOffset);

    axisCount = sscb->read2(sscb);

    if (axisCount > CFF2_MAX_AXES) {
        sscb->message(sscb, "invalid axis count in item variation region list");
        reset();
        return;
    }

    uint16_t regionCount = sscb->read2(sscb);

    /* cff2.scalars and cff2.regionIndices have size CFF2_MAX_MASTERS,
       so regionList.regionCount should not exceed CFF2_MAX_MASTERS */
    if (regionCount > CFF2_MAX_MASTERS) {
        sscb->message(sscb, "invalid region count in item variation region list");
        return;
    }

    if (ivsOffset + regionListOffset + IVS_VARIATION_REGION_LIST_HEADER_SIZE
        + REGION_AXIS_COORDINATES_SIZE * regionCount > tableLength) {
        sscb->message(sscb, "item variation region list out of bounds");
        return;
    }

    for (i = 0; i < regionCount; i++) {
        VariationRegion vrv;
        for (axis = 0; axis < axisCount; axis++) {
            Fixed start = F2DOT14_TO_FIXED((var_F2dot14)sscb->read2(sscb));
            Fixed peak = F2DOT14_TO_FIXED((var_F2dot14)sscb->read2(sscb));
            Fixed end = F2DOT14_TO_FIXED((var_F2dot14)sscb->read2(sscb));
            vrv.push_back(std::make_tuple(start, peak, end));
        }
        regionMap.emplace(vrv, (uint32_t) regions.size());
        regions.emplace_back(std::move(vrv));
    }

    /* load item variation data list */
    for (i = 0; i < ivdSubtableCount; i++) {
        uint32_t ivdSubtablesOffset;
        uint16_t itemCount;
        uint16_t shortDeltaCount;
        uint16_t subtableRegionCount;
        itemVariationDataSubtable ivd;
        uint16_t r, t, j;

        ivdSubtablesOffset = ivdSubtablesOffsets[i];
        if (ivsOffset + ivdSubtablesOffset + ITEM_VARIATION_DATA_HEADER_SIZE > tableLength) {
            sscb->message(sscb, "item variation data offset out of bounds");
            reset();
            return;
        }

        /* load item variation data sub-table header */
        sscb->seek(sscb, tableOffset + ivsOffset + ivdSubtablesOffset);

        itemCount = sscb->read2(sscb);
        shortDeltaCount = sscb->read2(sscb);
        subtableRegionCount = sscb->read2(sscb);
        if (subtableRegionCount > CFF2_MAX_MASTERS) {
            sscb->message(sscb, "item variation data: too many regions");
            reset();
            return;
        }

        /* load region indices */
        for (r = 0; r < subtableRegionCount; r++)
            ivd.regionIndices.push_back((int16_t)sscb->read2(sscb));

        /* load two-dimensional delta values array */
        j = 0;
        for (t = 0; t < itemCount; t++) {
            std::vector<int16_t> dvv;
            for (r = 0; r < subtableRegionCount; r++) {
                dvv.push_back((int16_t)((r < shortDeltaCount) ? sscb->read2(sscb) : (char)sscb->read1(sscb)));
            }
            ivd.deltaValues.emplace_back(std::move(dvv));
        }
        subtables.push_back(std::move(ivd));
    }
    maxUnbuiltSubtable = subtables.size() - 1;
}

int32_t itemVariationStore::getRegionIndices(uint16_t vsIndex,
                                             uint16_t *regionIndices,
                                             int32_t regionListCount) {
    long i;

    if (vsIndex >= subtables.size())
        return 0;

    auto &subtable = subtables[vsIndex];

    if (regionListCount < subtable.regionIndices.size())
        return 0;

    for (i = 0; i < subtable.regionIndices.size(); i++) {
        uint16_t index = subtable.regionIndices[i];
        if (index >= regionListCount) {  // XXX why?
            return 0;
        }
        regionIndices[i] = index;
    }

    return subtable.regionIndices.size();
}

/* calculate scalars for all regions given a normalized design vector. */
void itemVariationStore::calcRegionScalars(ctlSharedStmCallbacks *sscb,
                                           uint16_t &fvarAxisCount,
                                           Fixed *instCoords, float *scalars) {
    int32_t i;

    if (fvarAxisCount != axisCount) {
        sscb->message(sscb, "axis count in variation font region list does not match axis count in fvar table");
        fvarAxisCount = axisCount;
        for (i = 0; i < regions.size(); i++)
            scalars[i] = .0f;
        return;
    }

    i = 0;
    for (auto &vr: regions) {
        float scalar = 1.0f;

        uint16_t axis {0};
        for (auto &ar: vr) {
            float axisScalar;
            Fixed startCoord = std::get<0>(ar);
            Fixed peakCoord = std::get<1>(ar);
            Fixed endCoord = std::get<2>(ar);
            if (startCoord > peakCoord || peakCoord > endCoord)
                axisScalar = 1.0f;
            else if (startCoord < FIXED_ZERO && endCoord > FIXED_ZERO && peakCoord != FIXED_ZERO)
                axisScalar = 1.0f;
            else if (peakCoord == FIXED_ZERO)
                axisScalar = 1.0f;
            else if (instCoords[axis] < startCoord || instCoords[axis] > endCoord)
                axisScalar = .0f;
            else {
                if (instCoords[axis] == peakCoord)
                    axisScalar = 1.0f;
                else if (instCoords[axis] < peakCoord)
                    axisScalar = (float)(instCoords[axis] - startCoord) / (peakCoord - startCoord);
                else /* instCoords[axis] > peakCoord */
                    axisScalar = (float)(endCoord - instCoords[axis]) / (endCoord - peakCoord);
            }
            scalar = scalar * axisScalar;
            axis++;
        }

        scalars[i++] = scalar;
    }
    return;
}

uint32_t itemVariationStore::addValue(VarValueRecord &vvr,
                                      std::shared_ptr<slogger> logger) {
    ensureBuilder();
    return builder->addValue(vvr, logger);
}

uint32_t itemVariationStore::Builder::addValue(VarValueRecord &vvr,
                                               std::shared_ptr<slogger> logger) {
    uint32_t index = values.size();
    values.emplace_back(vvr.getDefault());
    return index;
}

static bool loadIndexMap(ctlSharedStmCallbacks *sscb, sfrTable *table,
                         uint32_t indexOffset, var_indexMap &ima) {
    uint16_t entryFormat;
    uint16_t mapCount;
    uint16_t entrySize;
    uint16_t innerIndexEntryMask;
    uint16_t outerIndexEntryShift;
    uint32_t mapDataSize;
    uint16_t i, j;

    ima.offset = indexOffset;
    if (indexOffset == 0) /* No index map */
        return true;

    if (indexOffset + DELTA_SET_INDEX_MAP_HEADER_SIZE > table->offset) {
        sscb->message(sscb, "invalid delta set index map table header");
        return false;
    }

    sscb->seek(sscb, table->offset + indexOffset);
    entryFormat = sscb->read2(sscb);
    mapCount = sscb->read2(sscb);

    entrySize = ((entryFormat & MAP_ENTRY_SIZE_MASK) >> MAP_ENTRY_SIZE_SHIFT) + 1;
    innerIndexEntryMask = (1 << ((entryFormat & INNER_INDEX_BIT_COUNT_MASK) + 1)) - 1;
    outerIndexEntryShift = (entryFormat & INNER_INDEX_BIT_COUNT_MASK) + 1;

    mapDataSize = (uint32_t)mapCount * entrySize;

    if (mapCount == 0 || indexOffset + DELTA_SET_INDEX_MAP_HEADER_SIZE + mapDataSize > table->length) {
        sscb->message(sscb, "invalid delta set index map table size");
        return false;
    }

    for (i = 0; i < mapCount; i++) {
        uint16_t entry = 0;
        var_indexPair ip;
        for (j = 0; j < entrySize; j++) {
            entry <<= 8;
            entry += sscb->read1(sscb);
        }
        ip.innerIndex = (entry & innerIndexEntryMask);
        ip.outerIndex = (entry >> outerIndexEntryShift);
        ima.map.push_back(ip);
    }

    return true;
}

static void lookupIndexMap(const var_indexMap &map, uint16_t gid,
                           var_indexPair &index) {
    if (map.map.size() <= gid) {
        if (map.map.size() == 0) {
            index.innerIndex = gid;
            index.outerIndex = 0;
        } else {
            index = map.map.back();
        }
    } else {
        index = map.map[gid];
    }
}

float itemVariationStore::applyDeltasForIndexPair(ctlSharedStmCallbacks *sscb,
                                                  const var_indexPair &pair,
                                                  float *scalars,
                                                  int32_t regionListCount) {
    float netAdjustment = .0f;

    if (pair.outerIndex >= subtables.size()) {
        sscb->message(sscb, "invalid outer index in index map");
        return netAdjustment;
    }

    auto &subtable = subtables[pair.outerIndex];

    /* If specific glyphs do not have any variation, they may be
     referenced by an ivdSubtable with a region count of 0. This is
     valid.
     */
    int32_t srSize = subtable.regionIndices.size();
    if (srSize == 0)
        return netAdjustment;

    if (srSize > regionListCount) {
        sscb->message(sscb, "out of range region count in item variation store subtable");
        return netAdjustment;
    }

    if (pair.innerIndex >= subtable.deltaValues.size()) {
        sscb->message(sscb, "invalid inner index in index map");
        return netAdjustment;
    }

    auto &deltaValues = subtable.deltaValues[pair.innerIndex];
    for (int32_t i = 0; i < subtable.regionIndices.size(); i++) {
        auto regionIndex = subtable.regionIndices[i];
        if (scalars[regionIndex])
            netAdjustment += scalars[regionIndex] * deltaValues[i];
    }

    return netAdjustment;
}

float itemVariationStore::applyDeltasForGid(ctlSharedStmCallbacks *sscb,
                                            var_indexMap &map, uint16_t gid,
                                            float *scalars,
                                            int32_t regionListCount) {
    var_indexPair pair;

    /* Use (0,gid) as the default index pair if the index map table is missing */
    if (map.map.size() == 0) {
        pair.outerIndex = 0;
        pair.innerIndex = gid;
    } else
        lookupIndexMap(map, gid, pair);

    return applyDeltasForIndexPair(sscb, pair, scalars, regionListCount);
}

/* HVAR / vmtx tables */

var_hmtx::var_hmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb) {
    sfrTable *table = NULL;
    uint32_t ivsOffset;
    uint32_t widthMapOffset;
    uint32_t lsbMapOffset;
    uint32_t rsbMapOffset;
    float defaultWidth;
    int32_t i;
    int32_t numGlyphs;

    /* read hhea table */
    table = sfrGetTableByTag(sfr, HHEA_TABLE_TAG);
    if (table == NULL || table->length < HHEA_TABLE_HEADER_SIZE) {
        sscb->message(sscb, "invalid/missing hhea table");
        return;
    }

    sscb->seek(sscb, table->offset);

    header.version = (Fixed)sscb->read4(sscb);
    if (header.version != HHEA_TABLE_VERSION) {
        sscb->message(sscb, "invalid hhea table version");
        return;
    }

    header.ascender = (int16_t)sscb->read2(sscb);
    header.descender = (int16_t)sscb->read2(sscb);
    header.lineGap = (int16_t)sscb->read2(sscb);
    header.advanceWidthMax = (uint16_t)sscb->read2(sscb);
    header.minLeftSideBearing = (int16_t)sscb->read2(sscb);
    header.minRightSideBearing = (int16_t)sscb->read2(sscb);
    header.xMaxExtent = (int16_t)sscb->read2(sscb);
    header.caretSlopeRise = (int16_t)sscb->read2(sscb);
    header.caretSlopeRun = (int16_t)sscb->read2(sscb);
    header.caretOffset = (int16_t)sscb->read2(sscb);
    for (i = 0; i < 4; i++) header.reserved[i] = (int16_t)sscb->read2(sscb);
    header.metricDataFormat = (int16_t)sscb->read2(sscb);
    header.numberOfHMetrics = (uint16_t)sscb->read2(sscb);
    if (header.numberOfHMetrics == 0) {
        sscb->message(sscb, "invalid numberOfHMetrics value in hhea table");
        return;
    }

    /* read hmtx table */
    table = sfrGetTableByTag(sfr, HMTX_TABLE_TAG);
    if (table == NULL)
        return;

    /* estimate the number of glyphs from the table size instead of reading the head table */
    numGlyphs = (table->length / 2) - header.numberOfHMetrics;
    if ((numGlyphs < header.numberOfHMetrics) || (numGlyphs > 65535)) {
        sscb->message(sscb, "invalid hmtx table size");
        return;
    }

    sscb->seek(sscb, table->offset);

    var_glyphMetrics gm;
    for (i = 0; i < header.numberOfHMetrics; i++) {
        gm.width = (uint16_t)sscb->read2(sscb);
        gm.sideBearing = (int16_t) sscb->read2(sscb);
        defaultMetrics.push_back(gm);
    }
    // gm still has width of last entry
    for (; i < numGlyphs; i++) {
        gm.sideBearing = (int16_t)sscb->read2(sscb);
        defaultMetrics.push_back(gm);
    }

    table = sfrGetTableByTag(sfr, HVAR_TABLE_TAG);
    if (table == NULL) { /* HVAR table is optional */
        return;
    }

    sscb->seek(sscb, table->offset);

    if (table->length < HVAR_TABLE_HEADER_SIZE) {
        sscb->message(sscb, "invalid HVAR table size");
        return;
    }

    /* Read and validate HVAR table version */
    if (sscb->read4(sscb) != HVAR_TABLE_VERSION) {
        sscb->message(sscb, "invalid HVAR table version");
        return;
    }

    ivsOffset = sscb->read4(sscb);
    widthMapOffset = sscb->read4(sscb);
    lsbMapOffset = sscb->read4(sscb);
    rsbMapOffset = sscb->read4(sscb);

    if (ivsOffset == 0) {
        sscb->message(sscb, "item variation store offset in HVAR is NULL");
        return;
    }

    ivs = std::make_unique<itemVariationStore>(sscb, table->offset,
                                               table->length, ivsOffset);

    // XXX Add error messages
    loadIndexMap(sscb, table, widthMapOffset, widthMap);
    loadIndexMap(sscb, table, lsbMapOffset, lsbMap);
    loadIndexMap(sscb, table, rsbMapOffset, rsbMap);
}

bool var_hmtx::lookup(ctlSharedStmCallbacks *sscb, uint16_t axisCount,
                      Fixed *instCoords, uint16_t gid,
                      var_glyphMetrics &metrics) {
    if (gid >= defaultMetrics.size()) {
        sscb->message(sscb, "var_lookuphmtx: invalid glyph ID");
        return false;
    }

    metrics = defaultMetrics[gid];

    /* modify the default metrics if the font has variable font tables */
    if (instCoords && (axisCount > 0)) {
        long regionListCount = ivs->getRegionCount();
        float scalars[CFF2_MAX_MASTERS];

        ivs->calcRegionScalars(sscb, axisCount, instCoords, scalars);
        metrics.width += ivs->applyDeltasForGid(sscb, widthMap, gid, scalars,
                                                regionListCount);
        if (lsbMap.offset > 0) /* if side bearing variation data are provided, index map must exist */
            metrics.sideBearing += ivs->applyDeltasForGid(sscb, lsbMap, gid,
                                                          scalars,
                                                          regionListCount);
    }

    return true;
}

var_vmtx::var_vmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb) {
    sfrTable *table = NULL;
    uint32_t ivsOffset;
    uint32_t widthMapOffset;
    uint32_t tsbMapOffset;
    uint32_t bsbMapOffset;
    uint32_t vorgMapOffset;
    float defaultWidth;
    int32_t i;
    int32_t numGlyphs;

    table = sfrGetTableByTag(sfr, VHEA_TABLE_TAG);
    if (table == NULL || table->length < VHEA_TABLE_HEADER_SIZE) {
        sscb->message(sscb, "invalid/missing vhea table");
        return;
    }

    sscb->seek(sscb, table->offset);

    header.version = (Fixed)sscb->read4(sscb);
    if (header.version != VHEA_TABLE_VERSION &&
        header.version != VHEA_TABLE_VERSION_1_1) {
        sscb->message(sscb, "invalid hhea table version");
        return;
    }

    header.vertTypoAscender = (int16_t)sscb->read2(sscb);
    header.vertTypoDescender = (int16_t)sscb->read2(sscb);
    header.vertTypoLineGap = (int16_t)sscb->read2(sscb);
    header.advanceHeightMax = (uint16_t)sscb->read2(sscb);
    header.minTop = (int16_t)sscb->read2(sscb);
    header.minBottom = (int16_t)sscb->read2(sscb);
    header.caretSlopeRise = (int16_t)sscb->read2(sscb);
    header.caretSlopeRun = (int16_t)sscb->read2(sscb);
    header.caretOffset = (int16_t)sscb->read2(sscb);
    for (i = 0; i < 4; i++) header.reserved[i] = (int16_t)sscb->read2(sscb);
    header.metricDataFormat = (int16_t)sscb->read2(sscb);
    header.numOfLongVertMetrics = (uint16_t)sscb->read2(sscb);
    if (header.numOfLongVertMetrics == 0) {
        sscb->message(sscb, "invalid numOfLongVertMetrics value in vhea table");
        return;
    }

    /* read vmtx table */
    table = sfrGetTableByTag(sfr, VMTX_TABLE_TAG);
    if (table == NULL)
        return;

    /* estimate the number of glyphs from the table size instead of reading the head table */
    numGlyphs = (table->length / 2) - header.numOfLongVertMetrics;
    if (numGlyphs < header.numOfLongVertMetrics) {
        sscb->message(sscb, "invalid vmtx table size");
        return;
    }

    sscb->seek(sscb, table->offset);

    var_glyphMetrics gm;
    for (i = 0; i < header.numOfLongVertMetrics; i++) {
        gm.width = (uint16_t)sscb->read2(sscb);
        gm.sideBearing = (int16_t)sscb->read2(sscb);
        defaultMetrics.push_back(gm);
    }
    // gm.width still has last value
    for (; i < numGlyphs; i++) {
        gm.sideBearing = (int16_t)sscb->read2(sscb);
        defaultMetrics.push_back(gm);
    }

    /* read optional VORG table */
    table = sfrGetTableByTag(sfr, VORG_TABLE_TAG);
    if (table != NULL) {
        int16_t defaultVertOriginY;
        uint16_t numVertOriginYMetrics;

        sscb->seek(sscb, table->offset);

        if (table->length < VORG_TABLE_HEADER_SIZE) {
            sscb->message(sscb, "invalid VVAR table size");
            return;
        }

        if (sscb->read4(sscb) != VORG_TABLE_VERSION) {
            sscb->message(sscb, "invalid VORG table version");
            return;
        }

        defaultVertOriginY = (int16_t)sscb->read2(sscb);
        numVertOriginYMetrics = (uint16_t)sscb->read2(sscb);
        if (table->length < (uint32_t)(VORG_TABLE_HEADER_SIZE + 4 * numVertOriginYMetrics)) {
            sscb->message(sscb, "invalid VORG table size");
            return;
        }

        for (i = 0; i < numGlyphs; i++)
            vertOriginY.push_back(defaultVertOriginY);

        for (i = 0; i < numVertOriginYMetrics; i++) {
            uint16_t glyphIndex = (uint16_t)sscb->read2(sscb);
            int16_t glyphVertOriginY = (int16_t)sscb->read2(sscb);

            if (glyphIndex >= numGlyphs) {
                sscb->message(sscb, "invalid glyph index in VORG table");
                return;
            }
            vertOriginY[glyphIndex] = glyphVertOriginY;
        }
    }

    table = sfrGetTableByTag(sfr, VVAR_TABLE_TAG);
    if (table == NULL) { /* VVAR table is optional */
        return;
    }

    sscb->seek(sscb, table->offset);

    /* Read and validate HVAR/VVAR table version */
    if (table->length < VVAR_TABLE_HEADER_SIZE) {
        sscb->message(sscb, "invalid VVAR table size");
        return;
    }

    if (sscb->read4(sscb) != VVAR_TABLE_VERSION) {
        sscb->message(sscb, "invalid VVAR table version");
        return;
    }

    ivsOffset = sscb->read4(sscb);
    widthMapOffset = sscb->read4(sscb);
    tsbMapOffset = sscb->read4(sscb);
    bsbMapOffset = sscb->read4(sscb);
    vorgMapOffset = sscb->read4(sscb);

    if (ivsOffset == 0) {
        sscb->message(sscb, "item variation store offset in VVAR is NULL");
        return;
    }

    ivs = std::make_unique<itemVariationStore>(sscb, table->offset,
                                               table->length, ivsOffset);

    // XXX add error messages
    loadIndexMap(sscb, table, widthMapOffset, widthMap);
    loadIndexMap(sscb, table, tsbMapOffset, tsbMap);
    loadIndexMap(sscb, table, bsbMapOffset, bsbMap);
    loadIndexMap(sscb, table, vorgMapOffset, vorgMap);
}

bool var_vmtx::lookup(ctlSharedStmCallbacks *sscb, uint16_t axisCount,
                      Fixed *instCoords, uint16_t gid,
                      var_glyphMetrics &metrics) {
    if (gid >= defaultMetrics.size()) {
        sscb->message(sscb, "var_lookupvmtx: invalid glyph ID");
        return false;
    }

    metrics = defaultMetrics[gid];

    /* modify the default metrics if the font has variable font tables */
    if (instCoords && (axisCount > 0)) {
        long regionListCount = ivs->getRegionCount();
        float scalars[CFF2_MAX_MASTERS];

        ivs->calcRegionScalars(sscb, axisCount, instCoords, scalars);
        metrics.width += ivs->applyDeltasForGid(sscb, widthMap, gid,
                                                scalars, regionListCount);
        if (tsbMap.offset > 0) /* if side bearing variation data are provided, index map must exist */
            metrics.sideBearing += ivs->applyDeltasForGid(sscb, tsbMap, gid,
                                                          scalars,
                                                          regionListCount);
    }
    return true;
}

/* MVAR table */

var_MVAR::var_MVAR(sfrCtx sfr, ctlSharedStmCallbacks *sscb) {
    uint32_t ivsOffset;
    uint16_t valueRecordSize;
    uint16_t valueRecordCount;
    uint16_t i;

    sfrTable *table = sfrGetTableByTag(sfr, MVAR_TABLE_TAG);
    if (table == NULL)
        return;

    sscb->seek(sscb, table->offset);

    /* Read and validate table version */
    if (table->length < MVAR_TABLE_HEADER_SIZE) {
        sscb->message(sscb, "invalid MVAR table size");
        return;
    }

    if (sscb->read4(sscb) != MVAR_TABLE_VERSION) {
        sscb->message(sscb, "invalid MVAR table version");
        return;
    }

    axisCount = sscb->read2(sscb);
    valueRecordSize = sscb->read2(sscb);
    valueRecordCount = sscb->read2(sscb);
    ivsOffset = (uint32_t)sscb->read2(sscb);

    if (ivsOffset == 0) {
        sscb->message(sscb, "item variation store offset in MVAR is NULL");
        return;
    }

    if (valueRecordSize < MVAR_TABLE_RECORD_SIZE) {
        if ((valueRecordSize > 0) || (valueRecordCount > 0)) {
            /* It is OK to have valueRecordSize != MVAR_TABLE_RECORD_SIZE if it is zero because there are no records. */
            sscb->message(sscb, "invalid MVAR record size");
            return;
        }
    }
    if (table->length < MVAR_TABLE_HEADER_SIZE + (uint32_t)valueRecordSize * valueRecordCount) {
        sscb->message(sscb, "invalid MVAR table size");
        return;
    }

    for (i = 0; i < valueRecordCount; i++) {
        MVARValueRecord mvr;
        mvr.valueTag = sscb->read4(sscb);
        mvr.pair.outerIndex = sscb->read2(sscb);
        mvr.pair.innerIndex = sscb->read2(sscb);
        for (int j = MVAR_TABLE_RECORD_SIZE; j < valueRecordSize; j++)
            (void)sscb->read1(sscb);
        values.push_back(mvr);
    }

    ivs = std::make_unique<itemVariationStore>(sscb, table->offset,
                                               table->length, ivsOffset);
}

bool var_MVAR::lookup(ctlSharedStmCallbacks *sscb, uint16_t axisCount,
                      Fixed *instCoords, ctlTag tag, float &value) {
    long top, bot, index;
    MVARValueRecord *rec = NULL;
    bool found = false;
    float scalars[CFF2_MAX_MASTERS];

    if (instCoords == 0 || axisCount == 0) {
        sscb->message(sscb, "zero instCoords/axis count specified for MVAR");
        return false;
    }

    bot = 0;
    top = (long)values.size() - 1;
    while (bot <= top) {
        index = (top + bot) / 2;
        rec = &values[index];
        if (rec->valueTag == tag) {
            found = true;
            break;
        }
        if (tag < rec->valueTag)
            top = index - 1;
        else
            bot = index + 1;
    }

    if (!found) {
        /* Specified tag was not found. */
        return false;
    }

    ivs->calcRegionScalars(sscb, axisCount, instCoords, scalars);

    /* Blend the metric value using the IVS table */
    value = ivs->applyDeltasForIndexPair(sscb, rec->pair, scalars,
                                         ivs->getRegionCount());

    return true;
}

/* Get version numbers of libraries. */
void varsupportGetVersion(ctlVersionCallbacks *cb) {
    if (cb->called & 1 << VAR_LIB_ID)
        return; /* Already enumerated */

    /* This library */
    cb->getversion(cb, VARSUPPORT_VERSION, "varsupport");

    /* Record this call */
    cb->called |= 1 << VAR_LIB_ID;
}
