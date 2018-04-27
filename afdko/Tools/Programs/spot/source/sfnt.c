/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)sfnt.c	1.33
 * Changed:    7/14/99 10:02:04
 ***********************************************************************/

/*
 * sfnt reading/dumping.
 */

#include <ctype.h>
#include <string.h>

#include "sfnt.h"
#include "sfnt_sfnt.h"

#include "BASE.h"
#include "BBOX.h"
#include "BLND.h"
#include "CFF_.h"
#include "CID_.h"
#include "ENCO.h"
#include "FNAM.h"
#include "GLOB.h"
#include "GDEF.h"
#include "GSUB.h"
#include "HFMX.h"
#include "LTSH.h"
#include "MMFX.h"
#include "MMVR.h"
#include "OS_2.h"
#include "TYP1.h"
#include "WDTH.h"
#include "cmap.h"
#include "fdsc.h"
#include "feat.h"
#include "fvar.h"
#include "gasp.h"
#include "glyf.h"
#include "hdmx.h"
#include "head.h"
#include "hhea.h"
#include "hmtx.h"
#include "kern.h"
#include "loca.h"
#include "maxp.h"
#include "name.h"
#include "post.h"
#include "trak.h"
#include "vhea.h"
#include "vmtx.h"
#include "GPOS.h"
#include "EBLC.h"
#include "VORG.h"
#include "META.h"
#include "SING.h"

#include "TTdumpinstrs.h"


static void dirDump(IntX level, LongN start);
static void dirFree(void);
static void addDumpTable(Card32 tag);
static void sfntTTCList(void);

/* sfnt table functions */
typedef struct
	{
	Card32 tag;						/* Table tag */
	void (*read)(LongN, Card32);	/* Reads table data structures */
	void (*dump)(IntX, LongN);		/* Dumps table data structures */
	void (*free)(void);				/* Frees table data structures */
	void (*usage)(void);			/* Prints table usage */
	} Function;

/* Table MUST be in ASCII tag order */
static Function function[] =
	{
	{BASE_, BASERead, BASEDump, BASEFree, BASEUsage},
	{BBOX_, BBOXRead, BBOXDump, BBOXFree},
	{BLND_, BLNDRead, BLNDDump, BLNDFree},
	{CFF__, CFF_Read, CFF_Dump, CFF_Free, CFF_Usage},
	{CID__, CID_Read, CID_Dump, CID_Free},
	{DSIG_, NULL, NULL, NULL},
	{EBLC_, EBLCRead, EBLCDump, EBLCFree},
	{ENCO_, ENCORead, ENCODump, ENCOFree},
	{FNAM_, FNAMRead, FNAMDump, FNAMFree},
	{GDEF_, GDEFRead, GDEFDump, GDEFFree,GDEFUsage},
	{GLOB_, GLOBRead, GLOBDump, GLOBFree},
	{GPOS_, GPOSRead, GPOSDump, GPOSFree, GPOSUsage},
	{GSUB_, GSUBRead, GSUBDump, GSUBFree, GSUBUsage},
	{HFMX_, HFMXRead, HFMXDump, HFMXFree},
	{KERN_, kernRead, kernDump, kernFree},
	{LTSH_, LTSHRead, LTSHDump, LTSHFree},
	{META_, METARead, METADump, METAFree},
	{MMFX_, MMFXRead, MMFXDump, MMFXFree},
	{MMVR_, MMVRRead, MMVRDump, MMVRFree},
	{OS_2_, OS_2Read, OS_2Dump, OS_2Free},
	{SING_, SINGRead, SINGDump, SINGFree},
	{TYP1_, TYP1Read, TYP1Dump, TYP1Free},
	{VORG_, VORGRead, VORGDump, VORGFree, VORGUsage},
	{WDTH_, WDTHRead, WDTHDump, WDTHFree},
	{bloc_, EBLCRead, EBLCDump, EBLCFree},
	{cmap_, cmapRead, cmapDump, cmapFree, cmapUsage},
	{fdsc_, fdscRead, fdscDump, fdscFree},
	{feat_, featRead, featDump, featFree},
	{fvar_, fvarRead, fvarDump, fvarFree},
	{gasp_, gaspRead, gaspDump, gaspFree},
	{glyf_, glyfRead, glyfDump, glyfFree, glyfUsage},
	{hdmx_, hdmxRead, hdmxDump, hdmxFree},
	{head_, headRead, headDump, headFree},
	{hhea_, hheaRead, hheaDump, hheaFree},
	{hmtx_, hmtxRead, hmtxDump, hmtxFree, hmtxUsage},
	{kern_, kernRead, kernDump, kernFree, kernUsage},
	{loca_, locaRead, locaDump, locaFree},
	{maxp_, maxpRead, maxpDump, maxpFree},
	{name_, nameRead, nameDump, nameFree, nameUsage},
	{post_, postRead, postDump, postFree},
	{sfnt_,     NULL,  dirDump,  dirFree},
	{trak_, trakRead, trakDump, trakFree},
	{vhea_, vheaRead, vheaDump, vheaFree},
	{vmtx_, vmtxRead, vmtxDump, vmtxFree},	
#if 0 /* Still to do */
	{lcar_, lcarRead, lcarDump, lcarFree},
	{CNAM_, CNAMRead, CNAMDump,     NULL},
	{CNPT_, CNPTRead, CNPTDump,     NULL},
	{CSNP_, CSNPRead, CSNPDump,     NULL},
	{KERN_, KERNRead, KERNDump,     NULL},
	{METR_, METRRead, METRDump,     NULL},
	{bsln_, bslnRead, bslnDump,     NULL},
	{just_, justRead, justDump,     NULL},
	{mort_, mortRead, mortDump,     NULL},
	{opbd_, opbdRead, opbdDump,     NULL},
	{prop_, propRead, propDump,     NULL},
#endif
	};

/* TT collection */
static ttcfTbl ttcf;
static struct
	{
	Card32 offset;
	Card16 loaded;
	Card16 dumped;
	Card32 Version;
	Card32 headrSize;
	Card16 DSIGdumped;
	da_DCL(Card32, sel);	/* Table directory offset selectors */
	} ttc= {0,0,0,0,0,0};

/* sfnt */
static sfntTbl sfnt;
static struct
	{
	Card32 offset;	/* Directory header offset */
	Int16 id;		/* Resource id (Mac) */
	CardX reportedMissing;		/* Resource id (Mac) */
	} dir;

/* Table dump list */
typedef struct
	{
	Card32 tag;
	Int16 level;
	} Dump;

static struct
	{
	Byte8 *arg;
	da_DCL(Dump, list);
	} dump;

/* Feature proof list */
typedef struct _Proof
	{
	Card32 tag;
	Int16 level;
	Int16 seen;
	} Proof;

static struct _proof
	{
	Byte8 *arg;
	da_DCL(Proof, list);
	} proof;

da_DCL(Card32, referencedLookups);

/* Compare Card32s */
static IntN cmpCard32s(const void *first, const void *second)
	{
	Card32 a = *(Card32 *)first;
	Card32 b = *(Card32 *)second;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Read TT Collection header */
void sfntTTCRead(LongN start)
	{
	IntX i;
	SEEK_ABS(start);

	ttc.Version =  0x00020000; /* A number of MS TTC fonts for Vista are v2 structure, but declare themselves to be v1.  Assume v2 until proven otherwise. */

	IN1(ttcf.TTCTag);
	IN1(ttcf.Version);
	IN1(ttcf.DirectoryCount);
	ttc.headrSize =  TTC_HDR_SIZEV1 + 
						sizeof(ttcf.TableDirectory[0]) * ttcf.DirectoryCount;
						
	ttcf.TableDirectory =
		memNew(sizeof(ttcf.TableDirectory[0]) * ttcf.DirectoryCount);
	for (i = 0; i < (IntX)ttcf.DirectoryCount; i++)
		{
		IN1(ttcf.TableDirectory[i]);
		if (ttcf.TableDirectory[i] == ttc.headrSize)
			ttc.Version =  0x00010000;
		}

	ttc.offset = start;
	ttc.loaded = 1;
	ttc.dumped = 0;
	
	if (ttc.Version ==  0x00020000)
		{
		ttc.headrSize =  TTC_HDR_SIZEV2 + 
						sizeof(ttcf.TableDirectory[0]) * ttcf.DirectoryCount;
		IN1(ttcf.DSIGTag);
		IN1(ttcf.DSIGLength);
		IN1(ttcf.DSIGOffset);
		}

	for (i = 0; i < (IntX)ttcf.DirectoryCount; i++)
		{
		Card32 offset = ttcf.TableDirectory[i];
		
		if (ttc.sel.cnt == 0 || 
			bsearch(&offset, ttc.sel.array, ttc.sel.cnt, 
					sizeof(Card32), cmpCard32s) != NULL)
		  {
			sfntRead(ttc.offset + offset, -1);
			sfntDump();
			sfntFree(0);
		  }
		}

	/* now free only the ttc tables */
	sfntFree(2);
/* Premature!
*-	memFree(ttcf.TableDirectory); 
*- 	ttc.loaded = 0;   
*/
	}

/* Dump TT Collection header */
static void ttcfDump(IntX level, LongN start)
	{
	IntX i;

	if (!ttc.loaded)
		return;	/* Not a TT Collection */

	DL(1, (OUTPUTBUFF, "### [ttcf] (%08lx)\n", start));

	DL(2, (OUTPUTBUFF, "TTCTag        =%c%c%c%c (%08x)\n",
		   TAG_ARG(ttcf.TTCTag), ttcf.TTCTag));
	DLV(2, "Version       =", ttcf.Version);
	if (ttc.Version ==  0x00020000)
		{
		DL(2, (OUTPUTBUFF, "TTC DSIG Tag  =%c%c%c%c (%08x)\n",
			   TAG_ARG(ttcf.DSIGTag), ttcf.DSIGTag));
		DLU(2, "DSIG Length   =", ttcf.DSIGLength);
		DLU(2, "DSIG Offset   =", ttcf.DSIGOffset);
		}

	DLU(2, "DirectoryCount=", ttcf.DirectoryCount);
	DL(2, (OUTPUTBUFF, "--- TableDirectory[index]=offset\n"));
	for (i = 0; i < (IntX)ttcf.DirectoryCount; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%08x ", i, ttcf.TableDirectory[i]));
	DL(2, (OUTPUTBUFF, "\n"));
	
	}

/* Read sfnt directory */
static void dirRead(LongN start)
	{
	IntX i;

	SEEK_ABS(start);
	dir.offset = start;

	IN1(sfnt.version);
	IN1(sfnt.numTables);
	IN1(sfnt.searchRange);
	IN1(sfnt.entrySelector);
	IN1(sfnt.rangeShift);

	sfnt.directory = memNew(sizeof(Entry) * sfnt.numTables);
	for (i = 0; i < sfnt.numTables; i++)
		{
		Entry *entry = &sfnt.directory[i];

		IN1(entry->tag);
		IN1(entry->checksum);
		IN1(entry->offset);
		IN1(entry->length);
		/*if (((entry->tag == GSUB_) || (entry->tag == GPOS_))
			&& (entry->length > MAX_CARD16))
		  {
			warning(SPOT_MSG_TABLETOOBIG, TAG_ARG(entry->tag));
		  }*/
		}
	}

/* Compare Dumps */
static IntN cmpDumps(const void *first, const void *second)
	{
	Card32 a = ((Dump *)first)->tag;
	Card32 b = ((Dump *)second)->tag;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Compare with dump table tag */
static IntN cmpDumpTags(const void *key, const void *value)
	{
	Card32 a = *(Card32 *)key;
	Card32 b = ((Dump *)value)->tag;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}
	
/* Set level for table in dump list */
static CardX setLevel(Card32 tag, IntX level)
	{
	Dump *dmp = NULL;
	CardX foundTag = 1;
	if (dump.list.cnt > 0)
	  dmp = (Dump *)bsearch(&tag, dump.list.array, dump.list.cnt,
								sizeof(Dump), cmpDumpTags);
	if (dmp != NULL)
		dmp->level = level;
	else
		foundTag = 0;
		
	return foundTag;
	}

/* Scan tag argument list */
static void scanTags(Byte8 *actualarg)
	{
	Byte8 *p;
	IntX level = 4;	/* Default dump level */
	IntN i;
	Byte8 arg[100];

	strcpy(arg, actualarg); /* otherwise "actualarg" (argv[*]) gets trounced */

	for (p = strtok(arg, ","); p != NULL; p = strtok(NULL, ","))
		{
		Byte8 buf[2];
		Byte8 tag[5];

		if (sscanf(p, "%4[^\n]=%d", tag, &level) == 2)
			;
		else if (sscanf(p, "%4[^\n]=%1[x]", tag, buf) == 2)
			level = -1;
		else if (sscanf(p, "%4[^\n]", tag) != 1 || strlen(tag) == 0)
			{
			warning(SPOT_MSG_BADTAGSTR, p);
			continue;
			}


		if (strcmp(tag, "uset") == 0)
			{
			/* Set level for all unset tables */
			IntX i;
			for (i = 0; i < dump.list.cnt; i++)
				{
				Dump *dmp = &dump.list.array[i];

				if (dmp->level == 0)
					dmp->level = level;
				}
			}
		else /* tag is valid */
		  {
			IntN taglen = strlen(tag);
			CardX foundTag;

			for (i = taglen; i < 4; i++)
			  tag[i] = ' ';

			foundTag = setLevel(STR2TAG(tag), level);
			if (strchr(tag, '_'))
			  {
				for (i = 0; i < 4; i++)
				  {
					if (tag[i] == '_') tag[i] = ' ';
				  }
				foundTag = setLevel(STR2TAG(tag), level);
			  }
			  
		 if (!foundTag)
		 	{
	  		addDumpTable(STR2TAG(tag));
			qsort(dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumps);
			foundTag = setLevel(STR2TAG(tag), level);
	  		}
		  }
		level = 4; /* re-init */
		}
	}

/* Add table to dump list */
static void addDumpTable(Card32 tag)
	{
	Dump *dmp = da_NEXT(dump.list);
	dmp->tag = tag;
	dmp->level = 0;
	}

/* Make table dump list */
static void preMakeDump(void)
	{
	  IntX i;
	  
	  /* Add all tables in font */
	  da_INIT(dump.list, 40, 10);
	  for (i = 0; i < sfnt.numTables; i++)
		addDumpTable(sfnt.directory[i].tag);
	  
	  /* Add ttcf header and sfnt directory and sort into place */
	  addDumpTable(ttcf_);
	  addDumpTable(sfnt_);
	  if (dump.list.cnt > 1)
		qsort(dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumps);
	}
 
 
static void initDump(void)
	{
	  IntX i;
	  for (i = 0; i < dump.list.cnt; i++)
		{
		  Dump *dmp = da_INDEX(dump.list, i);
		  dmp->level = 0;
		  dmp->level = 0;
		}
	}
  
static void makeDump(void)
	{
	if (opt_Present("-t"))
	  scanTags(dump.arg);
	else 
	  {
		IntX tags=0;
		if (opt_Present("-T")) /* list sfnt tables */
		  setLevel(sfnt_,4);
		if (opt_Present("-F")) /* list GPOS,GSUB features */
		  {
			setLevel(GPOS_, 5);
			setLevel(GSUB_, 5);
			tags += 2;
		  }
		if (opt_Present("-G") || opt_Present("-br")) /* print glyph-shape synopsis */
		  {
			setLevel(CFF__, 7);
			setLevel(glyf_, 7);
			tags += 2;
		  }
		if (opt_Present("-P")) /* proof GPOS,GSUB feature(s) */
		  {
			setLevel(GPOS_, 8);
			setLevel(GSUB_, 8);
			tags += 2;
		  }
		if (opt_Present("-A")) /* decompile GPOS,GSUB feature(s) */
		  {
			setLevel(GPOS_, 7);
			setLevel(GSUB_, 7);
			tags += 2;
		  }
		
		if ((tags == 0) && (!opt_Present("-n") && !opt_Present("-nc")) )
		  setLevel(sfnt_,4); /* no tables selected, so dump sfnt directory */
	  }
  }

/* Dump sfnt directory */
static void dirDump(IntX level, LongN start)
	{
	IntX i;

	if (dir.id < 0)
		DL(1, (OUTPUTBUFF, "### [sfnt] (%08lx)\n", start));
	else
		DL(1, (OUTPUTBUFF, "### [sfnt] (%08lx) id=%d\n", start, dir.id));

	DL(2, (OUTPUTBUFF, "--- offset subtable\n"));
	if (sfnt.version == OTTO_)
		DL(2, (OUTPUTBUFF, "version      =OTTO  [OpenType]\n"));
	else if (sfnt.version == typ1_)
		DL(2, (OUTPUTBUFF, "version      =typ1  [Type 1]\n"));
	else if (sfnt.version == true_)
		DL(2, (OUTPUTBUFF, "version      =true  [TrueType]\n"));
	else if (sfnt.version == mor0_)
		DL(2, (OUTPUTBUFF, "version      =mor0  [Morisawa encrypted TrueType]\n"));
	else if (sfnt.version == VERSION(1,0))
		DL(2, (OUTPUTBUFF, "version      =1.0  [TrueType]\n"));
	else
		DL(2, (OUTPUTBUFF, "version      =%c%c%c%c (%08x) [unknown]\n",
			   TAG_ARG(sfnt.version), sfnt.version));
	DLu(2, "numTables    =", sfnt.numTables);
	DLu(2, "searchRange  =", sfnt.searchRange);
	DLu(2, "entrySelector=", sfnt.entrySelector);
	DLu(2, "rangeShift   =", sfnt.rangeShift);

	DL(2, (OUTPUTBUFF, "--- table directory[index]={tag,checksum,offset,length}\n"));
	for (i = 0; i < sfnt.numTables; i++)
		{
		Entry *entry = &sfnt.directory[i];

		DL(2, (OUTPUTBUFF, "[%2d]={'%c%c%c%c',%08x,%08x,%08x}\n", i, TAG_ARG(entry->tag),
			   entry->checksum, entry->offset, entry->length));
		}
	}


/* Add Feature to proof list */
static void addProofTable(Card32 tag, IntX level)
	{
	Proof *prf = da_NEXT(proof.list);
	prf->tag = tag;
	prf->level = level;
	prf->seen = 0;
	}

/* Scan Feature argument list */
static void scanFeats(Byte8 *actualarg)
	{
	Byte8 *p;
	IntX level;	/* Default proof level */
	Byte8 arg[100];

	if (opt_Present("-P"))
	  level = 8; /* re-init default */
	else if (opt_Present("-A"))
	  level = 7; /* re-init default */

	strcpy(arg, actualarg); /* otherwise "actualarg" (argv[*]) gets trounced */
	for (p = strtok(arg, ","); p != NULL; p = strtok(NULL, ","))
		{
		Byte8 buf[2];
		Byte8 feat[5];

		if (sscanf(p, "%4[^\n]=%d", feat, &level) == 2)
			;
		else if (sscanf(p, "%4[^\n]=%1[x]", feat, buf) == 2)
			level = -1;
		else if (sscanf(p, "%4[^\n]", feat) != 1 || strlen(feat) != 4)
			{
			warning(SPOT_MSG_BADFEATSTR, p);
			continue;
			}
		if (opt_Present("-P"))
			{
			/* for -Proof or -PROOF 
			   to indicate proofing all features */
			if ((strcmp(feat, "roof") != 0) &&
				(strcmp(feat, "ROOF") != 0))
			  {
				addProofTable(STR2TAG(feat), level);
				if (strchr(feat, '_'))
				  {
					int i;
					for (i = 0; i < 4; i++)
					  {
						if (feat[i] == '_') feat[i] = ' ';
					  }
					addProofTable(STR2TAG(feat), level);
				  }
			  }
			}
		if (opt_Present("-A"))
			{
			/* for -All or -ALL
			   to indicate making feature-file dumps of all features */
			if ((strcmp(feat, "ll") != 0) &&
				(strcmp(feat, "LL") != 0))
			  {
				addProofTable(STR2TAG(feat), level);
				if (strchr(feat, '_'))
				  {
					int i;
					for (i = 0; i < 4; i++)
					  {
						if (feat[i] == '_') feat[i] = ' ';
					  }
					addProofTable(STR2TAG(feat), level);
				  }
			  }
			}
			
		if (opt_Present("-P"))
		  level = 8; /* re-init default */
		else if (opt_Present("-A"))
		  level = 7; /* re-init default */
		}
	}


/* Make Feature proof list */
static void initProof(void)
	{
	  da_INIT(proof.list, 10, 10);
	  proof.list.cnt = 0;
	}


static void makeProof(void)
	{
	if (opt_Present("-P") ||
		opt_Present("-A"))
		scanFeats(proof.arg);
	}

IntX sfntIsInFeatProofList(Card32 feat_tag)
	{
	  IntX i;
	  if (proof.list.cnt == 0) return (-1); /* let caller decide how to handle this */
	  for (i = 0; i < proof.list.cnt; i++)
		{
		  Proof *prf = da_INDEX(proof.list, i);
		  if (prf->tag == feat_tag){
		  	prf->seen = 1;  /*To detect invalid entries*/
			return prf->level;
		  }
		}
	  return 0; /* not in list */
	}
	
void sfntAllProcessedProofList()
{
	IntX i;
	  if (proof.list.cnt == 0) return ;
	  for (i = 0; i < proof.list.cnt; i++)
		{
		  Proof *prf = da_INDEX(proof.list, i);
		  if (prf->seen ==0) {
		  		char str[5];
		  		
		  		sprintf(str, "%c%c%c%c", TAG_ARG(prf->tag));
		  	 	warning(SPOT_MSG_BADFEATSTR, str);
			}
		}
}


/* Support referenced lookup list */
static void initReferencedList(void)
	{
	  da_INIT(referencedLookups, 10, 10);
	}

void resetReferencedList(void)
	{
	referencedLookups.cnt = 0;
	}
/* Support referenced lookup list */
void addToReferencedList(Card32 n)
	{
	Card32 *nextIndex;
	
	if (bsearch(&n, referencedLookups.array,referencedLookups.cnt, 
			sizeof(Card32), cmpCard32s) != NULL)
		return; /* it is already in the list */
		
	nextIndex = da_NEXT(referencedLookups);
	*nextIndex = n;
	qsort(referencedLookups.array,referencedLookups.cnt, sizeof(Card32), cmpCard32s);

	}

Card32 numReferencedList(void)
	{
	return referencedLookups.cnt;
	}

Int32 getReferencedListLookup( Card32 n)
	{
	return (Int32) referencedLookups.array[n];
	}

/* Free sfnt directory */
static void dirFree(void)
	{
	if (sfnt.directory != NULL)
		{
		memFree(sfnt.directory);
		sfnt.directory = NULL;
		}
	}

/* Compare with function table tag */
static IntN cmpFuncTags(const void *key, const void *value)
	{
	Card32 a = *(Card32 *)key;
	Card32 b = ((Function *)value)->tag;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}
	
/* Find tag in function table */
static Function *findFunc(Card32 tag)
	{
	return (Function *)bsearch(&tag, function, TABLE_LEN(function),
							   sizeof(Function), cmpFuncTags);
	}

/* Compare with directory entry tag */
static IntN cmpEntryTags(const void *key, const void *value)
	{
	Card32 a = *(Card32 *)key;
	Card32 b = ((Entry *)value)->tag;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}
	
/* Find entry in the sfnt directory */
static Entry *findEntry(Card32 tag)
	{
	int i;
	Card32 prevTag;
	Entry *currEntry;
	prevTag=0;
	currEntry=sfnt.directory;
	
	for (i=0; i<sfnt.numTables; i++)
	{
		if(currEntry->tag<prevTag)
			fprintf(stderr, "OTFproof [WARNING]: sfnt table is out of order.\n");
		if (tag==currEntry->tag)
			return currEntry;
		prevTag=currEntry->tag;
		currEntry++;
	}
	return NULL;
	
	/*return (Entry *)bsearch(&tag, sfnt.directory, sfnt.numTables,
							sizeof(Entry), cmpEntryTags);
	*/
	}

/* Hexadecimal table dump */
static void hexDump(Card32 tag, LongN start, Card32 length)
	{
	Card32 addr = 0;
	Int32 left = length;

	SEEK_ABS(start);

	fprintf(OUTPUTBUFF,  "### [%c%c%c%c] (%08lx)\n", TAG_ARG(tag), start);
	while (left > 0)
		{
		/* Dump one line */
		IntX i;
		Card8 data[16];

		IN_BYTES(left < 16 ? left : 16, data);

		/* Dump 8 hexadecimal words of data */
		fprintf(OUTPUTBUFF,  "%08x  ", addr);
		for (i = 0; i < 16; i++)
			{
			if (i < left)
				fprintf(OUTPUTBUFF,  "%02x", data[i]);
			else
				fprintf(OUTPUTBUFF,  "  ");
			if ((i + 1) % 2 == 0)
				fprintf(OUTPUTBUFF,  " ");
			}

		/* Dump ascii interpretation of data */
		fprintf(OUTPUTBUFF,  " |");
		for (i = 0; i < 16; i++)
			if (i < left)
				fprintf(OUTPUTBUFF,  "%c", isprint(data[i]) ? data[i] : data[i] ? '?' : '.');
			else
				fprintf(OUTPUTBUFF,  " ");
		fprintf(OUTPUTBUFF,  "|\n");

		addr += 16;
		left -= 16;
		}
	}

/* Dump as an array of FWORD*/
static void arrayDump(Card32 tag, LongN start, Card32 length)
{
	FWord data;
	Card32 i;
	
	SEEK_ABS(start);

	fprintf(OUTPUTBUFF,  "### [%c%c%c%c] (%08lx)\n", TAG_ARG(tag), start);
	fprintf(OUTPUTBUFF,  "--- [index] = value\n");
	
	for (i=0; i<length/2; i++)
	{
		IN_BYTES(2, (unsigned char *)(&data));
		fprintf(OUTPUTBUFF, "[%u] = %d\n", i, data);
	}
}

/* Dump as a set of TT instructions*/

static void TTDump(Card32 tag, LongN start, Card32 length)
{
	Card8 *data;
	SEEK_ABS(start);

	fprintf(OUTPUTBUFF,  "### [%c%c%c%c] (%08lx)\n", TAG_ARG(tag), start);
	data=(Card8 *)memNew(length+1);
	
	IN_BYTES(length, data);
	dumpInstrs(length, (char *)data);
	
	memFree(data);
		
}

/* Free tables. Table free functions are called regardless of whether the table
   was selected for dumping because some tables get read in order to supply
   values used for reading another table */
static void freeTables(void)
	{
	CardX i;
	if (sfnt.directory == NULL) 
	  return; /* nothing loaded, so nothing to free */

	for (i = 0; i < TABLE_LEN(function); i++)
		if (function[i].free != NULL)
			function[i].free();
			
	}

/* Process tables */
static void doTables(IntX read)
	{
	IntX i, error;
	CardX reportedMissing = 0;
	
	for (i = 0; i < dump.list.cnt; i++)
		{
		Card32 tag = dump.list.array[i].tag;
		IntX level = dump.list.array[i].level;
		error=0;
		if (level != 0)
			{
			Card32 start;
			Card32 length;
			Function *func = findFunc(tag);
			Entry *entry = findEntry(tag);

			switch (tag)
				{
			case ttcf_:
				/* Handle ttcf header specially */
				if (!ttc.dumped)
					{
					start = ttc.offset;
					length = ttc.headrSize;

					if (level < 0)
						hexDump(tag, start, length);
					else 
						ttcfDump(level, start);

					ttc.dumped = 1;	/* Ensure we only dump this table once */
					}
				continue;
			case sfnt_:
				/* Handle ttcf header specially */
				if (!ttc.dumped)
					{
					start = ttc.offset;
					length = ttc.headrSize;

					if (level < 0)
						hexDump(tag, start, length);
					else 
						ttcfDump(level, start);

					ttc.dumped = 1;	/* Ensure we only dump this table once */
					}
				/* Fake sfnt directory values */
				start = dir.offset;
				length = DIR_HDR_SIZE + (ENTRY_SIZE * sfnt.numTables);
				break;
			case DSIG_:
				if (ttc.Version == 0x00020000)/* Note: this is the ttc Version, which is correct, not ttcf.version, which often isn't */
					{
					if  (!ttc.DSIGdumped)
						{
						start = ttcf.DSIGOffset;
						length = ttcf.DSIGLength;
						hexDump(tag, start, length);
						ttc.DSIGdumped = 1;
						}
					break;
					}
				/* else fall through to the default handling of unsupported tables */
			default:
				if (entry==NULL)
				{
					error=1;
				   if (!dir.reportedMissing)
				      {
					   tableMissing(tag, sfnt_);
					   reportedMissing = 1;
					   }
				}else{
					start = (ttc.loaded ? ttc.offset : dir.offset) + entry->offset;
					length = entry->length;
				}
				break;
			}

			if (read)
				{
				if (level > 0 && func != NULL && func->read != NULL && !error)
					func->read(start, length);
				}
			else
				{
				if(!error)
				{
					if (level < 0 || func == NULL)
						{
						if (tag==prep_ || tag==fpgm_)
							TTDump(tag, start, length);
						else if (tag==cvt__)
							arrayDump(tag, start, length);
						else
							hexDump(tag, start, length);
						}
					else if (func->dump != NULL)
						func->dump(level, start);
					}
				}
			}
		}
		if(!read)
			sfntAllProcessedProofList();
	if (reportedMissing)
		dir.reportedMissing = 1;
	}

/* Read sfnt */
void sfntRead(LongN start, IntX id)
	{
	  dir.id = id;
	  dir.reportedMissing = 0;
	  dirRead(start);	/* Read sfnt directory */
	  preMakeDump();    /* build dump list */
	  makeDump();		/* Make dump list */
	  doTables(1);	/* Read */
	}

void sfntDump(void)
	{
	if (opt_Present("-n"))
		dumpAllGlyphNames(0);
	else if (opt_Present("-nc"))
		dumpAllGlyphNames(1);
	initReferencedList();
	initDump();
	makeDump();
	doTables(1);
	initProof();
	makeProof();  
	doTables(0);	/* Dump */
   }

void sfntFree(IntX freeTTC)
	{
	  if (ttc.loaded && freeTTC)
		{
		  ttc.offset= ttc.loaded = ttc.dumped = 0;
		  ttc.sel.cnt = 0;
		  memFree(ttcf.TableDirectory);
		  ttcf.DirectoryCount = 0;
		}
	if (freeTTC != 2)
		{
		freeTables();	/* Free */
		if (referencedLookups.size > 0)
			da_FREE(referencedLookups);
		if (proof.list.size > 0)
			da_FREE(proof.list);
		if (dump.list.size > 0)
			da_FREE(dump.list);
		if (ttc.sel.size > 0)
			da_FREE(ttc.sel);
		}
	}

/* Read requested table */
IntX sfntReadTable(Card32 tag)
	{
	Entry *entry = findEntry(tag);
	
	if (entry == NULL)
		return 1;
	
	findFunc(tag)->read((ttc.loaded ? ttc.offset : dir.offset) + entry->offset,
						entry->length); 
	return 0;
	}

/* Handle tag list argument */
int sfntTagScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argi == 0)
		return 0;	/* Initialization not required */

	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		dump.arg = argv[argi++];
	return argi;
	}

/* Handle GPOS,GSUB Feature Proof list argument */
int sfntFeatScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argi == 0)
		return 0;	/* Initialization not required */

	if (argi == argc)
	  {
		return 0;
	  }
	else
	  {
		proof.arg = argv[argi++];
	  }
	return argi;
	}

/* TTC directory offset list argument scanner */
int sfntTTCScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argi == 0)
		return 0; /* No initialization required */

	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		{
		Byte8 *p;
		Byte8 *arg = argv[argi++];

		da_INIT(ttc.sel, 5, 2);
		ttc.sel.cnt = 0;
		for (p = strtok(arg, ","); p != NULL; p = strtok(NULL, ","))
			{
			Card32 offset;

			if (sscanf(p, "%i", &offset) == 1)
				*da_NEXT(ttc.sel) = offset;
			else
				opt_Error(opt_Format, opt, arg);
			}
		qsort(ttc.sel.array, ttc.sel.cnt, sizeof(Card32), cmpCard32s);
		}
	return argi;
	}
	
static void sfntTTCList(void)
{
	int i;
	fprintf(OUTPUTBUFF,"(offsets: ");
	for(i=0; i<ttc.sel.cnt; i++)
		if (i==0)
			fprintf(OUTPUTBUFF, "%d", (Card32)da_INDEX(ttc.sel, i));
		else
			fprintf(OUTPUTBUFF, ", %d", (Card32)da_INDEX(ttc.sel, i));
	fprintf(OUTPUTBUFF,")");
}	

/* Print table usage infomation */
void sfntUsage(void)
	{
	CardX i;

	fprintf(OUTPUTBUFF,  "Supported tables:");
	for (i = 0; i < TABLE_LEN(function); i++)
		{
		if (i % 10 == 0)
			fprintf(OUTPUTBUFF,  "\n    ");
		fprintf(OUTPUTBUFF,  "%c%c%c%c%s", TAG_ARG(function[i].tag),
			   (i + 1 == TABLE_LEN(function)) ? "\n" : ", ");
		}
	}

/* Print table-specific usage */
void sfntTableSpecificUsage(void)
	{
	CardX i;

	fprintf(OUTPUTBUFF,  "Table-specific usage:\n");
	for (i = 0; i < TABLE_LEN(function); i++)
		if (function[i].usage != NULL)
			function[i].usage();

	quit(0);
	}

