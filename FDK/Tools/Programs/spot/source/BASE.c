/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BASE.c	1.8
 * Changed:    7/14/99 09:24:32
 ***********************************************************************/

#include "tto.h" /* for DeviceTable stuff */
#include "sfnt_BASE.h"
#include "BASE.h"
#include "sfnt.h"

static BASETbl *BASE = NULL;
static IntX loaded = 0;

static void readBaseTagList(Card32 base, Offset offset, BaseTagList *list)
	{
	IntX i;

	if (offset == 0)
		return;
	SEEK_ABS(base + offset);

	IN1(list->BaseTagCount);

	list->BaselineTag = memNew(sizeof(Tag) * list->BaseTagCount);
	for (i = 0; i < list->BaseTagCount; i++)
		IN1(list->BaselineTag[i]);
	}

static void *readBaseCoord1(Card32 base)
	{
	BaseCoordFormat1 *fmt = memNew(sizeof(BaseCoordFormat1));

	fmt->BaseCoordFormat = 1;	/* Already read */
	IN1(fmt->Coordinate);

	return fmt;
	}

static void *readBaseCoord2(Card32 base)
	{
	BaseCoordFormat2 *fmt = memNew(sizeof(BaseCoordFormat2));

	fmt->BaseCoordFormat = 2;	/* Already read */
	IN1(fmt->Coordinate);
	IN1(fmt->ReferenceGlyph);
	IN1(fmt->BaseCoordPoint);

	return fmt;
	}

static void *readBaseCoord3(Card32 base)
	{
	BaseCoordFormat3 *fmt = memNew(sizeof(BaseCoordFormat3));

	fmt->BaseCoordFormat = 3;	/* Already read */
	IN1(fmt->Coordinate);
	IN1(fmt->DeviceTable);

	ttoReadDeviceTable(base + fmt->DeviceTable, &fmt->_DeviceTable);

	return fmt;
	}

static void *readBaseCoord4(Card32 base)
	{
	BaseCoordFormat4 *fmt = memNew(sizeof(BaseCoordFormat4));

	fmt->BaseCoordFormat = 4;	/* Already read */
	IN1(fmt->IdBaseCoord);

	return fmt;
	}


static void *readBaseCoord(Card32 base, Offset offset)
	{
	Card16 format;
	Card32 newBase = base + offset;

	if (offset == 0)
		return NULL;
	SEEK_ABS(newBase);
	
	IN1(format);
	switch (format)
		{
	case 1:
		return readBaseCoord1(newBase);
	case 2:
		return readBaseCoord2(newBase);
	case 3:
		return readBaseCoord3(newBase);
	case 4:
		return readBaseCoord4(newBase);
	default:
		warning(SPOT_MSG_BASEUNKCOORD, format);
		return NULL;
		}
	}

static void readMinMax(Card32 base, Offset offset, MinMax *minMax)
	{
	IntX i;
	Card32 newBase = base + offset;

	if (offset == 0)
		return;
	SEEK_ABS(newBase);
	
	IN1(minMax->MinCoord);
	IN1(minMax->MaxCoord);
	IN1(minMax->FeatMinMaxCount);

	minMax->FeatMinMaxRecord =
		memNew(sizeof(FeatMinMaxRecord) * minMax->FeatMinMaxCount);
	for (i = 0; i < minMax->FeatMinMaxCount; i++)
		{
		FeatMinMaxRecord *record = &minMax->FeatMinMaxRecord[i];
		IN1(record->FeatureTableTag);
		IN1(record->MinCoord);
		IN1(record->MaxCoord);
		}

	minMax->_MinCoord = readBaseCoord(newBase, minMax->MinCoord);
	minMax->_MaxCoord = readBaseCoord(newBase, minMax->MaxCoord);

	for (i = 0; i < minMax->FeatMinMaxCount; i++)
		{
		FeatMinMaxRecord *record = &minMax->FeatMinMaxRecord[i];
		record->_MinCoord = readBaseCoord(newBase, record->MinCoord);
		record->_MaxCoord = readBaseCoord(newBase, record->MaxCoord);
		}
	}

static void readBaseValues(Card32 base, Offset offset, BaseValues *values)
	{
	IntX i;
	Card32 newBase = base + offset;

	if (offset == 0)
		return;
	SEEK_ABS(newBase);
	
	IN1(values->DefaultIndex);
	IN1(values->BaseCoordCount);
	
	values->BaseCoord = memNew(sizeof(Offset) * values->BaseCoordCount);
	values->_BaseCoord = 
		memNew(sizeof(values->_BaseCoord[0]) * values->BaseCoordCount);
	for (i = 0; i < values->BaseCoordCount; i++)
		IN1(values->BaseCoord[i]);

	for (i = 0; i < values->BaseCoordCount; i++)
		 values->_BaseCoord[i] = readBaseCoord(newBase, values->BaseCoord[i]);
	}

static void readBaseScript(Card32 base, Offset offset, BaseScript *script)
	{
	IntX i;
	Card32 save = TELL();
	Card32 newBase = base + offset;

	SEEK_ABS(newBase);

	IN1(script->BaseValues);
	IN1(script->DefaultMinMax);
	IN1(script->BaseLangSysCount);
	
	script->BaseLangSysRecord = 
		memNew(sizeof(BaseLangSysRecord) * script->BaseLangSysCount);
	for (i = 0; i < script->BaseLangSysCount; i++)
		{
		BaseLangSysRecord *record = &script->BaseLangSysRecord[i];
		IN1(record->BaseLangSysTag);
		IN1(record->MinMax);
		}
	
	readBaseValues(newBase, script->BaseValues, &script->_BaseValues);
	readMinMax(newBase, script->DefaultMinMax, &script->_DefaultMinMax);

	for (i = 0; i < script->BaseLangSysCount; i++)
		{
		BaseLangSysRecord *record = &script->BaseLangSysRecord[i];
		readMinMax(newBase, record->MinMax, &record->_MinMax);
		}
	SEEK_ABS(save);
	}

static void readBaseScriptList(Card32 base, Offset offset, 
							   BaseScriptList *list)
	{
	IntX i;
	Card32 newBase = base + offset;

	SEEK_ABS(newBase);
	
	IN1(list->BaseScriptCount);

	list->BaseScriptRecord = 
		memNew(sizeof(BaseScriptRecord) * list->BaseScriptCount);
	for (i = 0; i < list->BaseScriptCount; i++)
		{
		BaseScriptRecord *record = &list->BaseScriptRecord[i];
		IN1(record->BaseScriptTag);
		IN1(record->BaseScript);
		readBaseScript(newBase, record->BaseScript, &record->_BaseScript);
		}
	}

static void readAxis(Card32 base, Offset offset, Axis *axis)
	{
	Card32 newBase = base + offset;

	if (offset == 0)
		return;
	SEEK_ABS(newBase);

	IN1(axis->BaseTagList);
	IN1(axis->BaseScriptList);

	readBaseTagList(newBase, axis->BaseTagList, &axis->_BaseTagList);
	readBaseScriptList(newBase, axis->BaseScriptList, &axis->_BaseScriptList);
	}

void BASERead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	BASE = (BASETbl *)memNew(sizeof(BASETbl));

	SEEK_ABS(start);

	IN1(BASE->Version);
	IN1(BASE->HorizAxis);
	IN1(BASE->VertAxis);

	readAxis(start, BASE->HorizAxis, &BASE->_HorizAxis);
	readAxis(start, BASE->VertAxis, &BASE->_VertAxis);
	
	loaded = 1;
	}

static int *prevvalues=NULL;
static int currentvalue, numvalues, defaultvalue, nonmatch;

static void dumpBaseTagList(Offset offset, BaseTagList *list, IntX level)
{
	IntX i;

	if (offset == 0)
		return;
	switch(level)
	{
		case 4:
			DL(2, (OUTPUTBUFF, "--- BaseTagList (%04hx)\n", offset));
			DLu(2, "BaseTagCount=", list->BaseTagCount);

			if (list->BaseTagCount == 0)
				return;
			DL(2, (OUTPUTBUFF, "--- BaselineTag[index]=tag\n"));
			for (i = 0; i < list->BaseTagCount; i++)
				DL(2, (OUTPUTBUFF, "[%d]=%c%c%c%c ", i, TAG_ARG(list->BaselineTag[i])));
			DL(2, (OUTPUTBUFF, "\n"));
			break;
		case 5:
			if (list->BaseTagCount == 0)
				return;
			numvalues=list->BaseTagCount;
			prevvalues=memNew(numvalues*sizeof(int));
			for (i = 0; i < list->BaseTagCount; i++)
				fprintf(OUTPUTBUFF, "%c%c%c%c     ", TAG_ARG(list->BaselineTag[i]));
			fprintf(OUTPUTBUFF, "\n");
			break;
	}
}

static void dumpBaseCoord1(BaseCoordFormat1 *fmt, IntX level)
	{
	switch(level)
	{
		case 4:
			DLu(2, "BaseCoordFormat=", fmt->BaseCoordFormat);
			DLs(2, "Coordinate     =", fmt->Coordinate);
			break;
		case 5:
			if(currentvalue>=numvalues){
					fprintf(OUTPUTBUFF,"\nspot [WARNING]: nCoords not same as nTags\n");
					return;
			}
			if(currentvalue==defaultvalue)
				fprintf(OUTPUTBUFF, "<%6d> ", fmt->Coordinate);
			else
				fprintf(OUTPUTBUFF, " %6d  ", fmt->Coordinate);
			if(prevvalues[currentvalue]==-1) 
				prevvalues[currentvalue]=fmt->Coordinate;
			else if(prevvalues[currentvalue]!=fmt->Coordinate){
				nonmatch=1;
				prevvalues[currentvalue]=fmt->Coordinate;
			}
			
			currentvalue++;
			break;
	}
}

static void dumpBaseCoord2(BaseCoordFormat2 *fmt, IntX level)
	{
	if (level==5)
	{
		fprintf(OUTPUTBUFF,"\nspot [WARNING]: unsupported BaseCoordFormat\n");
		return;
	}
	DLu(2, "BaseCoordFormat=", fmt->BaseCoordFormat);
	DLs(2, "Coordinate     =", fmt->Coordinate);
	DLu(2, "ReferenceGlyph =", fmt->ReferenceGlyph);
	DLu(2, "BaseCoordPoint =", fmt->BaseCoordPoint);
	}

static void dumpBaseCoord3(BaseCoordFormat3 *fmt, IntX level)
	{
	if (level==5)
	{
		fprintf(OUTPUTBUFF,"\nspot [WARNING]: unsupported BaseCoordFormat\n");
		return;
	}
	DLu(2, "BaseCoordFormat=", fmt->BaseCoordFormat);
	DLs(2, "Coordinate     =", fmt->Coordinate);

	ttoDumpDeviceTable(fmt->DeviceTable, &fmt->_DeviceTable, level);
	}

static void dumpBaseCoord4(BaseCoordFormat4 *fmt, IntX level)
	{
	if (level==5)
	{
		fprintf(OUTPUTBUFF,"\nspot [WARNING]: unsupported BaseCoordFormat\n");
		return;
	}
	DLu(2, "BaseCoordFormat=", fmt->BaseCoordFormat);
	DLu(2, "IdBaseCoord     =", fmt->IdBaseCoord);
	}


static void dumpBaseCoord(Offset offset, void *fmt, IntX level)
	{
	if (level!=5)
		DL(2, (OUTPUTBUFF, "--- BaseCoord (%04hx)\n", offset));

	switch (((BaseCoordFormat1 *)fmt)->BaseCoordFormat)
		{
	case 1:
		dumpBaseCoord1(fmt, level);
		break;
	case 2:
		dumpBaseCoord2(fmt, level);
		break;
	case 3:
		dumpBaseCoord3(fmt, level);
		break;
	case 4:
		dumpBaseCoord4(fmt, level);
		break;
		}
	}

static void dumpMinMax(Offset offset, MinMax *minMax, IntX level)
	{
	IntX i;

	if (offset == 0)
		return;

	DL(2, (OUTPUTBUFF, "--- MinMax (%04hx)\n", offset));
	DLx(2, "MinCoord       =", minMax->MinCoord);
	DLx(2, "MaxCoord       =", minMax->MaxCoord);
	DLu(2, "FeatMinMaxCount=", minMax->FeatMinMaxCount);

	if (minMax->FeatMinMaxCount != 0)
		{
		DL(2, (OUTPUTBUFF, "--- FeatMinMaxRecord[index]="
			   "{FeatureTableTag,MinCoord,MaxCoord}\n"));
		for (i = 0; i < minMax->FeatMinMaxCount; i++)
			{
			FeatMinMaxRecord *record = &minMax->FeatMinMaxRecord[i];
			DL(2, (OUTPUTBUFF, "[%d]={%c%c%c%c,%04hx,%04hx} ", i,
				   TAG_ARG(record->FeatureTableTag), 
				   record->MinCoord, record->MaxCoord));
			}
		DL(2, (OUTPUTBUFF, "\n"));
		}
	dumpBaseCoord(minMax->MinCoord, minMax->_MinCoord, level);
	dumpBaseCoord(minMax->MaxCoord, minMax->_MaxCoord, level);

	for (i = 0; i < minMax->FeatMinMaxCount; i++)
		{
		FeatMinMaxRecord *record = &minMax->FeatMinMaxRecord[i];
		dumpBaseCoord(record->MinCoord, record->_MinCoord, level);
		dumpBaseCoord(record->MaxCoord, record->_MaxCoord, level);
		}
	}

static void dumpBaseValues(Offset offset, BaseValues *values, IntX level)
	{
	IntX i;
	if (offset == 0)
		return;

	switch(level)
	{
		case 4:
			DL(2, (OUTPUTBUFF, "--- BaseValues (%04hx)\n", offset));
			DLu(2, "DefaultIndex  =", values->DefaultIndex);
			DLu(2, "BaseCoordCount=", values->BaseCoordCount);
			
			DL(2, (OUTPUTBUFF, "--- BaseCoord[index]=offset\n"));
			for (i = 0; i < values->BaseCoordCount; i++)
				DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, values->BaseCoord[i]));
			DL(2, (OUTPUTBUFF, "\n"));

			for (i = 0; i < values->BaseCoordCount; i++)
				dumpBaseCoord(values->BaseCoord[i], values->_BaseCoord[i], level);
			break;
		case 5:
			defaultvalue=values->DefaultIndex;
			currentvalue=0;
			nonmatch=0;
			
			
			for (i = 0; i < values->BaseCoordCount; i++)
				dumpBaseCoord(values->BaseCoord[i], values->_BaseCoord[i], level);
			if(nonmatch)
				fprintf(OUTPUTBUFF,"\nspot [WARNING]: value of baseline differs by script\n");
			nonmatch=0;
			break;
	}
}
	


static void dumpBaseScript(Offset offset, BaseScript *script, 
						   Card32 lang, IntX level)
	{
	IntX i;
	switch(level)
	{
		case 4:
			DL(2, (OUTPUTBUFF, "--- BaseScript (%04hx) [%c%c%c%c]\n", offset, TAG_ARG(lang)));
			DLx(2, "BaseValues      =", script->BaseValues);
			DLx(2, "DefaultMinMax   =", script->DefaultMinMax);
			DLu(2, "BaseLangSysCount=", script->BaseLangSysCount);
			
			if (script->BaseLangSysCount != 0)
				{
				DL(2, (OUTPUTBUFF, "--- BaseLangSysRecord[index]={BaseLangSysTag,MinMax}\n"));
				for (i = 0; i < script->BaseLangSysCount; i++)
					{
					BaseLangSysRecord *record = &script->BaseLangSysRecord[i];
					DL(2, (OUTPUTBUFF, "[%d]={%c%c%c%c,%04hx} ", i, 
						   TAG_ARG(record->BaseLangSysTag), record->MinMax));
					}
				DL(2, (OUTPUTBUFF, "\n"));
				}
			dumpBaseValues(script->BaseValues, &script->_BaseValues, level);
			dumpMinMax(script->DefaultMinMax, &script->_DefaultMinMax, level);

			for (i = 0; i < script->BaseLangSysCount; i++)
				{
				BaseLangSysRecord *record = &script->BaseLangSysRecord[i];
				dumpMinMax(record->MinMax, &record->_MinMax, level);
				}
			break;
		case 5:
			if (script->BaseValues==0)
				fprintf(OUTPUTBUFF, "spot [WARNING]: no baseline values found in script.\n");
			dumpBaseValues(script->BaseValues, &script->_BaseValues, level);
			if (script->BaseLangSysCount!=0)
				fprintf(OUTPUTBUFF, "spot [WARNING]: BaseLangSysCount dump not supported.\n");
			break;
	}
}

static void dumpBaseScriptList(Offset offset, BaseScriptList *list, IntX level)
{
	IntX i;
	switch(level)
	{
		case 4:
			DL(2, (OUTPUTBUFF, "--- BaseScriptList (%04hx)\n", offset));
			DLu(2, "BaseScriptCount=", list->BaseScriptCount);

			DL(2, (OUTPUTBUFF, "--- BaseScriptRecord[index]={BaseScriptTag,BaseScript}\n"));
			for (i = 0; i < list->BaseScriptCount; i++)
				{
				BaseScriptRecord *record = &list->BaseScriptRecord[i];
				DL(2, (OUTPUTBUFF, "[%d]={%c%c%c%c,%04hx} ", i, 
					   TAG_ARG(record->BaseScriptTag), record->BaseScript));
				}
			DL(2, (OUTPUTBUFF, "\n"));
			
			for (i = 0; i < list->BaseScriptCount; i++)
			{
				BaseScriptRecord *record = &list->BaseScriptRecord[i];
				dumpBaseScript(record->BaseScript, &record->_BaseScript, 
							   record->BaseScriptTag, level);
			}
			break;
		case 5:
			for (currentvalue=0; currentvalue<numvalues; currentvalue++)
				prevvalues[currentvalue]=-1;
			for (i = 0; i < list->BaseScriptCount; i++)
			{
				BaseScriptRecord *record = &list->BaseScriptRecord[i];
				if (i>0)
					fprintf(OUTPUTBUFF, ".                        ");
				fprintf(OUTPUTBUFF, "%c%c%c%c ", TAG_ARG(record->BaseScriptTag));
				dumpBaseScript(record->BaseScript, &record->_BaseScript, 
							   record->BaseScriptTag, level);
				
				fprintf(OUTPUTBUFF, "\n");
			}
			fprintf(OUTPUTBUFF, "\n");
			break;
	}
}

static void dumpAxis(Offset offset, Axis *axis, Byte8 *type, IntX level)
	{
	if (offset == 0)
		return;
	switch (level)
	{
		case 4:
			DL(2, (OUTPUTBUFF, "--- Axis (%04hx) [%s]\n", offset, type));
			DLx(2, "BaseTagList   =", axis->BaseTagList);
			DLx(2, "BaseScriptList=", axis->BaseScriptList);
			break;
		case 5:
			fprintf(OUTPUTBUFF, "%sAxis.BaseTagList            ", type);
			break;
			
	}
	dumpBaseTagList(axis->BaseTagList, &axis->_BaseTagList, level);
	if (level==5)
	{
        
        if (*type=='v')
        {
            fprintf(OUTPUTBUFF, "%sAxis.BaseScriptList  ", type);
        }
		else
        {
			fprintf(OUTPUTBUFF, "%sAxis.BaseScriptList ", type);
        }
	}
    dumpBaseScriptList(axis->BaseScriptList, &axis->_BaseScriptList, level);
	if (level==5)
		memFree(prevvalues);
}

void BASEDump(IntX level, LongN start)
{
	if (level==4){
			DL(1, (OUTPUTBUFF, "### [BASE] (%08lx)\n", start));

		DLV(2, "Version  =", BASE->Version);
		DLx(2, "HorizAxis=", BASE->HorizAxis);
		DLx(2, "VertAxis =", BASE->VertAxis);
	}
	dumpAxis(BASE->HorizAxis, &BASE->_HorizAxis, "horiz", level);
	dumpAxis(BASE->VertAxis, &BASE->_VertAxis, "vert", level);
}

int BASEgetValue(Tag tag, char dir, Int16 *value)
{
	Axis *axis;
	BaseTagList *list;
	BaseScriptList *slist;
	BaseScript *script;
	BaseValues *values;
	BaseCoordFormat1 *fmt;
	BaseScriptRecord *record ;
	
	int i, tagIndex=-1;
	
	if (!loaded)
		{
		  if (sfntReadTable(BASE_))
			{
				return 0;
			}
		}
	
	
	if (dir=='h' && BASE->HorizAxis!=0)
		axis=&BASE->_HorizAxis;
	else if (dir=='v' && BASE->VertAxis!=0)
		axis=&BASE->_VertAxis;
	else
		return 0;
		
	list=&axis->_BaseTagList;
	for (i = 0; i < list->BaseTagCount; i++)
	{
		if (list->BaselineTag[i]==tag){
			tagIndex=i;
			break;
		}
	}
	if (tagIndex==-1)
		return 0;
		
	slist=&axis->_BaseScriptList;
	record = &slist->BaseScriptRecord[0];
	script=&record->_BaseScript;
	if (script->BaseValues==0)
		return 0;
	values=&script->_BaseValues;
	fmt=values->_BaseCoord[tagIndex];
	if(fmt->BaseCoordFormat!=1)
		return 0;
	*value=fmt->Coordinate;
	return 1;
}

static void freeBaseCoordFormat3(BaseCoordFormat3 *fmt)
	{
	ttoFreeDeviceTable(&fmt->_DeviceTable);
	}

static void freeBaseCoord(Offset offset, void *fmt)
	{
	if (offset == 0)
		return;

	switch (((BaseCoordFormat1 *)fmt)->BaseCoordFormat)
		{
	case 1: case 2:
		memFree(fmt);
		break;
	case 3:
		freeBaseCoordFormat3(fmt);
		memFree(fmt);
		break;
		}
	}

static void freeMinMax(Offset offset, MinMax *minMax)
	{
	IntX i;

	if (offset == 0)
		return;

	freeBaseCoord(minMax->MinCoord, minMax->_MinCoord);
	freeBaseCoord(minMax->MaxCoord, minMax->_MaxCoord);

	for (i = 0; i < minMax->FeatMinMaxCount; i++)
		{
		FeatMinMaxRecord *record = &minMax->FeatMinMaxRecord[i];
		freeBaseCoord(record->MinCoord, record->_MinCoord);
		freeBaseCoord(record->MaxCoord, record->_MaxCoord);
		}
	memFree(minMax->FeatMinMaxRecord);
	}

static void freeBaseValues(Offset offset, BaseValues *values)
	{
	IntX i;
	
	if (offset == 0)
		return;
	
	for (i = 0; i < values->BaseCoordCount; i++)
	{
		freeBaseCoord(values->BaseCoord[i], &values->_BaseCoord[i]);
		memFree(values->_BaseCoord[i]);
	}

	memFree(values->BaseCoord);
	memFree(values->_BaseCoord);
	}

static void freeBaseScript(Offset offset, BaseScript *script)
	{
	IntX i;
	
	freeBaseValues(script->BaseValues, &script->_BaseValues);
	freeMinMax(script->DefaultMinMax, &script->_DefaultMinMax);

	for (i = 0; i < script->BaseLangSysCount; i++)
		{
		BaseLangSysRecord *record = &script->BaseLangSysRecord[i];
		freeMinMax(record->MinMax, &record->_MinMax);
		}
	memFree(script->BaseLangSysRecord);
	}

static void freeBaseScriptList(BaseScriptList *list)
	{
	IntX i;

	for (i = 0; i < list->BaseScriptCount; i++)
		{
		BaseScriptRecord *record = &list->BaseScriptRecord[i];
		freeBaseScript(record->BaseScript, &record->_BaseScript);
		}
	memFree(list->BaseScriptRecord);
	}

static void freeAxis(Offset offset, Axis *axis)
	{
	if (offset == 0)
		return;

	if (axis->BaseTagList != 0)
		memFree(axis->_BaseTagList.BaselineTag);

	freeBaseScriptList(&axis->_BaseScriptList);
	}

void BASEFree(void)
	{
    if (!loaded)
		return;

	freeAxis(BASE->HorizAxis, &BASE->_HorizAxis);
	freeAxis(BASE->VertAxis, &BASE->_VertAxis);
	memFree(BASE); BASE = NULL;
	loaded = 0;
	}

void BASEUsage(void)
	{
	  fprintf(OUTPUTBUFF,  "--- BASE\n"
		 "=4  List BASE content\n"
		 "=5  Friendly list of BASE content\n"
		 );

	}
