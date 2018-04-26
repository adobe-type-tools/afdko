/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)sfnt.c	1.19
 * Changed:    3/23/98 16:04:22
 ***********************************************************************/

/*
 * sfnt reading/comparing
 */

#include <ctype.h>
#include <string.h>

#include "Dsfnt.h"
#include "sfnt_sfnt.h"

#if 1
#include "Dhead.h"
#include "Dname.h"
#else
#include "DBASE.h"
#include "DBBOX.h"
#include "DBLND.h"
#include "DCFF_.h"
#include "DCID_.h"
#include "DENCO.h"
#include "DFNAM.h"
#include "DGLOB.h"
#include "DGSUB.h"
#include "DHFMX.h"
#include "DLTSH.h"
#include "DMMVR.h"
#include "DOS_2.h"
#include "DTYP1.h"
#include "DWDTH.h"
#include "Dcmap.h"
#include "Dfdsc.h"
#include "Dfeat.h"
#include "Dfvar.h"
#include "Dgasp.h"
#include "Dglyf.h"
#include "Dhdmx.h"
#include "Dhhea.h"
#include "Dhmtx.h"
#include "Dkern.h"
#include "Dloca.h"
#include "Dmaxp.h"
#include "Dpost.h"
#include "Dtrak.h"
#include "Dvhea.h"
#include "Dvmtx.h"
#include "DGPOS.h"
#include "DEBLC.h"
#endif

static void dirDiff(Card32 start1, Card32 start2);
static void dirFree(Card8 which);

/* sfnt table functions */
typedef struct
	{
	Card32 tag;						/* Table tag */
	void (*read)(Card8, LongN, Card32);	/* Reads table data structures */
	void (*diff)(LongN, LongN);		/* diffs table data structures */
	void (*free)(Card8);				/* Frees table data structures */
	void (*usage)(void);			/* Prints table usage */
	} Function;


#if 1
static Function function[] =
	{
	{BASE_},
	{BBOX_},
	{BLND_},
	{CFF__},
	{CID__},
	{EBLC_},
	{ENCO_},
	{FNAM_},
	{GLOB_},
	{GPOS_},
	{GSUB_},
	{HFMX_},
	{KERN_},
	{LTSH_},
	{MMVR_},
	{OS_2_},
	{TYP1_},
	{WDTH_},
	{bloc_},
	{cmap_},
	{fdsc_},
	{feat_},
	{fvar_},
	{gasp_},
	{glyf_},
	{hdmx_},
	{head_, headRead, headDiff, headFree},
	{hhea_},
	{hmtx_},
	{kern_},
	{loca_},
	{maxp_},
	{name_, nameRead, nameDiff, nameFree},
	{post_},
	{sfnt_},
	{trak_},
	{vhea_},
	{vmtx_},
	};

#else
/* Table MUST be in tag order */
static Function function[] =
	{
	{BASE_, BASERead, BASEDiff, BASEFree},
	{BBOX_, BBOXRead, BBOXDiff, BBOXFree},
	{BLND_, BLNDRead, BLNDDiff, BLNDFree},
	{CFF__, CFF_Read, CFF_Diff, CFF_Free, CFF_Usage},
	{CID__, CID_Read, CID_Diff, CID_Free},
	{EBLC_, EBLCRead, EBLCDiff, EBLCFree},
	{ENCO_, ENCORead, ENCODiff, ENCOFree},
	{FNAM_, FNAMRead, FNAMDiff, FNAMFree},
	{GLOB_, GLOBRead, GLOBDiff, GLOBFree},
	{GPOS_, GPOSRead, GPOSDiff, GPOSFree, GPOSUsage},
	{GSUB_, GSUBRead, GSUBDiff, GSUBFree, GSUBUsage},
	{HFMX_, HFMXRead, HFMXDiff, HFMXFree},
	{KERN_, kernRead, kernDiff, kernFree},
	{LTSH_, LTSHRead, LTSHDiff, LTSHFree},
	{MMVR_, MMVRRead, MMVRDiff, MMVRFree},
	{OS_2_, OS_2Read, OS_2Diff, OS_2Free},
	{TYP1_, TYP1Read, TYP1Diff, TYP1Free},
	{WDTH_, WDTHRead, WDTHDiff, WDTHFree},
	{bloc_, EBLCRead, EBLCDiff, EBLCFree},
	{cmap_, cmapRead, cmapDiff, cmapFree, cmapUsage},
	{fdsc_, fdscRead, fdscDiff, fdscFree},
	{feat_, featRead, featDiff, featFree},
	{fvar_, fvarRead, fvarDiff, fvarFree},
	{gasp_, gaspRead, gaspDiff, gaspFree},
	{glyf_, glyfRead, glyfDiff, glyfFree, glyfUsage},
	{hdmx_, hdmxRead, hdmxDiff, hdmxFree},
	{head_, headRead, headDiff, headFree},
	{hhea_, hheaRead, hheaDiff, hheaFree},
	{hmtx_, hmtxRead, hmtxDiff, hmtxFree, hmtxUsage},
	{kern_, kernRead, kernDiff, kernFree, kernUsage},
	{loca_, locaRead, locaDiff, locaFree},
	{maxp_, maxpRead, maxpDiff, maxpFree},
	{name_, nameRead, nameDiff, nameFree},
	{post_, postRead, postDiff, postFree},
	{sfnt_,     NULL, dirDiff,  dirFree},
	{trak_, trakRead, trakDiff, trakFree},
	{vhea_, vheaRead, vheaDiff, vheaFree},
	{vmtx_, vmtxRead, vmtxDiff, vmtxFree},	
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
#endif

/* TT collection */
#ifdef TTC_SUPPORTED
static ttcfTbl ttcf1;
static ttcfTbl ttcf2;
#endif
typedef struct
	{
	Card32 offset;
	Card16 loaded;
	Card16 dumped;
	da_DCL(Card32, sel);	/* Table directory offset selectors */
	} _ttc_;
static _ttc_ ttc1 = {0,0,0};
static _ttc_ ttc2 = {0,0,0};


/* sfnt */
static sfntTbl sfnt1 = {0,0};
static sfntTbl sfnt2 = {0,0};
IntX loaded1 = 0;
IntX loaded2 = 0;

typedef struct
	{
	Card32 offset;	/* Directory header offset */
	Int16 id;		/* Resource id (Mac) */
	} _dir_;
static _dir_ dir1;
static _dir_ dir2;

/* Table dump list */
typedef struct
	{
	Card32 tag;
	Int16 level;
	} Dump;

#define EXCLUDED -999

static struct
	{
	Byte8 *arg;
	da_DCL(Dump, list);
	} dump;

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

static void printTag(Card32 tag)
{
  printf( "%c%c%c%c", TAG_ARG(tag));
}

/* Read TT Collection header */
void sfntTTCRead(Card8 which, LongN start)
	{
#if TTC_SUPPORTED
	IntX i;
	SEEK_ABS(which, start);

	if (which == 1)
	  {
		IN(1, ttcf1.TTCTag);
		IN(1, ttcf1.Version);
		IN(1, ttcf1.DirectoryCount);

		ttcf1.TableDirectory =
		  memNew(sizeof(ttcf1.TableDirectory[0]) * ttcf1.DirectoryCount);
		for (i = 0; i < ttcf1.DirectoryCount; i++)
		  IN(1, ttcf1.TableDirectory[i]);

		ttc1.offset = start;
		ttc1.loaded = 1;
		ttc1.dumped = 0;

		for (i = 0; i < ttcf1.DirectoryCount; i++)
		  {
			Card32 offset = ttcf1.TableDirectory[i];
			
			if (ttc1.sel.cnt == 0 || 
				bsearch(&offset, ttc1.sel.array, ttc1.sel.cnt, 
						sizeof(Card32), cmpCard32s) != NULL)
			  sfntRead(1, ttc1.offset + offset, -1);
		  }
		
		memFree(ttcf1.TableDirectory);
		ttc1.loaded = 0;
	  }
	else
	  {
		IN(2, ttcf2.TTCTag);
		IN(2, ttcf2.Version);
		IN(2, ttcf2.DirectoryCount);
		ttcf2.TableDirectory =
		  memNew(sizeof(ttcf2.TableDirectory[0]) * ttcf2.DirectoryCount);
		for (i = 0; i < ttcf2.DirectoryCount; i++)
		  IN(2, ttcf2.TableDirectory[i]);

		ttc2.offset = start;
		ttc2.loaded = 1;
		ttc2.dumped = 0;

		for (i = 0; i < ttcf2.DirectoryCount; i++)
		  {
			Card32 offset = ttcf2.TableDirectory[i];
			
			if (ttc2.sel.cnt == 0 || 
				bsearch(&offset, ttc2.sel.array, ttc2.sel.cnt, 
						sizeof(Card32), cmpCard32s) != NULL)
			  sfntRead(2, ttc2.offset + offset, -1);
		  }
		
		memFree(ttcf2.TableDirectory);
		ttc2.loaded = 0;
	  }
#endif
	}

/* Dump TT Collection header */
static void ttcfDump(Card8 which, LongN start)
	{
#if TTC_SUPPORTED
	IntX i;
	if (which == 1)
	  {
		if (!ttc1.loaded)
		  return;	/* Not a TT Collection */
		
		DL(1, ("### [ttcf] (%08lx)\n", start));
		
		DL(2, ("TTCTag        =%c%c%c%c (%08lx)\n", 
			   TAG_ARG(ttcf1.TTCTag), ttcf1.TTCTag));
		DLV(2, "Version       =", ttcf1.Version);
		DLU(2, "DirectoryCount=", ttcf1.DirectoryCount);
		
		DL(2, ("--- TableDirectory[index]=offset\n"));
		for (i = 0; i < ttcf1.DirectoryCount; i++)
		  DL(2, ("[%d]=%08lx ", i, ttcf1.TableDirectory[i]));
		DL(2, ("\n"));
	  }
	else
	  {
		if (!ttc2.loaded)
		  return;	/* Not a TT Collection */
		
		DL(1, ("### [ttcf] (%08lx)\n", start));
		
		DL(2, ("TTCTag        =%c%c%c%c (%08lx)\n", 
			   TAG_ARG(ttcf2.TTCTag), ttcf2.TTCTag));
		DLV(2, "Version       =", ttcf2.Version);
		DLU(2, "DirectoryCount=", ttcf2.DirectoryCount);
		
		DL(2, ("--- TableDirectory[index]=offset\n"));
		for (i = 0; i < ttcf2.DirectoryCount; i++)
		  DL(2, ("[%d]=%08lx ", i, ttcf2.TableDirectory[i]));
		DL(2, ("\n"));
	  }
#endif
	}

/* Read sfnt directories */
static void dirRead(LongN start1, LongN start2)
	{
	IntX i;

	if (!loaded1) 
	  {
		SEEK_SURE(1, start1);
		dir1.offset = start1;
		
		IN(1, sfnt1.version);
		IN(1, sfnt1.numTables);
		IN(1, sfnt1.searchRange);
		IN(1, sfnt1.entrySelector);
		IN(1, sfnt1.rangeShift);
		
		sfnt1.directory = memNew(sizeof(Entry) * sfnt1.numTables);
		for (i = 0; i < sfnt1.numTables; i++)
		  {
			Entry *entry = &sfnt1.directory[i];
			
			IN(1, entry->tag);
			IN(1, entry->checksum);
			IN(1, entry->offset);
			IN(1, entry->length);
		  }

		loaded1 = 1;
	  }
	
	if (!loaded2)
	  {
		SEEK_SURE(2, start2);
		dir2.offset = start2;
		
		IN(2, sfnt2.version);
		IN(2, sfnt2.numTables);
		IN(2, sfnt2.searchRange);
		IN(2, sfnt2.entrySelector);
		IN(2, sfnt2.rangeShift);
		
		sfnt2.directory = memNew(sizeof(Entry) * sfnt2.numTables);
		for (i = 0; i < sfnt2.numTables; i++)
		  {
			Entry *entry = &sfnt2.directory[i];
			
			IN(2, entry->tag);
			IN(2, entry->checksum);
			IN(2, entry->offset);
			IN(2, entry->length);
		  }
		loaded2 = 1;
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
	
static void setLevel(Card32 tag, IntX lvel)
        {
        Dump *dmp = (Dump *)bsearch(&tag, dump.list.array, dump.list.cnt,
									sizeof(Dump), cmpDumpTags);
        if (dmp != NULL)
                dmp->level = lvel;
        }

/* Scan tag-exclusion argument list */
static void scanTagsExclude(Byte8 *actualarg)
	{
	Byte8 *p;
	Byte8 arg[100];

	strcpy(arg, actualarg); /* otherwise "actualarg" (argv[*]) gets trounced */

	for (p = strtok(arg, ","); p != NULL; p = strtok(NULL, ","))
		{
		Byte8 tag[5];

		if (sscanf(p, "%4[^\n]", tag) != 1 || strlen(tag) != 4)
			{
			warning("bad tag <%s> (ignored)\n", p);
			continue;
			}
		setLevel(STR2TAG(tag), EXCLUDED);
		}
	}

static void	excludeAllTags(void)
{
  int i;
  for (i = 0; i < dump.list.cnt; i++) 
	{
	  dump.list.array[i].level = EXCLUDED;
	}
}


static void scanTagsInclude(Byte8 *actualarg)
	{
	Byte8 *p;
	Byte8 arg[100];

	strcpy(arg, actualarg); /* otherwise "actualarg" (argv[*]) gets trounced */

	for (p = strtok(arg, ","); p != NULL; p = strtok(NULL, ","))
		{
		Byte8 tag[5];

		if (sscanf(p, "%4[^\n]", tag) != 1 || strlen(tag) != 4)
			{
			warning("bad tag <%s> (ignored)\n", p);
			continue;
			}
		setLevel(STR2TAG(tag), level);
		}
	}

/* Add table to dump list */
static void addDumpTable(Card32 tag)
	{
	Dump *dmp;

	dmp = (Dump *)bsearch (&tag, dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumpTags);

	if (dmp == NULL)  /* not yet in table */
	  {
		dmp = da_NEXT(dump.list);
		dmp->tag = tag;
		dmp->level = 0;
        qsort(dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumps);
	  }
	}

/* Make table dump list */
static void preMakeDump(void)
	{
	IntX i;
	
	/* Add all tables in font */
	da_INIT_ONCE(dump.list, 40, 10);
	dump.list.cnt = 0;
	for (i = 0; i < sfnt1.numTables; i++)
    {
        addDumpTable(sfnt1.directory[i].tag);
    }

	for (i = 0; i < sfnt2.numTables; i++)
    {
       addDumpTable(sfnt2.directory[i].tag);
    }

	/* Add ttcf header and sfnt directory and sort into place */
	addDumpTable(ttcf_);
	addDumpTable(sfnt_);
	qsort(dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumps);
	}


static void makeDump(void)
	{		
		if (opt_Present("-x"))
		  scanTagsExclude(dump.arg);
		else if (opt_Present("-i"))
		  {
			excludeAllTags();
			scanTagsInclude(dump.arg);
		  }
  	}


static IntX findDirTag1(Card32 tag)
{
  IntX i;

	for (i = 0; i < sfnt1.numTables; i++)
		if (sfnt1.directory[i].tag == tag)
		  return (i);
  return (-1);
}

static IntX findDirTag2(Card32 tag)
{
  IntX i;

	for (i = 0; i < sfnt2.numTables; i++)
		if (sfnt2.directory[i].tag == tag)
		  return (i);
  return (-1);
}

static void initDump(void)
	{
	  IntX i;
	  for (i = 0; i < dump.list.cnt; i++)
		{
		  Dump *dmp = da_INDEX(dump.list, i);
		  dmp->level = 0;
		}
	}



/* Free sfnt directory */
static void dirFree(Card8 which)
	{
	  if (which == 1)
		{
		  memFree(sfnt1.directory);
		  sfnt1.directory = NULL;
		  sfnt1.numTables = 0;
		  loaded1 = 0;
		}
	  else
		{
		  memFree(sfnt2.directory);
		  sfnt2.directory = NULL;
		  sfnt2.numTables = 0;
		  loaded2 = 0;
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
static Entry *findEntry(Card8 which, Card32 tag)
	{
	  if (which == 1)
		{
		  if (sfnt1.numTables == 0)
			{
			  dirRead(0,0);
			}
		  return (Entry *)bsearch(&tag, sfnt1.directory, sfnt1.numTables,
								  sizeof(Entry), cmpEntryTags);
		}
	  else
		{
		  if (sfnt2.numTables == 0)
			{
			  dirRead(0,0);
			}
		  return (Entry *)bsearch(&tag, sfnt2.directory, sfnt2.numTables,
								  sizeof(Entry), cmpEntryTags);
		}
	}

/* Hexadecimal table dump */
void hexDump(Card8 which, Card32 tag, LongN start, Card32 length)
	{
	Card32 addr = 0;
	Int32 left = length;

	SEEK_ABS(which, start);

	printf( "### [%c%c%c%c] (%08lx)\n", TAG_ARG(tag), start);
	while (left > 0)
		{
		/* Dump one line */
		IntX i;
		Card8 data[16];

		IN_BYTES(which, (left < 16) ? left : 16, data);

		/* Dump 8 hexadecimal words of data */
		printf( "%08x  ", addr);

		for (i = 0; i < 16; i++)
			{
			if (i < left)
				printf( "%02x", data[i]);
			else
				printf( "  ");
			if ((i + 1) % 2 == 0)
				printf( " ");
			}

		/* Dump ascii interpretation of data */
		printf( " |");
		for (i = 0; i < 16; i++)
			if (i < left)
				printf( "%c", isprint(data[i]) ? data[i] : data[i] ? '?' : '.');
			else
				printf( " ");
		printf( "|\n");

		addr += 16;
		left -= 16;
		}
	}

void hexDiff(Card32 tag, 
			 LongN start1, Card32 length1, 
			 LongN start2, Card32 length2)
	{
	  Card32 addr = 0;
	  Int32 left;
	  Int32 diffaddr = (-1);
	  IntX noted = 0;
	  
	  if (length1 < length2)
		left = length1;
	  else
		left = length2;
	  
	  SEEK_SURE(1, start1); 
	  SEEK_SURE(2, start2);
	  
	  while (left > 0)
		{		  /* Dump one line */
		  IntX i;
		  Card8 data1[16];
		  Card8 data2[16];
		  
		  IN_BYTES(1, (left < 16) ? left : 16, data1);
		  IN_BYTES(2, (left < 16) ? left : 16, data2);
		  
		  for (i = 0; i < ((left < 16) ? left : 16); i++)
			{
			  if (data1[i] != data2[i])
				{
				  diffaddr = addr + i;
				  DiffExists++;
				  break;
				}
			}
		  
		  if (!noted && (diffaddr > 0))
			{
			  note("'%c%c%c%c' differs.\n", TAG_ARG(tag));
			  noted=1;
			}

		  if (diffaddr > 0)
			{
			  if (level == 0)
				{
				  return;
				}
			  else if (level == 1)
				{
				  note("< %08lx+%08lx=%08lx\n", start1, diffaddr, start1+diffaddr);
				  note("> %08lx+%08lx=%08lx\n", start2, diffaddr, start2+diffaddr);
				  return;
				}
			  else if (level == 2) 
				{
				  /* Dump 8 hexadecimal words of data */
				  printf( "< %08x  ", addr);

				  for (i = 0; i < 16; i++)
					{
					  if (i < left)
						printf( "%02x", data1[i]);
					  else
						printf( "  ");
					  if ((i + 1) % 2 == 0)
						printf( " ");
					}
				  /* Dump ascii interpretation of data */
				  printf( " |");
				  for (i = 0; i < 16; i++)
					if (i < left)
					  printf( "%c", isprint(data1[i]) ? data1[i] : data1[i] ? '?' : '.');
					else
					  printf( " ");
				  printf( "|\n");

				  /* Dump 8 hexadecimal words of data */
				  printf( "> %08x  ", addr);

				  for (i = 0; i < 16; i++)
					{
					  if (i < left)
						printf( "%02x", data2[i]);
					  else
						printf( "  ");
					  if ((i + 1) % 2 == 0)
						printf( " ");
					}
				  /* Dump ascii interpretation of data */
				  printf( " |");
				  for (i = 0; i < 16; i++)
					if (i < left)
					  printf( "%c", isprint(data2[i]) ? data2[i] : data2[i] ? '?' : '.');
					else
					  printf( " ");
				  printf( "|\n");
				  
				  printf( "\n");
				}
			}
		  addr += 16;
		  left -= 16;
		  diffaddr = (-1);
		}
	}

/* Diff sfnt directory entries */
static void dirDiff3(LongN start1, LongN start2)
	{
	IntX i;

	if ((dir1.id > 0) && (dir2.id > 0))
	  if (dir1.id != dir2.id)
		{
		  DiffExists++;
		  note("< sfnt DirectoryID=%d\n",dir1.id);
		  note("> sfnt DirectoryID=%d\n",dir2.id);
		}

	if (sfnt1.version != sfnt2.version)
	  {
		DiffExists++;
		note ("< sfnt ");
		if (sfnt1.version == OTTO_)
		  DL(3, ("version=OTTO  [OpenType]"));
		else if (sfnt1.version == typ1_)
		  DL(3, ("version=typ1  [Type 1]"));
		else if (sfnt1.version == true_)
		  DL(3, ("version=true  [TrueType]"));
		else if (sfnt1.version == VERSION(1,0))
		  DL(3, ("version= 1.0  [TrueType]"));
		else
		  DL(3, ("version=%c%c%c%c (%08x) [????]", 
			 TAG_ARG(sfnt1.version), sfnt1.version));

		note ("> sfnt ");
		if (sfnt2.version == OTTO_)
		  DL(3, ("version=OTTO  [OpenType]"));
		else if (sfnt2.version == typ1_)
		  DL(3, ("version=typ1  [Type 1]"));
		else if (sfnt2.version == true_)
		  DL(3, ("version=true  [TrueType]"));
		else if (sfnt2.version == VERSION(1,0))
		  DL(3, ("version= 1.0  [TrueType]"));
		else
		  DL(3, ("version=%c%c%c%c (%08x) [????]", 
			 TAG_ARG(sfnt2.version), sfnt2.version));
	  }

	if (sfnt1.numTables != sfnt2.numTables)
	  {
		DiffExists++;
		note("< sfnt numtables=%hu\n", sfnt1.numTables);
		note("> sfnt numtables=%hu\n", sfnt2.numTables);
	  }
	/* cross-check to see if each table in sfnt1 exists in sfnt2 and vice-versa.
	   if not, complain.
	   if so, compare the checksum and length values.
 */

	for (i = 0; i < sfnt1.numTables; i++)
	  {
		Entry *entry1 = &sfnt1.directory[i];
		IntX other = findDirTag2(entry1->tag);

		if (other < 0) 
		  {
			DiffExists++;
			note("< 'sfnt' table has '%c%c%c%c'\n",TAG_ARG(entry1->tag));
			note("> 'sfnt' table missing '%c%c%c%c'\n",TAG_ARG(entry1->tag));
		  }
		else
		  {
			Entry *entry2 = &sfnt2.directory[other];
			if (entry1->checksum != entry2->checksum)
			  {
				DiffExists++;
				note("< '%c%c%c%c' table checksum=%08lx\n", TAG_ARG(entry1->tag), entry1->checksum);
				note("> '%c%c%c%c' table checksum=%08lx\n", TAG_ARG(entry2->tag), entry2->checksum);
			  }
			if (entry1->length != entry2->length)
			  {
				DiffExists++;
				note("< '%c%c%c%c' table length=%08lx\n", TAG_ARG(entry1->tag), entry1->length);
				note("> '%c%c%c%c' table length=%08lx\n", TAG_ARG(entry2->tag), entry2->length);
			  }
		  }
	  }

	for (i = 0; i < sfnt2.numTables; i++)
	  {
		Entry *entry2 = &sfnt2.directory[i];
		IntX other = findDirTag1(entry2->tag);

		if (other < 0) 
		  {
			DiffExists++;
			note("< 'sfnt' table missing '%c%c%c%c'\n",TAG_ARG(entry2->tag));
			note("> 'sfnt' table has '%c%c%c%c'\n",TAG_ARG(entry2->tag));
		  }
	  }


	if (sfnt1.searchRange != sfnt2.searchRange)
	  {
		DiffExists++;
		note("< sfnt searchRange=%hu\n", sfnt1.searchRange);
		note("> sfnt searchRange=%hu\n", sfnt2.searchRange);
	  }
	if (sfnt1.entrySelector != sfnt2.entrySelector)
	  {
		DiffExists++;
		note("< sfnt entrySelector=%hu\n", sfnt1.entrySelector);
		note("> sfnt entrySelector=%hu\n", sfnt2.entrySelector);
	  }
	if (sfnt1.rangeShift != sfnt2.rangeShift)
	  {
		DiffExists++;
		note("< sfnt rangeShift=%hu\n", sfnt1.rangeShift);
		note("> sfnt rangeShift=%hu\n", sfnt2.rangeShift);
	  }
  }

static void dirDiff(Card32 start1, Card32 start2)
{
  if (level > 2) 
	{
	  dirDiff3(start1, start2);
	  return;
	}
  /* else perform byte-by-byte comparison */
  else 
	{
	  Card32 length1, length2;

	  if (start1 != dir1.offset)
		start1 = dir1.offset;
	  length1 = DIR_HDR_SIZE + (ENTRY_SIZE * sfnt1.numTables);
	  if (start2 != dir2.offset)
		start2 = dir2.offset;
	  length2 = DIR_HDR_SIZE + (ENTRY_SIZE * sfnt2.numTables);

	  hexDiff(sfnt_, start1, length1, start2, length2);
	}
}


/* Free tables. Table free functions are called regardless of whether the table
   was selected for dumping because some tables get read in order to supply
   values used for reading another table */
static void freeTables(Card8 which)
	{
	  CardX i;
	  if (which == 1) 
		{
		  if (sfnt1.directory == NULL) 
			return; /* nothing loaded, so nothing to free */
		}
	  else
		{
		  if (sfnt2.directory == NULL) 
			return; /* nothing loaded, so nothing to free */
		}
	  
	  for (i = 0; i < TABLE_LEN(function); i++)
		if (function[i].free != NULL)
		  function[i].free(which);
	}

/* Process tables */
static void doTables(IntX read)
	{
	IntX i;
	for (i = 0; i < dump.list.cnt; i++)
		{
		Card32 tag = dump.list.array[i].tag;

		Card32 start1, start2;
		Card32 length1, length2;
		
		Function *func = findFunc(tag);
		Entry *entry1 = findEntry(1, tag);
		Entry *entry2 = findEntry(2, tag);

		if (!entry1 && !entry2) continue;
		if (!entry1 || !entry2)
		  {
			continue;
		  }

		if (dump.list.array[i].level == EXCLUDED)
		  continue;
		
		switch (tag)
		  {
		  case 0:
			continue;

#if TTC_SUPPORTED
		  case ttcf_:
			/* Handle ttcf header specially */
			if (!ttc1.dumped)
			  {
				start1 = ttc1.offset;
				length1 = TTC_HDR_SIZE + 
				  sizeof(ttcf1.TableDirectory[0]) * ttcf1.DirectoryCount;
				
				start2 = ttc2.offset;
				length2 = TTC_HDR_SIZE + 
				  sizeof(ttcf2.TableDirectory[0]) * ttcf2.DirectoryCount;
				
				if (level < 0)
				  hexDump(tag, start, length);
				else 
				  ttcfDump(level, start1, start2);
				
				ttc1.dumped = 1;	/* Ensure we only dump this table once */
				ttc2.dumped = 1;	/* Ensure we only dump this table once */
			  }
			continue;
#endif
		  case sfnt_:
			/* Fake sfnt directory values */
			start1 = dir1.offset;
			length1 = DIR_HDR_SIZE + (ENTRY_SIZE * sfnt1.numTables);
			start2 = dir2.offset;
			length2 = DIR_HDR_SIZE + (ENTRY_SIZE * sfnt2.numTables);
			break;
		  default:
			start1 = (ttc1.loaded ? ttc1.offset : dir1.offset) + entry1->offset;
			length1 = entry1->length;
			start2 = (ttc2.loaded ? ttc2.offset : dir2.offset) + entry2->offset;
			length2 = entry2->length;
			break;
		  }
		
		if (read)
		  {
			if (func != NULL && func->read != NULL)
			  {
				func->read(1, start1, length1);
				func->read(2, start2, length2);
			  }
		  }
		else
		  {
			if (func == NULL) /* non-handled table */
			  {
				hexDiff(tag, start1, length1, start2, length2);
			  }
			else if (func->diff != NULL)
			  {
				if (level < 3)
				  hexDiff(tag, start1, length1, start2, length2);
				else
				  func->diff(start1, start2);
			  }
			else 
			  {
				if ((entry1->checksum != entry2->checksum) ||
					(entry1->length != entry2->length))
				  hexDiff(tag, start1, length1, start2, length2);
			  }
		  }
	  }
	}

void sfntReset()
{
	loaded1=loaded2=0;
	nameReset();
}

void sfntRead(LongN start1, IntX id1, LongN start2, IntX id2)
	{
	dir1.id = id1;
	dir2.id = id2;

	dirRead(start1, start2);	/* Read sfnt directories */

	dirDiff(start1, start2);

	preMakeDump(); /* build dump list */
	makeDump();		/* Make dump list */
	doTables(1);	/* Read */
	}

void sfntDump(void)
	{
	initDump();
	makeDump();
	doTables(1); /* Read */
	doTables(0);	/* Dump/diff */	
	}

void sfntFree(void)
	{
	freeTables(1);	/* Free */
	freeTables(2);	/* Free */
	dirFree(1);
	dirFree(2);
	}

/* Read requested tables */
IntX sfntReadTable(Card32 tag)
	{
	Entry *entry1, *entry2;
	
	entry1 = findEntry(1, tag);
	entry2 = findEntry(2, tag);

	if ((entry1 == NULL) || (entry2 == NULL)) 
	  return 1; /* missing a table? */

	findFunc(tag)->read(1, (ttc1.loaded ? ttc1.offset : dir1.offset) + entry1->offset,
						entry1->length); 

	findFunc(tag)->read(2, (ttc2.loaded ? ttc2.offset : dir2.offset) + entry2->offset,
						entry2->length); 

	return 0;
	}


IntX sfntReadATable(Card8 which, Card32 tag)
	{
	Entry *entry;
	
	if (which == 1)
	  entry = findEntry(1, tag);
	else
	  entry = findEntry(2, tag);

	if (entry == NULL)	  return 1; /* missing a table? */

	if (which == 1)
	  findFunc(tag)->read(1, (ttc1.loaded ? ttc1.offset : dir1.offset) + entry->offset,
						  entry->length); 
	else
	  findFunc(tag)->read(2, (ttc2.loaded ? ttc2.offset : dir2.offset) + entry->offset,
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


/* TTC directory offset list argument scanner */
int sfntTTCScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
#if TTC_SUPPORTED
	if (argi == 0)
		return 0; /* No initialization required */

	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		{
		Byte8 *p;
		Byte8 *arg = argv[argi++];

		da_INIT_ONCE(ttc.sel, 5, 2);
		ttc.sel.cnt = 0;
		for (p = strtok(arg, ","); p != NULL; p = strtok(NULL, ","))
			{
			Card32 offset;

			if (sscanf(p, "%li", &offset) == 1)
				*da_NEXT(ttc.sel) = offset;
			else
				opt_Error(opt_Format, opt, arg);
			}
		qsort(ttc.sel.array, ttc.sel.cnt, sizeof(Card32), cmpCard32s);
		}
	return argi;
#else
	return 0;
#endif
	}

/* Print table usage infomation */
void sfntUsage(void)
	{
	CardX i;

	printf( "Supported tables:");
	for (i = 0; i < TABLE_LEN(function); i++)
		{
		if (i % 10 == 0)
			printf( "\n    ");
		printf( "%c%c%c%c%s", TAG_ARG(function[i].tag),
			   (i + 1 == TABLE_LEN(function)) ? "\n" : ", ");
		}
	}

/* Print table-specific usage */
void sfntTableSpecificUsage(void)
	{
	CardX i;

	printf( "Table-specific usage:\n");
	for (i = 0; i < TABLE_LEN(function); i++)
		if (function[i].usage != NULL)
			function[i].usage();

	quit(1);
	}

