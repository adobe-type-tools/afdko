/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/
#include <string.h>
#include "tto.h"
#include "sfnt.h"

struct LookupFlagRec {
	  unsigned short val;
	 char * name;
	 };

struct 	LookupFlagRec lookupFlagRec[] = 
		 {
		 	{ RightToLeft, "RightToLeft"},
		 	{IgnoreBaseGlyphs, "IgnoreBaseGlyphs"},
		 	{ IgnoreLigatures, "IgnoreLigatures"},
		 	{ IgnoreMarks, "IgnoreMarks"},
		 	{ UseMarkFilteringSet, "UseMarkFilteringSet"},
		 	{ MarkAttachmentType, "MarkAttachmentType"},
			};
			
unsigned short kNamedLookupMask  = (1 + 2 + 4 + 8 + 16 +0xFF00);

typedef struct {
	Tag ScriptTag;
    Tag LangSysTag;
    Tag featTag;
    Offset featureOffset;
    char *param;
	IntX seen;
} SeenFeatureParam;

/* --- ScriptList --- */
static void readLangSys(Card32 offset, LangSys *lang)
	{
	IntX i;
	Card32 save = TELL();
	
	SEEK_ABS(offset);

	IN1(lang->LookupOrder);	/* Reserved for future use */
	IN1(lang->ReqFeatureIndex);
	IN1(lang->FeatureCount);

	lang->FeatureIndex = 
		memNew(sizeof(lang->FeatureIndex[0]) * lang->FeatureCount);
	for (i = 0; i < lang->FeatureCount; i++)
		IN1(lang->FeatureIndex[i]);

	SEEK_ABS(save);
	}

static void readScript(Card32 offset, Script *script)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);

	IN1(script->DefaultLangSys);
	if (script->DefaultLangSys!=0)
		readLangSys(offset + script->DefaultLangSys, &script->_DefaultLangSys);
	
	IN1(script->LangSysCount);

	script->LangSysRecord = 
		memNew(sizeof(LangSysRecord) * script->LangSysCount);
	for (i = 0; i < script->LangSysCount; i++)
		{
		LangSysRecord *record = &script->LangSysRecord[i];
		
		IN1(record->LangSysTag);
		IN1(record->LangSys);
		readLangSys(offset + record->LangSys, &record->_LangSys);
		}
	SEEK_ABS(save);
	}

void ttoReadScriptList(Card32 offset, ScriptList *list)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);

	IN1(list->ScriptCount);

	list->ScriptRecord = memNew(sizeof(ScriptRecord) * list->ScriptCount);
	for (i = 0; i < list->ScriptCount; i++)
		{
		ScriptRecord *record = &list->ScriptRecord[i];

		IN1(record->ScriptTag);
		IN1(record->Script);
		readScript(offset + record->Script, &record->_Script);
		}
	SEEK_ABS(save);
	}	

static void dumpLangSys(IntX which, Offset offset, LangSys *lang, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- LangSys [%d] (%04hx)\n", which, offset));
	
	DLx(2, "LookupOrder    =", lang->LookupOrder);
	DLu(2, "ReqFeatureIndex=", lang->ReqFeatureIndex);
	DLu(2, "FeatureCount   =", lang->FeatureCount);

	DL(2, (OUTPUTBUFF, "--- FeatureIndex[index]=value\n"));
	for (i = 0; i < lang->FeatureCount; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%hu ", i, lang->FeatureIndex[i]));
	DL(2, (OUTPUTBUFF, "\n"));	
	}

static void dumpScript(IntX which, Offset offset, Script *script, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- Script [%d] (%04hx)\n", which, offset));

	DLx(2, "DefaultLangSys=", script->DefaultLangSys);
	DLu(2, "LangSysCount  =", script->LangSysCount);
	if (script->LangSysCount > 0)
	  {
		DL(2, (OUTPUTBUFF, "--- LangSysRecord[index]={LangSysTag,LangSys}\n"));
	  }
	for (i = 0; i < script->LangSysCount; i++)
		{
		LangSysRecord *record = &script->LangSysRecord[i];

		DL(2, (OUTPUTBUFF, "[%d]={%c%c%c%c,%04hx} ", 
			   i, TAG_ARG(record->LangSysTag), record->LangSys));
		}
	DL(2, (OUTPUTBUFF, "\n"));

	if (script->DefaultLangSys != 0)
		dumpLangSys(-1, script->DefaultLangSys, &script->_DefaultLangSys, level);
		
	for (i = 0; i < script->LangSysCount; i++)
		{
		LangSysRecord *record = &script->LangSysRecord[i];
		dumpLangSys(i, record->LangSys, &record->_LangSys, level);
		}
	}

void ttoDumpScriptList(Offset offset, ScriptList *list, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- ScriptList (%04hx)\n", offset));

	DLu(2, "ScriptCount=", list->ScriptCount);

	DL(2, (OUTPUTBUFF, "--- ScriptRecord[index]={ScriptTag,Script}\n"));
	for (i = 0; i < list->ScriptCount; i++)
		{
		ScriptRecord *record = &list->ScriptRecord[i];
		
		DL(2, (OUTPUTBUFF, "[%d]={%c%c%c%c,%04hx} ",
			   i, TAG_ARG(record->ScriptTag), record->Script));
		}
	DL(2, (OUTPUTBUFF, "\n"));

	for (i = 0; i < list->ScriptCount; i++)
		{
		ScriptRecord *record = &list->ScriptRecord[i];
		dumpScript(i, record->Script, &record->_Script, level);
		}
		DL(2, (OUTPUTBUFF, "\n"));
	}

void ttoFreeScriptList(ScriptList *list)
	{
	IntX i;

	for (i = 0; i < list->ScriptCount; i++)
		{
		IntX j;
		Script *script = &list->ScriptRecord[i]._Script;
		if (!script) continue;
		if (script->DefaultLangSys != 0)
			memFree(script->_DefaultLangSys.FeatureIndex);

		for (j = 0; j < script->LangSysCount; j++)
			memFree(script->LangSysRecord[j]._LangSys.FeatureIndex);
		memFree(script->LangSysRecord);
		}
	memFree(list->ScriptRecord);
	}

/* --- FeatureList --- */
/* Check to see if values are plausible */
static int ValuesArePlausible(Card16* featureParam)
{
	int	plausible = 0;
	Card16 fDesignSize = featureParam[0];
	Card16 fSubfamilyID = featureParam[1];
	Card16 fNameID = featureParam[2];
	Card16 fMinSize = featureParam[3];
	Card16 fMaxSize = featureParam[4];
	
	if (fDesignSize == 0)
		{
		plausible = 0;
		}
	else if (fSubfamilyID == 0
			 && fNameID == 0
			 && fMinSize == 0
			 && fMaxSize == 0)
		{
		plausible = 1;
		}
	else if (fDesignSize < fMinSize
			 || fMaxSize < fDesignSize
			 || fNameID < 256
			 || fNameID > 32767)
		{
		plausible = 0;
		}
	else
		{
		plausible = 1;
		}

	return plausible;
}

static Card32 readFeature(Card32 offset, Feature *feature)
	{
	IntX i;
	Card32 save = TELL();
	Card32 end;

	SEEK_ABS(offset);

	IN1(feature->FeatureParam);
	IN1(feature->LookupCount);
	
	if(feature->LookupCount)
		feature->LookupListIndex = 
			memNew(sizeof(feature->LookupListIndex[0]) * feature->LookupCount);
	else
		feature->LookupListIndex = NULL;
	for (i = 0; i < feature->LookupCount; i++)
		IN1(feature->LookupListIndex[i]);
	end = TELL();
	SEEK_ABS(save);
	return end;
	}

void ttoReadFeatureList(Card32 offset, FeatureList *list) 
	{
        IntX i;
        Card32 save = TELL();
        Card32 save2;
        
        SEEK_SURE(offset);
        
        IN1(list->FeatureCount);
        
        list->FeatureRecord = memNew(sizeof(FeatureRecord) * list->FeatureCount);
        for (i = 0; i < list->FeatureCount; i++)
		{
            FeatureRecord *record = &list->FeatureRecord[i];
            IN1(record->FeatureTag);
            IN1(record->Feature);
            readFeature(offset + record->Feature, &record->_Feature);
		}
        for (i = 0; i < list->FeatureCount; i++)
		{
            FeatureRecord *record = &list->FeatureRecord[i];
            Feature *feature = &record->_Feature;
            if(feature->FeatureParam!=0){
                if(record->FeatureTag==TAG('s', 'i', 'z', 'e')){
                    Card16 *featureParam;
                    feature->_FeatureParam=memNew(12); /* 'size' has 5 16-bit values, and I add a 6th for result flags.*/
                    featureParam = feature->_FeatureParam;
                    featureParam[5] = 0;
                    save2=TELL();
                    /* try correct interpretation of FeatureParam: is offset from start of Feature table. */
                    SEEK_SURE(offset+record->Feature+feature->FeatureParam);
                    fileReadObject(2, featureParam);
                    fileReadObject(2, featureParam+1);
                    fileReadObject(2, featureParam+2);
                    fileReadObject(2, featureParam+3);
                    fileReadObject(2, featureParam+4);
                    if (!ValuesArePlausible(featureParam))
					{/* try old makeotf logic, with offset being from start of FeatureList. */
                        SEEK_SURE(offset+feature->FeatureParam);
                        fileReadObject(2, featureParam);
                        fileReadObject(2, featureParam+1);
                        fileReadObject(2, featureParam+2);
                        fileReadObject(2, featureParam+3);
                        fileReadObject(2, featureParam+4);
                        if (!ValuesArePlausible(featureParam))
						{
                            featureParam[5] = 1;
						}
                        else
                            featureParam[5] = 2;
					}
                    SEEK_SURE(save2);
                }
                else
                {
                    Card16 *featureParam;
                    char* cp = (char*)&record->FeatureTag;
                    if ((cp[3] == 'c') &&
                        (cp[2] == 'v'))
                    {
                        Card16 CharCount ;
                        feature->_FeatureParam=memNew(7*sizeof(Card16)); /* character variant parameter table has at least 7 entries. */
                        featureParam = feature->_FeatureParam;
                        save2=TELL();
                        SEEK_SURE(offset+record->Feature+feature->FeatureParam);
                        fileReadObject(2, featureParam);
                        fileReadObject(2, featureParam+1);
                        fileReadObject(2, featureParam+2);
                        fileReadObject(2, featureParam+3);
                        fileReadObject(2, featureParam+4);
                        fileReadObject(2, featureParam+5);
                        fileReadObject(2, featureParam+6);
                        CharCount = featureParam[6];
                        if (CharCount > 0)
                        {
                            unsigned int c = 0;
                            Card32* uvp;
                            Card16 *newParam = memNew(7*sizeof(Card16) + CharCount*sizeof(Card32)); /* need bigger memory block than when CharCount =0 */
                            unsigned int n = 0;
                            while (n < 7)
                            {
                                newParam[n] = featureParam[n];
                                n++;
                            }
                            memFree(featureParam);
                            featureParam = newParam;
                            /* I will read the 3 byte UV values into a Card32. */
                            uvp = (Card32*)(&featureParam[6+2]); // point to first 4 byte uv value
                            while (c < CharCount)
                            {
                                Card8 val1;
                                Card16 val2;
                                fileReadObject(1, &val1);
                                fileReadObject(2, &val2);
                                *uvp = (val1<<16) + val2;
                                uvp++;
                                c++;
                            }
                       }
                        SEEK_SURE(save2);
                    }
                    else if  ((cp[3] == 's') &&
                                  (cp[2] == 's'))
                    {
                        feature->_FeatureParam=memNew(4); /* stylistic alt name references have two 2 byte entries */
                        featureParam = feature->_FeatureParam;
                        save2=TELL();
                        SEEK_SURE(offset+record->Feature+feature->FeatureParam);
                        fileReadObject(2, featureParam);
                        fileReadObject(2, featureParam+1);
                        SEEK_SURE(save2);
                    }
                    else
                    {
                        fprintf(OUTPUTBUFF, "Unknown tag with FeatureParam\n");
                    }
                    
                }
                
                SEEK_SURE(save);
            }
        }
    }

static void dumpFeatureParams(FeatureRecord *record, Feature *feature, IntX level, Tag currentScript, Tag currentLang, SeenFeatureParam* lastFeatureParam)
{
    Card16 *FeatureParam = feature->_FeatureParam;
    if (lastFeatureParam->param != NULL)
    {
        /* Skip printing the param block if we have already printed it. */
        char* cp = (char*)&record->FeatureTag;
        
        if (record->FeatureTag==TAG('s', 'i', 'z', 'e'))
        {
            if ((0 == memcmp(lastFeatureParam->param, (char*)FeatureParam, 10)) && (record->FeatureTag == lastFeatureParam->featTag))
            {
                if ((level == 7) || (level < 5))
                {
                    if ((currentLang == 0) && (currentScript==0))
                    {
                        
                        fprintf(OUTPUTBUFF, "\n# Skipping optical size parameter block in feature '%c%c%c%c' offset %04hx. It was previously reported for feature '%c%c%c%c' offset %04hx.\n",
                                TAG_ARG(record->FeatureTag), record->Feature, TAG_ARG(lastFeatureParam->featTag), lastFeatureParam->featureOffset);
                    }
                    else
                    {
                        fprintf(OUTPUTBUFF, "\n# Skipping optical size parameter block in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'. It was previously reported for feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
                                TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(lastFeatureParam->featTag), TAG_ARG(lastFeatureParam->ScriptTag), TAG_ARG(lastFeatureParam->LangSysTag));
                    }
                }
                return;
            }
            else
            {
                lastFeatureParam->param = NULL;
            }
        }
        else if ((cp[3] == 's') &&
                 (cp[2] == 's'))
        {
            if ((0 == memcmp(lastFeatureParam->param, (char*)FeatureParam, 4)) && (cp[3] == 's') && (cp[2] == 's'))

            {
                if ((level == 7) || (level < 5))
                {
                    if ((currentLang == 0) && (currentScript==0))
                    {
                        
                        fprintf(OUTPUTBUFF, "\n# Skipping stylistic alternate parameter block in feature '%c%c%c%c' offset %04hx. It was previously reported for feature '%c%c%c%c' offset %04hx.\n",
                                TAG_ARG(record->FeatureTag), record->Feature, TAG_ARG(lastFeatureParam->featTag), lastFeatureParam->featureOffset);
                    }
                    else
                    {
                        
                        fprintf(OUTPUTBUFF, "\n# Skipping stylistic alternate parameter block in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'. It was previously reported for feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
                                TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(lastFeatureParam->featTag), TAG_ARG(lastFeatureParam->ScriptTag), TAG_ARG(lastFeatureParam->LangSysTag));
                    }
                }
                return;
            }
            else
            {
                lastFeatureParam->param = NULL;
            }
        }
        else if ((cp[3] == 'c') &&
                 (cp[2] == 'v'))
        {
            if ((0 == memcmp(lastFeatureParam->param, (char* )FeatureParam, 14))  && (cp[3] == 'c') && (cp[2] == 'v'))

            {
                if ((level == 7) || (level < 5))
                {
                    if ((currentLang == 0) && (currentScript==0))
                    {
                        
                        fprintf(OUTPUTBUFF, "\n# Skipping character variant parameter block in feature '%c%c%c%c' offset %04hx. It was previously reported for feature '%c%c%c%c' offset %04hx.\n",
                                TAG_ARG(record->FeatureTag), record->Feature, TAG_ARG(lastFeatureParam->featTag), lastFeatureParam->featureOffset);
                    }
                    else
                    {
                        fprintf(OUTPUTBUFF, "\n# Skipping character variant parameter block in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'. It was previously reported for feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
                                TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(lastFeatureParam->featTag), TAG_ARG(lastFeatureParam->ScriptTag), TAG_ARG(lastFeatureParam->LangSysTag));
                    }
                }
                return;
            }
            else
            {
                lastFeatureParam->param = NULL;
            }
        }
    }
    
    if (lastFeatureParam->param == NULL)
    {
        lastFeatureParam->param = (char *)FeatureParam;
        lastFeatureParam->LangSysTag = currentLang;
        lastFeatureParam->ScriptTag = currentScript;
        lastFeatureParam->featTag= record->FeatureTag;
        lastFeatureParam->featureOffset = record->Feature;
    }
    
    if(record->FeatureTag==TAG('s', 'i', 'z', 'e')){
        if (FeatureParam[5] == 1)
            fprintf(OUTPUTBUFF, "Data at size feature offset does not look right.\n");
        else if  (FeatureParam[5] == 2)
            fprintf(OUTPUTBUFF, "Using old incorrect FDK 1.6 rule for size feature FeatureParam offset.\n");
        if ( (FeatureParam[5] == 2) ||  (FeatureParam[5] == 0))
        {
            if (level == 7)
            {
                fprintf(OUTPUTBUFF, "\n# Printing optical size record in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
                        TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));
                fprintf (OUTPUTBUFF, "feature size {\n");
                fprintf (OUTPUTBUFF, "\tparameters %d # design size\n",  FeatureParam[0]);
                fprintf (OUTPUTBUFF, "\t\t\t %d # subfamily identifier\n",  FeatureParam[1]);
                fprintf (OUTPUTBUFF, "\t\t\t %d # range start \n",  FeatureParam[3]);
                fprintf (OUTPUTBUFF, "\t\t\t %d; # range end\n",  FeatureParam[4]);
                fprintf (OUTPUTBUFF, "\tsizemenuname ID %d; # must be replaced by string value from name table\n",  FeatureParam[2]);
                fprintf (OUTPUTBUFF, "} size;\n");
            }
            else if (level < 5) {
                DL(2, (OUTPUTBUFF, "   Offset: %04hx\n", feature->FeatureParam));
                DL(2, (OUTPUTBUFF, "   Design Size: %d\n", FeatureParam[0]));
                DL(2, (OUTPUTBUFF, "   Subfamily Identifier: %d\n", FeatureParam[1]));
                DL(2, (OUTPUTBUFF, "   name table name ID for common Subfamily name for size group: %d\n", FeatureParam[2]));
                DL(2, (OUTPUTBUFF, "   Range Start: %d\n", FeatureParam[3]));
                DL(2, (OUTPUTBUFF, "   Range End: %d\n", FeatureParam[4]));
            }
        }
    } else
    {
        if (FeatureParam[0] == 0)
        {
            char* cp = (char*)&record->FeatureTag;
            
            if ((cp[3] == 's') &&
                (cp[2] == 's'))
            {
                
                if (level == 7)
                {
                    fprintf(OUTPUTBUFF, "\n# Printing stylistic name records in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
                            TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));
                    fprintf (OUTPUTBUFF, "featureNames {\n");
                    fprintf (OUTPUTBUFF, "\tname %d; # Must be replaced by string values from name table.\n",  FeatureParam[1]);
                    fprintf (OUTPUTBUFF, "};\n");
                }
                else if (level < 5) {
                    DL(2, (OUTPUTBUFF, "   Stylistic Alternate name parameter version: %d\n", FeatureParam[0]));
                    DL(2, (OUTPUTBUFF, "   Stylistic Alternate name parameter name table name ID: %d\n", FeatureParam[1]));
                }
            }
            else if ((cp[3] == 'c') &&
                     (cp[2] == 'v'))
            {
                if (level == 7)
                {
                    fprintf(OUTPUTBUFF, "\n# Printing character variant parameter table in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
                            TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));
                    fprintf (OUTPUTBUFF, "cvParameters {\n");
                    
                    fprintf (OUTPUTBUFF, "\tFeatUILabelNameID {\n");
                    fprintf (OUTPUTBUFF, "\t\tname %d; # Must be replaced by string values from name table.\n",  FeatureParam[1]);
                    fprintf (OUTPUTBUFF, "\t};\n");
                    
                    fprintf (OUTPUTBUFF, "\tFeatUITooltipTextNameID {\n");
                    fprintf (OUTPUTBUFF, "\t\tname %d; # Must be replaced by string values from name table.\n",  FeatureParam[2]);
                    fprintf (OUTPUTBUFF, "\t};\n");
                    
                    fprintf (OUTPUTBUFF, "\tSampleTextNameID {\n");
                    fprintf (OUTPUTBUFF, "\t\tname %d; # Must be replaced by string values from name table.\n",  FeatureParam[3]);
                    fprintf (OUTPUTBUFF, "\t};\n");
                    
                    {
                        Card16 numNamedParameters = FeatureParam[4];
                        Card16 paramUILabelNameID  = FeatureParam[5];
                        Card16 n = 0;
                        while (n < numNamedParameters)
                        {
                            fprintf (OUTPUTBUFF, "\tParamUILabelNameID {\n");
                            fprintf (OUTPUTBUFF, "\t\tname %d; # Must be replaced by string values from name table.\n",  paramUILabelNameID);
                            fprintf (OUTPUTBUFF, "\t};\n");
                            paramUILabelNameID++;
                            n++;
                            
                        }
                    }
                    {
                        unsigned int c = 0;
                        Card16 CharCount = FeatureParam[6];
                        Card32* uvp = (Card32*)(&FeatureParam[6+2]);
                        DL(2, (OUTPUTBUFF, "# Character Variant CharCount: %d\n", CharCount));
                        if (CharCount > 0)
                        {
                            while (c < CharCount)
                            {
                                Card32 uv = uvp[c];
                                fprintf (OUTPUTBUFF, "\tCharacter 0x%04lx;\n", uv);
                                c++;
                            }
                        }
                    }
                    fprintf (OUTPUTBUFF, "};\n"); // end cvParameters block
                    
                }
                else if (level < 5) {
                    Card16 CharCount = 0;
                    unsigned int c = 0;
                    Card32* uvp = (Card32*)(&FeatureParam[6+2]);
                    DL(2, (OUTPUTBUFF, "   Character Variant Format: %d\n", FeatureParam[0]));
                    DL(2, (OUTPUTBUFF, "   Character Variant FeatUILabelNameID: %d\n", FeatureParam[1]));
                    DL(2, (OUTPUTBUFF, "   Character Variant FeatUITooltipTextNameID: %d\n", FeatureParam[2]));
                    DL(2, (OUTPUTBUFF, "   Character Variant SampleTextNameID: %d\n", FeatureParam[3]));
                    DL(2, (OUTPUTBUFF, "   Character Variant NumNamedParameters: %d\n", FeatureParam[4]));
                    DL(2, (OUTPUTBUFF, "   Character Variant FirstParamUILabelNameID: %d\n", FeatureParam[5]));
                    CharCount = FeatureParam[6];
                    DL(2, (OUTPUTBUFF, "   Character Variant CharCount: %d\n", CharCount));
                    while (c < CharCount)
                    {
                        Card32 uv = uvp[c];
                        DL(2, (OUTPUTBUFF, "   Character Variant index: %d. Character: 0x%04lx\n", c, uv));
                        c++;
                    }
                }
            }
            else
            {
                DL(2, (OUTPUTBUFF, "First short word of the FeatureParam is 0, but the FeatureParam is neither stylistic alternate name nor character variant parameter table!\n"));
            }
        }
        else
        {
            DL(2, (OUTPUTBUFF, "FeatureParam is not zero but is neither size nor stylistic alternate name\n"));
        }
        
    }
}


/* This is currently used only for a straight text dump */
void ttoDumpFeatureList(Card16 offset, FeatureList *list, IntX level)
	{
	IntX i, j;
    SeenFeatureParam lastFeatureParam;
    lastFeatureParam.param = NULL;

        if (level == 5)
	  {
		for (i = 0; i < list->FeatureCount; i++)
		  {
			FeatureRecord *record = &list->FeatureRecord[i];
			Feature *feature = &record->_Feature;
			fprintf(OUTPUTBUFF, "[%2d]='%c%c%c%c' (0x%lx) LookupListIndex: ", i, TAG_ARG(record->FeatureTag), record->FeatureTag);
			
			
			for (j = 0; j < feature->LookupCount; j++)
				  fprintf(OUTPUTBUFF,  "%hu ",  feature->LookupListIndex[j]);
			fprintf(OUTPUTBUFF,  "\n");

			/*ASP: Size specific code*/
			if(feature->FeatureParam!=0) {
				if(record->FeatureTag==TAG('s', 'i', 'z', 'e')){
					Card16 *FeatureParam = feature->_FeatureParam;
					if (FeatureParam[5] == 1)
						fprintf(OUTPUTBUFF, "Data at size feature offset does not look right.\n");
					else if  (FeatureParam[5] == 2)
						fprintf(OUTPUTBUFF, "Using old incorrect FDK 1.6 rule for size feature FeatureParam offset.\n");
				} else
					{
					Card16 *FeatureParam = feature->_FeatureParam;
					if (FeatureParam[0] != 0)
						/* doesn't look like stylistic feature name, either */
						fprintf(OUTPUTBUFF, "FeatureParam is not zero but feature tag is unknown!\n");
					}
			}
		  }
		fprintf(OUTPUTBUFF,  "\n");
	  }
	else
	  {
		DL(2, (OUTPUTBUFF, "--- FeatureList (%04hx)\n", offset));
		
		DLu(2, "FeatureCount=", list->FeatureCount);
		
		DL(2, (OUTPUTBUFF, "--- FeatureRecord[index]={FeatureTag,Feature}\n"));
		for (i = 0; i < list->FeatureCount; i++)
		  {
			FeatureRecord *record = &list->FeatureRecord[i];
			DL(2, (OUTPUTBUFF, "[%d]={'%c%c%c%c',%04hx} ", 
				   i, TAG_ARG(record->FeatureTag), record->Feature));
			if ((i % 4) == 3)
				DL(2, (OUTPUTBUFF, "\n"));
		  }
		DL(2, (OUTPUTBUFF, "\n"));
		
		for (i = 0; i < list->FeatureCount; i++)
		  {
			FeatureRecord *record = &list->FeatureRecord[i];
			Feature *feature = &record->_Feature;
			
			DL(2, (OUTPUTBUFF, "--- FeatureTable (%04hx)\n", record->Feature));
			
              DLx(2, "FeatureParam=", feature->FeatureParam);
			
			/*ASP: Size specific code*/
			if(feature->FeatureParam!=0) {
                dumpFeatureParams(record, feature, level, 0, 0, &lastFeatureParam);
            }
              DLu(2, "LookupCount =", feature->LookupCount);
              
              if(feature->LookupCount>0){
                  DL(2, (OUTPUTBUFF, "--- LookupListIndex[index]=value\n"));
                  for (j = 0; j < feature->LookupCount; j++)
                      DL(2, (OUTPUTBUFF, "[%d]=%hu ", j, feature->LookupListIndex[j]));
                  DL(2, (OUTPUTBUFF, "\n"));
              }			
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
static void readLookup(Card32 offset, Lookup *lookup, ttoReadCB readCB)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_ABS(offset);
	
	IN1(lookup->LookupType);
	IN1(lookup->LookupFlag);
	IN1(lookup->SubTableCount);

	lookup->SubTable = memNew(sizeof(Offset) * lookup->SubTableCount);
	lookup->_SubTable = 
		memNew(sizeof(lookup->_SubTable[0]) * lookup->SubTableCount);
	for (i = 0; i < lookup->SubTableCount; i++)
		{
		  Card32 safety; /* in case call-back fails */
		  IN1(lookup->SubTable[i]);
		  safety = TELL();
		  lookup->_SubTable[i] = 
			readCB(offset + lookup->SubTable[i], lookup->LookupType);
		  SEEK_SURE(safety);
		}
	if (lookup->LookupFlag & UseMarkFilteringSet)
		IN1(lookup->MarkFilteringSet);
	SEEK_SURE(save);
	}

void ttoReadLookupList(Card32 offset, LookupList *list, ttoReadCB readCB)
	{
	IntX i;
	Card32 save = TELL();

	SEEK_SURE(offset);

	IN1(list->LookupCount);

	list->Lookup = memNew(sizeof(Offset) * list->LookupCount);
	list->_Lookup = memNew(sizeof(Lookup) * list->LookupCount);
	for (i = 0; i < list->LookupCount; i++)
		{
		  Card32 safety; /* in case call-back fails */
		  IN1(list->Lookup[i]);
		  safety = TELL();
		  readLookup(offset + list->Lookup[i], &list->_Lookup[i], readCB);
		  SEEK_SURE(safety);
		}
	SEEK_SURE(save);
	}

static void dumpLookup(Offset offset, Lookup *lookup, IntX lookupindex, IntX level, 
					   ttoDumpCB dumpCB, void *arg)
	{
	IntX i;
	char commentChar;
	
	if (level == 8)
		commentChar = '%';
	else
		commentChar = '#';

	if (lookupindex >= 0)
	  {
		DL(2, (OUTPUTBUFF, "--- Lookup [%d] (%04hx)\n", lookupindex, offset));
		DDL(7, (OUTPUTBUFF, "%c --- Start Lookup [%d])\n", commentChar, lookupindex));
	  }
	else
	  {
		DL(2, (OUTPUTBUFF, "--- Lookup (%04hx)\n", offset));
	  }
	
	DLx(2, "LookupType   =", lookup->LookupType);
	
	if ((lookup->LookupFlag > 0) && (level == 7))
		{
		fprintf(OUTPUTBUFF, "lookupflag ");
			
		if (lookup->LookupFlag & ~kNamedLookupMask)
			{
			fprintf(OUTPUTBUFF, " %04hx;\n", lookup->LookupFlag);
			}
		else
			{
			char seenName = 0;
			for (i = 0; i < kNumNamedLookups; i++)
				{
				if (lookup->LookupFlag & lookupFlagRec[i].val)
					{
					if (seenName)
						fprintf(OUTPUTBUFF, " ");
					else
						seenName = 1;
					fprintf(OUTPUTBUFF, "%s", lookupFlagRec[i].name);
					if (lookupFlagRec[i].val == 0x10)
						fprintf(OUTPUTBUFF, " index %d", lookup->MarkFilteringSet);
					if (lookupFlagRec[i].val == 0xFF00)
						fprintf(OUTPUTBUFF, " @GDEF_MarkAttachClass_%d", (lookup->LookupFlag & 0xFF00) >>8);
						
					}
				
				}
			}
		fprintf(OUTPUTBUFF, ";\n");
		}
	else
		{
		DLx(2, "LookupFlag   =", lookup->LookupFlag);
		if (lookup->LookupFlag & UseMarkFilteringSet)
			DLu(2, "UseMarkFilteringSet @GDEF_MarkGlyphSet_", lookup->MarkFilteringSet);
		}
	
	DLu(2, "SubTableCount=", lookup->SubTableCount);
	

	DL(2, (OUTPUTBUFF, "--- SubTable[index]=offset\n"));
	for (i = 0; i < lookup->SubTableCount; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, lookup->SubTable[i]));
	DL(2, (OUTPUTBUFF, "\n"));

	for (i = 0; i < lookup->SubTableCount; i++){
		DDL(7, (OUTPUTBUFF, "%c -------- SubTable %d\n", commentChar, i));
		dumpCB(lookup->SubTable[i], lookup->LookupType, 
			   lookup->_SubTable[i], level, arg, lookupindex, i, lookup->SubTableCount, 0);
		}
	if (lookupindex >= 0)
	  {
		DDL(7, (OUTPUTBUFF, "%c --- End Lookup [%d])\n", commentChar, lookupindex));
	  }
	}

void ttoDumpLookupList(Offset offset, LookupList *list, IntX level, 
					   ttoDumpCB dumpCB)
	{
	IntX i;
	DL(2, (OUTPUTBUFF, "--- LookupList (%04hx)\n", offset));

	DLu(2, "LookupCount=", list->LookupCount);

	DL(2, (OUTPUTBUFF, "--- Lookup[index]=offset\n"));
	for (i = 0; i < list->LookupCount; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, list->Lookup[i]));
	DL(2, (OUTPUTBUFF, "\n"));
		   
	for (i = 0; i < list->LookupCount; i++)
		dumpLookup(list->Lookup[i], &list->_Lookup[i], i, level, dumpCB, (void *)NULL);
	}

void ttoDumpLookupListItem(Offset offset, LookupList *list, IntX which,
						   IntX level, ttoDumpCB dumpCB, void *feattag)
	{
	DL(2, (OUTPUTBUFF, "--- LookupList (%04hx)\n", offset));
	DL(2, (OUTPUTBUFF, "--- Lookup[%d]=%04hx\n", which, list->Lookup[which]));
	dumpLookup(list->Lookup[which], &list->_Lookup[which], which, level, dumpCB, feattag);
	}


typedef struct {
	Tag ScriptTag, LangSysTag;
	IntX seen;
} SeenLookup;

void ttoDumpLookupifFeaturePresent(LookupList *lookuplist,
								   FeatureList *featurelist,
								   ScriptList * scriptlist,
								   IntX level,
								   ttoDumpCB dumpCB, ttoMessageCB messageCB, ttoPostDumpCB postDumpCB)
	{
	IntX i, j, k, m, n, index;
	IntX ret;
	SeenLookup *SeenLookups;
    SeenFeatureParam lastFeatureParam;
    Tag currentScript, currentLang;
    lastFeatureParam.param = NULL;
	
	SeenLookups = (SeenLookup *)memNew(lookuplist->LookupCount*sizeof(SeenLookup));
	for (i=0; i< lookuplist->LookupCount; i++)
		SeenLookups[i].seen=0;
	
	for (i = 0; i < featurelist->FeatureCount; i++)
	  {
		FeatureRecord *record = &featurelist->FeatureRecord[i];
		Feature *feature = &record->_Feature;

		ret = sfntIsInFeatProofList(record->FeatureTag);
		if (ret == 0) continue; /* feature not in list */
		if ((ret < 0) /* empty list --> ?do all of them? */
			|| (ret > 0))
		{
		  /*Search for a script with a language that refers to this feature*/
		  for (j=0; j< scriptlist->ScriptCount; j++)
		  {
		  		ScriptRecord * srecord = &scriptlist->ScriptRecord[j];
		  		Script *script = &srecord->_Script;
		  		LangSys * defaultLangSys = &script->_DefaultLangSys;
		  		
		  		for (k=0; k< script->LangSysCount + 1; k++)
		  		{
		  			LangSysRecord *lrecord = NULL;
		  			LangSys * langsys;
		  			if (k == script->LangSysCount)
		  				{
						if (script->DefaultLangSys == 0)
							continue;

			  			langsys = defaultLangSys;
		  				}
		  			else
		  				{
			  			lrecord = &script->LangSysRecord[k];
			  			langsys = &lrecord->_LangSys;
		  				}
		  			for (m=0; m<langsys->FeatureCount; m++)
		  			{
		  				if (langsys->FeatureIndex[m]==i)
		  				{
	  						currentScript=srecord->ScriptTag;
		  					if (k == script->LangSysCount)
	  							currentLang=TAG('d', 'f', 'l', 't');
		  					else
		  						currentLang=lrecord->LangSysTag;

                            /*ASP: Size specific code*/
                            if(feature->FeatureParam!=0) {
                                dumpFeatureParams(record, feature, level, currentScript, currentLang, &lastFeatureParam);
                            }
                            
		  					for (n = 0; n < feature->LookupCount; n++)
							{
							  index = feature->LookupListIndex[n];
							  if(SeenLookups[index].seen)
							  {
									char temp[256];
							  		sprintf(temp, "Skipping lookup %d in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c' because already proofed in script '%c%c%c%c', language '%c%c%c%c'.\n", 
							  			index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(SeenLookups[index].ScriptTag), TAG_ARG(SeenLookups[index].LangSysTag));
							  		messageCB(temp, record->FeatureTag);
							  }else{
							  	  Tag tags[3];
							  	  tags[0]=record->FeatureTag;
							  	  tags[1]=currentScript;
							  	  tags[2]=currentLang;
								 /* fprintf(stderr, "\nPrinting lookup %d in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
								  			index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));*/
								  dumpLookup(lookuplist->Lookup[index],
										 &lookuplist->_Lookup[index], index,
										 (ret < 0) ? level : ret, dumpCB, tags); /*(void *)record->FeatureTag);  */
								  SeenLookups[index].seen++;
								  SeenLookups[index].ScriptTag=currentScript;
								  SeenLookups[index].LangSysTag=currentLang;
							  }
							 }
							 if ((void*)postDumpCB)
							 	postDumpCB(record->FeatureTag);
						}
					}
				}
								
			}
		}
	  }	
	  memFree(SeenLookups);
	}

void ttoFreeLookupList(LookupList *list, ttoFreeCB freeCB)
	{
	IntX i;
	
	for (i = 0; i < list->LookupCount; i++)
		{
		IntX j;
		Lookup *lookup = &list->_Lookup[i];

		for (j = 0; j < lookup->SubTableCount; j++)
			freeCB(lookup->LookupType, lookup->_SubTable[j]);
		memFree(lookup->SubTable);
		memFree(lookup->_SubTable);
		}
	memFree(list->Lookup);
	memFree(list->_Lookup);
	}

/* --- Coverage Table --- */
static void *readCoverage1(void)
	{
	IntX i;
	CoverageFormat1 *fmt = memNew(sizeof(CoverageFormat1));
		
	fmt->CoverageFormat = 1;	/* Already read */
	IN1(fmt->GlyphCount);
	
	fmt->GlyphArray = memNew(sizeof(fmt->GlyphArray[0]) * fmt->GlyphCount);
	for (i = 0; i <fmt->GlyphCount; i++)
		IN1(fmt->GlyphArray[i]);

	return fmt;
	}

static void *readCoverage2(void)
	{
	IntX i;
	CoverageFormat2 *fmt = memNew(sizeof(CoverageFormat2));
		
	fmt->CoverageFormat = 2;	/* Already read */
	IN1(fmt->RangeCount);
		
	fmt->RangeRecord = memNew(sizeof(RangeRecord) * fmt->RangeCount);
	for (i = 0; i <fmt->RangeCount; i++)
		{
		RangeRecord *record = &fmt->RangeRecord[i];
			
		IN1(record->Start);
		IN1(record->End);
		IN1(record->StartCoverageIndex);
		}
	return fmt;
	}

void *ttoReadCoverage(Card32 offset)
	{
	void *result;
	Card16 format;
	Card32 save = TELL();

	SEEK_SURE(offset);

	IN1(format);
	switch (format)
	  {
	  case 1:
		result = readCoverage1(); break;
	  case 2:
		result = readCoverage2(); break;
	  default:
		warning(SPOT_MSG_BADUNKCOVERAGE, format, offset);
		result = NULL;
		break;
	  }

	SEEK_SURE(save);

	return result;
	}

static void dumpCoverage1(CoverageFormat1 *fmt, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "CoverageFormat=1\n"));
	DLu(2, "GlyphCount    =", fmt->GlyphCount);
	if (level >= 4) 
	  {
		DL(4, (OUTPUTBUFF, "--- GlyphArray[index]=glyphId glyphName/CID\n"));
		for (i = 0; i < fmt->GlyphCount; i++)
		  DL(4, (OUTPUTBUFF, "[%d]=%hu (%s) ", i, fmt->GlyphArray[i], getGlyphName(fmt->GlyphArray[i], 0)));

	  }
	else 
	  {
		DL(3, (OUTPUTBUFF, "--- GlyphArray[index]=glyphId\n"));
		for (i = 0; i < fmt->GlyphCount; i++)
		  DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->GlyphArray[i]));
	  }

	DL(3, (OUTPUTBUFF, "\n"));
	}

static void dumpCoverage2(CoverageFormat2 *fmt, IntX level)
	{
	IntX i;
	IntX lastEnd = -1;
	DL(2, (OUTPUTBUFF, "CoverageFormat=2\n"));
	DLu(2, "RangeCount    =", fmt->RangeCount);
	if (level >= 4) 
	  {
		DL(4, (OUTPUTBUFF, "--- RangeRecord[index]={glyphId glyphName/CID, ....}\n"));
		for (i = 0; i <fmt->RangeCount; i++)
		  {
			RangeRecord *record = &fmt->RangeRecord[i];
			IntX a;
			DL(4, (OUTPUTBUFF, "StartCoverageIndex= %d\n", record->StartCoverageIndex));
			DL(4, (OUTPUTBUFF, "[%d]={ ", i));
			for (a = record->Start; a <= record->End; a++)
			  {
				DL(4,(OUTPUTBUFF, "%d (%s)  ", a, getGlyphName(a, 0)));
			  }
			DL(4, (OUTPUTBUFF, "}\n"));
			
			if (lastEnd >= record->Start)
				DL(4,(OUTPUTBUFF, "End of previous range (%d) is >=  range start (%d). range index: '%d'.", lastEnd, record->Start, i));
			lastEnd = record->End;
		  }
	  }
	else 
	  {
		DL(3, (OUTPUTBUFF, "--- RangeRecord[index]={Start,End,StartCoverageIndex}\n"));
		for (i = 0; i <fmt->RangeCount; i++)
		  {
			RangeRecord *record = &fmt->RangeRecord[i];
			DL(3, (OUTPUTBUFF, "[%d]={%hu,%hu,%hu} ", i, 
				   record->Start, record->End, record->StartCoverageIndex));
		  }
	  }
	DL(3, (OUTPUTBUFF, "\n"));
	}

void ttoDumpCoverage(Offset offset, void *coverage, IntX level)
	{
	  Card16 format;
	  if (((CoverageFormat1 *)coverage) == NULL) return;
	  DL(2, (OUTPUTBUFF, "--- Coverage (%04hx)\n", offset));
	  format = ((CoverageFormat1 *)coverage)->CoverageFormat;
	  switch (format)
		{
		case 1:
		  dumpCoverage1(coverage, level); break;
		case 2:
		  dumpCoverage2(coverage, level); break;
		default: 
		  warning(SPOT_MSG_BADUNKCOVERAGE, format, offset); break;
		}
	}

void ttoEnumerateCoverage(Offset offset, void *coverage,
						  ttoEnumRec *coverageenum, 
						  Card32 *numitems)
	{
	  IntX n;
	  IntX i, j;
	  Card16 format;

	  *numitems = 0;
	  if (((CoverageFormat1 *)coverage) == NULL)
		return;

	  coverageenum->mingid = 65535;
	  coverageenum->maxgid = 0;
	  da_INIT(coverageenum->glyphidlist, 10, 10);

	  format = ((CoverageFormat1 *)coverage)->CoverageFormat;
	  if (format == 1)
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
	  else if (format == 2)
		{
		  CoverageFormat2 *fmt2 = (CoverageFormat2 *)coverage;
		  RangeRecord *rec;
		  n = 0;
		  j = 0;
		  
		  for (i = 0; i < fmt2->RangeCount; i++) {
			rec = &(fmt2->RangeRecord[i]);
			if (rec->Start <= j)
				warning(SPOT_MSG_DUP_IN_COV, rec->Start);
			for (j = rec->Start; j <= rec->End; j++)
			  {
				*da_NEXT(coverageenum->glyphidlist) = j;
				n++;
			  }
			if (rec->Start < coverageenum->mingid)
			  coverageenum->mingid = rec->Start;
			if (rec->Start > coverageenum->maxgid)
			  coverageenum->maxgid = rec->Start;  
			if (rec->End < coverageenum->mingid)
			  coverageenum->mingid = rec->End;
			if (rec->End > coverageenum->maxgid)
			  coverageenum->maxgid = rec->End;
		  }
		  *numitems = n;
		}
	}

IntX ttoGlyphIsInCoverage(Offset offset, void *coverage, GlyphId gid, IntX *where)
	{
	  IntX i, n;
	  Card16 format;

	  if (((CoverageFormat1 *)coverage) == NULL)
		{
		  *where = (-1);
		  return 0;
		}

	  format = ((CoverageFormat1 *)coverage)->CoverageFormat;
	  if (format == 1)
		{
		  CoverageFormat1 *fmt1 = (CoverageFormat1 *)coverage;

		  for (i = 0; i < fmt1->GlyphCount; i++) 
			if (gid == fmt1->GlyphArray[i])
			  {
				*where = i;
				return 1;
			  }
		}
	  else if (format == 2)
		{
		  CoverageFormat2 *fmt2 = (CoverageFormat2 *)coverage;
		  RangeRecord *rec;

		  n = 0;
		  for (i = 0; i < fmt2->RangeCount; i++) {
			rec = &(fmt2->RangeRecord[i]);
			if ((rec->Start <= gid) && (gid <= rec->End))
			  {
				n += (gid - rec->Start);
				*where = n;
				return 1;
			  }
			else
			  n += (rec->End - rec->Start) + 1;
		  }
		}

	  *where = (-1);
	  return 0;
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
	  Card16 format;
	  if (((CoverageFormat1 *)coverage) == NULL) return;
	  format = ((CoverageFormat1 *)coverage)->CoverageFormat;
	  if (format == 1)
		{
		  freeCoverage1(coverage);
		  memFree(coverage);
		}
	  else if (format == 2)
		{
		  freeCoverage2(coverage);
		  memFree(coverage);
		}
	}
	 
/* --- Class Table --- */
static void *readClass1(void)
	{
	IntX i;
	ClassDefFormat1 *fmt = memNew(sizeof(ClassDefFormat1));
		
	fmt->ClassFormat = 1;	/* Already read */
	IN1(fmt->StartGlyph);
	IN1(fmt->GlyphCount);
	
	fmt->ClassValueArray =
		memNew(sizeof(fmt->ClassValueArray[0]) * fmt->GlyphCount);
	for (i = 0; i <fmt->GlyphCount; i++)
		IN1(fmt->ClassValueArray[i]);

	return fmt;
	}

static void *readClass2(void)
	{
	IntX i;
	ClassDefFormat2 *fmt = memNew(sizeof(ClassDefFormat2));
		
	fmt->ClassFormat = 2;	/* Already read */
	IN1(fmt->ClassRangeCount);
		
	fmt->ClassRangeRecord =
		memNew(sizeof(ClassRangeRecord) * fmt->ClassRangeCount);
	for (i = 0; i < fmt->ClassRangeCount; i++)
		{
		ClassRangeRecord *record = &fmt->ClassRangeRecord[i];
			
		IN1(record->Start);
		IN1(record->End);
		IN1(record->Class);
		}
	return fmt;
	}

void *ttoReadClass(Card32 offset)
	{
	void *result;
	Card16 format;
	Card32 save = TELL();

	SEEK_SURE(offset);
	IN1(format);
	switch (format)
	  {
	  case 1:
		result = readClass1(); break;
	  case 2:
		result = readClass2(); break;
	  default:
		warning(SPOT_MSG_BADUNKCLASS, format, offset);
		result = NULL;
		break;
	  }

	SEEK_SURE(save);
	
	return result;
	}

static void dumpClass1(ClassDefFormat1 *fmt, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "ClassFormat=1\n"));
	DLu(2, "StartGlyph =", fmt->StartGlyph);
	DLu(2, "GlyphCount =", fmt->GlyphCount);
	if (level >= 4)
	  {
		DL(4, (OUTPUTBUFF, "--- ClassValueArray[index]=glyphId glyphName/CID classValue\n"));
		for (i = 0; i <fmt->GlyphCount; i++)
		  DL(4, (OUTPUTBUFF, "[%d]=%d (%s) %hu  ", i, fmt->StartGlyph + i, getGlyphName(fmt->StartGlyph + i, 0), fmt->ClassValueArray[i]));
	  }
	else 
	  {
		DL(3, (OUTPUTBUFF, "--- ClassValueArray[index]=value\n"));
		for (i = 0; i <fmt->GlyphCount; i++)
		  DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, fmt->ClassValueArray[i]));
	  }
	DL(3, (OUTPUTBUFF, "\n"));
	}

static void dumpClass2(ClassDefFormat2 *fmt, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "ClassFormat    =2\n"));
	DLu(2, "ClassRangeCount=", fmt->ClassRangeCount);
	if (level >= 4) 
	  {
		DL(4, (OUTPUTBUFF, "--- ClassRangeRecord[index]={glyphId glyphName/CID=classValue, ...}\n"));
		for (i = 0; i <fmt->ClassRangeCount; i++)
		  {
			IntX a;
			ClassRangeRecord *record = &fmt->ClassRangeRecord[i];
			DL(4, (OUTPUTBUFF, "[%d]={  ", i));
			for (a = record->Start; a <= record->End; a++)
			  { 
				DL(4,(OUTPUTBUFF, "%d (%s)=%hu  ", a, getGlyphName(a, 0), record->Class));
			  }
			DL(4, (OUTPUTBUFF, "}\n"));
		  }
	  }
	else 
	  {
		DL(3, (OUTPUTBUFF, "--- ClassRangeRecord[index]={Start,End,Class}\n"));
		for (i = 0; i <fmt->ClassRangeCount; i++)
		  {
			ClassRangeRecord *record = &fmt->ClassRangeRecord[i];
			DL(3, (OUTPUTBUFF, "[%d]={%hu,%hu,%hu} ", i, 
			   record->Start, record->End, record->Class));
		  }
	  }
	DL(3, (OUTPUTBUFF, "\n"));
	}

void ttoDumpClass(Offset offset, void *class, IntX level)
	{
	  Card16 format;

	  if (((ClassDefFormat1 *)class) == NULL) return;
	  DL(2, (OUTPUTBUFF, "--- Class (%04hx)\n", offset));
	  format = ((ClassDefFormat1 *)class)->ClassFormat;
	  switch (format)
		{
		case 1:
		  dumpClass1(class, level); break;
		case 2:
		  dumpClass2(class, level); break;
		default:
		  warning(SPOT_MSG_BADUNKCLASS, format, offset);
		  break;
		}
	}


void ttoEnumerateClass(Offset offset, void *class,
					   IntX numclasses,
					   ttoEnumRec *classenumarray,
					   Card32 *numitems)
	{
	  IntX i, glyph, classnum;
	  Card16 format;

	  *numitems = 0;
	  if (((ClassDefFormat1 *)class) == NULL) return;
	  for (i = 0; i < numclasses; i++) 
		{
		  classenumarray[i].mingid = 65535;
		  classenumarray[i].maxgid = 0;
		  da_INIT(classenumarray[i].glyphidlist, 10, 10);
		}


	  format = ((ClassDefFormat1 *)class)->ClassFormat;
	  if (format == 1)
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
	  else if (format == 2)
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

IntX ttoGlyphIsInClass(Offset offset, void *class, GlyphId gid, IntX *whichclassnum)
	{	
	  IntX i;
	  Card16 format;

	  if (((ClassDefFormat1 *)class) == NULL)
		{
		  *whichclassnum = (-1);
		  return 0;
		}

	  format = ((ClassDefFormat1 *)class)->ClassFormat;
	  if (format == 1)
		{  
		  ClassDefFormat1 *fmt1 = (ClassDefFormat1 *)class;
		  GlyphId EndGlyph = fmt1->StartGlyph + (fmt1->GlyphCount - 1);
		  if ((fmt1->StartGlyph <= gid) && (gid <= EndGlyph))
			{
			  i =  gid - fmt1->StartGlyph;
			  *whichclassnum = fmt1->ClassValueArray[i];
			  return 1;
			}
		}
	  else if (format == 2)
		{
		  ClassDefFormat2 *fmt2 = (ClassDefFormat2 *)class;
		  ClassRangeRecord *rec;
		  for (i = 0; i < fmt2->ClassRangeCount; i++) {
			rec = &(fmt2->ClassRangeRecord[i]);
			if ((rec->Start <= gid) && (gid <= rec->End))
			  {
				*whichclassnum = rec->Class;
				return 1;
			  }
		  }
		}

	  *whichclassnum = (-1);
	  return 0;
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
	  Card16 format;
	  if (((ClassDefFormat1 *)class) == NULL) return;
	  format = ((ClassDefFormat1 *)class)->ClassFormat;
	  if (format == 1)
		{
		  freeClass1(class);
		  memFree(class);
		}
	  else if (format == 2)
		{
		  freeClass2(class);
		  memFree(class);
		}
	}

/* Used only with level 5 */
void ttoDumpFeaturesByScript(ScriptList *scriptlist, FeatureList *featurelist, LookupList *lookuplist,
                             ttoDumpCB dumpCB, IntX level)
{
	IntX  j,k,m,n, index, errorcount;
	Tag currentScript, currentLang;
	
	errorcount=0;
    
    
	for (j=0; j< scriptlist->ScriptCount; j++)
	{
  		ScriptRecord * srecord = &scriptlist->ScriptRecord[j];
  		Script *script = &srecord->_Script;
  		LangSys * defaultLangSys = &script->_DefaultLangSys;
        SeenFeatureParam lastFeatureParam;
        lastFeatureParam.param = NULL;
		
		currentScript=srecord->ScriptTag;
		
  		fprintf(OUTPUTBUFF, "Script '%c%c%c%c'\n",TAG_ARG(srecord->ScriptTag));
  		
  		/*First check the default lang sys*/
  		if (script->DefaultLangSys!=0)
		{
			fprintf(OUTPUTBUFF, "\tDefault language system\n");
			currentLang=TAG('d', 'f', 'l', 't');
			
			for (m=0; m<defaultLangSys->FeatureCount; m++)
			{
				FeatureRecord *record;
				Feature *feature;
				IntX featureNum = defaultLangSys->FeatureIndex[m];
				if (featureNum>=featurelist->FeatureCount){
					errorcount++;
					break;
				}
				record =  &featurelist->FeatureRecord[featureNum];
	  			feature=&record->_Feature;
				fprintf(OUTPUTBUFF, "\t\t[%2d]='%c%c%c%c' LookupListIndex: ", m,
                        TAG_ARG(record->FeatureTag));
				
				
				for (n = 0; n < feature->LookupCount; n++)
				{
                    index = feature->LookupListIndex[n];
                    
                    fprintf(OUTPUTBUFF,  "%hu ",  (Card16)index);
                    if (level==5)
					{
                        Tag tags[3];
                        tags[0]=record->FeatureTag;
                        tags[1]=currentScript;
                        tags[2]=currentLang;
                        dumpLookup(lookuplist->Lookup[index],
                                   &lookuplist->_Lookup[index], index,
                                   level, dumpCB, tags); /*(void *)record->FeatureTag);  */
					}
				}
				fprintf(OUTPUTBUFF,  "\n");
                
				/*ASP: Size specific code*/
                if(feature->FeatureParam!=0) {
                    dumpFeatureParams(record, feature, level, currentScript, currentLang, &lastFeatureParam);
                }
                
			}
		}
  		/*Then do the non-default languages*/
  		for (k=0; k< script->LangSysCount; k++)
  		{
  			LangSysRecord *lrecord = &script->LangSysRecord[k];
  			LangSys * langsys = &lrecord->_LangSys;
  			fprintf(OUTPUTBUFF, "\tLanguage '%c%c%c%c'\n", TAG_ARG(lrecord->LangSysTag));
            
			currentLang=lrecord->LangSysTag;
            
 			for (m=0; m<langsys->FeatureCount; m++)
  			{
  				FeatureRecord *record =  &featurelist->FeatureRecord[langsys->FeatureIndex[m]];
  				Feature *feature=&record->_Feature;
  				
  				fprintf(OUTPUTBUFF, "\t\t[%2d]='%c%c%c%c' LookupListIndex: ", m,
                        TAG_ARG(record->FeatureTag));
                
  				for (n = 0; n < feature->LookupCount; n++)
				{
                    index = feature->LookupListIndex[n];
                    
                    fprintf(OUTPUTBUFF,  "%hu ", (Card16)index);
                    if (level==5)
					{
                        Tag tags[3];
                        tags[0]=record->FeatureTag;
                        tags[1]=currentScript;
                        tags[2]=currentLang;
                        dumpLookup(lookuplist->Lookup[index],
                                   &lookuplist->_Lookup[index], index,
                                   level, dumpCB, tags); /*(void *)record->FeatureTag);  */
					}
                    
				}
				fprintf(OUTPUTBUFF,  "\n");
                /*ASP: Size specific code*/
				if(feature->FeatureParam!=0) {
                    dumpFeatureParams(record, feature, level, currentScript, currentLang, &lastFeatureParam);
				}
			}
		}
	}					
}

/* Used only with elvels 6 and 7 */
void ttoDecompileByScript(ScriptList *scriptlist, FeatureList *featurelist, LookupList *lookuplist, 
						  ttoDumpCB dumpCB, IntX level)
	{
	   IntX i,j,k,m,n, index;
	   SeenLookup *SeenLookups;
	   Tag currentScript, currentLang;
		IntX numReferencedLookups = 0; /* number of lookups referenced from chain context statements that must also be dumped */
        SeenFeatureParam lastFeatureParam;
        lastFeatureParam.param = NULL;

		SeenLookups = (SeenLookup *)memNew(lookuplist->LookupCount*sizeof(SeenLookup));
		for (i=0; i< lookuplist->LookupCount; i++)
			SeenLookups[i].seen=0;
	   
	   for (j=0; j< scriptlist->ScriptCount; j++)
		   {
		   ScriptRecord * srecord = &scriptlist->ScriptRecord[j];
		   Script *script = &srecord->_Script;
		   LangSys * defaultLangSys = &script->_DefaultLangSys;
		   
		   for (k=0; k< script->LangSysCount; k++)
			   {
			   LangSysRecord *lrecord = &script->LangSysRecord[k];
			   LangSys * langsys = &lrecord->_LangSys;
			   for (m=0; m<langsys->FeatureCount; m++)
				   {
				   FeatureRecord *record =  &featurelist->FeatureRecord[langsys->FeatureIndex[m]];
				   Feature *feature=&record->_Feature;
				   resetReferencedList();
				   
				   currentLang=lrecord->LangSysTag;
				   currentScript=srecord->ScriptTag;
				   
				   /* skip all features but kerning for AFM dump */
				   if ((level==6) && (! (record->FeatureTag == STR2TAG("kern") || (record->FeatureTag == STR2TAG("vkrn")) )))
					   continue;
				   
                       if (level==7)
					   {
                           if(feature->FeatureParam!=0) {
                                dumpFeatureParams(record, feature, level, currentScript, currentLang, &lastFeatureParam);
                           }
					   }
                       
                       for (n = 0; n < feature->LookupCount; n++)
					   {
					   index = feature->LookupListIndex[n];
					   if ((level!=6)  && SeenLookups[index].seen)
						   {
						   fprintf(OUTPUTBUFF, "\n# Skipping lookup %d in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c' \n# because already dumped in script '%c%c%c%c', language '%c%c%c%c'.\n", 
								   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(SeenLookups[index].ScriptTag), TAG_ARG(SeenLookups[index].LangSysTag));
						   }else{
							   int ret = sfntIsInFeatProofList(record->FeatureTag);
							   if ((ret < 0) /* empty list --> do all of them */|| (ret > 0))
								   {		
									   Tag tags[3];
									   tags[0]=record->FeatureTag;
									   tags[1]=currentScript;
									   tags[2]=currentLang;
									   if (level!=6)
										   fprintf(OUTPUTBUFF, "\n# Printing lookup %d in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
												   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));
									   
									   dumpLookup(lookuplist->Lookup[index],
												  &lookuplist->_Lookup[index], index,
												  level, dumpCB, tags); /*(void *)record->FeatureTag);  */
									   
									   if (level!=6)
										   {
										   SeenLookups[index].seen++;
										   SeenLookups[index].ScriptTag=currentScript;
										   SeenLookups[index].LangSysTag=currentLang;
										   }
									   
								   }
						   }
					   }
				   
				   numReferencedLookups = numReferencedList();
				   
				   if ((level!=6)  && (numReferencedLookups > 0) && sfntIsInFeatProofList(record->FeatureTag))
					   {
					   for (n = 0; n < numReferencedLookups; n++)
						   {
						   index = getReferencedListLookup(n);
						   if (index < 0) /* index not in referenced list - bad font construction! */
							   continue;
						   
						   if ((level!=6)  && SeenLookups[index].seen)
							   {
							   fprintf(OUTPUTBUFF, "\n# Skipping lookup %d referenced by chain context in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'\n# because already dumped in script '%c%c%c%c', language '%c%c%c%c'.\n", 
									   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(SeenLookups[index].ScriptTag), TAG_ARG(SeenLookups[index].LangSysTag));
							   }else{
								   int ret = sfntIsInFeatProofList(record->FeatureTag);
								   if ((ret < 0) /* empty list --> do all of them */|| (ret > 0))
									   {
									   Tag tags[3];
									   tags[0]=record->FeatureTag;
									   tags[1]=currentScript;
									   tags[2]=currentLang;
									   if (level!=6)
										   fprintf(OUTPUTBUFF, "\n# Printing lookup %d referenced by chain context in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
												   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));
									   dumpLookup(lookuplist->Lookup[index],
												  &lookuplist->_Lookup[index], index,
												  level, dumpCB, tags); /*(void *)record->FeatureTag, currentScript, currentLang);*/  
									   if (level!=6)
										   {
										   SeenLookups[index].seen++;
										   SeenLookups[index].ScriptTag=currentScript;
										   SeenLookups[index].LangSysTag=currentLang;
										   }
									   }
							   }
						   
						   } /* end doing referenced lookups */
					   } /* end if there are referenced lookups */
				   
				   }
			   }	
		   
		   /*Also check the default lang sys*/
		   if (script->DefaultLangSys != 0)
			   {
			   for (m=0; m<defaultLangSys->FeatureCount; m++)
				   {
				   FeatureRecord *record =  &featurelist->FeatureRecord[defaultLangSys->FeatureIndex[m]];
				   Feature *feature=&record->_Feature;
				   
				   currentLang=TAG('d', 'f', 'l', 't');
				   currentScript=srecord->ScriptTag;
				   resetReferencedList();
				   
				   /* skip all features but kerning for AFM dump */
				   if ((level==6) && (! (record->FeatureTag == STR2TAG("kern") || (record->FeatureTag == STR2TAG("vkrn")) )))
					   continue;
				   
				   if (level==7)
					   {
					   if(feature->FeatureParam!=0) {
                            dumpFeatureParams(record, feature, level, currentScript, currentLang, &lastFeatureParam);
					   }
                    }
				   
				   for (n = 0; n < feature->LookupCount; n++) 
					   {
					   index = feature->LookupListIndex[n];
					   if ((level!=6)  && SeenLookups[index].seen)
						   {
						   fprintf(OUTPUTBUFF, "\n# Skipping lookup %d in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'\n# because already dumped in script '%c%c%c%c', language '%c%c%c%c'.\n", 
								   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(SeenLookups[index].ScriptTag), TAG_ARG(SeenLookups[index].LangSysTag));
						   }else{
							   int ret = sfntIsInFeatProofList(record->FeatureTag);
							   if ((ret < 0) /* empty list --> do all of them */|| (ret > 0))
								   {
								   Tag tags[3];
								   tags[0]=record->FeatureTag;
								   tags[1]=currentScript;
								   tags[2]=currentLang;
								   if (level!=6)
									   fprintf(OUTPUTBUFF, "\n# Printing lookup %d in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
											   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));
								   dumpLookup(lookuplist->Lookup[index],
											  &lookuplist->_Lookup[index], index,
											  level, dumpCB, tags); /*(void *)record->FeatureTag, currentScript, currentLang);*/  
								   if (level!=6)
									   {
									   SeenLookups[index].seen++;
									   SeenLookups[index].ScriptTag=currentScript;
									   SeenLookups[index].LangSysTag=currentLang;
									   }
								   }
						   }
					   }
				   numReferencedLookups = numReferencedList();
				   
				   if ((level!=6)  && (numReferencedLookups > 0) && sfntIsInFeatProofList(record->FeatureTag))
					   {
					   for (n = 0; n < numReferencedLookups; n++)
						   {
						   index = getReferencedListLookup(n);
						   if (index < 0) /* index not in referenced list - bad font construction! */
							   continue;
						   
						   if ((level!=6)  && SeenLookups[index].seen)
							   {
							   fprintf(OUTPUTBUFF, "\n# Skipping lookup %d referenced by chain context in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'\n# because already dumped in script '%c%c%c%c', language '%c%c%c%c'.\n", 
									   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang), TAG_ARG(SeenLookups[index].ScriptTag), TAG_ARG(SeenLookups[index].LangSysTag));
							   }else{
								   int ret = sfntIsInFeatProofList(record->FeatureTag);
								   if ((ret < 0) /* empty list --> do all of them */|| (ret > 0))
									   {
									   Tag tags[3];
									   tags[0]=record->FeatureTag;
									   tags[1]=currentScript;
									   tags[2]=currentLang;
									   if (level!=6)
										   fprintf(OUTPUTBUFF, "\n# Printing lookup %d referenced by chain context in feature '%c%c%c%c' for script '%c%c%c%c' language '%c%c%c%c'.\n",
												   index, TAG_ARG(record->FeatureTag), TAG_ARG(currentScript), TAG_ARG(currentLang));
									   dumpLookup(lookuplist->Lookup[index],
												  &lookuplist->_Lookup[index], index,
												  level, dumpCB, tags); /*(void *)record->FeatureTag, currentScript, currentLang);*/  
									   if (level!=6)
										   {
										   SeenLookups[index].seen++;
										   SeenLookups[index].ScriptTag=currentScript;
										   SeenLookups[index].LangSysTag=currentLang;
										   }
									   }
							   }
						   
						   } /* end doing referenced lookups */
					   } /* end if there are referenced lookups */
				   }
			   }
		   }
	  memFree(SeenLookups);
	   /* for each script:
	   		for each feature of this script:
	   			for each lookup of this feature:
	   				if this lookup is not yet done (LookupFlag & DumpLookupSeen):
	   					for each subtable of this lookup:
	   						dumpCB(subtable)
	   				else
	   					output "same" marker
	   	*/
	}


/* --- Device Table --- */
void ttoReadDeviceTable(Card32 offset, DeviceTable *table) 
	{
	IntX i;
	IntX nWords;
	Card32 save = TELL();

	SEEK_ABS(offset);

	IN1(table->StartSize);
	IN1(table->EndSize);
	IN1(table->DeltaFormat);

	nWords = ((table->EndSize - table->StartSize + 1) * 
			  (1<<table->DeltaFormat) + 15) / 16;
	table->DeltaValue = memNew(sizeof(table->DeltaValue[0]) * nWords);	
	for (i = 0; i < nWords; i++)
		IN1(table->DeltaValue[i]);

	SEEK_ABS(save);
	}

void ttoDumpDeviceTable(Offset offset, DeviceTable *table, IntX level)
	{
	IntX i;
	IntX numSizes = table->EndSize - table->StartSize + 1;
	IntX nWords = (numSizes *  (1<<table->DeltaFormat) + 15) / 16;
	if (level == 7)
		{
		IntX size = table->StartSize;
		Card16 mask;
		CardX numBits;
		Card16 format = table->DeltaFormat;
		
		switch(format)
			{
			case 1:
 				mask = 0x3; /* mask for Type 1; top 2 bits are first delta value */
 				numBits = 2;
 				break;
			case 2:
 				mask = 0xF; /* mask for Type 2*/
 				numBits = 4;
 				break;
			case 3:
 				mask = 0xFF; /* mask for Type 3; top 8 bits are first delta value */
 				numBits = 8;
 				break;
 			default:
				warning(SPOT_MSG_GPOSUFMTDEV, format, offset);
				return;
			}
		fprintf(OUTPUTBUFF, "<device ");
		for (i = 0; i < numSizes; i++)
			{
			IntX deltaIndex = (i *  (1<<format)) / 16;
			IntX bitIndex =  (i *  (1<<format)) % 16;
			IntX deltaVal;
			Card16 deltaWord = table->DeltaValue[deltaIndex];
			deltaWord = deltaWord >> ((16 -numBits) - bitIndex);
			deltaWord = mask & deltaWord;
			switch(format)
				{
				case 1:
	 				if (deltaWord > 1)
	 					deltaVal = -1;
	 				else
	 					deltaVal = deltaWord;
	 				break;
				case 2:
	 				if (deltaWord > 7)
	 					deltaVal = deltaWord - 16;
	 				else
	 					deltaVal = deltaWord;
	 				break;
				case 3:
	 				if (deltaWord > 127)
	 					deltaVal = deltaWord - 256;
	 				else
	 					deltaVal = deltaWord;
	 				break;
				}
			
			if (i > 0)
				fprintf(OUTPUTBUFF, ", ");
			fprintf(OUTPUTBUFF, "%d %d", size + i, deltaVal);
			}
		fprintf(OUTPUTBUFF, ">");
		}
	else
		{
		DL(2, (OUTPUTBUFF, "--- DeviceTable (%04hx)\n", offset));
		
		DLu(2, "StartSize  =", table->StartSize);
		DLu(2, "EndSize    =", table->EndSize);
		DLu(2, "DeltaFormat=", table->DeltaFormat);

		DL(3, (OUTPUTBUFF, "--- DeltaValue[index]=value\n"));
		for (i = 0; i < nWords; i++)
			DL(3, (OUTPUTBUFF, "[%d]=%04hx ", i, table->DeltaValue[i]));
		DL(3, (OUTPUTBUFF, "\n"));
		}
	}

void ttoFreeDeviceTable(DeviceTable *table)
	{
	memFree(table->DeltaValue);
	}

