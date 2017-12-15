/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)tto.c	1.3
 * Changed:    3/18/98 11:39:02
 ***********************************************************************/

#include "Dtto.h"
#include "Dsfnt.h"

/* --- ScriptList --- */
static void readLangSys(Card8 which, Card32 offset, LangSys *lang)
	{
	IntX i;
	Card32 save = TELL(which);
	
	SEEK_ABS(which, offset);

	IN(which, lang->LookupOrder);	/* Reserved for future use */
	IN(which, lang->ReqFeatureIndex);
	IN(which, lang->FeatureCount);

	lang->FeatureIndex = 
		memNew(sizeof(lang->FeatureIndex[0]) * lang->FeatureCount);
	for (i = 0; i < lang->FeatureCount; i++)
		IN(which, lang->FeatureIndex[i]);

	SEEK_ABS(which, save);
	}

static void readScript(Card8 which, Card32 offset, Script *script)
	{
	IntX i;
	Card32 save = TELL(which);

	SEEK_ABS(which, offset);

	IN(which, script->DefaultLangSys);
	readLangSys(which, offset + script->DefaultLangSys, &script->_DefaultLangSys);
	
	IN(which, script->LangSysCount);

	script->LangSysRecord = 
		memNew(sizeof(LangSysRecord) * script->LangSysCount);
	for (i = 0; i < script->LangSysCount; i++)
		{
		LangSysRecord *record = &script->LangSysRecord[i];
		
		IN(which, record->LangSysTag);
		IN(which, record->LangSys);
		readLangSys(which, offset + record->LangSys, &record->_LangSys);
		}
	SEEK_ABS(which, save);
	}

void ttoReadScriptList(Card8 which, Card32 offset, ScriptList *list)
	{
	IntX i;
	Card32 save = TELL(which);

	SEEK_ABS(which, offset);

	IN(which, list->ScriptCount);

	list->ScriptRecord = memNew(sizeof(ScriptRecord) * list->ScriptCount);
	for (i = 0; i < list->ScriptCount; i++)
		{
		ScriptRecord *record = &list->ScriptRecord[i];

		IN(which, record->ScriptTag);
		IN(which, record->Script);
		readScript(which, offset + record->Script, &record->_Script);
		}
	SEEK_ABS(which, save);
	}	

static void dumpLangSys(Offset offset, LangSys *lang, IntX level)
	{
	IntX i;

	DL(2, ("--- LangSys (%04hx)\n", offset));
	
	DLx(2, "LookupOrder    =", lang->LookupOrder);
	DLu(2, "ReqFeatureIndex=", lang->ReqFeatureIndex);
	DLu(2, "FeatureCount   =", lang->FeatureCount);

	DL(2, ("--- FeatureIndex[index]=value\n"));
	for (i = 0; i < lang->FeatureCount; i++)
		DL(2, ("[%d]=%hu ", i, lang->FeatureIndex[i]));
	DL(2, ("\n"));	
	}

static void dumpScript(Offset offset, Script *script, IntX level)
	{
	IntX i;

	DL(2, ("--- Script (%04hx)\n", offset));

	DLx(2, "DefaultLangSys=", script->DefaultLangSys);
	DLu(2, "LangSysCount  =", script->LangSysCount);
	
	DL(2, ("--- LangSysRecord[index]={LangSysTag,LangSys}\n"));
	for (i = 0; i < script->LangSysCount; i++)
		{
		LangSysRecord *record = &script->LangSysRecord[i];

		DL(2, ("[%d]={%c%c%c%c,%04hx} ", 
			   i, TAG_ARG(record->LangSysTag), record->LangSys));
		}
	DL(2, ("\n"));

	dumpLangSys(script->DefaultLangSys, &script->_DefaultLangSys, level);
	for (i = 0; i < script->LangSysCount; i++)
		{
		LangSysRecord *record = &script->LangSysRecord[i];
		dumpLangSys(record->LangSys, &record->_LangSys, level);
		}
	}

void ttoDumpScriptList(Offset offset, ScriptList *list, IntX level)
	{
	IntX i;

	DL(2, ("--- ScriptList (%04hx)\n", offset));

	DLu(2, "ScriptCount=", list->ScriptCount);

	DL(2, ("--- ScriptRecord[index]={ScriptTag,Script}\n"));
	for (i = 0; i < list->ScriptCount; i++)
		{
		ScriptRecord *record = &list->ScriptRecord[i];
		
		DL(2, ("[%d]={%c%c%c%c,%04hx} ",
			   i, TAG_ARG(record->ScriptTag), record->Script));
		}
	DL(2, ("\n"));

	for (i = 0; i < list->ScriptCount; i++)
		{
		ScriptRecord *record = &list->ScriptRecord[i];
		dumpScript(record->Script, &record->_Script, level);
		}
	}

void ttoFreeScriptList(ScriptList *list)
	{
	IntX i;

	for (i = 0; i < list->ScriptCount; i++)
		{
		IntX j;
		Script *script = &list->ScriptRecord[i]._Script;
		if (!script) continue;
		memFree(script->_DefaultLangSys.FeatureIndex);

		for (j = 0; j < script->LangSysCount; j++)
			memFree(script->LangSysRecord[j]._LangSys.FeatureIndex);
		memFree(script->LangSysRecord);
		}
	memFree(list->ScriptRecord);
	}

/* --- FeatureList --- */
static void readFeature(Card8 which, Card32 offset, Feature *feature)
	{
	IntX i;
	Card32 save = TELL(which);

	SEEK_ABS(which, offset);

	IN(which, feature->FeatureParam);	/* Reserved for future use */
	IN(which, feature->LookupCount);
	
	feature->LookupListIndex = 
		memNew(sizeof(feature->LookupListIndex[0]) * feature->LookupCount);
	for (i = 0; i < feature->LookupCount; i++)
		IN(which, feature->LookupListIndex[i]);

	SEEK_ABS(which, save);
	}

void ttoReadFeatureList(Card8 which, Card32 offset, FeatureList *list) 
	{
	IntX i;
	Card32 save = TELL(which);
	
	SEEK_SURE(which, offset);

	IN(which, list->FeatureCount);

	list->FeatureRecord = memNew(sizeof(FeatureRecord) * list->FeatureCount);
	for (i = 0; i < list->FeatureCount; i++)
		{
		FeatureRecord *record = &list->FeatureRecord[i];

		IN(which, record->FeatureTag);
		IN(which, record->Feature);
		readFeature(which, offset + record->Feature, &record->_Feature);
		}
	SEEK_SURE(which, save);
	}

void ttoDumpFeatureList(Card16 offset, FeatureList *list, IntX level)
	{
	IntX i;
	if (level == 5) 
	  {
		for (i = 0; i < list->FeatureCount; i++)
		  {
			FeatureRecord *record = &list->FeatureRecord[i];
			printf( "[%d]='%c%c%c%c' (0x%lx)\n", i, TAG_ARG(record->FeatureTag), record->FeatureTag);
		  }
		printf( "\n");
	  }
	else 
	  {
		DL(2, ("--- FeatureList (%04hx)\n", offset));
		
		DLu(2, "FeatureCount=", list->FeatureCount);
		
		DL(2, ("--- FeatureRecord[index]={FeatureTag,Feature}\n"));
		for (i = 0; i < list->FeatureCount; i++)
		  {
			FeatureRecord *record = &list->FeatureRecord[i];
			DL(2, ("[%d]={'%c%c%c%c',%04hx} ", 
				   i, TAG_ARG(record->FeatureTag), record->Feature));
		  }
		DL(2, ("\n"));
		
		for (i = 0; i < list->FeatureCount; i++)
		  {
			IntX j;
			FeatureRecord *record = &list->FeatureRecord[i];
			Feature *feature = &record->_Feature;
			
			DL(2, ("--- FeatureTable (%04hx)\n", record->Feature));
			
			DLx(2, "FeatureParam=", feature->FeatureParam);
			DLu(2, "LookupCount =", feature->LookupCount);
			
			DL(2, ("--- LookupListIndex[index]=value\n"));
			for (j = 0; j < feature->LookupCount; j++)
			  DL(2, ("[%d]=%hu ", j, feature->LookupListIndex[j]));
			DL(2, ("\n"));
		  }
	  }
	}

void ttoFreeFeatureList(FeatureList *list) 
	{
	IntX i;
	for (i = 0; i < list->FeatureCount; i++)
		memFree(list->FeatureRecord[i]._Feature.LookupListIndex);
	memFree(list->FeatureRecord);
	}

/* --- Lookup --- */
static void readLookup(Card8 which, Card32 offset, Lookup *lookup, ttoReadCB readCB)
	{
	IntX i;
	Card32 save = TELL(which);

	SEEK_ABS(which, offset);
	
	IN(which, lookup->LookupType);
	IN(which, lookup->LookupFlag);
	IN(which, lookup->SubTableCount);

	lookup->SubTable = memNew(sizeof(Offset) * lookup->SubTableCount);
	lookup->_SubTable = 
		memNew(sizeof(lookup->_SubTable[0]) * lookup->SubTableCount);
	for (i = 0; i < lookup->SubTableCount; i++)
		{
		  Card32 safety; /* in case call-back fails */
		  IN(which, lookup->SubTable[i]);
		  safety = TELL(which);
		  lookup->_SubTable[i] = 
			readCB(which, offset + lookup->SubTable[i], lookup->LookupType);
		  SEEK_SURE(which, safety);
		}
	SEEK_SURE(which, save);
	}

void ttoReadLookupList(Card8 which, Card32 offset, LookupList *list, ttoReadCB readCB)
	{
	IntX i;
	Card32 save = TELL(which);

	SEEK_SURE(which, offset);

	IN(which, list->LookupCount);

	list->Lookup = memNew(sizeof(Offset) * list->LookupCount);
	list->_Lookup = memNew(sizeof(Lookup) * list->LookupCount);
	for (i = 0; i < list->LookupCount; i++)
		{
		  Card32 safety; /* in case call-back fails */
		  IN(which, list->Lookup[i]);
		  safety = TELL(which);
		  readLookup(which, offset + list->Lookup[i], &list->_Lookup[i], readCB);
		  SEEK_SURE(which, safety);
		}
	SEEK_SURE(which, save);
	}

static void dumpLookup(Offset offset, Lookup *lookup, IntX level, 
					   ttoDumpCB dumpCB, void *arg)
	{
	IntX i;

	DL(2, ("--- Lookup (%04hx)\n", offset));
	
	DLu(2, "LookupType   =", lookup->LookupType);
	DLx(2, "LookupFlag   =", lookup->LookupFlag);
	DLu(2, "SubTableCount=", lookup->SubTableCount);

	DL(2, ("--- SubTable[index]=offset\n"));
	for (i = 0; i < lookup->SubTableCount; i++)
		DL(2, ("[%d]=%04hx ", i, lookup->SubTable[i]));
	DL(2, ("\n"));

	for (i = 0; i < lookup->SubTableCount; i++)
		dumpCB(lookup->SubTable[i], lookup->LookupType, 
			   lookup->_SubTable[i], level, arg);
	}

void ttoDumpLookupList(Offset offset, LookupList *list, IntX level, 
					   ttoDumpCB dumpCB)
	{
	IntX i;
	DL(2, ("--- LookupList (%04hx)\n", offset));

	DLu(2, "LookupCount=", list->LookupCount);

	DL(2, ("--- Lookup[index]=offset\n"));
	for (i = 0; i < list->LookupCount; i++)
		DL(2, ("[%d]=%04hx ", i, list->Lookup[i]));
	DL(2, ("\n"));
		   
	for (i = 0; i < list->LookupCount; i++)
		dumpLookup(list->Lookup[i], &list->_Lookup[i], level, dumpCB, (void *)NULL);
	}

void ttoDumpLookupListItem(Offset offset, LookupList *list, IntX which,
						   IntX level, ttoDumpCB dumpCB)
	{
	DL(2, ("--- LookupList (%04hx)\n", offset));
	DL(2, ("--- Lookup[%d]=%04hx\n", which, list->Lookup[which]));
	dumpLookup(list->Lookup[which], &list->_Lookup[which], level, dumpCB, (void *)NULL);
	}



void ttoFreeLookupList(LookupList *list, ttoFreeCB freeCB)
	{
	IntX i;
	
	for (i = 0; i < list->LookupCount; i++)
		{
		IntX j;
		Lookup *lookup = &list->_Lookup[i];

		for (j = 0; j < lookup->SubTableCount; j++)
			freeCB(lookup->_SubTable[j]);
		memFree(lookup->SubTable);
		memFree(lookup->_SubTable);
		}
	memFree(list->Lookup);
	memFree(list->_Lookup);
	}

/* --- Coverage Table --- */
static void *readCoverage1(Card8 which)
	{
	IntX i;
	CoverageFormat1 *fmt = memNew(sizeof(CoverageFormat1));
		
	fmt->CoverageFormat = 1;	/* Already read */
	IN(which, fmt->GlyphCount);
	
	fmt->GlyphArray = memNew(sizeof(fmt->GlyphArray[0]) * fmt->GlyphCount);
	for (i = 0; i <fmt->GlyphCount; i++)
		IN(which, fmt->GlyphArray[i]);

	return fmt;
	}

static void *readCoverage2(Card8 which)
	{
	IntX i;
	CoverageFormat2 *fmt = memNew(sizeof(CoverageFormat2));
		
	fmt->CoverageFormat = 2;	/* Already read */
	IN(which, fmt->RangeCount);
		
	fmt->RangeRecord = memNew(sizeof(RangeRecord) * fmt->RangeCount);
	for (i = 0; i <fmt->RangeCount; i++)
		{
		RangeRecord *record = &fmt->RangeRecord[i];
			
		IN(which, record->Start);
		IN(which, record->End);
		IN(which, record->StartCoverageIndex);
		}
	return fmt;
	}

void *ttoReadCoverage(Card8 which, Card32 offset)
	{
	void *result;
	Card16 format;
	Card32 save = TELL(which);

	SEEK_SURE(which, offset);

	IN(which, format);
	result = (format == 1) ? readCoverage1(which) : readCoverage2(which);

	SEEK_SURE(which, save);

	return result;
	}

static void dumpCoverage1(Card8 which, CoverageFormat1 *fmt, IntX level)
	{
	IntX i;

	DL(2, ("CoverageFormat=1\n"));
	DLu(2, "GlyphCount    =", fmt->GlyphCount);
	if (level >= 4) 
	  {
		DL(4, ("--- GlyphArray[index]=glyphId glyphName/CID\n"));
		for (i = 0; i < fmt->GlyphCount; i++)
		  DL(4, ("[%d]=%hu (%s) ", i, fmt->GlyphArray[i], getGlyphName(which, fmt->GlyphArray[i])));

	  }
	else 
	  {
		DL(3, ("--- GlyphArray[index]=glyphId\n"));
		for (i = 0; i < fmt->GlyphCount; i++)
		  DL(3, ("[%d]=%hu ", i, fmt->GlyphArray[i]));
	  }

	DL(3, ("\n"));
	}

static void dumpCoverage2(Card8 which, CoverageFormat2 *fmt, IntX level)
	{
	IntX i;

	DL(2, ("CoverageFormat=2\n"));
	DLu(2, "RangeCount    =", fmt->RangeCount);
	if (level >= 4) 
	  {
		DL(4, ("--- RangeRecord[index]={glyphId glyphName/CID, ....}\n"));
		for (i = 0; i <fmt->RangeCount; i++)
		  {
			RangeRecord *record = &fmt->RangeRecord[i];
			IntX a;
			DL(4, ("[%d]={ ", i));
			for (a = record->Start; a <= record->End; a++)
			  {
				DL(4,("%d (%s)  ", a, getGlyphName(which, a)));
			  }
			DL(4, ("}\n"));
		  }
	  }
	else 
	  {
		DL(3, ("--- RangeRecord[index]={Start,End,StartCoverageIndex}\n"));
		for (i = 0; i <fmt->RangeCount; i++)
		  {
			RangeRecord *record = &fmt->RangeRecord[i];
			DL(3, ("[%d]={%hu,%hu,%hu} ", i, 
				   record->Start, record->End, record->StartCoverageIndex));
		  }
	  }
	DL(3, ("\n"));
	}

void ttoDumpCoverage(Card8 which, Offset offset, void *coverage, IntX level)
	{
	DL(2, ("--- Coverage (%04hx)\n", offset));
	if (((CoverageFormat1 *)coverage)->CoverageFormat == 1)
		dumpCoverage1(which, coverage, level);
	else
		dumpCoverage2(which, coverage, level);
	}

void ttoEnumerateCoverage(Offset offset, void *coverage,
						  ttoEnumRec *coverageenum, 
						  Card32 *numitems)
	{
	  IntX n;
	  IntX i, j;

	  coverageenum->mingid = 65535;
	  coverageenum->maxgid = 0;
	  da_INIT(coverageenum->glyphidlist, 10, 10);
	  *numitems = 0;

	  if (((CoverageFormat1 *)coverage)->CoverageFormat == 1)
		{  
		  CoverageFormat1 *fmt1 = (CoverageFormat1 *)coverage;
		  n = 0;
		  for (i = 0; i < fmt1->GlyphCount; i++) 
			{
			  j = fmt1->GlyphArray[i];
			  *da_NEXT(coverageenum->glyphidlist) = j;
			  if (j < coverageenum->mingid)
				coverageenum->mingid = j;
			  if (j > coverageenum->maxgid)
				coverageenum->maxgid = j;
			  n++;
			}
		  *numitems = n;
		}
	  else 
		{
		  CoverageFormat2 *fmt2 = (CoverageFormat2 *)coverage;
		  RangeRecord *rec;
		  n = 0;
		  for (i = 0; i < fmt2->RangeCount; i++) {
			rec = &(fmt2->RangeRecord[i]);
			for (j = rec->Start; j <= rec->End; j++)
			  {
				*da_NEXT(coverageenum->glyphidlist) = j;
				if (j < coverageenum->mingid)
				  coverageenum->mingid = j;
				if (j > coverageenum->maxgid)
				  coverageenum->maxgid = j;
				n++;
			  }
		  }
		  *numitems = n;
		}
	}

static void freeCoverage1(CoverageFormat1 *fmt)
	{
	memFree(fmt->GlyphArray);
	}

static void freeCoverage2(CoverageFormat2 *fmt)
	{
	memFree(fmt->RangeRecord);
	}

void ttoFreeCoverage(void *coverage) 
	{
	if (((CoverageFormat1 *)coverage)->CoverageFormat == 1)
		freeCoverage1(coverage);
	else
		freeCoverage2(coverage);
	memFree(coverage);
	}

/* --- Class Table --- */
static void *readClass1(Card8 which)
	{
	IntX i;
	ClassDefFormat1 *fmt = memNew(sizeof(ClassDefFormat1));
		
	fmt->ClassFormat = 1;	/* Already read */
	IN(which, fmt->StartGlyph);
	IN(which, fmt->GlyphCount);
	
	fmt->ClassValueArray =
		memNew(sizeof(fmt->ClassValueArray[0]) * fmt->GlyphCount);
	for (i = 0; i <fmt->GlyphCount; i++)
		IN(which, fmt->ClassValueArray[i]);

	return fmt;
	}

static void *readClass2(Card8 which)
	{
	IntX i;
	ClassDefFormat2 *fmt = memNew(sizeof(ClassDefFormat2));
		
	fmt->ClassFormat = 2;	/* Already read */
	IN(which, fmt->ClassRangeCount);
		
	fmt->ClassRangeRecord =
		memNew(sizeof(ClassRangeRecord) * fmt->ClassRangeCount);
	for (i = 0; i < fmt->ClassRangeCount; i++)
		{
		ClassRangeRecord *record = &fmt->ClassRangeRecord[i];
			
		IN(which, record->Start);
		IN(which, record->End);
		IN(which, record->Class);
		}
	return fmt;
	}

void *ttoReadClass(Card8 which, Card32 offset)
	{
	void *result;
	Card16 format;
	Card32 save = TELL(which);

	SEEK_SURE(which, offset);
	IN(which, format);
	result = (format == 1) ? readClass1(which) : readClass2(which);

	SEEK_SURE(which, save);
	
	return result;
	}

static void dumpClass1(Card8 which, ClassDefFormat1 *fmt, IntX level)
	{
	IntX i;

	DL(2, ("ClassFormat=1\n"));
	DLu(2, "StartGlyph =", fmt->StartGlyph);
	DLu(2, "GlyphCount =", fmt->GlyphCount);
	if (level >= 4)
	  {
		DL(4, ("--- ClassValueArray[index]=glyphId glyphName/CID classValue\n"));
		for (i = 0; i <fmt->GlyphCount; i++)
		  DL(4, ("[%d]=%d (%s) %hu  ", i, fmt->StartGlyph + i, 
				 getGlyphName(which, fmt->StartGlyph + i), 
				 fmt->ClassValueArray[i]));
	  }
	else 
	  {
		DL(3, ("--- ClassValueArray[index]=value\n"));
		for (i = 0; i <fmt->GlyphCount; i++)
		  DL(3, ("[%d]=%hu ", i, fmt->ClassValueArray[i]));
	  }
	DL(3, ("\n"));
	}

static void dumpClass2(Card8 which, ClassDefFormat2 *fmt, IntX level)
	{
	IntX i;

	DL(2, ("ClassFormat    =2\n"));
	DLu(2, "ClassRangeCount=", fmt->ClassRangeCount);
	if (level >= 4) 
	  {
		DL(4, ("--- ClassRangeRecord[index]={glyphId glyphName/CID=classValue, ...}\n"));
		for (i = 0; i <fmt->ClassRangeCount; i++)
		  {
			IntX a;
			ClassRangeRecord *record = &fmt->ClassRangeRecord[i];
			DL(4, ("[%d]={  ", i));
			for (a = record->Start; a <= record->End; a++)
			  { 
				DL(4,("%d (%s)=%hu  ", a, getGlyphName(which, a), record->Class));
			  }
			DL(4, ("}\n"));
		  }
	  }
	else 
	  {
		DL(3, ("--- ClassRangeRecord[index]={Start,End,Class}\n"));
		for (i = 0; i <fmt->ClassRangeCount; i++)
		  {
			ClassRangeRecord *record = &fmt->ClassRangeRecord[i];
			DL(3, ("[%d]={%hu,%hu,%hu} ", i, 
			   record->Start, record->End, record->Class));
		  }
	  }
	DL(3, ("\n"));
	}

void ttoDumpClass(Card8 which, Offset offset, void *class, IntX level)
	{
	DL(2, ("--- Class (%04hx)\n", offset));
	if (((ClassDefFormat1 *)class)->ClassFormat == 1)
		dumpClass1(which, class, level);
	else
		dumpClass2(which, class, level);
	}


void ttoEnumerateClass(Offset offset, void *class,
					   IntX numclasses,
					   ttoEnumRec *classenumarray,
					   Card32 *numitems)
	{
	  IntX i, glyph, classnum;

	  for (i = 0; i < numclasses; i++) 
		{
		  classenumarray[i].mingid = 65535;
		  classenumarray[i].maxgid = 0;
		  da_INIT(classenumarray[i].glyphidlist, 10, 10);
		}

	  if (((ClassDefFormat1 *)class)->ClassFormat == 1)
		{  
		  ClassDefFormat1 *fmt1 = (ClassDefFormat1 *)class;
		  for (i = 0; i < fmt1->GlyphCount; i++) 
			{
			  glyph = i + fmt1->StartGlyph;
			  classnum = fmt1->ClassValueArray[i];
			  *da_NEXT(classenumarray[classnum].glyphidlist) = glyph;
			  if (glyph < classenumarray[classnum].mingid)
				classenumarray[classnum].mingid = glyph;
			  if (glyph > classenumarray[classnum].maxgid)
				classenumarray[classnum].maxgid = glyph;
			}
		  *numitems = fmt1->StartGlyph + fmt1->GlyphCount;
		}
	  else 
		{
		  ClassDefFormat2 *fmt2 = (ClassDefFormat2 *)class;
		  ClassRangeRecord *rec;
		  IntX n = 0;

		  for (i = 0; i < fmt2->ClassRangeCount; i++) {
			rec = &(fmt2->ClassRangeRecord[i]);
			classnum = rec->Class;
			for (glyph = rec->Start; glyph <= rec->End; glyph++)
			  {
				*da_NEXT(classenumarray[classnum].glyphidlist) = glyph;
				if (glyph < classenumarray[classnum].mingid)
				  classenumarray[classnum].mingid = glyph;
				if (glyph > classenumarray[classnum].maxgid)
				  classenumarray[classnum].maxgid = glyph;				
				n++;
			  }
		  }
		  *numitems = n;
		}
	}


static void freeClass1(ClassDefFormat1 *fmt)
	{
	memFree(fmt->ClassValueArray);
	}

static void freeClass2(ClassDefFormat2 *fmt)
	{
	memFree(fmt->ClassRangeRecord);
	}

void ttoFreeClass(void *class) 
	{
	if (((ClassDefFormat1 *)class)->ClassFormat == 1)
		freeClass1(class);
	else
		freeClass2(class);
	memFree(class);
	}

/* --- Device Table --- */
void ttoReadDeviceTable(Card8 which, Card32 offset, DeviceTable *table) 
	{
	IntX i;
	IntX nWords;
	Card32 save = TELL(which);

	SEEK_ABS(which, offset);

	IN(which, table->StartSize);
	IN(which, table->EndSize);
	IN(which, table->DeltaFormat);

	nWords = ((table->EndSize - table->StartSize + 1) * 
			  (1<<table->DeltaFormat) + 15) / 16;
	table->DeltaValue = memNew(sizeof(table->DeltaValue[0]) * nWords);	
	for (i = 0; i < nWords; i++)
		IN(which, table->DeltaValue[i]);

	SEEK_ABS(which, save);
	}

void ttoDumpDeviceTable(Offset offset, DeviceTable *table, IntX level)
	{
	IntX i;
	IntX nWords = ((table->EndSize - table->StartSize + 1) * 
				   (1<<table->DeltaFormat) + 15) / 16;

	DL(2, ("--- DeviceTable (%04hx)\n", offset));
	
	DLu(2, "StartSize  =", table->StartSize);
	DLu(2, "EndSize    =", table->EndSize);
	DLu(2, "DeltaFormat=", table->DeltaFormat);

	DL(3, ("--- DeltaValue[index]=value\n"));
	for (i = 0; i < nWords; i++)
		DL(3, ("[%d]=%04hx ", i, table->DeltaValue[i]));
	DL(3, ("\n"));
	}

void ttoFreeDeviceTable(DeviceTable *table)
	{
	memFree(table->DeltaValue);
	}

