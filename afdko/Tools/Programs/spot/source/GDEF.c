/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)GDEF.c	1.20
 * Changed:    8/4/99 11:28:28
 ***********************************************************************/
 
#include <string.h>
#include "tto.h"
#include "sfnt_GDEF.h"

#include "GDEF.h"
#include "head.h"
#include "sfnt.h"


static GDEFTbl GDEF;
static IntX loaded = 0;
static Card32 GDEFStart;


static void readAttachList(Card32 offset, AttachList* attachList)
	{
	int i, j;
	Card32 save = TELL();
	SEEK_SURE(offset);
	IN1( attachList->Coverage);
	IN1( attachList->GlyphCount);

	attachList->_Coverage = ttoReadCoverage(offset +attachList->Coverage);

	attachList->AttachPoint = memNew(sizeof(Offset) * attachList->GlyphCount);
	attachList->_AttachPoint = 
		memNew(sizeof(attachList->_AttachPoint[0]) * attachList->GlyphCount);
	for (i = 0; i < attachList->GlyphCount; i++)
		{
		IN1(attachList->AttachPoint[i]); /* array of offsets */
		}
		
	for (i = 0; i < attachList->GlyphCount; i++)
		{
		Card32 attachPointeOffset = offset + attachList->AttachPoint[i];
		AttachPoint * attachPoint = &attachList->_AttachPoint[i];
		SEEK_SURE(attachPointeOffset);
		IN1(attachPoint->PointCount);
		attachPoint->PointIndex =
			memNew(sizeof(attachPoint->PointIndex[0]) * attachPoint->PointCount);
		for  (j = 0; j < attachPoint->PointCount; j++)
			IN1(attachPoint->PointIndex[j]);
	
		}
	SEEK_SURE(save);
	}



static void readLigCaretList(Card32 offset, LigCaretList* ligCaretList)
	{
	int i, j;
	Card32 save = TELL();
	SEEK_SURE(offset);
		
	IN1( ligCaretList->Coverage);
	IN1( ligCaretList->LigGlyphCount);
	
	ligCaretList->_Coverage = ttoReadCoverage(offset +ligCaretList->Coverage);

	ligCaretList->LigGlyph = memNew(sizeof(Offset) * ligCaretList->LigGlyphCount);
	ligCaretList->_LigGlyph = 
		memNew(sizeof(ligCaretList->_LigGlyph[0]) * ligCaretList->LigGlyphCount);
	for (i = 0; i < ligCaretList->LigGlyphCount; i++)
		{
		IN1(ligCaretList->LigGlyph[i]);
		}
		
	for (i = 0; i < ligCaretList->LigGlyphCount; i++)
		{
		Card32 ligGlyphOffset = offset + ligCaretList->LigGlyph[i];
		LigGlyph * ligGlyph = &ligCaretList->_LigGlyph[i];
		SEEK_SURE(ligGlyphOffset);
		
		IN1(ligGlyph->CaretCount);
		ligGlyph->CaretValue = memNew(sizeof(Offset) * ligGlyph->CaretCount);
		ligGlyph->_CaretValue =
			memNew(sizeof(ligGlyph->_CaretValue[0]) * ligGlyph->CaretCount);
			
		for  (j = 0; j < ligGlyph->CaretCount; j++)
			{
			IN1(ligGlyph->CaretValue[j]);
			}
		for  (j = 0; j < ligGlyph->CaretCount; j++)
			{
			Card32 caretValuehOffset = ligGlyphOffset + ligGlyph->CaretValue[j];
			CaretValueFormat3 * caretValue = &ligGlyph->_CaretValue[j];
			SEEK_SURE(caretValuehOffset);
			
			IN1(caretValue->CaretValueFormat);
			IN1(caretValue->Coordinate); /* for formats 2 and 4 this has another name, but is still a Card16 */
			if (caretValue->CaretValueFormat == 3)
				{
				IN1(caretValue->DeviceTable);
				ttoReadDeviceTable(caretValuehOffset + caretValue->DeviceTable, &caretValue->_DeviceTable);
				}
			}
		}
	
	SEEK_SURE(save);
	}

static void readMarkGlyphSetsDef(Card32 offset, MarkGlyphSetsDef* markGlyphSetsDef)
	{
	int i;
	Card32 save = TELL();
	SEEK_SURE(offset);
		
	IN1( markGlyphSetsDef->MarkSetTableFormat);
	IN1( markGlyphSetsDef->MarkSetCount);


	markGlyphSetsDef->Coverage = memNew(sizeof(LOffset) * markGlyphSetsDef->MarkSetCount);
	markGlyphSetsDef->_Coverage = memNew(sizeof(void*) * markGlyphSetsDef->MarkSetCount);
	for (i = 0; i < markGlyphSetsDef->MarkSetCount; i++)
		{
		IN1(markGlyphSetsDef->Coverage[i]);
		markGlyphSetsDef->_Coverage[i]= ttoReadCoverage(offset + markGlyphSetsDef->Coverage[i]);
		}
	SEEK_SURE(save);
	}
	
void GDEFRead(LongN start, Card32 length)
	{
	if (loaded)
		return;
	SEEK_ABS(start);
	GDEFStart = start;
	IN1( GDEF.Version);
	IN1( GDEF.GlyphClassDef);
	IN1( GDEF.AttachList);
	IN1( GDEF.LigCaretList);
	IN1( GDEF.MarkAttachClassDef);
	GDEF.hasMarkAttachClassDef = 0;
	
	if (GDEF.Version >= (Fixed)0x00010002)
		IN1( GDEF.MarkGlyphSetsDef);
	else
		GDEF.MarkGlyphSetsDef = 0;
	
	
	if (GDEF.GlyphClassDef)
		GDEF._GlyphClassDef = ttoReadClass(start + GDEF.GlyphClassDef);
		
	if (GDEF.AttachList)
		readAttachList(start + GDEF.AttachList, &GDEF._AttachList);
		
	if (GDEF.LigCaretList)
		readLigCaretList(start + GDEF.LigCaretList, &GDEF._LigCaretList);

	/* Some versions of the GDEF don't have a MarkAttachClassDef entry.
	If they don't, then one of the offsets is at 0x0a */
	if (   GDEF.GlyphClassDef != 0x0a
		&&   GDEF.AttachList != 0x0a
		&& GDEF.LigCaretList != 0x0a
		&&  GDEF.MarkAttachClassDef != 0)
		   {
		   GDEF._MarkAttachClassDef = ttoReadClass(start + GDEF.MarkAttachClassDef);
			GDEF.hasMarkAttachClassDef = 1;
	       }
	else
	   	GDEF.MarkAttachClassDef = 0;

	if (GDEF.MarkGlyphSetsDef)
		readMarkGlyphSetsDef(start + GDEF.MarkGlyphSetsDef, &GDEF._MarkGlyphSetsDef);
	   	
	length = 0; /* supress compiler complaint */
}	
	
static void freeAttachList(AttachList *attachList)
	{
	int i;
	
	if (attachList == NULL)
		return;

	ttoFreeCoverage(attachList->_Coverage);
	
	for (i = 0; i < attachList->GlyphCount; i++)
		{
		AttachPoint * attachPoint = &attachList->_AttachPoint[i];
		memFree(attachPoint->PointIndex);
		}
	memFree(attachList->_AttachPoint);
	memFree(attachList->AttachPoint);
		
	}


static void freeLigCaretList(LigCaretList* ligCaretList)
	{
	int i;
	
	if (ligCaretList == NULL)
		return;

	ttoFreeCoverage(ligCaretList->_Coverage);
	
	for (i = 0; i < ligCaretList->LigGlyphCount; i++)
		{
		LigGlyph * ligGlyph = &ligCaretList->_LigGlyph[i];
		memFree(ligGlyph->_CaretValue);
		memFree(ligGlyph->CaretValue);
		}
	memFree(ligCaretList->_LigGlyph);
	memFree(ligCaretList->LigGlyph);
	}


static void freedMarkGlyphSetsDef(MarkGlyphSetsDef* markGlyphSetsDef)
	{
	int i;
	
	if (markGlyphSetsDef == NULL)
		return;

	for (i = 0; i < markGlyphSetsDef->MarkSetCount; i++)
		{
		ttoFreeCoverage(markGlyphSetsDef->_Coverage[i]);
		}
	memFree(markGlyphSetsDef->_Coverage);
	memFree(markGlyphSetsDef->Coverage);
	}


static void dumpAttachList(Offset attachListOffset, AttachList * attachList, int level)
	{
	int i, j;
  	ttoEnumRec CovList;
  	Card32 nitems;
	
	if (attachListOffset == 0)
		return;
	
	DL(2, (OUTPUTBUFF, "--- AttachList (%04hx)\n", attachListOffset));
	DLu(2, "GlyphCount=", attachList->GlyphCount);

	ttoDumpCoverage(attachList->Coverage, attachList->_Coverage, level);
	ttoEnumerateCoverage(attachList->Coverage, attachList->_Coverage, &CovList, &nitems);
	
	DL(2, (OUTPUTBUFF, "--- AttachPoint[index]=offset\n"));
	for (i = 0; i < attachList->GlyphCount; i++)
		{
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, attachList->AttachPoint[i]));
		}
	DL(2, (OUTPUTBUFF, "\n"));

	for (i = 0; i < attachList->GlyphCount; i++)
		{
		char name[64];
		GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
		AttachPoint * attachPoint = &attachList->_AttachPoint[i];

		strcpy(name, getGlyphName(glyphId, 0));
		if (level == 7)
			{
			fprintf(OUTPUTBUFF, "\n\tAttach %s", name);
			for (j = 0; j < attachPoint->PointCount; j++)
				{
				fprintf(OUTPUTBUFF, " %d", attachPoint->PointIndex[j]);
				}
			fprintf(OUTPUTBUFF, "%s;", name);
			}
		else
			{
			DL(2, (OUTPUTBUFF, "--- AttachPoint index [%d] offset [%04hx]\n", i, attachList->AttachPoint[i]));
			DLu(2, "PointCount=", attachPoint->PointCount);
			DL(2, (OUTPUTBUFF, "Glyph name (%s)\n",name));
			DL(2, (OUTPUTBUFF, "Attachment points= "));
			for (j = 0; j < attachPoint->PointCount; j++)
				{
				DL(2, (OUTPUTBUFF, "%hu ", attachPoint->PointIndex[j]));
				}
			DL(2, (OUTPUTBUFF, "\n"));
			}
		}
	if (level == 7)
		fprintf(OUTPUTBUFF, "\n");
	else
		DL(2, (OUTPUTBUFF, "\n"));
	}

const char* kLigCaretNames[] = {"LigatureCaretZeroTypeName", "LigatureCaretByPos", "LigatureCaretByIndex", "LigatureCaretByPos"};

static void dumpLigCaretList(Offset ligCaretListoffset, LigCaretList* ligCaretList, int level)
	{
	int i, j;
  	ttoEnumRec CovList;
  	Card32 nitems;
	if (ligCaretListoffset == 0)
		return;

	DL(2, (OUTPUTBUFF, "--- LigCaretList (%04hx)\n", ligCaretListoffset));
	DLu(2, "LigGlyphCount=", ligCaretList->LigGlyphCount);

	ttoDumpCoverage(ligCaretList->Coverage, ligCaretList->_Coverage, level);
	ttoEnumerateCoverage(ligCaretList->Coverage, ligCaretList->_Coverage, &CovList, &nitems);

	DL(2, (OUTPUTBUFF, "--- LigCaretList[index]=offset\n"));
	for (i = 0; i < ligCaretList->LigGlyphCount; i++)
		{
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, ligCaretList->LigGlyph[i]));
		}
	DL(2, (OUTPUTBUFF, "\n"));
	
	for (i = 0; i < ligCaretList->LigGlyphCount; i++)
		{
		char name[64];
		CaretValueFormat3 * caretValue;
		GlyphId glyphId = *da_INDEX(CovList.glyphidlist, i);
		LigGlyph * ligGlyph = &ligCaretList->_LigGlyph[i];
		caretValue = &ligGlyph->_CaretValue[0];
		strcpy(name, getGlyphName(glyphId, 0));

		if (level == 7)
			{
			fprintf(OUTPUTBUFF, "\n\t%s %s ", kLigCaretNames[caretValue->CaretValueFormat], name);
			}
		else
			{
			DL(2, (OUTPUTBUFF, "--- LigGlyph index [%d] offset (%04hx)\n", i, ligCaretList->LigGlyph[i]));
			DLu(2, "CaretCount=", ligGlyph->CaretCount);
			DL(2, (OUTPUTBUFF, "LigGlyph name (%s)\n",name));
			}
			
		for  (j = 0; j < ligGlyph->CaretCount; j++)
			{
			CaretValueFormat3 * caretValue = &ligGlyph->_CaretValue[j];
			
			if (level == 7)
				{
				fprintf(OUTPUTBUFF, " %d", caretValue->Coordinate);
				if (caretValue->CaretValueFormat == 3)
					ttoDumpDeviceTable(caretValue->DeviceTable, &caretValue->_DeviceTable, level);
				}
			else
				{
				DL(2, (OUTPUTBUFF, "--- CaretValue  Table index [%d] offset [%04hx]\n", j, ligGlyph->CaretValue[j]));
				DLu(2, "CaretValueFormat   =", caretValue->CaretValueFormat);
				if ((caretValue->CaretValueFormat == 1) || (caretValue->CaretValueFormat == 3))
					{
					DLu(2, "Coordinate   =", caretValue->Coordinate);
					if (caretValue->CaretValueFormat == 3)
						ttoDumpDeviceTable(caretValue->DeviceTable, &caretValue->_DeviceTable, level);
					}
				else
					DLu(2, "IdCaretValue   =", caretValue->Coordinate);
				}
			} /* end caret values for current glyph */
			
		if (level == 7)
			fprintf(OUTPUTBUFF, ";");
		else
			DL(2, (OUTPUTBUFF, "\n"));
			
		} /* end for all glyphs */
	if (level == 7)
		fprintf(OUTPUTBUFF, "\n");
	else
		DL(2, (OUTPUTBUFF, "\n"));
	}


static void dumpMarkGlyphSetsDef(Offset offset, MarkGlyphSetsDef* markGlyphSetsDef, int level)
	{
	int i;
  	ttoEnumRec CovList;
  	Card32 nitems;
	
	if (offset == 0)
		return;

	DL(2, (OUTPUTBUFF, "--- markGlyphSets (%04hx)\n", offset));
	DLu(2, "MarkSetTableFormat=", markGlyphSetsDef->MarkSetTableFormat);
	DLu(2, "MarkSetCount=", markGlyphSetsDef->MarkSetCount);

	DL(2, (OUTPUTBUFF, "--- Coverage[index]=offset\n"));
	for (i = 0; i < markGlyphSetsDef->MarkSetCount; i++)
		{
		DL(2, (OUTPUTBUFF, "[%d]=%04x ", i, markGlyphSetsDef->Coverage[i]));
		}
	DL(2, (OUTPUTBUFF, "\n"));
	for (i = 0; i < markGlyphSetsDef->MarkSetCount; i++)
		{
		Offset localOffset = (Offset)markGlyphSetsDef->Coverage[i];
		if (level == 7)
			{
			long c;
			ttoEnumerateCoverage(localOffset, markGlyphSetsDef->_Coverage[i], &CovList, &nitems);
			if (CovList.glyphidlist.cnt > 0)
				{
				fprintf(OUTPUTBUFF, "\t@GDEF_MarkGlyphSetClass_%d = [", i);
				for (c = 0; c < CovList.glyphidlist.cnt; c++)
					{
					GlyphId glyphId = *da_INDEX(CovList.glyphidlist, c);
					fprintf(OUTPUTBUFF, " %s", getGlyphName(glyphId, 0) );
					}
				fprintf(OUTPUTBUFF, "];\n");
				}
			}
		else
			ttoDumpCoverage(localOffset, markGlyphSetsDef->_Coverage[i], level);
		}
	}

const int kNumGDEFGlyphClasses = 5;
const int kMaxMarkAttachClasses = 16;
const char* kGlyphClassNames[] = {
							"Simple Glyphs - Class 0",
							"Base Glyphs - Class 1",
							"Ligature Glyphs - Class 2",
							"Mark Glyphs - Class 3",
							"Component Glyphs - Class 4",
							};
							
void GDEFDump(IntX level, LongN start)
	{
	  if (!loaded) 
		{
		  if (sfntReadTable(GDEF_))
			return;
		}
	initGlyphNames();

	if (level == 7)
		fprintf(OUTPUTBUFF, "table GDEF {\n");
		
	DL(1, (OUTPUTBUFF, "### [GDEF] (%08lx)\n", start));

	DLV(2, "Version    =", GDEF.Version);
	DLx(2, "GlyphClassDef =", GDEF.GlyphClassDef);
	DLx(2, "AttachList=", GDEF.AttachList);
	DLx(2, "LigCaretList =", GDEF.LigCaretList);
	DLx(2, "MarkAttachClassDef =", GDEF.MarkAttachClassDef);
	if (GDEF.Version >= (Fixed)0x00010002)
		DLx(2, "MarkGlyphSetsDef =", GDEF.MarkGlyphSetsDef);
	
	fprintf(OUTPUTBUFF, "\n\t# Glyph Class Definitions\n");
	if (level == 7)
		{
		ttoEnumRec *classList;
		Card32 classGlyphCount;
		int classId;

		fprintf(OUTPUTBUFF, "\tGlyphClassDef\n");
		classList = memNew(sizeof(ttoEnumRec) * kNumGDEFGlyphClasses);
		ttoEnumerateClass(GDEF.GlyphClassDef, GDEF._GlyphClassDef,
							kNumGDEFGlyphClasses,
							classList, &classGlyphCount);
		for (classId = 1; classId < kNumGDEFGlyphClasses; classId++)
			{
			ttoEnumRec *class;
			long c;
			
			class = &(classList[classId]);
			
			if (class->glyphidlist.cnt == 0)
				{
				/* Handle class with no glyphs assigned. */
				if (classId < (kNumGDEFGlyphClasses -1))
					fprintf(OUTPUTBUFF, "\t\t, # %s\n", kGlyphClassNames[classId]);
				else
					fprintf(OUTPUTBUFF, "\t\t; # %s\n", kGlyphClassNames[classId]);
				}
			else
				{
				fprintf(OUTPUTBUFF, "\t\t# %s\n\t\t[", kGlyphClassNames[classId]);
				for (c = 0; c < class->glyphidlist.cnt; c++)
					{
					GlyphId glyphId = *da_INDEX(class->glyphidlist, c);
					fprintf(OUTPUTBUFF, " %s", getGlyphName(glyphId, 0) );
					}
				if (classId < (kNumGDEFGlyphClasses -1))
					fprintf(OUTPUTBUFF, "],\n");
				else
					fprintf(OUTPUTBUFF, "];\n");
				}
			}
		fprintf(OUTPUTBUFF, "\t# end GlyphClass definitions\n");
		memFree(classList);
		}
	else
		ttoDumpClass(GDEF.GlyphClassDef, GDEF._GlyphClassDef, level);
	
	if (GDEF.AttachList)
		{
		fprintf(OUTPUTBUFF, "\n\t# AttachList Definitions\n");
		dumpAttachList( GDEF.AttachList, &GDEF._AttachList, level);
		}

	if (GDEF.LigCaretList)
		{
		fprintf(OUTPUTBUFF, "\n\t# Ligature Caret List Definitions\n");
		dumpLigCaretList( GDEF.LigCaretList, &GDEF._LigCaretList, level);
		}

	if (GDEF.MarkAttachClassDef)
		{
		fprintf(OUTPUTBUFF, "\n\t# Mark Attach Class Definitions\n");
		if (level == 7)
			{
			ttoEnumRec *classList;
			Card32 classGlyphCount;
			int classId;
			classList = memNew(sizeof(ttoEnumRec) * kMaxMarkAttachClasses);
			ttoEnumerateClass(GDEF.MarkAttachClassDef, GDEF._MarkAttachClassDef,
								kMaxMarkAttachClasses,
								classList, &classGlyphCount);
			for (classId = 0; classId < kMaxMarkAttachClasses; classId++)
				{
				ttoEnumRec *class;
				long c;
				class = &(classList[classId]);
				if (class->glyphidlist.cnt > 0)
					{
					fprintf(OUTPUTBUFF, "\t@GDEF_MarkAttachClass_%d = [", classId);
					for (c = 0; c < class->glyphidlist.cnt; c++)
						{
						GlyphId glyphId = *da_INDEX(class->glyphidlist, c);
						fprintf(OUTPUTBUFF, " %s", getGlyphName(glyphId, 0) );
						}
					fprintf(OUTPUTBUFF, "];\n");
					}
				}
				
			memFree(classList);
			}
		else
			ttoDumpClass(GDEF.MarkAttachClassDef, GDEF._MarkAttachClassDef, level);
		}
			
	if (GDEF.MarkGlyphSetsDef)
		{
		fprintf(OUTPUTBUFF, "\n\t# Mark Glyph Sets Definitions\n");
		dumpMarkGlyphSetsDef( GDEF.MarkGlyphSetsDef, &GDEF._MarkGlyphSetsDef, level);
		}

	if (level == 7)
		fprintf(OUTPUTBUFF, "} GDEF;\n");

	}


void GDEFFree(void)
	{
	  if (!loaded)
		return;
		
	ttoFreeClass(GDEF._GlyphClassDef);
	
	freeAttachList(&GDEF._AttachList);

	freeLigCaretList(&GDEF._LigCaretList);
	
	freedMarkGlyphSetsDef(&GDEF._MarkGlyphSetsDef);

	}


void GDEFUsage(void)
	{
	  fprintf(OUTPUTBUFF,  "--- GDEF\n"
			 "=7  output the more readoble feature file format\n");

	}

