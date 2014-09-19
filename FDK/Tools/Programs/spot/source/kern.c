/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)kern.c	1.10
 * Changed:    7/14/99 09:24:30
 ***********************************************************************/

#include "kern.h"
#include "sfnt_kern.h"
#include "BLND.h"
#include "sfnt.h"
#include "post.h"

static IntX ms = 0;			/* Flags Microsoft-style table */
static kernTbl *kern = NULL;		/* Apple-style kern table */
static MSkernTbl *MSkern = NULL;	/* Microsoft-style kern table */
static IntX loaded = 0;
static IntX nMasters;

/* Read format 0 */
static Format0 *read0(Card32 offset, Card32 length)
	{
	IntX i;
	IntX nPairs;
	Card32 calcSize;
	Format0 *fmt = memNew(sizeof(Format0));
	/* printf("Offset at start of table: %d.\n", fileTell());
	*/
	IN1(fmt->nPairs);
	IN1(fmt->searchRange);
	IN1(fmt->entrySelector);
	IN1(fmt->rangeShift);
	/* printf("Offset at start of kern pairs: %d.\n", fileTell());
	*/
	/* Determine nPairs from table length (table may include sentinel) */
	calcSize = SUBTABLE_HDR_SIZE + FORMAT0_HDR_SIZE + (fmt->nPairs+1)*PAIR_SIZE(1);
	if  (length >= calcSize)
		nPairs = fmt->nPairs + 1;
	else 
		nPairs = fmt->nPairs;

		
	fmt->pair = memNew(sizeof(Pair) * nPairs);
	for (i = 0; i < nPairs; i++)
		{
		IntX j;
		Pair *pair = &fmt->pair[i];

		IN1(pair->left);
		IN1(pair->right);

		pair->value = memNew(sizeof(FWord) * nMasters);
		for (j = 0; j < nMasters; j++)
			IN1(pair->value[j]);
		/* printf("Offset at nd of kern pairr '%d': %d.\n", i, fileTell());
		*/
		}

	return fmt;
	}

/* Read format 2 class */
static void read2Class(Class *class, Card32 offset)
	{
	IntX i;

	SEEK_ABS(offset);

	IN1(class->firstGlyph);
	IN1(class->nGlyphs);
	
	class->classes = memNew(sizeof(Card16) * class->nGlyphs);
	for (i = 0; i < class->nGlyphs; i++)
		IN1(class->classes[i]);
	}

/* Read format 2 */
static Format2 *read2(Card32 offset, Card32 length)
	{
	IntX i;
	IntX words;
	Format2 *fmt = memNew(sizeof(Format2));
	
	IN1(fmt->rowWidth);
	IN1(fmt->leftClassOffset);
	IN1(fmt->rightClassOffset);
	IN1(fmt->arrayOffset);
	
	read2Class(&fmt->left, offset + fmt->leftClassOffset);
	read2Class(&fmt->right, offset + fmt->rightClassOffset);

	/* Determine array size from table length */
	words = (length - (FORMAT2_HDR_SIZE + CLASS_SIZE(fmt->left.nGlyphs) + 
					   CLASS_SIZE(fmt->right.nGlyphs))) / sizeof(FWord);
	fmt->array = memNew(sizeof(FWord) * words);

	/* Read value array */
	SEEK_ABS(offset + fmt->arrayOffset);
	for (i = 0; i < words; i++)
		IN1(fmt->array[i]);

	return fmt;
	}

/* Read format 3 */
static Format3 *read3(Card32 offset, Card32 length)
	{
	Card32 i;
	Format3 *fmt = memNew(sizeof(Format3));

	Card32 kernIndexSize,tableSize;

	IN1(fmt->glyphCount);
	IN1(fmt->kernValueCount);
	IN1(fmt->leftClassCount);
	IN1(fmt->rightClassCount);
	IN1(fmt->flags);
	
	kernIndexSize = fmt->leftClassCount*fmt->rightClassCount;
	tableSize = SUBTABLE_HDR_SIZE + FORMAT3_HDR_SIZE + fmt->kernValueCount*sizeof(FWord) + fmt->glyphCount + fmt->glyphCount + kernIndexSize;

	if (tableSize != length)
	{
		if (tableSize > length)
		{
			fprintf(OUTPUTBUFF, "Error: kern subtable format 3 size '%lu' is greater than subtable length '%lu'. Skipping.\n", tableSize, length);
			return fmt;
		}
		else
			fprintf(OUTPUTBUFF, "Error: kern subtable format 3 size '%lu' is less than subtable length '%lu'.\n", tableSize, length);
	}

	
	fmt->kernValue = (FWord*)memNew(sizeof(FWord) * fmt->kernValueCount);
	fmt->leftClass = (Card8*)memNew(sizeof(Card8) * fmt->glyphCount);
	fmt->rightClass = (Card8*)memNew(sizeof(Card8) * fmt->glyphCount);
	fmt->rightClass = (Card8*)memNew(sizeof(Card8) * fmt->glyphCount);
	fmt->kernIndex = (Card8*)memNew(sizeof(Card8) * kernIndexSize);

	/* Read value arrays */
	for (i = 0; i < fmt->kernValueCount; i++)
		IN1(fmt->kernValue[i]);
	for (i = 0; i < fmt->glyphCount; i++)
		IN1(fmt->leftClass[i]);
	for (i = 0; i < fmt->glyphCount; i++)
		IN1(fmt->rightClass[i]);
	for (i = 0; i < kernIndexSize; i++)
		IN1(fmt->kernIndex[i]);
	return fmt;
	}

/* Read format-specific subtable */
static void *readSubtable(IntX type, Card32 offset, Card32 length)
	{
	switch (type)
		{
	case 0:	
		return read0(offset, length);
	case 2:
		return read2(offset, length);
	case 3:
		return read3(offset, length);
	default:
		warning(SPOT_MSG_kernUNSTAB, type);
		return NULL;
		}
	}

/* Read Microsoft-style kern table */
static void readMS(LongN start)
	{
	IntX i;
	LongN offset = start + MS_TBL_HDR_SIZE;

	ms = 1;
	MSkern = (MSkernTbl *)memNew(sizeof(MSkernTbl));
	SEEK_ABS(start);

	IN1(MSkern->version);
	IN1(MSkern->nTables);

	MSkern->subtable = memNew(sizeof(MSSubtable) * MSkern->nTables);
	for (i = 0; i < MSkern->nTables; i++)
		{
		MSSubtable *subtable = &MSkern->subtable[i];

		SEEK_ABS(offset);
		IN1(subtable->version);
		IN1(subtable->length);
		IN1(subtable->coverage);
		subtable->format =
			readSubtable((subtable->coverage & MS_COVERAGE_FORMAT)>>8,
						 offset, subtable->length);
		offset += subtable->length;
		}
	}

/* Read Apple-style kern table */
static void readApple(LongN start)
	{
	IntX i;
	Card32 offset = start + TBL_HDR_SIZE;

	/* Already read version */
	IN1(kern->nTables);

	kern->subtable = memNew(sizeof(Subtable) * kern->nTables);

	for (i = 0 ; i < (IntX)kern->nTables; i++)
		{
		Subtable *subtable = &kern->subtable[i];

		SEEK_ABS(offset);
		/* Read subtable header */
		IN1(subtable->length);
		IN1(subtable->coverage);
		IN1(subtable->tupleIndex);
		/* read rest of subtable */
		subtable->format = readSubtable(subtable->coverage & COVERAGE_FORMAT,
										offset, subtable->length);
		offset += subtable->length;
		}
	}

void kernRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	kern = (kernTbl *)memNew(sizeof(kernTbl));

	if (sfntReadTable(BLND_))
		nMasters = 1;	/* No BLND table so not MM font */
	else
		nMasters = BLNDGetNMasters();

	SEEK_ABS(start);
	
	IN1(kern->version);
	if (kern->version != VERSION(1,0))
		readMS(start);
	else
		readApple(start);

	loaded = 1;
	}

/* Dump kerning value */
static void dumpValue(FWord *valueP, IntX level)
	{
	FWord value = 0;
		
	if (nMasters == 1)
		{
		if (valueP != NULL)
			value = valueP[0];
		DL(3, (OUTPUTBUFF, "%hd", value));
		}
	else
		{
		IntX i;
		DL(3, (OUTPUTBUFF, "{"));
		for (i = 0; i < nMasters; i++)
			{
			if (valueP != NULL)
				value = valueP[i];
			DL(3, (OUTPUTBUFF, "%s%hd", (i == 0) ? "" : ",", value));
			}
		DL(3, (OUTPUTBUFF, "}"));
		}
	}

static void dump0(Format0 *fmt, Card32 length, IntX level)
	{
	IntX i;
	IntX nPairs = (length - FORMAT0_HDR_SIZE) / PAIR_SIZE(nMasters);
	IntX dumpNames = level == 4;

	if (dumpNames)
		initGlyphNames();

	DL(2, (OUTPUTBUFF, "--- format 0\n"));
	DLu(2, "nPairs       =", fmt->nPairs);
	DLu(2, "searchRange  =", fmt->searchRange);
	DLu(2, "entrySelector=", fmt->entrySelector);
	DLu(2, "rangeShift   =", fmt->rangeShift);

	if (nPairs > fmt->nPairs + 1)
		nPairs = fmt->nPairs + 1;	/* Table larger than # pairs + sentinel */

	DL(3, (OUTPUTBUFF, "--- pair[index]={left,right,value+}\n"));
	for (i = 0; i < fmt->nPairs; i++)
		{
		Pair *pair = &fmt->pair[i];
		
		if (dumpNames)
			{
			fprintf(OUTPUTBUFF,  "[%d]={<%s>,", i, getGlyphName(pair->left, 0));
			fprintf(OUTPUTBUFF,  "<%s>,", getGlyphName(pair->right, 0));
			}
		else
			DL(3, (OUTPUTBUFF, "[%d]={%hu,%hu,", i, pair->left, pair->right));
		dumpValue(pair->value, level);
		DL(3, (OUTPUTBUFF, "} "));
		}
	DL(3, (OUTPUTBUFF, "\n"));

	}

/* Dump format 2 class */
static void dump2Class(Byte8 *label, Class *class, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- %s class table\n", label));
	DLu(2, "firstGlyph=", class->firstGlyph);
	DLu(2, "nGlyphs   =", class->nGlyphs);

	DL(3, (OUTPUTBUFF, "--- classes[index]=class\n"));
	for (i = 0; i < class->nGlyphs; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%04hx ", i, class->classes[i]));
	DL(3, (OUTPUTBUFF, "\n"));
	}

/* Dump format 2 array */
static void dump2Array(Format2 *fmt, Card32 length, IntX level)
	{
	IntX i;
	IntX j;
	IntX elementSize = sizeof(FWord) * nMasters;
	IntX elements =
		(length - (FORMAT2_HDR_SIZE + CLASS_SIZE(fmt->left.nGlyphs) + 
				   CLASS_SIZE(fmt->right.nGlyphs))) / elementSize;
	IntX cols = fmt->rowWidth / elementSize;
	IntX rows = elements / cols;

	DL(2, (OUTPUTBUFF, "--- value array[left,right]=non-zero value\n"));
	for (i = 0; i < rows; i++)
		for (j = 0; j < cols; j++)
			{
			IntX k;
			FWord *value = &fmt->array[(i * cols + j) * nMasters];

			for (k = 0; k < nMasters; k++)
				if (value[k] != 0)
					goto dump;

			continue;	/* Skip all-zero value */

		dump:
			DL(3, (OUTPUTBUFF, "[%d,%d]=", i, j));
			dumpValue(value, level);
			DL(3, (OUTPUTBUFF, " "));
			}
	DL(3, (OUTPUTBUFF, "\n"));
	}

static void dump2(Format2 *fmt, Card32 length, IntX level)
	{
	DL(2, (OUTPUTBUFF, "--- format 2\n"));

	/* Dump header */
	DLu(2, "rowWidth        =", fmt->rowWidth);
	DLx(2, "leftClassOffset =", fmt->leftClassOffset);
	DLx(2, "rightClassOffset=", fmt->rightClassOffset);
	DLx(2, "arrayOffset     =", fmt->arrayOffset);

	dump2Class("left", &fmt->left, level);
	dump2Class("right", &fmt->right, level);
	dump2Array(fmt, length, level);
	}

static void dump3(Format3 *fmt, Card32 length, IntX level)
	{
	Card32 i;
	IntX dumpNames = level == 4;
	Card32 kernIndexSize;
	Card32 tableSize;
	kernIndexSize = fmt->leftClassCount*fmt->rightClassCount;
	tableSize = FORMAT3_HDR_SIZE + fmt->kernValueCount*sizeof(FWord) + fmt->glyphCount + fmt->glyphCount + fmt->leftClassCount*fmt->rightClassCount;

	DL(2, (OUTPUTBUFF, "--- format 3\n"));

	/* Dump header */
	DLu(2, "glyphCount        =", fmt->glyphCount);
	DLu(2, "kernValueCount =", (Card16)fmt->kernValueCount);
	DLu(2, "leftClassCount=", (Card16)fmt->leftClassCount);
	DLu(2, "rightClassCount =", (Card16)fmt->rightClassCount);
	DLu(2, "flags=", (Card16)fmt->flags);

	if (tableSize > length)
	{
		printf("kern subtable format 3 size is larger than length!\n");
		return;
	}

	if (level > 2)
	{
	
	if (dumpNames)
		initGlyphNames();
		
	DL(3, (OUTPUTBUFF, "--- kernValue[index]=value\n"));
	for (i = 0; i < fmt->kernValueCount; i++)
		{

		DL(3, (OUTPUTBUFF, "[%lu]=", i));
		dumpValue(&fmt->kernValue[i], level);
		DL(3, (OUTPUTBUFF, ", "));
		}
	DL(3, (OUTPUTBUFF, "\n"));
	
	DL(3, (OUTPUTBUFF, "--- leftClass[glyphIndex]=class value\n"));
	for (i = 0; i < fmt->glyphCount; i++)
		{
		if (dumpNames)
			{
			fprintf(OUTPUTBUFF,  "[%lu]<%s>=%d ", i, getGlyphName((GlyphId)i, 0), fmt->leftClass[i]);
			}
		else
			DL(3, (OUTPUTBUFF,  "[%lu=%d ", i, fmt->leftClass[i]));
		}
	DL(3, (OUTPUTBUFF, "\n"));
	
	DL(3, (OUTPUTBUFF, "--- rightClass[glyphIndex]=class value\n"));
	for (i = 0; i < fmt->glyphCount; i++)
		{
		if (dumpNames)
			{
			fprintf(OUTPUTBUFF,  "[%lu]<%s>=%d ", i, getGlyphName((GlyphId)i, 0), fmt->rightClass[i]);
			}
		else
			DL(3, (OUTPUTBUFF,  "[%lu]=%d ", i, fmt->rightClass[i]));
		}
	DL(3, (OUTPUTBUFF, "\n"));
	
	DL(3, (OUTPUTBUFF, "--- kernValueIndex[index]=kern value index\n"));
	for (i = 0; i < kernIndexSize; i++)
		{
			DL(3, (OUTPUTBUFF,  "[%lu]=%d ", i, fmt->kernIndex[i]));
		}
	DL(3, (OUTPUTBUFF, "\n"));
	}
	}

/* Interpret format 0 table */
static void run0(Format0 *fmt, Card32 length, IntX level)
	{
	IntX i;
	IntX nPairs = (length - FORMAT0_HDR_SIZE) / PAIR_SIZE(nMasters);

	if (nMasters > 1)
		return;
	initGlyphNames();

	fprintf(OUTPUTBUFF,  "--- format 0 [left right value]\n");

	nPairs = fmt->nPairs + 1;	/* Table larger than # pairs + sentinel */

	for (i = 0; i < nPairs; i++)
		{
		Pair *pair = &fmt->pair[i];
		fprintf(OUTPUTBUFF,  "%s ", getGlyphName(pair->left, 0));
		fprintf(OUTPUTBUFF,  "%s %d\n", getGlyphName(pair->right, 0), *pair->value);
		}
	}

/* Interpret format 2 array */
static void run2(Format2 *fmt, Card32 length, IntX level)
	{
	IntX i;
	IntX j;

	if (nMasters > 1)
		return;
	initGlyphNames();

	fprintf(OUTPUTBUFF,  "--- format 2 [left right value]\n");

	for (i = 0; i < fmt->left.nGlyphs; i++)
		{
		long left = fmt->left.classes[i];

		if (left == fmt->arrayOffset)
			continue;	/* Ignore row 0 */

		for (j = 0; j < fmt->right.nGlyphs; j++)
			{
			FWord value;
			long right = fmt->right.classes[j];

			if (right == 0)
				continue;	/* Ignore column 0 */

			value = fmt->array[((left - fmt->arrayOffset) + right) / 2];
			if (value != 0)
				{
				fprintf(OUTPUTBUFF,  "%s ", getGlyphName(fmt->left.firstGlyph + i, 0));
				fprintf(OUTPUTBUFF,  "%s %d\n", getGlyphName(fmt->right.firstGlyph + j, 0),
					   value);
				}
			}
		}
	}

/* Interpret format 0 table */
static void run3(Format3 *fmt, Card32 length, IntX level)
	{
	IntX giL, giR;
	Card32 kernIndexSize, tableSize;
	
	if (nMasters > 1)
		return;


	kernIndexSize = fmt->leftClassCount*fmt->rightClassCount;
	tableSize = FORMAT3_HDR_SIZE + fmt->kernValueCount*sizeof(FWord) + fmt->glyphCount + fmt->glyphCount + kernIndexSize;

	if (tableSize > length)
	{
		fprintf(OUTPUTBUFF, "Error: kern subtable format 3 size '%lu' is greater than subtable length '%lu'. Skipping.\n", tableSize, length);
		return ;
	}
	initGlyphNames();

	fprintf(OUTPUTBUFF,  "--- format 3 [left right value]\n");

	for (giL = 0; giL < fmt->glyphCount; giL++)
		{
		CardX leftClass = fmt->leftClass[giL];

		for (giR = 0; giR < fmt->glyphCount; giR++)
			{
			FWord value;
			CardX rightClass = fmt->rightClass[giR];
			Card32 kernValueIndex = leftClass * fmt->rightClassCount + rightClass;
			kernValueIndex = fmt->kernIndex[kernValueIndex];
			value = fmt->kernValue[kernValueIndex];
			if (value != 0)
				{
				fprintf(OUTPUTBUFF,  "%s ", getGlyphName(giL, 0));
				fprintf(OUTPUTBUFF,  "%s %d\n", getGlyphName(giR, 0),
					   value);
				}
			}
		}
	}
	
/* Dump format-specific subtable */
static void dumpSubtable(IntX type, void *fmt, Card32 length, IntX level)
	{
	switch (type)
		{
	case 0:
		if (level == 5)
			run0(fmt, length, level);
		else
			dump0(fmt, length, level);
		break;
	case 2:
		if (level == 5)
			run2(fmt, length, level);
		else
			dump2(fmt, length, level);
		break;
	case 3:
		if (level == 5)
			run3(fmt, length, level);
		else
			dump3(fmt, length, level);
		break;
		}
	}

/* Dump Microsoft-style table */
static void dumpMS(IntX level)
	{
	IntX i;

	DLu(1, "version=", MSkern->version);
	DLu(1, "nTables=", MSkern->nTables);

	for (i = 0; i < MSkern->nTables; i++)
		{
		IntX type;
		MSSubtable *subtable = &MSkern->subtable[i];
		if (subtable == NULL)
			continue;
		type = (subtable->coverage & MS_COVERAGE_FORMAT)>>8;

		DL(2, (OUTPUTBUFF, "--- subtable[%d]\n", i));
		DLu(2, "version =", subtable->version);
		DLu(2, "length  =", subtable->length);
		DLx(2, "coverage=", subtable->coverage);

		dumpSubtable(type, subtable->format, 
					 subtable->length - MS_SUBTABLE_HDR_SIZE, level);
		}
	}

/* Dump Apple-style table */
static void dumpApple(IntX level)
	{
	IntX i;

	DLV(1, "version=", kern->version);
	DLU(1, "nTables=", kern->nTables);

	for (i = 0; i < (IntX)kern->nTables; i++)
		{
		IntX type;
		Subtable *subtable = &kern->subtable[i];
		if (subtable == NULL)
			continue;
		type = subtable->coverage & COVERAGE_FORMAT;

		DL(2, (OUTPUTBUFF, "--- subtable[%d]\n", i));
		DLU(2, "length    =", subtable->length);
		DLx(2, "coverage  =", subtable->coverage);
		DLu(2, "tupleIndex=", subtable->tupleIndex);

		dumpSubtable(type, subtable->format, 
					 subtable->length - SUBTABLE_HDR_SIZE, level);
		}
	}

void kernDump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [%s] (%08lx) [%s format]\n", 
		   (nMasters == 1) ? "kern" : "KERN", start, 
		   ms ? "Microsoft" : "Apple"));
	if (ms)
		dumpMS(level);
	else
		dumpApple(level);
	}

/* Free format 0 subtable */
static void free0(Format0 *fmt, Card32 length)
	{
	IntX i;
	IntX nPairs = (length - FORMAT0_HDR_SIZE) / PAIR_SIZE(nMasters);

	if (nPairs > fmt->nPairs + 1)
		nPairs = fmt->nPairs + 1;

	for (i = 0; i < nPairs; i++)
		memFree(fmt->pair[i].value);
	memFree(fmt->pair);
	}

/* Free format 2 subtable */
static void free2(Format2 *fmt, Card32 length)
	{
	memFree(fmt->array);
	memFree(fmt->left.classes);
	memFree(fmt->right.classes);
	}

static void free3(Format3 *fmt, Card32 length)
	{
	memFree(fmt->kernValue);
	memFree(fmt->leftClass);
	memFree(fmt->rightClass);
	memFree(fmt->kernIndex);
	}

/* Free format-specific table */
static void freeSubtable(IntX type, void *fmt, Card32 length)
	{
	switch (type)
		{
	case 0:
		free0(fmt, length);
		memFree(fmt);
		break;
	case 2:
		free2(fmt, length);
		memFree(fmt);
		break;
	case 3:
		free3(fmt, length);
		memFree(fmt);
		break;
		}
	}

/* Free Microsoft format table */
static void freeMS(void)
	{
	IntX i;
	IntX type;

	for (i = 0; i < MSkern->nTables; i++)
		{
		MSSubtable *subtable = &MSkern->subtable[i];
		if (subtable == NULL)
			continue;
		type = (subtable->coverage & MS_COVERAGE_FORMAT)>>8;

		freeSubtable(type, subtable->format, subtable->length);
		}
	memFree(MSkern->subtable);
	memFree(MSkern);
	MSkern = NULL;
	}

/* Free Apple format table */
static void freeApple(void)
	{
	IntX i;
	IntX type;

	for (i = 0; i < (IntX)kern->nTables; i++)
		{
		Subtable *subtable = &kern->subtable[i];
		if (subtable == NULL)
			continue;
		type = subtable->coverage & COVERAGE_FORMAT;
		freeSubtable(type, subtable->format, subtable->length);
		}
	memFree(kern->subtable);
	}

void kernFree(void)
	{
    if (!loaded)
		return;

	if (ms)
		freeMS();
	else
		freeApple();
	loaded = 0;
	memFree(kern);
	kern = NULL;
	ms = 0;
	}

void kernUsage(void)
	{
	fprintf(OUTPUTBUFF,  "--- kern\n"
		   "=2  low level dump of subtable headers\n"
		   "=3  low level dump without glyph names\n"
		   "=4  low level dump with glyph names\n"
		   "=5  Print kern pair list\n");
	}
