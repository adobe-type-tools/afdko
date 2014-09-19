/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)cmap.h	1.5
 * Changed:    9/4/98 16:34:27
 ***********************************************************************/

/*
 * Character mapping table format definition.
 */

#ifndef FORMAT_CMAP_H
#define FORMAT_CMAP_H

/* Macintosh */
#define cmap_MAC			1	/* Platform: Macintosh */

/* Microsoft */
#define cmap_MS				3	/* Platform: Microsoft */
#define cmap_MS_SYMBOL		0	/* Script: Undefined index scheme */
#define cmap_MS_UGL			1	/* Script: UGL */

/* Custom */
#define cmap_CUSTOM			4	/* Platform: Custom */
#define cmap_CUSTOM_DEFAULT	0	/* Script: Default */

/* Common header for all formats */
typedef struct
	{
	Card16 format;
	Card16 length;
	Card16 languageId;	/* Documented as revision */
	} FormatHdr;

typedef struct
	{
	Card16 format;
	Card16 length;
	Card16 languageId;	/* Documented as revision */
	Card8 glyphId[256];
	} Format0;
#define FORMAT0_SIZE (SIZEOF(Format0, format) + \
					  SIZEOF(Format0, length) + \
					  SIZEOF(Format0, languageId) + \
					  SIZEOF(Format0, glyphId[0]) * 256)

typedef struct
	{
	Card16 firstCode;
	Card16 entryCount;
	Int16 idDelta;
	Card16 idRangeOffset;
	} Segment2;
#define SEGMENT2_SIZE (SIZEOF(Segment2, firstCode) + \
					   SIZEOF(Segment2, entryCount) + \
					   SIZEOF(Segment2, idDelta) + \
					   SIZEOF(Segment2, idRangeOffset))

typedef struct
	{
	Card16 format;
	Card16 length;
	Card16 languageId;	/* Documented as revision */
	Card16 segmentKeys[256];
	Card16 _nSegments;
	DCL_ARRAY(Segment2, segment);
	Card16 _nGlyphs;
	DCL_ARRAY(GlyphId, glyphId);
	} Format2;
#define FORMAT2_SIZE(segs,glyphs) (SIZEOF(Format2, format) + \
								   SIZEOF(Format2, length) + \
								   SIZEOF(Format2, languageId) + \
								   SIZEOF(Format2, segmentKeys[0]) * 256 + \
								   SEGMENT2_SIZE * (segs) + \
								   sizeof(GlyphId) * (glyphs))

typedef struct
	{
	Card16 format;
	Card16 length;
	Card16 languageId;	/* Documented as revision */
	Card16 segCountX2;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	Card16 *endCode;
	Card16 password;
	Card16 *startCode;
	Int16 *idDelta;
	Card16 *idRangeOffset;
	DCL_ARRAY(GlyphId, glyphId);
	} Format4;
#define FORMAT4_SIZE(segs,glyphs) \
	(SIZEOF(Format4, format) + \
	 SIZEOF(Format4, length) + \
	 SIZEOF(Format4, languageId) + \
	 SIZEOF(Format4, segCountX2) + \
	 SIZEOF(Format4, searchRange) + \
	 SIZEOF(Format4, entrySelector) + \
	 SIZEOF(Format4, rangeShift) + \
	 SIZEOF(Format4, password) + \
	 (SIZEOF(Format4, endCode[0]) + \
	  SIZEOF(Format4, startCode[0]) + \
	  SIZEOF(Format4, idDelta[0]) + \
	  SIZEOF(Format4, idRangeOffset[0])) * (segs) + \
	 sizeof(GlyphId) * (glyphs))

typedef struct
	{
	Card16 format;
	Card16 length;
	Card16 languageId;
	Card16 firstCode;
	Card16 entryCount;
	Card16 *glyphId;
	} Format6;
#define FORMAT6_SIZE(glyphs) (SIZEOF(Format6, format) + \
							  SIZEOF(Format6, length) + \
							  SIZEOF(Format6, languageId) + \
							  SIZEOF(Format6, firstCode) + \
							  SIZEOF(Format6, entryCount) + \
							  SIZEOF(Format6, glyphId[0]) * (glyphs))

typedef struct 
	{
	Card32 startCharCode;
	Card32 endCharCode;
	Card32 startGlyphID;
	}Format12Group;
	
typedef struct
	{
	Card16 format;
	Card16 reserved;
	Card32 length;
	Card32 languageId;	/* Documented as revision */
	Card32 nGroups;
	DCL_ARRAY(Format12Group, group);
	} Format12;

typedef struct
	{
	Card32 uv;		/* first Unicode value in range of UV's for default UVS's */
	Card8 addlCnt; /* Number of consecutive UV's after first */
	} DefaultUVSRecord;

typedef struct
	{
	Card32 uv;		/*  Unicode value  */
	Card16 glyphID; /*glyphID for UVS glyph */
	} ExtendedUVSRecord;

typedef struct
	{
	Card32 uvs;
	Card32 defaultUVSoffset;
	Card32 extUVSOffset;
	Card32 numDefEntries;
	Card32 numExtEntries;
	DCL_ARRAY(DefaultUVSRecord, defUVSEntries);
	DCL_ARRAY(ExtendedUVSRecord, extUVSEntries);
	} UVSRecord;

typedef struct
	{
	Card16 format;
	Card32 length;
	Card32 numUVSRecords;	/* Documented as revision */
	DCL_ARRAY(UVSRecord, uvsRecs);
	} Format14;


/*#define FORMAT12_SIZE(segs,glyphs) \
	(SIZEOF(Format12, format) + \
	 SIZEOF(Format12, length) + \
	 SIZEOF(Format12, languageId) + \
	 SIZEOF(Format12, nGroups) + \
	 sizeof(Format12Group) * (glyphs))
*/

typedef struct
	{
	Card16 platformId;
	Card16 scriptId;
	Card32 offset;
	void *format;
	} Encoding;
#define ENCODING_SIZE (SIZEOF(Encoding, platformId) + \
					   SIZEOF(Encoding, scriptId) + \
					   SIZEOF(Encoding, offset))

typedef struct
	{
	Card16 version;
	Card16 nEncodings;
	DCL_ARRAY(Encoding, encoding);
	} cmapTbl;
#define TBL_HDR_SIZE (SIZEOF(cmapTbl, version) + \
					  SIZEOF(cmapTbl, nEncodings))

#endif /* FORMAT_CMAP_H */
