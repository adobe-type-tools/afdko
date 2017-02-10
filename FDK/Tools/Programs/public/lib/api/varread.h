/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef VARREAD_H
#define VARREAD_H

#include "sfntread.h"
#include "supportpublictypes.h"
#include "supportfp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Variable Font Tables Reader Library
   =======================================
   This library parses tables common tables used by variable OpenType fonts.
*/

typedef Frac   var_F2dot14; /* 2.14 fixed point number */

/* glyph width and sidebearing */
typedef struct glyphMetrics_ {
    float           width;
    float           sideBearing;
} var_glyphMetrics;

/* variable font axis tables */
struct var_axes_;
typedef struct var_axes_ *var_axes;

/* convert 2.14 fixed value to float */
float var_F2dot14ToFloat(var_F2dot14 v);

var_axes var_loadaxes(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

/*  var_loadaxes() loads the axes table data (fvar and avar tables) from an SFNT font.
    If not found, NULL is returned.

    sfr - context pointer created by calling sfrNew in sfntread library.

    sscb - a pointer to shared stream callback functions.
*/

void var_freeaxes(ctlSharedStmCallbacks *sscb, var_axes axes);

/*  var_freeaxes() frees the axes table data loaded by calling var_loadaxes().

    sscb - a pointer to shared stream callback functions.

    axes - a pointer to the axes table data to be freed.
*/

/* item variation store sub-table */
struct var_itemVariationStore_;
typedef struct var_itemVariationStore_ *var_itemVariationStore;

unsigned short var_getAxisCount(var_axes axes);

/*  var_getAxisCount() retrieves the number of axes from the axes table data.

    axes - a pointer to the axes table data.
*/

int var_getAxis(var_axes axes, unsigned short index, unsigned long *tag, Fixed *minValue, Fixed *defaultValue, Fixed *maxValue, unsigned short *flags);

/*  var_getAxis() retrieves info on an anxis from the axes table data.
    return 0 for success, 1 for failure.

    axes - a pointer to the axes table data.
    
    index - specifies an axis index.

    tag - where the four-letter axis tag is returned.
    May be NULL if tag is not needed.
    
    minValue - where the minimum coordinate value for the axis is returned.
    May be NULL if the minimum value is not needed.

    defaultValue - where the default coordinate value for the axis is returned.
    May be NULL if the minimum value is not needed.
    
    maxValue - where the maximum coordinate value for the axis is returned.
    May be NULL if the maximum value is not needed.
    
    flags - where the axis flags value is returned.
    May be NULL if the flags value is not needed.
*/

int var_normalizeCoords(ctlSharedStmCallbacks *sscb, var_axes axes, Fixed *userCoords, var_F2dot14 *normCoords);

/*  var_normalizeCoords() normalizes a variable font value coordinates.
    return 0 for success, 1 for failure.

    sscb - a pointer to shared stream callback functions.

    axes - a pointer to the axes table data.
    
    userCoords - a pointer to user coordinates to be normalized (unmodified).

    normCoords - where the resulting normalized coordinates are returned as 2.14 fixed point values.
*/

unsigned short var_getInstanceCount(var_axes axes);

/*  var_getInstanceCount() returns the number of named instances from the axes table data.

    axes - a pointer to the axes table data.
*/

int var_findInstance(var_axes axes, float *userCoords, unsigned short axisCount, unsigned short *subfamilyID, unsigned short *postscriptID);

/*  var_findInstance() searches for a named instance matching the given coordinates.
    return the instance index if >= 0, -1 if not found.

    axes - a pointer to the axes table data.
    
    axisCount - the number of axes.
    
    subfamilyID - where the name ID of the subfamily name of the instance is returned.

    postscriptID - where the name ID of the PostScript name of the instance is returned.
    0 is returned if no PostScript name ID is provided.
*/

var_itemVariationStore var_loadItemVariationStore(ctlSharedStmCallbacks *sscb, unsigned long tableOffset, unsigned long tableLength, unsigned long ivsOffset);

/*  var_loadItemVariationStore() parses the Item Variation Store (IVS) sub-table.
    return a pointer to IVS data, or NULL if not found.

    sscb - a pointer to shared stream callback functions.
 
    tableOffset - offset to the beginning of the container table from the beginning of the stream.
    
    tableLength - length of the container table.
    
    ivsOffset - offset to the IVS data from the beginning of the container table.
*/

void var_freeItemVariationStore(ctlSharedStmCallbacks *sscb, var_itemVariationStore ivs);

/*  var_freeItemVariationStore() frees the IVS data loaded by var_loadItemVariationStore.

    sscb - a pointer to shared stream callback functions.
 
    ivs - a pointer to the IVS data to be freed.
*/

unsigned short var_getIVSRegionCount(var_itemVariationStore ivs);

/*  var_getIVSRegionCount() returns the number of regions in the region list in the IVS data.

    ivs - a pointer to the IVS data.
*/

unsigned short var_getIVSRegionCountForIndex(var_itemVariationStore ivs, unsigned short vsIndex);

/*  var_getIVSRegionCountForIndex() returns the number of sub-regions applicable to an IVS sub-table.

    ivs - a pointer to the IVS data.

    vsIndex - IVS sub-table index.
*/

long var_getIVSRegionIndices(var_itemVariationStore ivs, unsigned short vsIndex, unsigned short *regionIndices, long regionCount);

/*  var_getIVSRegionIndices() returns indices of sub-regions applicable to an an IVS sub-table.

    ivs - a pointer to the IVS data.

    vsIndex - IVS sub-table index.
    
    regionIndices - where region indices are returned.
    
    regionCount - the length of regionIndices array.
*/

void     var_calcRegionScalars(ctlSharedStmCallbacks *sscb, var_itemVariationStore ivs, unsigned short axisCount, var_F2dot14 *instCoords, float *scalars);

/*  var_calcRegionScalars() calculates scalars for all regions given a normalized design vector for an instance.

    sscb - a pointer to shared stream callback functions.

    ivs - a pointer to the IVS data.

    axisCount - the number axes.
    
    instCoords - a pointer to normalized design vector of a font instance.
    
    scalars - where scalars are returned as float values.
*/

/* horizontal metrics tables */
struct var_hmtx_;
typedef struct var_hmtx_ *var_hmtx;

var_hmtx var_loadhmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

/*  var_loadhmtx() loads the horizontal metrics tables.
    If not found, NULL is returned.

    sfr - context pointer created by calling sfrNew in sfntread library.

    sscb - a pointer to shared stream callback functions.
*/

void var_freehmtx(ctlSharedStmCallbacks *sscb, var_hmtx hmtx);

/*  var_freehmtx() frees the horizontal metrics table data loaded by calling var_loadhmtx().

    sscb - a pointer to shared stream callback functions.

    hmtx - a pointer to the horizontal metrics table data to be freed.
*/

int var_lookuphmtx(ctlSharedStmCallbacks *sscb, var_hmtx hmtx, unsigned short axisCount, float *scalars, unsigned short gid, var_glyphMetrics *metrics);

/*  var_lookuphmtx() lookup horizontal metrics for a glyph optionally blended using font instance scalars.
    returns 0 if successful, otherwise non-zero.

    sscb - a pointer to shared stream callback functions.

    hmtx - a pointer to the horizontal metrics table.

    axisCount - the number of axes.

    scalars - a pointer to font instance scalars. May be NULL if no blending required.

    gid - the glyph ID to be looked up.

    metrics - where the horizontal glyph metrics are returned.
*/

/* vertical metrics tables */
struct var_vmtx_;
typedef struct var_vmtx_ *var_vmtx;

var_vmtx var_loadvmtx(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

/*  var_loadvmtx() loads the vertical metrics tables.
    If not found, NULL is returned.

    sfr - context pointer created by calling sfrNew in sfntread library.

    sscb - a pointer to shared stream callback functions.
*/

void var_freevmtx(ctlSharedStmCallbacks *sscb, var_vmtx vmtx);

/*  var_freevmtx() frees the vertical metrics table data loaded by calling var_loadvmtx().

    sscb - a pointer to shared stream callback functions.

    vmtx - a pointer to the horizontal metrics table data to be freed.
*/

int var_lookupvmtx(ctlSharedStmCallbacks *sscb, var_vmtx vmtx, unsigned short axisCount, float *scalars, unsigned short gid, var_glyphMetrics *metrics);

/*  var_lookupvmtx() lookup vertical metrics for a glyph optionally blended using font instance scalars.
    returns 0 if successful, otherwise non-zero.

    sscb - a pointer to shared stream callback functions.

    vmtx - a pointer to the vertical metrics table.

    axisCount - the number of axes.

    scalars - a pointer to font instance scalars. May be NULL if no blending required.

    gid - the glyph ID to be looked up.

    metrics - where the vertical glyph metrics are returned.
*/

/* MVAR table */
struct var_MVAR_;
typedef struct var_MVAR_ *var_MVAR;

var_MVAR var_loadMVAR(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

/*  var_loadMVAR() loads the MVAR table.
    If not found, NULL is returned.

    sfr - context pointer created by calling sfrNew in sfntread library.

    sscb - a pointer to shared stream callback functions.
*/

void var_freeMVAR(ctlSharedStmCallbacks *sscb, var_MVAR mvar);

/*  var_freeMVAR() frees the metrics table data loaded by calling var_loadMVAR().

    sscb - a pointer to shared stream callback functions.

    mvar - a pointer to the horizontal metrics table data to be freed.
*/

#ifdef __cplusplus
}
#endif

#endif /* VARREAD_H */
