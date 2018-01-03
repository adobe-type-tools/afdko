/* @(#)CM_VerSion cff_struct.h atm09 1.3 16563.eco sum= 19329 atm09.004 */
/* @(#)CM_VerSion cff_struct.h atm08 1.2 16380.eco sum= 29218 atm08.007 */
/*
  cff_struct.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CFF_STRUCT_H
#define CFF_STRUCT_H

#if defined PSENVIRONMENT && PSENVIRONMENT
/* PostScript headers */
#include PACKAGE_SPECS
#include PUBLICTYPES
#include FP
#include BUILDCH
#else  /* PSENVIRONMENT */
/* standard coretype environment */
#include "supportpublictypes.h"
#include "supportfp.h"
#include "buildch.h"
#endif /* PSENVIRONMENT */

#ifndef MAXAXES
#define MAXAXES 15
#endif

#ifndef MAXWEIGHT
#define MAXWEIGHT 16
#endif

typedef Card16 cffSID;   /* string identifier */

#define CFF_INVALID_SID 0xffff
#define MAX_XUID_ENTRIES 16

/* info that the client wants when GetData or ReleaseData is called */
typedef void * FontSetHook;

/* DESCRIPTOR_DATA will always be used in non-Chameleon fonts */
typedef enum {DESCRIPTOR_DATA, MASTER_DATA} WhichChamData;

/* Client must return a valid pointer or raise an exception.
 * If the client wishes to return more than the requested data,
 * it should adjust the plength parameter appropriately.
 * The client must return at least the requested data.
 */
typedef Card8 * (* GetDataProto) (
    Card32    offset,         /* (R) if offset is out of bounds, fatal error */
    Card32 *  plength,        /* (W) overwritten before return with valid length. */
    WhichChamData which,      /* (R) whether the descriptor file or master file
                               * is to be accessed. Can be ignored by clients not 
                               * handling Chameleon fonts.
                               */
    FontSetHook fontsethook); /* info the client wants passed through */

/* client must raise an exception if a serious problem occurs */
typedef void (* ReleaseDataProto) (
    Card8 *dataptr,           /* (R) ptr previously returned by GetData */
    FontSetHook fontsethook); /* info the client wants passed through */

typedef struct _t_ArrayInfo {
    void * window;         /* private to cff_parse */
    Card32 offOffsetArray; /* the offset of the offset array */
    Card32 offData;        /* the offset of the data portion of the index */
    Card16 nEntries;       /* number of entries in the index */
    Card8  offsetSize;     /* number of bytes in each offset entry */
} ArrayInfo, *PArrayInfo;

/********************************************************
 * CFFSetInfo holds information about the fontset
 * as a whole.  The client fills out the GetData, ReleaseData,
 * fontsethook, and ASZone portions of the structure.
 * CFFGet_FontSet fills out the remaining elements.
 ********************************************************/
typedef struct _t_CFFSetInfo {
    GetDataProto GetData;         /* method to get font data from client */
    ReleaseDataProto ReleaseData; /* method to release data retrieved via GetData */
    FontSetHook fontsethook;      /* client-specified data sent to Get/ReleaseData */

    /* See ASZone.h for an explanation of ASZone objects in general.
     * The following additional requirements apply to ASZone objects
     * that are to be used with cff_parse:
     *
     * 1. The methods that the ASZone needs to support are
     * Malloc, Calloc, SureMalloc, SureRealloc, and Free.
     *
     * 2. The SureMalloc and SureRealloc methods must handle out-of-memory
     * errors by raising an exception, not by returning NULL.
     */
    ASZone zone;                  /* zone object for memory allocation */

    ArrayInfo FontNames;          /* how to find the name index */
    ArrayInfo FontDicts;          /* how to find the top dict index */
    ArrayInfo globalSubrs;        /* how to find the global subroutines */
    ArrayInfo stringIndex;        /* how to find the string index */
} CFFSetInfo, *PCFFSetInfo;

/********************************************************
 * CFFFontInfo holds information about individual fonts
 * in the fontset.  
 ********************************************************/
typedef struct _t_CFFFontInfo {
    /* There are two methods used to indicate that a field's value 
     * was retrieved from the font:  
     *
     * 1) If a field has a default defined in the cff spec,
     * we use counts to identify if they are found in the font (eg,
     * charstringTypeCount is the count for charstringType).  If the
     * associated count is 0, the information is not in the font (it is
     * a default).  The count reflects the number of times that the operator
     * is found in the font's top and font dictionaries.  If the font is
     * a synthetic font, it also counts the number of times it appears
     * in the base font.  cff_parse assumes overflow will not occur 
     * for the count fields.  
     * 
     * 2) If a field does not have a default defined in the cff spec,
     * we default the values to something invalid.
     * The comments following the fields explain how to 
     * tell that the field is not in the font. If the comment holds 
     * true, than the value was not retrieved from the font.  
     * eg, if uniqueID == 0, then cff_uniqueID was NOT in the font.
     */

    PCFFSetInfo pSetInfo; 
    void *    priv;                   /* data private to cff_parse */
#if NT_KERNEL_NO_FPU
    FracMtx   fontMatrix;             /* fontMatrixCount == 0 */
#else
    RealMtx   fontMatrix;             /* fontMatrixCount == 0 */
#endif
    Card32    xuid[MAX_XUID_ENTRIES]; /* xuidlen == 0 */
    Card32    encodingOffset;         /* encodingOffsetCount == 0 */
    Card32    privateOffset;          /* privateOffset == 0
                                       * if chamGlyphCount > 0, this
                                       * tells the offset of the Chameleon
                                       * font descriptor
                                       */
    Card32    privateSize;            /* privateOffset == 0 
                                       * if chamGlyphCount > 0, this
                                       * tells the size of the Chameleon
                                       * font descriptor
                                       */
    Card32    uniqueID;               /* uniqueID == 0 */
    Fixed     italicAngle;            /* italicAngleCount == 0 */
    Fixed     strokeWidth;            /* strokeWidthCount == 0 */
    Fixed     underlinePosition;      /* underlinePositionCount == 0 */
    Fixed     underlineThickness;     /* underlineThicknessCount == 0 */
    FCdBBox   fontBBox;               /* fontBBox.tr.x == fontBBox.tr.y == 0 */
    Card16    chamGlyphCount;         /* chamGlyphCount == 0,
                                       * if > 0, this is a Chameleon font
                                       */
    Card16    paintType;              /* paintTypeCount == 0 */
    Card16    charstringType;         /* charstringTypeCount == 0 */
    cffSID    postscript;             /* postscript == CFF_INVALID_SID */
    cffSID    version;                /* version == CFF_INVALID_SID */
    cffSID    notice;                 /* notice == CFF_INVALID_SID */
    cffSID    copyright;              /* copyright == CFF_INVALID_SID */
    cffSID    fullName;               /* fullName == CFF_INVALID_SID */
    cffSID    familyName;             /* familyName == CFF_INVALID_SID */
    cffSID    weight;                 /* weight == CFF_INVALID_SID */
    cffSID    baseFontName;           /* baseFontName == CFF_INVALID_SID */
    ArrayInfo charStrings;            /* charStrings.nEntries == 0 */
    ArrayInfo charset;                /* charset.offsetSize == 0 
                                       * (note that the fields in this structure
                                       * are used differently than they are for
                                       * an index)
                                       */
    boolean   isFixedPitch;           /* isFixedPitchCount == 0 */

    /* this information is in non-Chameleon private dictionaries, 
     * but it is needed by clients.  it will be filled out when the
     * private dictionary is parsed.
     */
    struct {
        Int32     lenIV;          /* lenIVCount == 0 */
        Fixed     hBaselineShift; /* hBaselineShiftCount == 0 */
        ArrayInfo localSubr;      /* localSubr.nEntries == 0 */
        Card8     lenIVCount;
        Card8     hBaselineShiftCount;
    } privInfo;

    struct {
        Int32     syntheticBase;          /* syntheticBase == -1 */

        /* offset to the base font's private dictionary */
        Card32    privateOffset;          /* privateOffset == 0 */
        Card32    privateSize;            /* privateOffset == 0 */
    } synthetic;

    /* multiple master font entries (valid if lenTransientArray > 0) */
    struct {
        Fixed  baseFontBlend[MAXAXES];  /* baseFontBlendLen == 0
                                         * udv of snapshotted font 
                                         */
        Card16 lenTransientArray;       /* lenTransientArray == 0 */
        cffSID blendAxisTypes[MAXAXES]; /* blendAxisTypesLen == 0 */
        cffSID ndv;                     /* ndv == CFF_INVALID_SID */
        cffSID cdv;                     /* cdv == CFF_INVALID_SID */
        Card8  baseFontBlendLen; 
        Card8  blendAxisTypesLen; 
        Card8 numMasters;               /* lenTransientArray == 0 */
    } mm;

    /* cid font entries (valid if registry != CFF_INVALID_SID) */
    struct {
        FCd       vToHOrigin;      /* vToHOriginCount == 0 */
        Fixed     defaultWidthY;   /* defaultWidthYCount == 0 */
        Card32    uidBase;         /* uidBaseCount == 0 */
        Card32    cidCount;        /* cidCountCount = 0 */
        Fixed     cidFontVersion;  /* cidFontVersionCount == 0 */
        Fixed     cidFontRevision; /* cidFontRevisionCount == 0 */
        Card16    cidFontType;     /* cidFontTypeCount == 0 */
        cffSID    registry;        /* registry == CFF_INVALID_SID */
        cffSID    ordering;        /* ordering == CFF_INVALID_SID */
        Card16    supplement;      /* registry == CFF_INVALID_SID */
        Card16    fdindex;         /* fdindex this structure corresponds to */
        cffSID    fontName;        /* fontName == CFF_INVALID_SID */

        ArrayInfo fdarray;         /* fdarray.nEntries == 0 */
        ArrayInfo fdSelect;        /* fdSelect.offOffsetArray == 0 
                                    * (note that the fields in this structure
                                    * are used differently than they are for
                                    * an index)
                                    */

        Card8     uidBaseCount;
        Card8     defaultWidthYCount;
        Card8     vToHOriginCount;
        Card8     cidCountCount;
        Card8     cidFontVersionCount;
        Card8     cidFontRevisionCount;
        Card8     cidFontTypeCount;

    } cid;

    Card8     charstringTypeCount;
    Card8     encodingOffsetCount;
    Card8     fontMatrixCount;
    Card8     isFixedPitchCount;
    Card8     italicAngleCount;
    Card8     paintTypeCount;
    Card8     strokeWidthCount;
    Card8     underlinePositionCount;
    Card8     underlineThicknessCount;
    Card8     xuidlen;

} CFFFontInfo, *PCFFFontInfo;

#endif  /* CFF_STRUCT_H */
