/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)cmap.c	1.28
 * Changed:    8/3/99 10:12:07
 ***********************************************************************/

#include "cmap.h"
#include "sfnt_cmap.h"
#include "post.h"
#include "desc.h"
#include "sfnt.h"
#include "string.h"
#include "proof.h"
#include "head.h"

IntX cmapSelected = (-1); 
static ProofContextPtr proofctx = NULL;
static Card16 unitsPerEm = 0;
static Card16 proofElementCounter = 0;

static cmapTbl cmap;
static IntX loaded = 0;

#if UNUSED
/* Unicode to glyph name mapping */
typedef struct
	{
	Card16 unicode;
	Byte8 *name;
	} UnicodeMap;

static UnicodeMap unicodeMap[] =
	{
	#include "sfnt_unicode.h"
	};
#endif /* UNUSED */

static Byte8 **glyphNames = NULL;

/* Read format 0 cmap */
static Format0 *readFormat0(void)
	{
	IntX i;
	Format0 *fmt = memNew(sizeof(Format0));

	fmt->format = 0;

	IN1(fmt->length);
	IN1(fmt->languageId);

	for (i = 0; i < 256; i++)
		IN1(fmt->glyphId[i]);

	return fmt;
	}

/* Read format 2 cmap */
static Format2 *readFormat2(void)
	{
	IntX i;
	Format2 *fmt = memNew(sizeof(Format2));
	IntX maxIndex = 0;

	fmt->format = 2;

	IN1(fmt->length);
	IN1(fmt->languageId);

	for (i = 0; i < 256; i++)
	  {
		IN1(fmt->segmentKeys[i]);
		if ((fmt->segmentKeys[i] / 8) > maxIndex) 
		  maxIndex = (fmt->segmentKeys[i] / 8);
	  }
	fmt->_nSegments = maxIndex + 1;

	/* Read segments */
	fmt->segment = memNew(sizeof(Segment2) * (fmt->_nSegments));
	for (i = 0; i < fmt->_nSegments; i++)
		{
		  Segment2 *segment = &fmt->segment[i];

		  IN1(segment->firstCode);
		  IN1(segment->entryCount);
		  IN1(segment->idDelta);
		  IN1(segment->idRangeOffset);
		}

	/* Read glyphs */
	fmt->_nGlyphs = (fmt->length - FORMAT2_SIZE(fmt->_nSegments, 0)) / sizeof(GlyphId);
	fmt->glyphId = memNew(sizeof(GlyphId) * fmt->_nGlyphs);
	for (i = 0; i < fmt->_nGlyphs; i++)
		IN1(fmt->glyphId[i]);

	return fmt;
	}

/* Read format 4 cmap */
static Format4 *readFormat4(void)
	{
	IntX i;
	IntX nGlyphs;
	IntX nSegments;
	Format4 *fmt = memNew(sizeof(Format4));

	fmt->format = 4;

	IN1(fmt->length);
	IN1(fmt->languageId);
	IN1(fmt->segCountX2);
	IN1(fmt->searchRange);
	IN1(fmt->entrySelector);
	IN1(fmt->rangeShift);

	nSegments = fmt->segCountX2 / 2;
	fmt->endCode = memNew(sizeof(fmt->endCode[0]) * nSegments);
	fmt->startCode = memNew(sizeof(fmt->startCode[0]) * nSegments);
	fmt->idDelta = memNew(sizeof(fmt->idDelta[0]) * nSegments);
	fmt->idRangeOffset = memNew(sizeof(fmt->idRangeOffset[0]) * nSegments);

	for (i = 0; i < nSegments; i++)
		IN1(fmt->endCode[i]);

	IN1(fmt->password);

	for (i = 0; i < nSegments; i++)
		IN1(fmt->startCode[i]);

	for (i = 0; i < nSegments; i++)
		IN1(fmt->idDelta[i]);

	for (i = 0; i < nSegments; i++)
		IN1(fmt->idRangeOffset[i]);

	nGlyphs = 
		(fmt->length - FORMAT4_SIZE(nSegments, 0)) / sizeof(fmt->glyphId[0]);
	fmt->glyphId = memNew(sizeof(fmt->glyphId[0]) * nGlyphs);
	for (i = 0; i < nGlyphs; i++)
		IN1(fmt->glyphId[i]);

	return fmt;
	}

/* Read format 6 cmap */
static Format6 *readFormat6(void)
	{
	IntX i;
	Format6 *fmt = memNew(sizeof(Format6));

	fmt->format = 6;

	IN1(fmt->length);
	IN1(fmt->languageId);
	IN1(fmt->firstCode);
	IN1(fmt->entryCount);

	fmt->glyphId = memNew(sizeof(fmt->glyphId[0]) * fmt->entryCount);
	for (i = 0; i < fmt->entryCount; i++)
		IN1(fmt->glyphId[i]);

	return fmt;
	}
	
static Format12 *readFormat12(void)
{
	IntX i;
	Format12 *fmt = memNew(sizeof(Format12));

	fmt->format = 12;

	IN1(fmt->reserved);
	IN1(fmt->length);
	IN1(fmt->languageId);
	IN1(fmt->nGroups);

	fmt->group = memNew(sizeof(Format12Group)*fmt->nGroups);

	for (i = 0; i < (IntX)fmt->nGroups; i++)
	{	
		IN1(fmt->group[i].startCharCode);
		IN1(fmt->group[i].endCharCode);
		IN1(fmt->group[i].startGlyphID);
	}
	return fmt;
}

static unsigned long read24(void)
	{
	unsigned long retval = 0;
	unsigned char bval;
	IN_BYTES(1, &bval);
	retval = ((unsigned long)bval);
	retval = retval<<8;
	IN_BYTES(1, &bval);
	retval += ((unsigned long)bval);
	retval = retval<<8;
	IN_BYTES(1, &bval);
	retval += ((unsigned long)bval);
	return retval;
	}
	
	
static Format14 *readFormat14(void)
{
	IntX i;
	Format14 *fmt = memNew(sizeof(Format14));
	Card32	tableOffset = TELL() -2; /* -2 becuase we have already red in the format record. */
	fmt->format = 14;

	IN1(fmt->length);
	IN1(fmt->numUVSRecords);

	fmt->uvsRecs = memNew(sizeof(UVSRecord)*fmt->numUVSRecords);

	for (i = 0; i < (IntX)fmt->numUVSRecords; i++)
	{
		fmt->uvsRecs[i].uvs = read24();
		IN1(fmt->uvsRecs[i].defaultUVSoffset);
		IN1(fmt->uvsRecs[i].extUVSOffset);
	}

	for (i = 0; i < (IntX)fmt->numUVSRecords; i++)
	{
		Card32 offset;
		Card32 numEntries;
			Card32 j;
		
		offset =  fmt->uvsRecs[i].defaultUVSoffset;
		fmt->uvsRecs[i].defUVSEntries = NULL;
		if (offset > 0)
			{
			DefaultUVSRecord* uvsRec1;
			offset += tableOffset;
			SEEK_ABS( offset);
			IN1(numEntries);
			uvsRec1 = memNew(sizeof(DefaultUVSRecord)*numEntries);
			fmt->uvsRecs[i].numDefEntries = numEntries;
			fmt->uvsRecs[i].defUVSEntries = uvsRec1;
			for (j = 0; j < numEntries; j++)
				{
				uvsRec1[j].uv = read24();
				IN1(uvsRec1[j].addlCnt);
				}
			}

		offset =  fmt->uvsRecs[i].extUVSOffset;
		fmt->uvsRecs[i].extUVSEntries = NULL;
		if (offset > 0)
			{
			ExtendedUVSRecord* uvsRec2;
			
			offset += tableOffset;
			SEEK_ABS( offset);
			IN1(numEntries);
			uvsRec2 = memNew(sizeof(ExtendedUVSRecord)*numEntries);
			fmt->uvsRecs[i].numExtEntries = numEntries;
			fmt->uvsRecs[i].extUVSEntries = uvsRec2;
			for (j = 0; j < numEntries; j++)
				{
				uvsRec2[j].uv = read24();
				IN1(uvsRec2[j].glyphID);
				}
			}
	}
	return fmt;
}


void cmapRead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	SEEK_ABS(start);

	IN1(cmap.version);
	IN1(cmap.nEncodings);

	cmap.encoding = memNew(sizeof(Encoding) * cmap.nEncodings);
	for (i = 0; i < cmap.nEncodings; i++)
		{
		IN1(cmap.encoding[i].platformId);
		IN1(cmap.encoding[i].scriptId);
		IN1(cmap.encoding[i].offset);
		}

	for (i = 0; i < cmap.nEncodings; i++)
		{
		Card16 format;
		Encoding *encoding = &cmap.encoding[i];

		SEEK_ABS(encoding->offset + start);

		IN1(format);
		switch (format)
			{
		case 0:
			encoding->format = readFormat0();
			break;
		case 2:
			encoding->format = readFormat2();
			break;
		case 4:
			encoding->format = readFormat4();
			break;
		case 6:
			encoding->format = readFormat6();
			break;
		case 12:
			encoding->format = readFormat12();
			break;
		case 14:
			encoding->format = readFormat14();
			break;
		default:
			fatal(SPOT_MSG_cmapBADTBL);	
		
			}
		}
	loaded = 1;
	}

/* Print mapping header if appropriate to level */
static void printMappingHdr(Card16 languageId, IntX level)
	{
	if (level > 8)
		return;
	fprintf(OUTPUTBUFF,  "languageId=%hu\n", languageId);
	fprintf(OUTPUTBUFF,  "--- [code]=%s\n", 
		   (level == 7 || level == 8) ? "<name/CID>" : "glyphId");
	}

/* Print mapping end if appropriate to level */
static void printMappingEnd(IntX level)
	{
	if (level > 8)
		return;
	fprintf(OUTPUTBUFF,  "\n");
	}




/* Print mapping in various formats */
static void printMapping(Card32 code, GlyphId glyphId, 
						 IntX precision, IntX level)
	{
	switch (level)
		{
	case 5:
		fprintf(OUTPUTBUFF,  "[%0*lX]=%hu \n", precision, code, glyphId);
		break;
	case 6:
		fprintf(OUTPUTBUFF,  "[%lu]=%hu \n", code, glyphId);
		break;
	case 7: case 8:
		{
		Byte8 *name = getGlyphName(glyphId, 0);
				
		if (level == 7)
			fprintf(OUTPUTBUFF,  "[%0*lX]=<%s> \n", precision, code, name);
		else
			fprintf(OUTPUTBUFF,  "[%lu]= <%s> \n", code, name);
		}
		break;
	case 9: case 10:
		{
		Byte8 str[10];
		IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig;
		Byte8 *name = getGlyphName(glyphId, 0);
		
		if (level == 9)
			sprintf(str, "%0*lX", precision, code);
		else
			sprintf(str, "%lu", code);
			
		getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
		proofCheckAdvance(proofctx, 1000 + 2*width);
		proofDrawGlyph(proofctx, 
						   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
						   name, ANNOT_SHOWIT|((proofElementCounter++ % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
						   NULL, 0, /* altlabel,altlabelflags */
						   0,0, /* originDx,originDy */
						   0,0, /* origin, originflags */
						   width, 0, /* width,widthflags */
						   NULL, yorig, str
						   );

		}
		break;
		}
	}

/* Print mapping in various formats */
static void printUVSMapping(Card32 uvs, Card32 uv, GlyphId glyphId, IntX level)
	{
	switch (level)
		{
	case 5:
		if (glyphId == 0xffff)
			fprintf(OUTPUTBUFF,  "[%04lX %04lX]= -\n", uv, uvs);
		else
			fprintf(OUTPUTBUFF,  "[%04lX %04lX]= %hu\n", uv, uvs, glyphId);
		break;
	case 6:
		if (glyphId == 0xffff)
			fprintf(OUTPUTBUFF,  "[%lu %lu]= -\n", uv, uvs);
		else
			fprintf(OUTPUTBUFF,  "[%lu %lu]= %hu\n", uv, uvs, glyphId);
		break;
	case 7: case 8:
		{
		Byte8 *name;
		if (glyphId == 0xffff)
			name = "-";
		else
			name = getGlyphName(glyphId, 0);
				
		if (level == 7)
			fprintf(OUTPUTBUFF,  "[%04lX %04lX]= <%s>\n", uv, uvs, name);
		else
			fprintf(OUTPUTBUFF,  "[%lu %lu]= <%s>\n", uv, uvs, name);
		}
		break;
	case 9: case 10:
		{
		Byte8 str[10];
		IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig;
		Byte8 *name = getGlyphName(glyphId, 0);
		
		if (level == 9)
			sprintf(str, "UVS: %04lX %04lX", uv, uvs);
		else
			sprintf(str, "UVS: %lu %lu", uv, uvs);

		getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
		proofCheckAdvance(proofctx, 1000 + 2*width);
		proofDrawGlyph(proofctx, 
						   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
						   name, ANNOT_SHOWIT|((proofElementCounter++ % 2)?ANNOT_ATBOTTOMDOWN1:ANNOT_ATBOTTOM),   /* glyphname,glyphnameflags */
						   NULL, 0, /* altlabel,altlabelflags */
						   0,0, /* originDx,originDy */
						   0,0, /* origin, originflags */
						   width, 0, /* width,widthflags */
						   NULL, yorig, str
						   );
		}
		break;
		}
	}

/* Dump format 0 mapping */
static void dumpMapping0(Format0 *fmt, IntX level)
	{
	IntX i;

	printMappingHdr(fmt->languageId, level);
	for (i = 0; i < 256; i++)
		printMapping(i, fmt->glyphId[i], 2, level);
	printMappingEnd(level);
	}

/* Dump format 2 mapping */
static void dumpMapping2(Format2 *fmt, IntX level)
	{
	IntX i;
	IntX hi;
	Card8 seen[256];

	printMappingHdr(fmt->languageId, level);
	/* Print the single byte mappings in a separate loop - they take a slightly
	different logic than the double-byte mappings. */
	for (i = 0; i < 256; i++)
		{
		IntX key = fmt->segmentKeys[i]/8;
		if (key == 0)
			{ /* it is a single-byte char code */
			Segment2 *segment = &fmt->segment[key];
			if ( (i >= segment->firstCode) && (i <= (segment->firstCode + segment->entryCount)))
				{
				Card16 segdelta = (segment->idRangeOffset -
								   ((fmt->_nSegments - key - 1) * SEGMENT2_SIZE +
									SIZEOF(Segment2, idRangeOffset))) / sizeof(GlyphId);
				GlyphId glyphId = fmt->glyphId[ segdelta + i - segment->firstCode];
				Card16 code = i;

				if (glyphId != 0)
					{
					glyphId = (glyphId + segment->idDelta) % 0x10000;
					printMapping(code, glyphId,  2, level);
					}
				}
			seen[i] = 1;
			}
		else
			seen[i] = 0;
		}
		
	for (hi = 1; hi < 256; hi++)
		{
		if (!seen[hi])
			{
			IntX key = fmt->segmentKeys[hi] / 8;
			Segment2 *segment = &fmt->segment[key];
			for (i = 0; i < segment->entryCount; i++)
				{
				Card16 segdelta = (segment->idRangeOffset -
								   ((fmt->_nSegments - key - 1) * SEGMENT2_SIZE +
									SIZEOF(Segment2, idRangeOffset))) / sizeof(GlyphId);
				GlyphId glyphId = fmt->glyphId[ segdelta + i ];
				Card16 lo = segment->firstCode + i;
				Card16 code = (hi<<8) | ((lo > 255) ? 0 : lo);

				if (glyphId != 0)
					{
					glyphId = (glyphId + segment->idDelta) & 0x0FFFF;
					printMapping(code, glyphId, (hi == 0) ? 2 : 4, level);
					}
				}
			seen[key] = 1;
			}
		}
	printMappingEnd(level);
	}

/* Dump format 4 mapping */
static void dumpMapping4(Format4 *fmt, IntX level)
	{
	IntX i;
	IntX nSegments = fmt->segCountX2 / 2;

	printMappingHdr(fmt->languageId, level);
	for (i = 0; i < nSegments; i++)
		{
		Card32 code;
		Card16 endCode = fmt->endCode[i];
		Card16 startCode = fmt->startCode[i];
		Int16 idDelta = fmt->idDelta[i];
		Card16 idRangeOffset = fmt->idRangeOffset[i];
		Card16 idRangeBytes = (nSegments - i) * sizeof(fmt->idRangeOffset[0]);

		for (code = startCode; code <= endCode; code++)
			{
			GlyphId glyphId;
			if (idRangeOffset == 0xFFFF) /* Fontographer BUG */
			  idRangeOffset = 0;
			glyphId =  (GlyphId)((idRangeOffset == 0) ? (idDelta + code) & 0x0FFFF :
				fmt->glyphId[(idRangeOffset - idRangeBytes) /
							 sizeof(fmt->glyphId[0]) + code - startCode]);
			if (code != 0xffff && glyphId != 0)
				{
				if (idRangeOffset != 0)
					glyphId = (glyphId + idDelta) & 0x0FFFF;
				printMapping(code, glyphId, 4, level);
				}
			}
		}
	printMappingEnd(level);
	}

/* Dump format 6 mapping */
static void dumpMapping6(Format6 *fmt, IntX level)
	{
	IntX i;
	IntX precision = (fmt->entryCount < 256) ? 2 : 4;

	printMappingHdr(fmt->languageId, level);
	for (i = 0; i < fmt->entryCount; i++)
		printMapping(i + fmt->firstCode, fmt->glyphId[i], precision, level);
	printMappingEnd(level);
	}
	
/* Dump format 12 mapping */
static void dumpMapping12(Format12 *fmt, IntX level)
		{
	IntX i;
	
	printMappingHdr((Card16)fmt->languageId, level);
	for (i = 0; i < (IntX)fmt->nGroups; i++)
		{
		Card32 code;
		Format12Group *group=&(fmt->group[i]);
		GlyphId glyphId;
		
		for (code = group->startCharCode, glyphId=(GlyphId)group->startGlyphID; code <= group->endCharCode; code++, glyphId++)
			{
			if (code != 0xffff && glyphId != 0)
				printMapping(code, glyphId, 8, level);
			}
		}
	printMappingEnd(level);
	}

static void dumpMapping14(Format14 *fmt, IntX level)
		{
	Card32 i,j,k;
	Card32 numEntries;

	/* Do print header */
	if (level > 8)
		return;
	fprintf(OUTPUTBUFF,  "--- [UVS]= %s\n", 
		   (level == 7 || level == 8) ? "<name/CID>" : "glyphId");

	/* print body */
	for (i = 0; i < fmt->numUVSRecords; i++)
		{
		DefaultUVSRecord* uvsRecs1 =  fmt->uvsRecs[i].defUVSEntries;
		ExtendedUVSRecord* uvsRecs2 =  fmt->uvsRecs[i].extUVSEntries;
		
		numEntries = fmt->uvsRecs[i].numDefEntries;
		for (j = 0; j < numEntries; j++)
			{
			Card32 addlCnt = uvsRecs1[j].addlCnt;
			printUVSMapping(fmt->uvsRecs[i].uvs, uvsRecs1[j].uv, 0xffff, level);
			for (k=1; k <= addlCnt ; k++)
				printUVSMapping(fmt->uvsRecs[i].uvs, uvsRecs1[j].uv +k, 0xffff, level);
			}
		
		numEntries = fmt->uvsRecs[i].numExtEntries;
		for (j = 0; j < numEntries; j++)
			{
			printUVSMapping(fmt->uvsRecs[i].uvs, uvsRecs2[j].uv,uvsRecs2[j].glyphID,  level);
			}
		}

	/* do print end */
	if (level > 8)
		return;
	fprintf(OUTPUTBUFF,  "\n");
	}

/* Dump cmaps as code->glyghId/glyphName/glyphShape mapping */
static void dumpMapping(IntX level)
	{
	IntX i;
	initGlyphNames();
	headGetUnitsPerEm(&unitsPerEm, cmap_);
	for (i = 0; i < cmap.nEncodings; i++)
		{
		Encoding *encoding = &cmap.encoding[i];
		void *format = encoding->format;

		if (cmapSelected >= 0)
		  {
			if (cmapSelected != i)
			  continue;
		  }

		switch (level)
			{
		case 5: case 6: case 7: case 8:
			fprintf(OUTPUTBUFF,  "--- encoding[%d]\n", i);
			fprintf(OUTPUTBUFF,  "platformId=%hu\n", encoding->platformId);
			fprintf(OUTPUTBUFF,  "scriptId  =%hu\n", encoding->scriptId);
			break;
		case 9: case 10:
			{
			Byte8 title[64];
			Card16 platformId = encoding->platformId;
			Card16 scriptId = encoding->scriptId;
			Card16 languageId = ((FormatHdr *)encoding->format)->languageId;
			if (i > 0)
				{
				proofOnlyNewPage(proofctx);
				}
			else
				{
				proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
											STDPAGE_TOP, STDPAGE_BOTTOM,
											"",
											proofCurrentGlyphSize(), 
											0.0, unitsPerEm, 
											0, 1, NULL);
				}
			sprintf(title, "cmap:subtable index %d,%s,%s,%s", i,
					descPlat(platformId), 
					descScript(platformId, scriptId), 
					descLang(1, platformId, languageId));
			proofMessage(proofctx, title);
			proofElementCounter = 0;


			}
			break; 
			}

		switch (*(Card16 *)format)
			{
		case 0:
			dumpMapping0(format, level);
			break;
		case 2:
			dumpMapping2(format, level);
			break;
		case 4:
			dumpMapping4(format, level);
			break;
		case 6:
			dumpMapping6(format, level);
			break;
		case 12:
			dumpMapping12(format, level);
			break;
		case 14:
			dumpMapping14(format, level);
			break;
			}

		switch (level)
			{
		case 9: case 10:
			  proofSynopsisFinish();
			break;
			}
		}
	if (proofctx)
	  proofDestroyContext(&proofctx);
	}

/* Dump list of supported platform, script, and languages */
static void dumpSupported(void)
	{
	IntX i;

	fprintf(OUTPUTBUFF,  "--- encoding[index]={platform,script,language}\n");

	for (i = 0; i < cmap.nEncodings; i++)
		{
		Encoding *encoding = &cmap.encoding[i];
		Card16 platformId = encoding->platformId;
		Card16 scriptId = encoding->scriptId;
		Card16 format = ((FormatHdr *)encoding->format)->format;
		Card16 languageId;
		
		if (format  == 14)
			languageId =  0;
		else if  (format  == 12)
			languageId =  ((Card16)((Format12 *)encoding->format)->languageId);
		else
			languageId =  ((FormatHdr *)encoding->format)->languageId;
	
		fprintf(OUTPUTBUFF,  "[%2d]={%s,%s,%s}\n", i, 
			   descPlat(platformId), 
			   descScript(platformId, scriptId), 
			   descLang(1, platformId, languageId));
		}
	}

/* Dump format 0 cmap */
static void dumpFormat0(Format0 *fmt, Card16 platformId, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "format    =%hu\n", fmt->format));
	DL(2, (OUTPUTBUFF, "length    =%04hx\n", fmt->length));
	DL(2, (OUTPUTBUFF, "languageId=%hu ", fmt->languageId));
	DL(4, (OUTPUTBUFF, "[%s]", descLang(1, platformId, fmt->languageId)));
	DL(2, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- glyphId[code]=glyphId\n"));
	for (i = 0; i < 256; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, (Card16)fmt->glyphId[i]));
	DL(3, (OUTPUTBUFF, "\n"));
	}

/* Dump format 2 cmap */
static void dumpFormat2(Format2 *fmt, Card16 platformId, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "format       =%hu\n", fmt->format));
	DL(2, (OUTPUTBUFF, "length       =%04hx\n", fmt->length));
	DL(2, (OUTPUTBUFF, "languageId   =%hu ", fmt->languageId));
	DL(4, (OUTPUTBUFF, "[%s]", descLang(1, platformId, fmt->languageId)));
	DL(2, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- segmentKeys[index]=key\n"));
	for (i = 0; i < 256; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->segmentKeys[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- segment[index]={code,count,delta,offset}\n"));

	for (i = 0; i < fmt->_nSegments; i++)
		{
		  Segment2 *segment = &fmt->segment[i];
		  
		  DL(3, (OUTPUTBUFF, "[%d]={%hu,%hu,%hd,%04hx} ", i,
				 segment->firstCode, segment->entryCount,
				 segment->idDelta, segment->idRangeOffset));
		}
	DL(3, (OUTPUTBUFF, "\n"));

	DL(4, (OUTPUTBUFF, "--- glyphId[index]=code\n"));
	for (i = 0; i < fmt->_nGlyphs; i++)
		DL(4, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->glyphId[i]));
	DL(4, (OUTPUTBUFF, "\n"));
	}

/* Dump format 4 cmap */
static void dumpFormat4(Format4 *fmt, Card16 platformId, IntX level)
	{
	IntX i;
	IntX nGlyphs;
	IntX nSegments = fmt->segCountX2 / 2;

	DL(2, (OUTPUTBUFF, "format       =%hu\n", fmt->format));
	DL(2, (OUTPUTBUFF, "length       =%04hx\n", fmt->length));
	DL(2, (OUTPUTBUFF, "languageId   =%hu ", fmt->languageId));
	DL(4, (OUTPUTBUFF, "[%s]", descLang(1, platformId, fmt->languageId)));
	DL(2, (OUTPUTBUFF, "\n"));
	DL(2, (OUTPUTBUFF, "segCountX2   =%hu\n", fmt->segCountX2));
	DL(2, (OUTPUTBUFF, "searchRange  =%hu\n", fmt->searchRange));
	DL(2, (OUTPUTBUFF, "entrySelector=%hu\n", fmt->entrySelector));
	DL(2, (OUTPUTBUFF, "rangeShift   =%hu\n", fmt->rangeShift));

	DL(3, (OUTPUTBUFF, "--- endCode[index]=code\n"));
	for (i = 0; i < nSegments; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->endCode[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	DL(2, (OUTPUTBUFF, "password=%hu\n", fmt->password));

	DL(3, (OUTPUTBUFF, "--- startCode[index]=code\n"));
	for (i = 0; i < nSegments; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->startCode[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- idDelta[index]=code\n"));
	for (i = 0; i < nSegments; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hd ", i, (Int16)fmt->idDelta[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- idRangeOffset[index]=code\n"));
	for (i = 0; i < nSegments; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%04hx ", i, fmt->idRangeOffset[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- glyphId[index]=glyphId\n"));
	nGlyphs = 
		(fmt->length - FORMAT4_SIZE(nSegments, 0)) / sizeof(fmt->glyphId[0]);
	for (i = 0; i < nGlyphs; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->glyphId[i]));
	DL(3, (OUTPUTBUFF, "\n"));
	}

/* Dump format 6 cmap */
static void dumpFormat6(Format6 *fmt, Card16 platformId, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "format    =%hu\n", fmt->format));
	DL(2, (OUTPUTBUFF, "length    =%04hx\n", fmt->length));
	DL(2, (OUTPUTBUFF, "languageId=%hu ", fmt->languageId));
	DL(4, (OUTPUTBUFF, "[%s]", descLang(1, platformId, fmt->languageId)));
	DL(2, (OUTPUTBUFF, "\n"));
	DL(3, (OUTPUTBUFF, "firstCode =%hu\n", fmt->firstCode));
	DL(3, (OUTPUTBUFF, "entryCount=%hu\n", fmt->entryCount));

	DL(3, (OUTPUTBUFF, "--- glyphId[code]=glyphId\n"));
	for (i = 0; i < fmt->entryCount; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->glyphId[i]));
	DL(3, (OUTPUTBUFF, "\n"));
	}

static void dumpFormat12(Format12 *fmt, Card16 platformId, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "format    =%hu\n", fmt->format));
	DL(2, (OUTPUTBUFF, "length    =%04lx\n", fmt->length));
	DL(2, (OUTPUTBUFF, "languageId=%lu ", fmt->languageId));
	DL(4, (OUTPUTBUFF, "[%s]", descLang(1, platformId, (Card16)fmt->languageId)));
	DL(2, (OUTPUTBUFF, "\n"));
	DL(3, (OUTPUTBUFF, "nGroups=%lu\n", fmt->nGroups));

	DL(3, (OUTPUTBUFF, "--- Group[index]={startCharCode,endCharCode,startGlyphID}  \n"));
	for (i = 0; i < (IntX)fmt->nGroups; i++)
		DL(3, (OUTPUTBUFF, "[%d]={%lu,%lu,%lu} ", i, fmt->group[i].startCharCode, fmt->group[i].endCharCode, fmt->group[i].startGlyphID));
	DL(3, (OUTPUTBUFF, "\n"));
	}

static void dumpFormat14(Format14 *fmt,  IntX level)
	{
	Card32 i,j;
	Card32 numEntries;
	
	DL(2, (OUTPUTBUFF, "format    =%hu\n", fmt->format));
	DL(2, (OUTPUTBUFF, "length    =%04lx\n", fmt->length));
	DL(2, (OUTPUTBUFF, "number Variation Sequence Records   =%lu ", fmt->numUVSRecords));
	DL(2, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "---UVS Record [ndex]={uvs, default UVS Table Offset  non-default UVS Table Offset}  \n"));
	for (i = 0; i < fmt->numUVSRecords; i++)
		DL(3, (OUTPUTBUFF, "[%ld]={%04lx,%04lx,%04lx} \n", i, fmt->uvsRecs[i].uvs, fmt->uvsRecs[i].defaultUVSoffset, fmt->uvsRecs[i].extUVSOffset));
	DL(3, (OUTPUTBUFF, "\n"));

	for (i = 0; i < fmt->numUVSRecords; i++)
		{
		DefaultUVSRecord* uvsRecs1 =  fmt->uvsRecs[i].defUVSEntries;
		ExtendedUVSRecord* uvsRecs2 =  fmt->uvsRecs[i].extUVSEntries;
		
		numEntries = fmt->uvsRecs[i].numDefEntries;
		DL(3, (OUTPUTBUFF,  "---UVS Record [%ld]\n", i));
		DL(3, (OUTPUTBUFF,  "   Default entry offset [%04lx] numEntries %ld.  Default UVS Entry[index] = {uv, additonal UV count}\n", fmt->uvsRecs[i].defaultUVSoffset, numEntries));
		for (j = 0; j < numEntries; j++)
			{
			DL(3, (OUTPUTBUFF,  "   [%ld]={%04lx,%d} ", j,  uvsRecs1[j].uv, uvsRecs1[j].addlCnt));
			}
		if (numEntries)
			DL(3, (OUTPUTBUFF, "\n\n"));
		
		numEntries = fmt->uvsRecs[i].numExtEntries;
		DL(3, (OUTPUTBUFF,  "   Extended entry offset [%04lx] numEntries %ld.  Extended UVS Entry[index] = {uv, glyphID}\n", fmt->uvsRecs[i].extUVSOffset, numEntries));
		for (j = 0; j < numEntries; j++)
			{
			DL(3, (OUTPUTBUFF,  "   [%ld]={%04lx,%d} ", j,  uvsRecs2[j].uv, uvsRecs2[j].glyphID));
			}
		if (numEntries)
			DL(3, (OUTPUTBUFF, "\n"));
			
		DL(3, (OUTPUTBUFF, "\n"));
		}
	DL(3, (OUTPUTBUFF, "\n"));
	}

void cmapDump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [cmap] (%08lx)\n", start));

	switch (level)
		{
	case 5: case 6: case 7: case 8: case 9: case 10:
		dumpMapping(level);
		return;
	case 11:
		dumpSupported();
		return;
		}

	DL(2, (OUTPUTBUFF, "version   =%04hx\n", cmap.version));
	DL(2, (OUTPUTBUFF, "nEncodings=%04hx\n", cmap.nEncodings));

	for (i = 0; i < cmap.nEncodings; i++)
		{
		Card16 scriptId = cmap.encoding[i].scriptId;
		Card16 platformId = cmap.encoding[i].platformId;

		if (cmapSelected >= 0)
		  {
			if (cmapSelected != i)
			  continue;
		  }

		DL(2, (OUTPUTBUFF, "--- encoding[%d]\n", i));
		DL(2, (OUTPUTBUFF, "platformId=%hu ", platformId));
		DL(4, (OUTPUTBUFF, "[%s]", descPlat(platformId)));
		DL(2, (OUTPUTBUFF, "\n"));
		DL(2, (OUTPUTBUFF, "scriptId  =%hu ", scriptId));
		DL(4, (OUTPUTBUFF, "[%s]", descScript(platformId, scriptId)));
		DL(2, (OUTPUTBUFF, "\n"));
		DL(2, (OUTPUTBUFF, "offset    =%08lx\n", cmap.encoding[i].offset));
		}

	for (i = 0; i < cmap.nEncodings; i++)
		{
		void *format = cmap.encoding[i].format;
		Card16 platformId = cmap.encoding[i].platformId;

		if (cmapSelected >= 0)
		  {
			if (cmapSelected != i)
			  continue;
		  }

		DL(2, (OUTPUTBUFF, "--- mapping[%08lx]\n", cmap.encoding[i].offset));
		switch (*(Card16 *)format)
			{
		case 0:
			dumpFormat0(format, platformId, level);
			break;
		case 2:
			dumpFormat2(format, platformId, level);
			break;
		case 4:
			dumpFormat4(format, platformId, level);
			break;
		case 6:
			dumpFormat6(format, platformId, level);
			break;
		case 12:
			dumpFormat12(format, platformId, level);
			break;
		case 14:
			dumpFormat14(format, level);
			break;
			}
		}
	}

/* Free format 0 cmap */
static void freeFormat0(Format0 *fmt)
	{	
	}

/* Free format 2 cmap */
static void freeFormat2(Format2 *fmt)
	{
	memFree(fmt->segment);
	memFree(fmt->glyphId);
	}

/* Free format 4 cmap */
static void freeFormat4(Format4 *fmt)
	{
	memFree(fmt->endCode);
	memFree(fmt->startCode);
	memFree(fmt->idDelta);
	memFree(fmt->idRangeOffset);
	memFree(fmt->glyphId);
	}

/* Free format 6 cmap */
static void freeFormat6(Format6 *fmt)
	{
	memFree(fmt->glyphId);
	}
	
static void freeFormat12(Format12 *fmt)
	{
	memFree(fmt->group);
	}

static void freeFormat14(Format14 *fmt)
	{
	int i;
	for (i = 0; i < (int)fmt->numUVSRecords; i++)
		{
		DefaultUVSRecord* uvsRecs1 =  fmt->uvsRecs[i].defUVSEntries;
		ExtendedUVSRecord* uvsRecs2 =  fmt->uvsRecs[i].extUVSEntries;
		if (uvsRecs1 != NULL)
			memFree(uvsRecs1);
		if (uvsRecs2 != NULL)
			memFree( uvsRecs2);
		}
	memFree(fmt->uvsRecs);
	}


void cmapFree(void)
	{
	IntX i;

	if (!loaded)
		return;

	for (i = 0; i < cmap.nEncodings; i++)
		{
		  void *format = cmap.encoding[i].format;

		switch (*(Card16 *)format)
			{
		case 0:
		    freeFormat0(format);
			memFree(format);	
			break;
		case 2:
			freeFormat2(format);
			memFree(format);	
			break;
		case 4:				  
			freeFormat4(format);
			memFree(format);	
			break;			  
		case 6:				  
			freeFormat6(format);
			memFree(format);	
			break;
		case 12:
			freeFormat12(format);
			memFree(format);
			break;
		case 14:
			freeFormat14(format);
			memFree(format);
			break;
		default:
			break;			  
			}
		}
	memFree(cmap.encoding);
	loaded = 0;
	}

#if UNUSED
/* Compare with UnicodeMap */
static IntN cmpUnicodes(const void *key, const void *value)
	{
	Card16 a = *(Card16 *)key;
	Card16 b = ((UnicodeMap *)value)->unicode;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}
	
/* Find glyph name from Unicode */
static Byte8 *unicodeName(Card16 unicode)
	{
	UnicodeMap *map = 
		(UnicodeMap *)bsearch(&unicode, unicodeMap, TABLE_LEN(unicodeMap),
							  sizeof(UnicodeMap), cmpUnicodes);
	return (map == NULL) ? NULL : map->name;
	}

/* Make glyphId->unicode mapping, sorted by glyphId */
static void makeInverseMap(Format4 *fmt)
	{
	IntX i;
	IntX nSegments = fmt->segCountX2 / 2;

	/* Allocate and initialize glyph names array */
	if (glyphNames == NULL)
		glyphNames = memNew(sizeof(glyphNames[0]) * nGlyphs);
	else
		glyphNames = memResize(glyphNames, sizeof(glyphNames[0]) * nGlyphs);
	for (i = 0; i < nGlyphs; i++)
		glyphNames[i] = NULL;

	for (i = 0; i < nSegments; i++)
		{
		Card32 unicode;
		Card16 endCode = fmt->endCode[i];
		Card16 startCode = fmt->startCode[i];
		Int16 idDelta = fmt->idDelta[i];
		Card16 idRangeOffset = fmt->idRangeOffset[i];
		Card16 idRangeBytes = (nSegments - i) * sizeof(fmt->idRangeOffset[0]);

		for (unicode = startCode; unicode <= endCode; unicode++)
			{
			GlyphId glyphId = 
				(idRangeOffset == 0) ? idDelta + unicode :
					fmt->glyphId[(idRangeOffset - idRangeBytes) /
								 sizeof(fmt->glyphId[0]) + unicode - 
								 startCode];
			if (glyphId < nGlyphs && unicode != MAX_CARD16)
				glyphNames[glyphId] = unicodeName(unicode);
			}
		}
	}
#endif

/* Initialize name fetching */
IntX cmapInitName(void)
	{
#if VESTIGIAL
	IntX i;
	if (!loaded)
		{
		if (sfntReadTable(cmap_))
			return 0;
		}
	if (getNGlyphs(&nGlyphs, cmap_))
		return 0;


	for (i = 0; i < cmap.nEncodings; i++)
		{
		Encoding *encoding = &cmap.encoding[i];
		if (encoding->platformId == cmap_MS &&
			encoding->scriptId == cmap_MS_UGL)
			{
			Format4 *fmt = encoding->format;
			if (fmt->format != 4)
				{
				warning(SPOT_MSG_cmapBADMSFMT, fmt->format);
				return 0;
				}
			makeInverseMap(fmt);
			return 1;
			}
		}
#endif
	return 0;
	}

/* Get glyph name for glyphId */
Byte8 *cmapGetName(GlyphId glyphId, IntX *length)
	{
	Byte8 *name;
	if (glyphNames == NULL) {*length = 0; return NULL;}
	name = glyphNames[glyphId];
	*length = (name == NULL) ? 0 : strlen(name);
	return name;
	}

void cmapUsage(void)
	{
	fprintf(OUTPUTBUFF,  "--- cmap\n"
		   "=5  Print hex character code to glyph id mapping\n"
		   "=6  Print dec character code to glyph id mapping\n"
		   "=7  Print hex character code to glyphName/CID mapping\n"
		   "=8  Print dec character code to glyphName/CID mapping\n"
		   "=9  Proof hex character code to glyph shape mapping\n"
		   "=10 Proof dec character code to glyph shape mapping\n"
		   "=11 List platform/script/language support\n"
		   "    Options: [-Cindex]\n"
		   "    -C  select the cmap encoding index to use (use '-tcmap=11' for indices)\n"
		   );
	}
