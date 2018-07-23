/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef ABSFONT_H
#define ABSFONT_H

#include "ctlshare.h"
#include "dynarr.h"
#include "txops.h"

#define ABF_VERSION CTL_MAKE_VERSION(1,0,53)

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Abstract Font Library
   =====================
   This library provides an abstract representation of a font that is suitable
   for describing Type 1, CID-keyed, TrueType fonts. Global font information is
   specified by a number of "dict" structures that loosely correspond to
   PostScript font dictionaries. Glyph information is specified by a set of
   callbacks that convey glyph data between various CoreType libraries. */

#define ABF_EMPTY_ARRAY 0
#define ABF_UNSET_PTR   0
#define ABF_UNSET_INT   -1
#define ABF_UNSET_REAL  -1.0

/* These macros specify initial field values that may later be distinguished
   from explicitly set values. */

typedef struct abfFontDict_ abfFontDict;
typedef struct                  /* FontMatrix */
    {
    long cnt;                   /* ABF_EMPTY_ARRAY */
    float array[6];         
    } abfFontMatrix;

typedef struct                  /* String */
    {
    char *ptr;                  /* ABF_UNSET_PTR */
    long impl;                  /* ABF_UNSET_INT */
    } abfString;

/* Textual data is represented by the abfString type which contains a
   null-terminated C string pointer field (ptr), and an
   implementation-specific numeric field (impl).

   The CoreType libraries always use the ptr field externally and only make use
   of the impl field for internal implementation purposes. A client may use the
   impl field as they see fit since it will always be reinitialized by a
   library that makes use of it. */

enum                            /* srcFontType values */
    {
    abfSrcFontTypeType1Name,
    abfSrcFontTypeType1CID,
    abfSrcFontTypeCFFName,
    abfSrcFontTypeCFFCID,
    abfSrcFontTypeSVGName,
    abfSrcFontTypeUFOName,
    abfSrcFontTypeTrueType
    };

typedef struct
    {
    long flags;                 /* 0 */
#define ABF_CID_FONT    (1<<0)  /* CID-keyed font */
#define ABF_SYN_FONT    (1<<1)  /* Synthethic font */
#define ABF_SING_FONT   (1<<2)  /* SING glyphlet font */
    long srcFontType;           /* ABF_UNSET_INT */
    char *filename;             /* ABF_UNSET_PTR */
    long UnitsPerEm;            /* 1000 */
    long nGlyphs;               /* ABF_UNSET_INT */
    } abfSupplement;

/* Supplementary data that has not traditionally been stored in font outline
   data is specified in the abfSupplement data structure. The "flags" field
   will be consistently set and used by all libraries. 

   The remaining supplementary fields are primarily intended for implementation
   use and will only be set if a library can do so reliably. Accordingly,
   clients shouldn't rely on these values but are free to use these fields as
   they see fit.

   CID-keyed fonts are indicated by setting the ABF_CID_FONT flag bit to 1
   and specifying CID-specific data via the abfTopDict.cid fields.

   Synthetic fonts are indicated by setting the ABF_SYN_FONT flag bit to 1
   and the SynBaseFontName field records the FontName of the font that the
   synthetic variant is based on. */

enum                            /* OrigFontType field values */
    {
    abfOrigFontTypeType1,
    abfOrigFontTypeCID,
    abfOrigFontTypeTrueType,
    abfOrigFontTypeOCF,
    abfOrigFontTypeUFO
    };

/* item variation store  */
struct var_itemVariationStore_;
typedef struct var_itemVariationStore_ *var_itemVariationStore;
    

/* Top dictionary data. Comments indicate default/initial values. */
typedef struct
    {                           
    abfString version;          /* Unset */
    abfString Notice;           /* Unset */
    abfString Copyright;        /* Unset */
    abfString FullName;         /* Unset */
    abfString FamilyName;       /* Unset */
    abfString Weight;           /* Unset */
    long isFixedPitch;          /* 0=false */
    float ItalicAngle;          /* 0. */
    float UnderlinePosition;    /* -100. */
    float UnderlineThickness;   /* 50. */
    long UniqueID;              /* ABF_UNSET_INT */
    float FontBBox[4];          /* 0. 0. 0. 0. */
    float StrokeWidth;          /* 0. */
    struct
        {
        long cnt;               /* ABF_EMPTY_ARRAY */
        long array[16];         
        } XUID;
    abfString PostScript;       /* Unset (CFF PostScript op value) */
    abfString BaseFontName;     /* Unset (Acrobat) */
    struct
        {
        long cnt;               /* ABF_EMPTY_ARRAY */
        long array[15];
        } BaseFontBlend;
    long FSType;                /* ABF_UNSET_INT */
    long OrigFontType;          /* ABF_UNSET_INT */
    long WasEmbedded;           /* 0=false */
    abfString SynBaseFontName;  /* Unset */
    struct                      /* CID-specific extensions */
        {
        abfFontMatrix FontMatrix;/* 1. 0. 0. 1. 0. 0. */
        abfString CIDFontName;  /* Unset */
        abfString Registry;     /* Unset */
        abfString Ordering;     /* Unset */
        long Supplement;        /* ABF_UNSET_INT */
        float CIDFontVersion;   /* 0. */
        long CIDFontRevision;   /* 0 */
        long CIDCount;          /* 8720 */
        long UIDBase;           /* ABF_UNSET_INT */
        } cid;
        struct                      /* Font dict array */
        {
            long cnt;               /* Value set by client to match array */
            abfFontDict *array;     /* Array allocated by client to match cnt */
        } FDArray;
        abfSupplement sup;          /* Supplementary data */
        unsigned short maxstack;     /* Supplementary data */
        var_itemVariationStore varStore;
    } abfTopDict;

/* Private dictionary data. Comments indicate default/initial values. */

    typedef struct
    {
        /* Used to hold stack items from a font with blended values */
        float value;
        int numBlends;
        float* blendValues;
    } abfOpEntry;
    
    typedef struct
    {
        /* Used to hold stack items from a font with blended values */
        float value;
        int     hasBlend;
        float blendValues[CFF2_MAX_OP_STACK];
    } abfBlendArg;
    
    
typedef struct
    {
        long cnt;               /* ABF_EMPTY_ARRAY */
        abfOpEntry array[CFF2_MAX_OP_STACK];
    } abfOpEntryArray;

typedef struct
    {
    /* Note that even for a font with blended values,
     the non-blended values are always set, using the values from the default font.
     */
    struct
        {
        long cnt;               /* ABF_EMPTY_ARRAY */
        float array[96];
        } BlueValues;
    struct
        {
        long cnt;               /* ABF_EMPTY_ARRAY */
        float array[96];
        } OtherBlues;
    struct
        {
        long cnt;               /* ABF_EMPTY_ARRAY */
        float array[96];
        } FamilyBlues;
    struct
        {
        long cnt;               /* ABF_EMPTY_ARRAY */
        float array[96];
        } FamilyOtherBlues;
    float BlueScale;            /* 0.039625 */
    float BlueShift;            /* 7 */
    float BlueFuzz;             /* 1 */
    float StdHW;                /* ABF_UNSET_REAL */
    float StdVW;                /* ABF_UNSET_REAL */
    struct
    {
        long cnt;               /* ABF_EMPTY_ARRAY */
        float array[96];
    } StemSnapH;
    struct
    {
        long cnt;               /* ABF_EMPTY_ARRAY */
        float array[96];
    } StemSnapV;
    long ForceBold;             /* 0=false */
    long LanguageGroup;         /* 0 */
    float ExpansionFactor;      /* 0.06 */
    float initialRandomSeed;    /* 0. */
    unsigned short vsindex;
    int numRegions;
    struct
    {
        abfOpEntryArray  BlueValues;
        abfOpEntryArray  OtherBlues;
        abfOpEntryArray FamilyBlues;
        abfOpEntryArray FamilyOtherBlues;
        abfOpEntry BlueScale;
        abfOpEntry BlueShift;
        abfOpEntry BlueFuzz;
        abfOpEntry StdHW;
        abfOpEntry StdVW;
        abfOpEntryArray StemSnapH;
        abfOpEntryArray StemSnapV;
   } blendValues;
    } abfPrivateDict;

/* Font dictionary data. Comments indicate default/initial values. */
struct abfFontDict_
    {                           
    abfString FontName;         /* Unset (top level in name-keyed) */
    long PaintType;             /* 0 (top level in name-keyed) */
    abfFontMatrix FontMatrix;   /* .001 0. 0. .001 0. 0. (top in name-keyed) */
    abfPrivateDict Private;     
    };              

/* The data structures described here represent Postscript dictionary data for
   name-keyed (traditional Type 1) and CID-keyed fonts. The PostScript
   representation is translated to a C language representation as follows:

   PostScript name-keyed        C-struct name-keyed
   ---------------------        -------------------
   font dict                    abfTopDict fields (excluding cid fields)
                                FDArray[0].FontName
                                FDArray[0].PaintType
                                FDArray[0].FontMatrix
       Private dict                 FDArray[0].Private

   PostScript CID-keyed         C-struct CID-keyed
   --------------------         ------------------
   CIDFont dict                 abfTopDict fields (including cid fields)
       FDArray[0]                   FDArray[0]
           font dict
               Private dict
       FDArray[1]                   FDArray[1]
           font dict
               Private dict
           ...
       FDArray[N-1]                 FDArray[N-1]
           font dict
               Private dict
           
   It can be seen that there is a direct mapping for CID-keyed fonts but the
   name-keyed fonts are represented using a single element FDArray whose font
   dict fields are treated as an extension of the font dict. */
                                
void abfInitAllDicts(abfTopDict *top);
void abfInitTopDict(abfTopDict *top);
void abfInitFontDict(abfFontDict *font);

/* The client is responsible for allocating the font dictionary data structures
   in memory and correctly initializing the abfTopDict.FDArray.cnt fields to
   specify the number of element allocated in the abfTopDict.FDArray.array.
   This will be 1 for name-keyed fonts and >=1 (the number being dependent on
   the complexity of the font) for CID-keyed fonts. Once this has been done
   abfInitAllDicts() may be called to initialize the entire data structure to
   the values shown in the comments. Alternatively, the abfInitTopDict() and
   abfInitFontDict() may be called to selectively initialize the top dictionary
   or a single font dictionary, respectively.

   For example, the dictionary data structures for a name-keyed font may be
   created and initialized as follows (ignoring malloc failures!):

   abfTopDict *top = malloc(sizeof(abfTopDict));
   top->FDArray.cnt = 1;
   top->FDArray.array = malloc(top->FDArray.cnt * sizeof(abfFontDict));
   abfInitAllDicts(top);

   CID fonts may be similarly handled except that the font dictionary array
   will typically have more than one element. */

typedef struct abfErrCallbacks_ abfErrCallbacks;
struct abfErrCallbacks_
    {
    void *ctx;
    void (*report_error)(abfErrCallbacks *cb, int err_code, int iFD);
    };

void abfCheckAllDicts(abfErrCallbacks *cb, abfTopDict *top);

/* abfCheckAllDicts() is will check contents of all the dictionaries passed via
   the "top" parameter. Any errors arising during the analysis will be called
   back using the abfErrCallbacks structure passed via the "cb" parameter,
   which may be NULL to turn off reporting. The "ctx" field of this structure
   can be used to provide the callee with a context if one is needed, and the
   "report_error" field provides the address of the error callback function.
   This function provides the client context via the "ctx" parameter, the error
   code via the "err_code" parameter, and the index of the font dictionary via
   the "iFD" parameter. If the font dictionary index isn't applicable to the
   error or if the font only has a single font dictionary the "iFD" parameter
   is set to -1.

   Error codes are enumerated in the abferr.h file and may be converted to
   strings via abfErrStr() described below. */

int abfIsDefaultFontMatrix(const abfFontMatrix *FontMatrix);

/* abfIsDefaultMatrix() returns 1 if the "FontMatrix" parameter represents the
   default matrix (.001 0 0 .001 0 0) and returns 0 otherwise. */

/* --------------------------- Font Merge Support -------------------------- */

int abfCompareTopDicts(abfTopDict *top1, abfTopDict *top2);

/* abfCompareTopDicts() will compare two top dicts to see if they are similar
   enough to merge. It returns 0 if they can be merged else 1.
   
   Fields in the top dict are checked for name-keyed and cid-keyed fonts but
   the font and private dicts are compared for name-keyed fonts only.

   FontNames are NOT checked.
   
   For CID-keyed fonts, registry and order are checked, but not supplement. */

int abfCompareFontDicts(abfFontDict *font1, abfFontDict *font2);

/* abfCompareFontDicts() will compare two font dicts to see if they are
   similar enough to merge. It returns 0 if they can be merged else 1.
    
   abfCompareFontDicts() compares the FontName, FontMatrix, and all the
   BlueValues.
   
   abfCompareFontDicts() is used in the destination font writing module to
   compare FDArrays of fonts with more than one element in the FDArray,
   CID fonts. */

/* ---------------------------- Glyph Callbacks ---------------------------- */

/* Several libraries share the following glyph callback definitions and are
   used whenever glyph information is passed from one component to the next. */

typedef struct abfEncoding_ abfEncoding;
struct abfEncoding_             /* Encoding node */
    {
    abfEncoding *next;
    unsigned long code;
#define ABF_GLYPH_UNENC 0xffffffffUL    /* Unencoded glyph */
    };

typedef struct                  /* Glyph information */
    {
    short flags;                /* Attribute flags */
#define ABF_GLYPH_CID   (1<<0)  /* Glyph from CID-keyed font */
#define ABF_GLYPH_SEEN  (1<<1)  /* Path data already returned to client */
#define ABF_GLYPH_UNICODE (1<<2)/* Encoding is Unicode */
#define ABF_GLYPH_LANG_1 (1<<3) /* Render with LanguageGroup 1 rules */
#define ABF_GLYPH_CUBE_GSUBR (1<<4)		/* This a GSUBR charstring. */
    unsigned short tag;         /* Unique tag */
    abfString gname;            /* Name-keyed: glyph name */
    abfEncoding encoding;       /* Name-keyed: encoding list */
    unsigned short cid;         /* CID-keyed: CID */
    unsigned char iFD;          /* CID-keyed: FD index */
    ctlRegion sup;              /* Supplementary data */
    struct {
        unsigned short vsindex;
        unsigned short maxstack;
        unsigned short numRegions;
        float *blendDeltaArgs;
        } blendInfo;/* Supplementary data */
    } abfGlyphInfo;

/* The "flags" field specifies glyph attributes. Libraries may augment this
   definition by specifying additional bits (see specific library API for
   details).

   The "tag" field is used to identify the glyph being processed in a font-
   technology-specific way. The tag is incremented for each glyph processed,
   starting from zero. Thus, the last glyph processed is assigned a tag value
   that is one less than the number of glyphs in the font.

   For OTF or TTF fonts that tag is the same as the glyph index. For a
   naked-CID font it is derived by counting non-empty intervals in the CIDMap,
   starting from CID 0. Thus, in a full set CID-keyed font the tag and CID will
   be the same for each glyph. For a Type 1 font it is the order number of the
   charstring in the CharStrings dictionary, starting from 0.

   Warning: the tag values assigned to glyphs in a font may change if the font
   is updated or subset.

   If the font is CID-keyed the ABF_GLYPH_CID bit in the "flags" field is set
   and the "cid" and "iFD" fields specify the CID and FD index, respectively.

   If the font is name-keyed the ABF_GLYPH_CID bit is clear and the "gname" and
   "encoding" fields specify the glyph name and encoding, respectively. The
   glyph is unencoded if the "encoding.code" field is equal to ABF_GLYPH_UNENC.
   Otherwise, the "encoding" field specifies the first node in an encoding
   list. (Glyphs having more than one encoding (multiple encoding nodes) are
   rare.)

   All the glyphs for one font are either name-keyed or CID-keyed. 

   The ABF_GLYPH_SEEN bit in the flags field is set if the path data for this
   glyph has already been returned to the client. It is primarily intended to
   by used when fetching seac components in a font subset so as to avoid
   repeated glyph definitions. 

   The ABF_GLYPH_LANG_1 bit indicates that the glyph is rendered with
   LanguageGroup 1 rules. It is simply a copy of the value of the LanguageGroup
   field taken from the corresponding Private dictionary. It is used by the 
   t1write library in order to determine the encoding of counter stem hints. 

   The "sup" field specifies the stream region corresponding to the charstring
   data for the glyph. */

void abfInitGlyphInfo(abfGlyphInfo *info);

/* abfInitGlyphInfo() is provided for client convienience and initializes the
   "info" parameter to a standard safe state. */

typedef struct abfGlyphCallbacks_ abfGlyphCallbacks;

struct abfGlyphCallbacks_
    {
    void *direct_ctx;
    void *indirect_ctx;
    abfGlyphInfo *info;
    int (*beg)      (abfGlyphCallbacks *cb, abfGlyphInfo *info); 
    void (*width)   (abfGlyphCallbacks *cb, float hAdv);
    void (*move)    (abfGlyphCallbacks *cb, float x0, float y0);
    void (*line)    (abfGlyphCallbacks *cb, float x1, float y1);
    void (*curve)   (abfGlyphCallbacks *cb,
                     float x1, float y1, 
                     float x2, float y2, 
                     float x3, float y3);
    void (*stem)    (abfGlyphCallbacks *cb,
                     int flags, float edge0, float edge1);
    void (*flex)    (abfGlyphCallbacks *cb, float depth,
                     float x1, float y1, 
                     float x2, float y2, 
                     float x3, float y3,
                     float x4, float y4, 
                     float x5, float y5, 
                     float x6, float y6);
    void (*genop)   (abfGlyphCallbacks *cb, int cnt, float *args, int op);
    void (*seac)    (abfGlyphCallbacks *cb, 
                     float adx, float ady, int bchar, int achar);
	void (*end)     (abfGlyphCallbacks *cb);
	void (*cubeBlend)     (abfGlyphCallbacks *cb, unsigned int nBlends, unsigned int numVals, float* blendVals);
	void (*cubeSetwv)     (abfGlyphCallbacks *cb, unsigned int numDV);
	void (*cubeCompose)     (abfGlyphCallbacks *cb, int cubeLEIndex, float x0, float y0, int numDV, float* ndv);
							/* Note: ndv is the raw value from the CFF, a value in the range -100 to +100, which corresponds to the
							normalized design vector range 0.0-1.0.  x0 and y0 are the absolute x and y of the LE origin in the glyph design space.*/
	void (*cubeTransform)     (abfGlyphCallbacks *cb, float rotate, float scaleX, float scaleY, float skewX, float skewY);
    void (*moveVF)    (abfGlyphCallbacks *cb, abfBlendArg* x0, abfBlendArg* y0);
    void (*lineVF)    (abfGlyphCallbacks *cb, abfBlendArg* x1, abfBlendArg* y1);
    void (*curveVF)   (abfGlyphCallbacks *cb,
                     abfBlendArg* x1, abfBlendArg* y1,
                     abfBlendArg* x2, abfBlendArg* y2,
                     abfBlendArg* x3, abfBlendArg* y3);
    void (*stemVF)    (abfGlyphCallbacks *cb,
                     int flags, abfBlendArg* edge0, abfBlendArg* edge1);
    };

/* beg() is called to begin a new glyph definition and end() is called to
   terminate that definition. Between these bracketing calls the other
   callbacks may be called zero or more times. All beg() implementations are
   required to copy their "info" parameter to the "info" field of callback
   structure so that it is available to other callback functions.

   The width() callback is called with the horizontal advance width of the
   glyph. 

   Marking glyphs are drawn as an outline which is simply a sequence of
   coordinate points that describe a path (which may be empty for non-marking
   glyphs such as space). A path is composed of one or more subpaths that
   describe closed regions. Each subpath is composed of a straight and curved
   segments. (See TN#5177 for a detailed description of glyph path and hint
   specification.)

   Path data is specified by absolute coordinates in character space units.

   Each subpath must begin with a call to move() in order to specify the
   starting coordinate of the subpath via its parameters.

   line() and curve() extend the path with a line or bezier curve segments,
   respectively, that are drawn from last point on the last segment or from the
   point specified with move().

   The stem() and flex() callbacks are used to pass hinting information back to
   the client. These callbacks will typically be interleaved with the path
   construction callbacks described above. Note that the flex() callback
   specifies both path as well as hint information.

   stem() specifies the absolute edge positions of a hint stem. The "flags"
   parameter specifies stem attributes and additional hinting information as
   follows: */

enum
    {
    ABF_VERT_STEM   = 1<<0,
    ABF_CNTR_STEM   = 1<<1,
    ABF_STEM3_STEM  = 1<<2,
    ABF_NEW_HINTS   = 1<<3,
    ABF_NEW_GROUP   = 1<<4
    };

/* The ABF_VERT_STEM bit specifies a vertical stem direction if set and
   horizontal if clear.

   When the ABF_CNTR_STEM bit is set it specifies that the stem participates in
   global coloring counter (white space) control.

   When the ABF_STEM3_STEM bit is set it specifies that the stem participates
   in h/vstem3 counter control.

   When the ABF_NEW_HINTS bit is set it specifies that a hint substitution
   (previous hint map cleared) should be performed before the new stem is added
   to the hint map.

   When the ABF_NEW_GROUP bit is set it specifies that a new global coloring
   counter group should be started before the new stem is added.

   These bits may be combined as follows:

   ABF_VERT_STEM / ABF_NEW_HINTS                    h/vstem
   ABF_VERT_STEM / ABF_STEM3_STEM / ABF_NEW_HINTS   h/vstem3
   ABF_VERT_STEM / ABF_CNTR_STEM / ABF_NEW_GROUP    global coloring

   The calculation edge1-edge0 computes the width of the stem which MUST be
   positive except for two special cases which specify an edge (rather that
   stem) hint: */

enum
    {
    ABF_EDGE_HINT_LO    =-21,   /* Bottom/left edge hint delta */
    ABF_EDGE_HINT_HI    =-20    /* Top/right edge hint delta */
    };

/* Edge hints are not permitted for stems that set the ABF_CNTR_STEM bit.

   flex() describes two adjoining curves with coordinates 1-3 and 4-6 and the
   flex depth via the "depth" parameter. 

   The stem() and flex() callback functions may be disabled by setting them to
   NULL if hint information is of no interest to the client. In this case the
   two curve components of flex features will be called back to the client via
   two consecutive curve() callbacks.

   genop() permits general, non-path, non-hint operator to be passed to a
   client. The "cnt" parameter specifies the number of arguments passed via
   the "args" array. The "cnt" parameter may be 0. The "op" parameter specifies
   the operator type as defined in txops.h.

   The seac() function is used to specify a seac operator (see "Adobe Type 1
   Font Format", however, the asb operand in the book description has been
   combined with the adx operand in this function). This operator supports the
   specification a new glyph as a composite of two others. It is normally used
   to construct accented glyphs. The "bchar" and "achar" parameters specify
   names of the the base glyph and accent glyph via encodings in the standard
   encoding table, respectively. The "adx" and "ady" parameters specify the
   translation that should be applied to the accent glyph relative to the base
   glyph. */

enum                    /* Callback return values */
    {
    ABF_CONT_RET,
    ABF_WIDTH_RET,
    ABF_SKIP_RET,
    ABF_QUIT_RET,
    ABF_FAIL_RET
    };

/* A typical application of this callback set is to provide a link between a
   glyph data source like a font reading library (the caller), and a glyph data
   sink like a font writing library (the callee). The "direct_ctx" field can be
   used to provide the callee with a context if one is needed.

   The callee may control the caller by the value returned from the beg()
   callback. A return of ABF_WIDTH_RET will skip the remainder of the current
   glyph after the width() callback has been called. A return of ABF_SKIP_RET
   skips the remainder of the glyph definition for the current glyph (no
   further callbacks will be called for the current glyph). A return of
   ABF_QUIT_RET quits processing of the current font (no further callbacks will
   be called for the current or subsequent glyphs). A return of ABF_CONT_RET
   continues normal operation.

   Callee control via callback return values affords very flexible control of
   the information that is parsed by the caller. For example, the callee may
   subset the font by returning ABF_CONT_RET from the beg() callback only for
   glyphs in the subset and ABF_SKIP_RET otherwise. A important optimization
   the callee might make is to keep track of which glyphs have been subset and
   issue ABF_QUIT_RET when all subset glyphs have been seen. ABF_FAIL_RET is
   returned from the callee if a fatal error is encountered during glyph
   processing. */

typedef int (*abfGlyphBegCallback)(abfGlyphCallbacks *cb, abfGlyphInfo *info); 

/* Another possible configuration is to have a client provide the glue layer
   between the two libraries described above. Glyph subset filtering can then
   be implemented in this layer by defining a beg() function that calls the
   callee library's beg() function only for glyphs in a the subset. The
   "indirect_ctx" field is provided if this client layer needs its own context.
   The abfGlyphBegCallback typedef is provided as a convienence for clients
   that need to implement this approach. 

   The required components of a glyph callback sequence are: beg(), width(),
   and end(), in that order. The call to width() must imediately follow the
   call to beg(). Marking glyphs will also make calls to move(), line(), and
   curve(). Hinted glyphs may make calls the stem() and/or flex(). Finally,
   some glyphs may make calls to genop() and/or seac().

   Some of the facilities provided below or in other libraries may make an
   abfGlyphCallbacks set template available that the client may use to directly
   call the glyph callback functions. When such a template exist it is
   guaranteed to have valid function address for all the glyph callbacks even
   if the facility ignores some of the calls. Thus, a client doesn't need to
   check the function addresses against NULL before calling them. 

   The "info" field points to the "info" parameter that was passed to the beg()
   callback at the start of a glyph. The various beg() callback implementations
   are required to copy this value when they are called. */

/* ------------------------ Abstract Font Descriptor ----------------------- */

#include "abfdesc.h"

typedef struct
    {
    long CharstringType;
    long lenSubrArray;
    float defaultWidthX;
    float nominalWidthX;
    } abfFontDescSpecialValues;

typedef struct abfFontDescCallbacks_ abfFontDescCallbacks;
struct abfFontDescCallbacks_
    {
    void *ctx; 
    void (*getSpecialValues)(abfFontDescCallbacks *cb, int iFD, 
                             abfFontDescSpecialValues *sv);
    };

abfFontDescHeader *abfNewFontDesc(ctlMemoryCallbacks *mem_cb, 
                                  abfFontDescCallbacks *desc_cb,
                                  long lenGSubrArray,
                                  abfTopDict *top);

void abfFreeFontDesc(ctlMemoryCallbacks *mem_cb, abfFontDescHeader *hdr);

/* abfNewFontDesc() makes a font descriptor from the information supplied via
   its parameters that is suitable for use with the abfSetUpValues() function
   in the buildch library. Most of the imformation needed to do this is
   obtained via "top" parameter which would normally be obtained by calling one
   of the BegFont() functions in the font parsing libraries.

   However, additional non-abstract information is also required that is
   obtained using the callbacks passed via the "desc_cb" parameter. The
   getSpecialValues() callback is called several times for each font dict. This
   callback must supply values for the fields in the abfFontDescSpecialValues
   struct for the font dict specified by the "iFD" parameter.

   The "CharstringType" field must have a value of 1 or 2 indicating Type 1 or
   Type 2 charstrings, respectively. The "lenSubrArray" field contines the
   count of the local subroutines or 0 if there are none. The "defaultWidthX"
   and "nominalWidthX" fields contain the values of associated with the
   corresponding operators in the CFF Private DICT or 0 if the operators are
   not present in the font.

   The count of the global subroutines in the font is passed via the
   "lenGSubrArray" parameter which must be set to 0 if there are none.

   The client passes a set of memory callback functions (described in
   ctlshare.h) to the library via "mem_cb" parameter. These are used to
   allocate a single block of memory for the font descriptor which is returned
   by the function. The client should free this memory when the font descriptor
   is no longer needed by calling abfFreeFontDesc().

   abfNewFontDesc() returns a NULL pointer if the font descriptor could not be
   created. */

float *abfGetFontDescMatrix(abfFontDescHeader *hdr, int iFD);

/* abfGetFontDescMatrix() returns a pointer to the 6-element FontMatrix array
   of values. The "iFD" parameter selects the element from which to extract the
   FontMatrix from the font descriptor. NULL is returned if either the element
   didn't contain a FontMatrix (because it matched the default) or the "iFD"
   parameter was invalid. */

/* ------------------------- Glyph Metrics Facility ------------------------ */

/* The bounding box and width metrics for a glyph may be determined using the
   interface defined in this section. */

typedef struct abfMetricsCtx_ *abfMetricsCtx;
struct abfMetricsCtx_
    {
    long flags;         /* Control flags */
#define ABF_MTX_TRANSFORM   (1<<0) /* Apply matrix to path */
    float matrix[6];    /* Transformation matrix */
    int err_code;        /* Result code */
    struct              /* Real glyph metrics */
        {
        float left;
        float bottom;
        float right;
        float top;
        float hAdv;
        } real_mtx;
    struct              /* Integer glyph metrics */
        {
        long left;
        long bottom;
        long right;
        long top;
        long hAdv;
        } int_mtx;
    float x;            /* Last segment x end-point */
    float y;            /* Last segment y end-point */
    };

/* abfMetricsCtx is an abstract font metrics context that is created by the
   client and used by the facility to keep intermediate data and return the
   results to the client.

   The "flags" field (which must be initialized) allows the client control of
   the facility by setting bits as follows:

   ABF_MTX_TRANSFORM - transform path using the "matrix" field. The client must
   intialize the "matrix" field appropriately when using this option. */

extern const abfGlyphCallbacks abfGlyphMetricsCallbacks;

/* abfGlyphMetricsCallbacks is a glyph callback set template that will compute
   bounding box and horizontal advance width metrics for data passed to these
   callbacks. The facilty may be used as follows:

   struct abfMetricsCtx_ ctx;
   abfGlyphCallbacks cb = abfGlyphMetricsCallbacks; // copy template

   ctx.flags = 0;           // or set ABF_MTX_TRANSFORM and ctx.matrix
   cb.direct_ctx = &ctx;    // set metrics context

   // parse charstring and call cb functions, e.g. with t1cParse()
   // result now in metrics context

   if (ctx.err_code != abfSuccess)
       // handle error
   else
       // real glyph metrics available in ctx.real_mtx
       // integer glyph metrics available in ctx.int_mtx

*/

/* --------------------------- Font Dump Support --------------------------- */

/* An abstract font may be dumped in a human-readable text format using the
   interface defined in this section. */

typedef struct abfDumpCtx_ *abfDumpCtx;
struct abfDumpCtx_
    {
    FILE *fp;       /* Output stream */
    int flags;      /* Control flags */
    int level;      /* Dump level */
    int left;       /* Columns left in line (characters) */
    int excludeSubset;
    int fdCnt;         /* Index of font dict to dump or -1 for all dicts */
    int *fdArray;         /* Index of font dict to dump or -1 for all dicts */
    };
    
/* abfDumpCtx is the abstract font dumping context. It must be created by the
   client and then initialized appropriately before each dump. It is
   subsequently passed to abfDumpBegFont() and used to set the "direct_ctx"
   field in the abfGlyphCallbacks structure.
   
   The "fp" field specifies an open stream to which the dump data will be
   written. 

   The "flags" field is for internal use only.

   The "level" field may be used to set the reporting level for font data:

   0    dict data
   1    dict data; glyph data (very short)
   2    dict data; glyph data (short)
   3    dict data; glyph data (long)
   4               glyph data (very short)
   5               glyph data (short)
   6               glyph data (long)

   The "left" field is for internal use only.

   The "fd" field specifies the index of the font dict to dump. If you want to
   dump all dicts set this field to -1. */

void abfDumpBegFont(abfDumpCtx h, abfTopDict *top);

/* abfDumpFont() dumps the dictionaries pointed to by the "top" parameter. */

extern const abfGlyphCallbacks abfGlyphDumpCallbacks;

/* abfDumpGlyphCallbacks is a glyph callback set template that will dump the
   data passed to these callbacks when they are called by the client using the
   level previously set in the context. Clients should make a copy of this data
   structure and set "direct_ctx" field to the context they created and
   initialized. */

/* --------------------------- Font Draw Support --------------------------- */

/* An abstract font may be printed to a PostScript using the interface defined
   in this section. */

typedef struct abfDrawCtx_ *abfDrawCtx;
struct abfDrawCtx_
    {
    long flags;         /* Control flags */
#define ABF_FLIP_TICS   (1<<0)  /* Flip coord tics to otherside of path */
#define ABF_SHOW_BY_ENC (1<<1)  /* Show by 8-bit encoding */
#define ABF_NO_LABELS   (1<<2)  /* Don't show numeric labels */
#define ABF_DUPLEX_PRINT (1<<3) /* Prepend duplex print enable */
    FILE *fp;           /* Output stream */
    int level;          /* Draw level */
    int showglyph;      /* Show the current glyph */
    struct              /* Metric data */
        {
        struct abfMetricsCtx_ ctx;
        abfGlyphCallbacks cb;
        } metrics;
    struct              /* Tile mode data */
        {
        int h;          /* h origin */
        int v;          /* v origin */
        } tile;
    struct              /* Glyph data */
        {
        float hAdv; 
        float scale;    /* Glyph drawing scale factor (points/unit) */
        } glyph;
    struct              /* Path data */
        {
        float bx;       /* Last point */
        float by;
        float cx;       /* Last but one point */
        float cy;
        float fx;       /* First point in path */
        float fy;
        float sx;       /* Second point in path */
        float sy;
        int cnt;        /* Point count */
        int moves;
        int lines;
        int curves;
        } path;
    int pageno;         /* Page number */
    abfTopDict *top;
    };
    
/* abfDrawCtx is the abstract font drawing context. It must be created by the
   client and then initialized appropriately before each font is processed. It
   is subsequently passed to abfDrawBegFont() and used to set the "direct_ctx"
   field in the abfGlyphCallbacks structure.
   xxx more */

void abfDrawBegFont(abfDrawCtx h, abfTopDict *top);
void abfDrawEndFont(abfDrawCtx h);

/* abfDrawFont() draws the dictionaries pointed to by the "top" parameter. */

extern const abfGlyphCallbacks abfGlyphDrawCallbacks;

/* abfDrawGlyphCallbacks is a glyph callback set template that will draw the
   data passed to these callbacks when they are called by the client using the
   level previously set in the context. Clients should make a copy of this data
   structure and set "direct_ctx" field to the context they created and
   initialized. */

/* ------------------------------ AFM Support ------------------------------ */

/* A simple Adobe Font Metrics (AFM) file may be generated from an abstract
   font using the interfaces described in this section. This facility is
   primarily intended for checking and debugging purposes. It should not be
   used as a means of generating production quality AFM files since conformance
   to the AFM specification (TN#5004) is not guaranteed. */

typedef struct abfAFMCtx_ *abfAFMCtx;
struct abfAFMCtx_
    {
    FILE *fp;           /* Output stream */
    int err_code;
    struct              /* Metric data */
        {
        struct abfMetricsCtx_ ctx;
        abfGlyphCallbacks cb;
        } metrics;
    };
    
void abfAFMBegFont(abfAFMCtx h, abfTopDict *top);
void abfAFMEndFont(abfAFMCtx h);

extern const abfGlyphCallbacks abfGlyphAFMCallbacks;

/* ----------------------------- Path Support ------------------------------ */

/* Glyph outlines may be manipulated using the functions defined in this
   section. */

typedef struct abfCtx_ *abfCtx;
abfCtx abfNew(ctlMemoryCallbacks *mem_cb, CTL_CHECK_ARGS_DCL);

#define ABF_CHECK_ARGS CTL_CHECK_ARGS_CALL(ABF_VERSION)

/* abfNew() initializes the path component of the abstract font library and
   returns an opaque context (abfCtx) that is subsequently passed to all the
   other library functions (or NULL if the library cannot be initialized). It
   must be the first function in this section of the library to be called by
   the client. The client passes a set of memory callback functions (described
   in ctlshare.h) to the library via "mem_cb" parameter.

   The ABF_CHECK_ARGS macro is passed as the last parameter to abfNew() in
   order to perform a client/library compatibility check. */

int abfBegFont(abfCtx h, abfTopDict *top);

/* abfBegFont() is called to initialize a new font. */

int abfEndFont(abfCtx h, long flags, abfGlyphCallbacks *glyph_cb);

enum                                /* abfEndFont(flags) bits */
    {
    ABF_PATH_REMOVE_OVERLAP = 1<<0  /* Remove overlapping paths */
    };

extern const abfGlyphCallbacks abfGlyphPathCallbacks;

/* abfEndFont() is called to end the new font that was begun with a call to
   abfBegFont(). Between these two calls one or more glyphs may be accumulated
   by calling the glyph callbacks (see above for description) defined in
   abfGlyphPathCallbacks.

   The action of calling abfEndFont() causes the accumulated glyphs to be
   processed according to the action specified via the "flags" parameter. After
   the glyphs have been processed, a new (possibly unmodified) path is then
   returned to the client using another set of glyph callbacks passed via the
   "glyph_cb" parameter. */

int abfFree(abfCtx h);

/* abfFree() destroys the library context and all the resources allocated to
   it. */

enum
    {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)    name,
#include "abferr.h"
    abfErrCount
    };

/* Library functions return either zero (abfSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from abferr.h. */

char *abfErrStr(int err_code);

/* abfErrStr() maps the "errcode" parameter to a null-terminated error 
   string. */

void abfGetVersion(ctlVersionCallbacks *cb);

/* abfGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#ifdef __cplusplus
}
#endif

#endif /* ABSFONT_H */
