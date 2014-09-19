/* @(#)CM_VerSion cff_parse.h atm09 1.2 16563.eco sum= 65534 atm09.004 */
/* @(#)CM_VerSion cff_parse.h atm08 1.5 16390.eco sum= 11493 atm08.007 */
/* @(#)CM_VerSion cff_parse.h atm07 1.11 16164.eco sum= 16700 atm07.012 */
/*
  cff_parse.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 * NOTE TO MAINTAINERS: The cff_parse package originates in the atm
 * world, which is periodically copied to the 2ps world; therefore,
 * bugs in cff_parse should be posted to the 'buildch' database not
 * '2ps', and all code submissions should be to the atm world, not to
 * any 2ps world.
 */

#ifndef CFF_PARSE_H
#define CFF_PARSE_H

/*
 * cff_parse api
 * --------------
 * This package may be used to parse the various parts of
 * a cff fontset. The primary structures used for parsing
 * are CFFSetInfo (which holds information relevant to an entire
 * fontset) and CFFFontInfo (which holds information for one particular
 * font).  Both structures are defined in buildch/cff_struct.h
 * The CFFFontInfo structure primarily contains information
 * from the top and font dictionaries.  If it is not a Chameleon
 * font, it also contains some information from the private dictionary.
 * Since most of the private dictionary entries are only interesting
 * to the rasterizer, most of the entries are not stored anywhere that is
 * client-visible.
 * 
 * If a client wants Chameleon support, it should call
 * ChamInit one time.  Support for Chameleon will then be enabled.
 * However, the chameleon code currently only compiles in the PostScript
 * environment.
 * 
 * The client can call CFFGet_FontSet to find the portions of the
 * fontset that are shared by all of the fonts in the fontset:
 * the font name index, the font dictionary index, the global
 * subroutine index and the string index.  Each call to CFFGet_FontSet
 * must be paired with a call to CFFRelease_FontSet when the client
 * is finished with the CFFSetInfo structure (unless CFFGet_FontSet
 * raises an exception, in which case CFFRelease_FontSet should not
 * be called).
 *
 * Once CFFGet_FontSet has been called, individual elements of the name index 
 * may be read by calling CFFGet_FontName; individual elements of the string 
 * index may be read by calling CFFGet_String; and whole portions of the 
 * global subroutine index may be read by calling CFFGet_IndexedArray. If the
 * client reads in a large portion of an index with CFFGet_IndexedArray,
 * it may wish to free this memory independently of other operations
 * on the font.  It may do so by calling CFFRelease_IndexedArray.
 *
 * The client can call CFFInitialize_FontInfo
 * to put default values into a CFFFontInfo structure.  Each call
 * to CFFInitialize_FontInfo must be paired with a call to 
 * CFFRelease_FontInfo when the client is finished with the CFFFontInfo
 * structure (unless CFFInitialize_FontInfo raises an exception, in
 * which case CFFRelease_FontInfo should not be called).
 *
 * Once the client has called CFFGet_FontSet and CFFInitialize_FontInfo, 
 * it can parse a top dictionary by calling CFFEnumerateDict.  
 * CFFEnumerateDict will only recognize (and act upon) those operators 
 * which the CFF specification lists as top dictionary operators.  
 * 
 * Once CFFEnumerateDict has been called for the top dictionary, the
 * client can read the font's encoding via CFFGet_Encoding.  
 *
 * Also, after calling CFFEnumerateDict, the client can perform
 * three operations to view the charset.  CFFCheckForIdentityCharset
 * allows the client to see if the charset is simply the identity
 * mapping.  If so, the client would have no need to look at the
 * charset again.  Also, the client can retrieve
 * all of the charset via CFFGet_Charset, which should be followed
 * by CFFRelease_Charset when the client is finished with the charset
 * (unless CFFGet_Charset raises an exception, in which case 
 * CFFRelease_Charset should not be called).
 * If the client believes that the charset is too big to be retrieved at
 * one time, it may retrieve individual elements of the charset
 * by calling CFFGet_CharsetElement. 
 * charstrings by calling CFFGet_IndexedArray.  For Chameleon charstrings,
 * the client must additionally call SetUpValues before calling
 * CFFGet_IndexedArray.
 *
 * Once SetUpValues has been called for the font instance, the client
 * can look up local subroutines by calling CFFGet_IndexedArray.
 *
 * In the case of a CID font, the client can retrieve the font dictionary
 * index for a given glyph by calling CFFGet_FDElement.  The client can 
 * then parse the appropriate font dictionary by again calling
 * CFFEnumerateDict.
 *
 * See ATMInitBC in buildch/buildch.h for information on how the
 * private dictionary gets parsed for these fonts.
 */

#if defined PSENVIRONMENT && PSENVIRONMENT
/* PostScript headers */
#include PACKAGE_SPECS
#include CFF_STRUCT
#else  /* PSENVIRONMENT */
/* standard coretype environment */
#include "cff_struct.h"
#endif /* PSENVIRONMENT */

/* Predefined charsets (cff_charset operands) */
#define cff_ISOAdobeCharset         0
#define cff_ExpertCharset           1
#define cff_ExpertSubsetCharset     2

/* Predefined encodings (cff_Encoding operands) */
#define cff_StandardEncoding        0
#define cff_ExpertEncoding          1
   
typedef CardX CFFResult;

/* the possible values for CFFResult: */
#define CFFE_NOERR       0     /* No error */
#define CFFE_READ        1     /* font data not accessible. */
#define CFFE_CALLER      2     /* Callback proc reported problem */
#define CFFE_BADFILE     3     /* Found something unexpected in font file */
#define CFFE_CANTHAPPEN  4     /* assertion failure---internal problem */
#define CFFE_BADVERSION  5     /* CFF version information was inconsistent */
#define CFFE_BADARG      6     /* bad argument from client */
#define CFFE_MEMORY      7     /* could not allocate memory */

/* Exceptions */

#define ecInvalidFont (ecComponentPrivBase + 0)
/* This is the only exception raised directly by any of the functions
   in the chameleon interface. However, the client call-back functions can
   raise other errors (e.g., ecLimitCheck), which propagate through.
 */

typedef enum {
    TopDict = 1,
    CIDFontDict
} DictID;

/* CFFGet_FontSet() reads the header info and the offset info about the
 * name, dictionary, string and global subr indices.  
 */
extern CFFResult CFFGet_FontSet(PCFFSetInfo pSetInfo);

/* CFFRelease_FontSet cleans up memory allocated by CFFGet_FontSet */
extern CFFResult CFFRelease_FontSet(PCFFSetInfo pSetInfo);

/* CFFInitialize_FontInfo fills pFontInfo with default values.  */
extern void CFFInitialize_FontInfo(
    PCFFSetInfo pSetInfo,    /* (R) returned from CFFGet_FontSet */
    PCFFFontInfo pFontInfo); /* (W) will be filled with defaults */

/* CFFRelease_FontInfo cleans up memory allocated by CFFInitialize_FontInfo */
extern CFFResult CFFRelease_FontInfo(
    PCFFFontInfo pFontInfo
);

/* CFFEnumerateDict parses the top (dicttype == TopDict) or font 
 * dictionary (dicttype == CIDFontDict) into a CFFFontInfo 
 * structure.  It will interpret t2 charstrings 
 * encountered in mm fonts.  It parses the dictionary of
 * the base font if the font is synthetic. If a base font
 * contains a SyntheticBase operator, it ignores it.  It
 * only alters fields in the CFFFontInfo structure that 
 * have associated operators in the font, and it only recognizes
 * operators that are identified as top dictionary operators
 * in the CFF specification.
 *
 * The xuid in the returned CFFFontInfo is the one in the font.
 * 
 * If a fontmatrix is found in both the top dict and the font dict of
 * a cid font, the second fontMatrix is multiplied with the
 * first. If the NT_KERNEL_NO_FPU flag is set at
 * compile time, the elements of the fontMatrix are stored in Frac's
 * instead of floating point numbers.  This way all floating point
 * operations are avoided. This conversion to fixed point will obviously
 * result in some loss of precision.  Please note that both buildch and
 * cff_parse use the cff_parse.h interface, so both packages will need
 * to be compiled with this flag in order for things to work correctly.
 *
 * If the paintType field has been filled in the pFontInfo, subsequent
 * paintType operators are ignored.
 *
 * The Registrant type is defined in buildch.h.  The client
 * needs to define Registrant registry[REGISTRY_ITEMS].  registry[REG_WV]
 * is a place to hold the weightvector for this instance.  
 * registry[REG_UDV] is a place to hold the user design vector for
 * this instance.  registry[REG_NDV] is a place to hold the normalized
 * design vector. If any of these values is 
 * unknown, set its count to 0, but registry[*].data must 
 * point to valid space (excepting registry[REG_STACKCONTENTS].data,
 * which will be allocated by cff_parse if needed).  If 
 * registry[REG_WV].count > 0, it assumes
 * the desired weightvector is being sent so it leaves registry alone.
 * Otherwise, CFFEnumerateDict uses registry to compute a weightvector,
 * which it returns in registry[REG_WV].  If all the counts are 0, 
 * CFFEnumerateDict uses the default udv to compute the weightvector.
 *
 * Typically, the same pFontInfo structure will be used when the
 * client is parsing the TopDict and when the client is parsing
 * the CIDFontDict.  If this is not the case, however, the client
 * must call CFFInitialize_FontInfo for each pFontInfo.  After calling
 * CFFInitialize_FontInfo for the CIDFontDict pFontInfo, the client must 
 * copy cid.fdarray from the TopDict pFontInfo to the CIDFontDict pFontInfo.
 * The client will have to merge the two pFontInfo's before
 * calling SetUpValues.
 */
extern CFFResult CFFEnumerateDict(
      PCFFFontInfo pFontInfo,  /* (W) values found in dictionary will 
                                * overwrite current values 
                                */
      DictID dicttype,         /* (R) type of dictionary to parse */
      Card16 fontindex,        /* (R) which dictionary to parse 
                                * (which entry in top dict index or fdindex) 
                                */
      Registrant registry[REGISTRY_ITEMS] /* (RW) contains the udv, ndv, and/or wv for 
                                           * the desired instance.  If CFFEnumerateDict 
                                           * computes the wv, it is returned in here.
                                           */
);

/* CFFGet_FontName() looks up the name of a font at a given index.
 * Retrieved names are released during successive calls, so if 
 * a client wishes to retain ppFontName, it should copy it to a 
 * permanent buffer.
 *
 * If the requested data is not in the font, CFFE_BADARG is returned.
 */
extern CFFResult CFFGet_FontName(
    PCFFSetInfo pSetInfo,   /* (R) */
    Card16  fontIndex,      /* (R) index of font whose name is to be returned */
    Char8 ** ppFontName,    /* (W) must be consumed before calling CFFGet_FontName again */
    Card32* pFontNameLength /* (W) */
);

/* CFFGet_String looks up a string from the string table.  
 * Retrieved strings are released during successive calls, so if a client
 * wishes to retain ppString, it should copy it to a permanent buffer.
 *
 * If the requested data is not in the font, CFFE_BADARG is returned.
 */
extern CFFResult CFFGet_String(
    PCFFSetInfo pSetInfo,  /* (R) */
    Card16 sid,            /* (R) string to be read */
    Char8 ** ppString,     /* (W) */
    Card32 * pStringLength /* (W) */
);

/* CFFGet_IndexArrayOffset can look up the offset for an element in
 * an index after CFFGet_IndexedArray has been called. i is assumed to
 * be between firstIndex and firstIndex + nentries that was given
 * to CFFGet_IndexedArray.
 */
extern Card32 CFFGet_IndexArrayOffset(
    ArrayInfo *arrinfo, /* (R) */
    Card32 i,           /* (R) */
    Card8 *offptr);     /* (R) */

typedef enum {GLOBAL_SUBROUTINES, LOCAL_SUBROUTINES, CHARSTRINGS} CFFFetchType;

/* CFFGet_IndexedArray() can look up elements in the local subr index,
 * global subr index or charstring index.  The raw index data is
 * mapped to pData and pOffsets, and pData is biased so that you
 * have to use pOffsets to access the data. CFFGet_IndexArrayOffset
 * can be used to fetch the offset. For example, 
 * *pData + CFFGet_IndexArrayOffset(&arrayInfo, 0, *pOffsets) would
 * return a pointer to the beginning of the first returned index element.
 *
 * If the requested data is not in the font, CFFE_BADARG is returned.
 *
 * If you call this routine more than once with the same fetchType (for example,
 * you call it twice for global subroutine data), memory allocated during the
 * first call is released during the second call.
 *
 * If you are requesting Chameleon charstring data, nentries must be 1.
 * The offset size used to read pOffsets will be stored in 
 * pFontInfo->charStrings.offsetSize, just as it is in regular cff fonts.
 * Note: This routine cannot be called for Chameleon charstrings until
 * after SetUpValues has been called for the font.
 */
extern CFFResult CFFGet_IndexedArray(
    PCFFFontInfo pFontInfo,   /* (R) */
    CFFFetchType fetchType,   /* (R) */
    Card16       firstIndex,  /* (R) */
    Card16       nentries,    /* (R) */
    Card8 **     pOffsets,    /* (W)  returns a pointer to (nentries+1) offsets */
    Card8 **     pData        /* (W) returns a biased pointer to (nentries) strings */
);

/*
 * CFFRelease_IndexedArray() frees the memory allocated during the last call to
 * CFFGet_IndexedArray with the given ArrayInfo.  It can also free memory allocated
 * during a call to CFFGet_String or CFFGet_FontName by giving it the appropriate
 * ArrayInfo.
 */
extern CFFResult CFFRelease_IndexedArray(
    PArrayInfo pArrayInfo /* (RW) */
);

typedef void (* CFFArrayCallback) (
    Card16   code,     /* (R) */
    Card16   sid,      /* (R) */
    FontSetHook hook);

/* CFFGet_Encoding() looks up the entire encoding and calls the
 * encCallback for each sid/code pair.  Since
 * the encoding array must be fairly short (because codes are Card8
 * and are unique), a routine to fetch individual elements shouldn't
 * be necessary and using a callback for each code shouldn't be too slow.  
 */
extern CFFResult CFFGet_Encoding(
    PCFFFontInfo pFontInfo,       /* (R) */
    CFFArrayCallback encCallback, /* (X) Sent the code and sid for each entry 
                                   * in the encoding 
                                   */
    FontSetHook hook              /* argument sent to callback */
);

/* CFFCheckForIdentityCharset looks to see if the charset is the identity
 * mapping.  If so, it returns true.  Else it returns false.
 */
extern boolean CFFCheckForIdentityCharset(
    PCFFFontInfo pFontInfo /* (R) */
);

/* CFFGet_Charset() looks up the entire charset. The returned sidArray
 * is an array of sids that is indexed by
 * gid.  Memory for sidArray will be allocated by the routine.  When the
 * client is finished with sidArray, it should call CFFRelease_Charset.
 * CFFGet_Charset is likely to be used with smallish fonts.
 */
extern CFFResult CFFGet_Charset(
    PCFFFontInfo pFontInfo,  /* (R) */
    cffSID ** sidArray,      /* (W) the array of string ids */
    Card16 * arrayLength     /* (W) the length of the returned sidArray */
);

/* CFFRelease_Charset() frees the space allocated by CFFGet_Charset */
extern CFFResult CFFRelease_Charset(
    PCFFFontInfo pFontInfo,
    cffSID * sidArray
);

/* CFFGet_CharsetElement() fetches the glyph id associated with a
 * given sid/cid.  This is likely to be used with larger fonts where
 * reading the whole charset might be too much of a memory hog.  When
 * this routine calls GetData, it looks to see what it knows about the
 * charset and adjusts its fetch size accordingly.  
 */
extern CFFResult CFFGet_CharsetElement(
    PCFFFontInfo pFontInfo, /* (R) */
    Card16 sid,             /* (R) the desired sid/cid */
    Card16 *gid             /* (W) the gid matching the sid/cid. *gid will
                             * be 0 if sid == 0 or if the sid/cid is not found
                             * in the charset
                             */
);


/* CFFGet_FDElement can be used to find index of the font dictionary that 
 * is associated with a gid in a cid font.  When
 * this routine calls GetData, it looks to see what it knows about the
 * fdSelect and adjusts its fetch size accordingly.  
 */
extern CFFResult CFFGet_FDElement (
    PCFFFontInfo pFontInfo, /* (R) */
    Card16 gid,             /* (R) the gid whose fd index is desired */
    Card8 *fdindex          /* (W) the index of the font dict associated with gid  */
);

/* ReadOffsetArrayEntry() can be used to look up an entry in an array.
 * As far as cff parsing goes, this is only useful
 * for reading individual elements of format 0 fdselect arrays.  So 
 * it probably isn't useful to cff_parse clients, although 
 * cff_parse itself may use it internally.  
 *
 * Why is it in the interface?  If you look at the
 * definition of a cidmap or a subrmap in Type 1 cid fonts, they have
 * almost the exact same format as format 0 cff fdselect arrays,
 * but there was no general api call that reads elements of 
 * such an array.  This provides it.
 *
 * When using with Type 1 fonts:
 * pArrayInfo->window should be NULL.
 * pArrayInfo->offOffsetArray should be the offset of entry 0.
 * Each entry can be composed of 1 or 2 elements.  The
 * first element is returned in firstElement.  firstElementLength ( <= 4 )
 * is the length in bytes of the first element.  
 * pArrayInfo->offsetSize - firstElementLength (<= 2) is
 * the length of the second element.  The second element
 * is assumed to be an offset into an array located at pArrayInfo->offData
 * Thus, (pArrayInfo->offData + the second element) is the real offset
 * of the desired information and is returned in secondElement.
 * Entry entryNumber+1 is fetched and the difference between the second 
 * elements is returned in dataLength.
 * 
 * So, for example, when parsing the cidmap, the arguments should be
 * pArrayInfo->nEntries = cidcount
 * pArrayInfo->offsetSize =  FDBytes + GDBytes
 * pArrayInfo->offOffsetArray =  cidmap offset
 * pArrayInfo->offData =  offset of charstrings
 * firstElementLength =  FDBytes
 * firstElement will contain the fdindex.  secondElement will
 * be the offset of the associated charstring, and datalength will be the length
 * of the charstring.
 *
 * When parsing the subrmap, the arguments should be
 * pArrayInfo->nEntries = subrcount
 * pArrayInfo->offsetSize =  SDBytes
 * pArrayInfo->offOffsetArray =  subrmap offset
 * pArrayInfo->offData =  offset of subrs
 * firstElementLength =  0
 * firstElement will be unchanged.  The second element will be
 * the offset of the subroutine.  dataLength will be the length
 * of the subroutine.
 */
extern CFFResult ReadOffsetArrayEntry(
    GetDataProto GetData,         /* method to get font data from client */
    ReleaseDataProto ReleaseData, /* method to release data retrieved via GetData */
    PArrayInfo pArrayInfo,      /* (R) */
    Card8  firstElementLength,  /* (R) */
    Card16 entryNumber,         /* (R) the entry in the array to be looked up */
    Card32 *firstElement,       /* (W) */
    Card32 *secondElement,      /* (W) */
    Card16 *dataLength,         /* (W) */
    FontSetHook hook            /* sent to GetData and ReleaseData */
);

#endif  /* CFF_PARSE_H */
