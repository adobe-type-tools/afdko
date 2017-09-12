/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef VARREAD_H
#define VARREAD_H

#include "sfntread.h"
#include "supportpublictypes.h"
#include "supportfp.h"
#include "absfont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Variable Font Tables Reader Library
   =======================================
   This library parses tables common tables used by variable OpenType fonts.
*/

#define VARREAD_VERSION CTL_MAKE_VERSION(1,0,4)
#define F2DOT14_TO_FIXED(v)         (((Fixed)(v))<<2)
#define FIXED_TO_F2DOT14(v)         ((var_F2dot14)(((Fixed)(v)+0x00000002)>>2))

typedef Int16   var_F2dot14; /* 2.14 fixed point number */

/* item variation store */

typedef struct region_ {
    Fixed   startCoord;
    Fixed   peakCoord;
    Fixed   endCoord;
} variationRegion;

typedef struct variationRegionList_ {
    unsigned short    axisCount;
    unsigned short    regionCount;
    dnaDCL(variationRegion, regions);
} variationRegionList;

typedef dnaDCL(unsigned short, indexArray);

typedef struct itemVariationDataSubtable_ {
    unsigned short  itemCount;
    unsigned short  regionCount;
    dnaDCL(short, regionIndices);
    dnaDCL(short, deltaValues);
} itemVariationDataSubtable;

typedef struct itemVariationDataSubtableList_ {
    dnaDCL(itemVariationDataSubtable, ivdSubtables);
} itemVariationDataSubtableList;

struct var_itemVariationStore_ {
    variationRegionList     regionList;
    itemVariationDataSubtableList   dataList;
};
    

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

int var_normalizeCoords(ctlSharedStmCallbacks *sscb, var_axes axes, Fixed *userCoords, Fixed *normCoords);

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

void     var_calcRegionScalars(ctlSharedStmCallbacks *sscb, var_itemVariationStore ivs, unsigned short axisCount, Fixed *instCoords, float *scalars);

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

/* Predefined MVAR tags */
#define MVAR_hasc_tag   CTL_TAG('h','a','s','c')    /* ascender OS/2.sTypoAscender */
#define MVAR_hdsc_tag   CTL_TAG('h','d','s','c')    /* horizontal descender	OS/2.sTypoDescender */
#define MVAR_hlgp_tag   CTL_TAG('h','l','g','p')    /* horizontal line gap	OS/2.sTypoLineGap */
#define MVAR_hcla_tag   CTL_TAG('h','c','l','a')    /* horizontal clipping ascent	OS/2.usWinAscent */
#define MVAR_hcld_tag   CTL_TAG('h','c','l','d')    /* horizontal clipping descent	OS/2.usWinDescent */
#define MVAR_vasc_tag   CTL_TAG('v','a','s','c')    /* vertical ascender	vhea.ascent */
#define MVAR_vdsc_tag   CTL_TAG('v','d','s','c')    /* vertical descender	vhea.descent */
#define MVAR_vlgp_tag   CTL_TAG('v','l','g','p')    /* vertical line gap    vhea.lineGap */
#define MVAR_hcrs_tag   CTL_TAG('h','c','r','s')    /* horizontal caret rise        hhea.caretSlopeRise */
#define MVAR_hcrn_tag   CTL_TAG('h','c','r','n')    /* horizontal caret run hhea.caretSlopeRun */
#define MVAR_hcof_tag   CTL_TAG('h','c','o','f')    /* horizontal caret offset      hhea.caretOffset */
#define MVAR_vcrs_tag   CTL_TAG('v','c','r','s')    /* vertical caret rise  vhea.caretSlopeRise */
#define MVAR_vcrn_tag   CTL_TAG('v','c','r','n')    /* vertical caret run   vhea.caretSlopeRun */
#define MVAR_vcof_tag   CTL_TAG('v','c','o','f')    /* vertical caret offset        vhea.caretOffset */
#define MVAR_xhgt_tag   CTL_TAG('x','h','g','t')    /* x height     OS/2.sxHeight */
#define MVAR_cpht_tag   CTL_TAG('c','p','h','t')    /* cap height   OS/2.sCapHeight */
#define MVAR_sbxs_tag   CTL_TAG('s','b','x','s')    /* subscript em x size  OS/2.ySubscriptXSize */
#define MVAR_sbys_tag   CTL_TAG('s','b','y','s')    /* subscript em y size  OS/2.ySubscriptYSize */
#define MVAR_sbxo_tag   CTL_TAG('s','b','x','o')    /* subscript em x offset        OS/2.ySubscriptXOffset */
#define MVAR_sbyo_tag   CTL_TAG('s','b','y','o')    /* subscript em y offset        OS/2.ySubscriptYOffset */
#define MVAR_spxs_tag   CTL_TAG('s','p','x','s')    /* superscript em x size        OS/2.ySuperscriptXSize */
#define MVAR_spys_tag   CTL_TAG('s','p','y','s')    /* superscript em y size        OS/2.ySuperscriptYSize */
#define MVAR_spxo_tag   CTL_TAG('s','p','x','o')    /* superscript em x offset      OS/2.ySuperscriptXOffset */
#define MVAR_spyo_tag   CTL_TAG('s','p','y','o')    /* superscript em y offset      OS/2.ySuperscriptYOffset */
#define MVAR_strs_tag   CTL_TAG('s','t','r','s')    /* strikeout size       OS/2.yStrikeoutSize */
#define MVAR_stro_tag   CTL_TAG('s','t','r','o')    /* strikeout offset     OS/2.yStrikeoutPosition */
#define MVAR_unds_tag   CTL_TAG('u','n','d','s')    /* underline size       post.underlineThickness */
#define MVAR_undo_tag   CTL_TAG('u','n','d','o')    /* underline offset     post.underlinePosition */
#define MVAR_gsp0_tag   CTL_TAG('g','s','p','0')    /* gaspRange[0] gasp.gaspRange[0].rangeMaxPPEM */
#define MVAR_gsp1_tag   CTL_TAG('g','s','p','1')    /* gaspRange[1] gasp.gaspRange[1].rangeMaxPPEM */
#define MVAR_gsp2_tag   CTL_TAG('g','s','p','2')    /* gaspRange[2] gasp.gaspRange[2].rangeMaxPPEM */
#define MVAR_gsp3_tag   CTL_TAG('g','s','p','3')    /* gaspRange[3] gasp.gaspRange[3].rangeMaxPPEM */
#define MVAR_gsp4_tag   CTL_TAG('g','s','p','4')    /* gaspRange[4] gasp.gaspRange[4].rangeMaxPPEM */
#define MVAR_gsp5_tag   CTL_TAG('g','s','p','5')    /* gaspRange[5] gasp.gaspRange[5].rangeMaxPPEM */
#define MVAR_gsp6_tag   CTL_TAG('g','s','p','6')    /* gaspRange[6] gasp.gaspRange[6].rangeMaxPPEM */
#define MVAR_gsp7_tag   CTL_TAG('g','s','p','7')    /* gaspRange[7] gasp.gaspRange[7].rangeMaxPPEM */
#define MVAR_gsp8_tag   CTL_TAG('g','s','p','8')    /* gaspRange[8] gasp.gaspRange[8].rangeMaxPPEM */
#define MVAR_gsp9_tag   CTL_TAG('g','s','p','9')    /* gaspRange[9] gasp.gaspRange[9].rangeMaxPPEM */

/* MVAR table */
struct var_MVAR_;
typedef struct var_MVAR_ *var_MVAR;

var_MVAR var_loadMVAR(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

/*  var_loadMVAR() loads the MVAR table.
    If not found, NULL is returned.

    sfr - context pointer created by calling sfrNew in sfntread library.

    sscb - a pointer to shared stream callback functions.
*/

int var_lookupMVAR(ctlSharedStmCallbacks *sscb, var_MVAR mvar, unsigned short axisCount, float *scalars, ctlTag tag, float *value);

/*  var_lookupMVAR() lookup font-wide metric values for a tag blended using font instance scalars.
    returns 0 if successful, otherwise non-zero.

    sscb - a pointer to shared stream callback functions.

    mvar - a pointer to the MVAR metrics table.

    axisCount - the number of axes.

    scalars - a pointer to font instance scalars. May be NULL if no blending required.

    tag - the tag of the metric value to be looked up.

    value - where the blended metric value is returned.
*/

void var_freeMVAR(ctlSharedStmCallbacks *sscb, var_MVAR mvar);

/*  var_freeMVAR() frees the metrics table data loaded by calling var_loadMVAR().

    sscb - a pointer to shared stream callback functions.

    mvar - a pointer to the horizontal metrics table data to be freed.
*/

void varreadGetVersion(ctlVersionCallbacks *cb);
    
/* varreadGetVersion() returns the library version number and name via the client
    callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* VARREAD_H */
