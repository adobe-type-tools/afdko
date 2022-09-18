/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * sfnt reading/comparing
 */

#include <ctype.h>
#include <string.h>

#include "Dsfnt.h"
#include "sfnt_sfnt.h"
#include "slogger.h"

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

static void dirDiff(uint32_t start1, uint32_t start2);
static void dirFree(uint8_t which);

/* sfnt table functions */
typedef struct {
    uint32_t tag;                           /* Table tag */
    void (*read)(uint8_t, int32_t, uint32_t); /* Reads table data structures */
    void (*diff)(int32_t, int32_t);             /* diffs table data structures */
    void (*free)(uint8_t);                  /* Frees table data structures */
    void (*usage)(void);                    /* Prints table usage */
} Function;

#if 1
static Function function[] = {
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
        {head_, sdHeadRead, sdHeadDiff, sdHeadFree},
        {hhea_},
        {hmtx_},
        {kern_},
        {loca_},
        {maxp_},
        {name_, sdNameRead, sdNameDiff, sdNameFree},
        {post_},
        {sfnt_},
        {trak_},
        {vhea_},
        {vmtx_},
};

#else
/* Table MUST be in tag order */
static Function function[] = {
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
        {name_, nameRead, sdNameDiff, sdNameFree},
        {post_, postRead, postDiff, postFree},
        {sfnt_, NULL, dirDiff, dirFree},
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
typedef struct {
    uint32_t offset;
    uint16_t loaded;
    uint16_t dumped;
    da_DCL(uint32_t, sel); /* Table directory offset selectors */
} _ttc_;
static _ttc_ ttc1 = {0, 0, 0};
static _ttc_ ttc2 = {0, 0, 0};

/* sfnt */
static sfntTbl sfnt1 = {0, 0};
static sfntTbl sfnt2 = {0, 0};
int loaded1 = 0;
int loaded2 = 0;

typedef struct {
    uint32_t offset; /* Directory header offset */
    int16_t id;      /* Resource id (Mac) */
} _dir_;
static _dir_ dir1;
static _dir_ dir2;

/* Table dump list */
typedef struct {
    uint32_t tag;
    int16_t level;
} Dump;

#define EXCLUDED -999

static struct {
    int8_t *arg;
    da_DCL(Dump, list);
} dump;

/* Read sfnt directories */
static void dirRead(int32_t start1, int32_t start2) {
    int i;

    if (!loaded1) {
        SEEK_SURE(1, start1);
        dir1.offset = start1;

        IN(1, sfnt1.version);
        IN(1, sfnt1.numTables);
        IN(1, sfnt1.searchRange);
        IN(1, sfnt1.entrySelector);
        IN(1, sfnt1.rangeShift);

        sfnt1.directory = sMemNew(sizeof(Entry) * sfnt1.numTables);
        for (i = 0; i < sfnt1.numTables; i++) {
            Entry *entry = &sfnt1.directory[i];

            IN(1, entry->tag);
            IN(1, entry->checksum);
            IN(1, entry->offset);
            IN(1, entry->length);
        }

        loaded1 = 1;
    }

    if (!loaded2) {
        SEEK_SURE(2, start2);
        dir2.offset = start2;

        IN(2, sfnt2.version);
        IN(2, sfnt2.numTables);
        IN(2, sfnt2.searchRange);
        IN(2, sfnt2.entrySelector);
        IN(2, sfnt2.rangeShift);

        sfnt2.directory = sMemNew(sizeof(Entry) * sfnt2.numTables);
        for (i = 0; i < sfnt2.numTables; i++) {
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
static int cmpDumps(const void *first, const void *second) {
    uint32_t a = ((Dump *)first)->tag;
    uint32_t b = ((Dump *)second)->tag;
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

/* Compare with dump table tag */
static int cmpDumpTags(const void *key, const void *value) {
    uint32_t a = *(uint32_t *)key;
    uint32_t b = ((Dump *)value)->tag;
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

static void setLevel(uint32_t tag, int lvel) {
    Dump *dmp = (Dump *)bsearch(&tag, dump.list.array, dump.list.cnt,
                                sizeof(Dump), cmpDumpTags);
    if (dmp != NULL)
        dmp->level = lvel;
}

/* Scan tag-exclusion argument list */
static void scanTagsExclude(int8_t *actualarg) {
    int8_t *p;
    int8_t arg[100];
    char *stsave;

    STRCPY_S(arg, sizeof(arg), actualarg);  // otherwise "actualarg" (argv[*]) gets trounced

    for (p = STRTOK_R(arg, ",", &stsave); p != NULL; p = STRTOK_R(NULL, ",", &stsave)) {
        int8_t tag[5];

        if (sscanf(p, "%4[^\n]", tag) != 1 || strlen(tag) != 4) {
            sdWarning("bad tag <%s> (ignored)\n", p);
            continue;
        }
        setLevel(STR2TAG(tag), EXCLUDED);
    }
}

static void excludeAllTags(void) {
    int i;
    for (i = 0; i < dump.list.cnt; i++) {
        dump.list.array[i].level = EXCLUDED;
    }
}

static void scanTagsInclude(int8_t *actualarg) {
    int8_t *p;
    int8_t arg[100];
    char *stsave;

    STRCPY_S(arg, sizeof(arg), actualarg);  // otherwise "actualarg" (argv[*]) gets trounced

    for (p = STRTOK_R(arg, ",", &stsave); p != NULL; p = STRTOK_R(NULL, ",", &stsave)) {
        int8_t tag[5];

        if (sscanf(p, "%4[^\n]", tag) != 1 || strlen(tag) != 4) {
            sdWarning("bad tag <%s> (ignored)\n", p);
            continue;
        }
        setLevel(STR2TAG(tag), level);
    }
}

/* Add table to dump list */
static void addDumpTable(uint32_t tag) {
    Dump *dmp;

    dmp = (Dump *)bsearch(&tag, dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumpTags);

    if (dmp == NULL) {  // not yet in table
        dmp = da_NEXT(dump.list);
        dmp->tag = tag;
        dmp->level = 0;
        qsort(dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumps);
    }
}

/* Make table dump list */
static void preMakeDump(void) {
    int i;

    /* Add all tables in font */
    da_INIT_ONCE(dump.list, 40, 10);
    dump.list.cnt = 0;
    for (i = 0; i < sfnt1.numTables; i++) {
        addDumpTable(sfnt1.directory[i].tag);
    }

    for (i = 0; i < sfnt2.numTables; i++) {
        addDumpTable(sfnt2.directory[i].tag);
    }

    /* Add ttcf header and sfnt directory and sort into place */
    addDumpTable(ttcf_);
    addDumpTable(sfnt_);
    qsort(dump.list.array, dump.list.cnt, sizeof(Dump), cmpDumps);
}

static void makeDump(void) {
    if (opt_Present("-x"))
        scanTagsExclude(dump.arg);
    else if (opt_Present("-i")) {
        excludeAllTags();
        scanTagsInclude(dump.arg);
    }
}

static int findDirTag1(uint32_t tag) {
    int i;

    for (i = 0; i < sfnt1.numTables; i++)
        if (sfnt1.directory[i].tag == tag)
            return (i);
    return (-1);
}

static int findDirTag2(uint32_t tag) {
    int i;

    for (i = 0; i < sfnt2.numTables; i++)
        if (sfnt2.directory[i].tag == tag)
            return (i);
    return (-1);
}

static void initDump(void) {
    int i;
    for (i = 0; i < dump.list.cnt; i++) {
        Dump *dmp = da_INDEX(dump.list, i);
        dmp->level = 0;
    }
}

/* Free sfnt directory */
static void dirFree(uint8_t which) {
    if (which == 1) {
        sMemFree(sfnt1.directory);
        sfnt1.directory = NULL;
        sfnt1.numTables = 0;
        loaded1 = 0;
    } else {
        sMemFree(sfnt2.directory);
        sfnt2.directory = NULL;
        sfnt2.numTables = 0;
        loaded2 = 0;
    }
}

/* Compare with function table tag */
static int cmpFuncTags(const void *key, const void *value) {
    uint32_t a = *(uint32_t *)key;
    uint32_t b = ((Function *)value)->tag;
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

/* Find tag in function table */
static Function *findFunc(uint32_t tag) {
    return (Function *)bsearch(&tag, function, TABLE_LEN(function),
                               sizeof(Function), cmpFuncTags);
}

/* Compare with directory entry tag */
static int cmpEntryTags(const void *key, const void *value) {
    uint32_t a = *(uint32_t *)key;
    uint32_t b = ((Entry *)value)->tag;
    if (a < b)
        return -1;
    else if (a > b)
        return 1;
    else
        return 0;
}

/* Find entry in the sfnt directory */
static Entry *findEntry(uint8_t which, uint32_t tag) {
    if (which == 1) {
        if (sfnt1.numTables == 0) {
            dirRead(0, 0);
        }
        return (Entry *)bsearch(&tag, sfnt1.directory, sfnt1.numTables,
                                sizeof(Entry), cmpEntryTags);
    } else {
        if (sfnt2.numTables == 0) {
            dirRead(0, 0);
        }
        return (Entry *)bsearch(&tag, sfnt2.directory, sfnt2.numTables,
                                sizeof(Entry), cmpEntryTags);
    }
}

/* Hexadecimal table dump */
void hexDump(uint8_t which, uint32_t tag, int32_t start, uint32_t length) {
    uint32_t addr = 0;
    int32_t left = length;

    SEEK_ABS(which, start);

    printf("### [%c%c%c%c] (%08lx)\n", TAG_ARG(tag), start);
    while (left > 0) {
        /* Dump one line */
        int i;
        uint8_t data[16];

        IN_BYTES(which, (left < 16) ? left : 16, data);

        /* Dump 8 hexadecimal words of data */
        printf("%08x  ", addr);

        for (i = 0; i < 16; i++) {
            if (i < left)
                printf("%02x", data[i]);
            else
                printf("  ");
            if ((i + 1) % 2 == 0)
                printf(" ");
        }

        /* Dump ascii interpretation of data */
        printf(" |");
        for (i = 0; i < 16; i++)
            if (i < left)
                printf("%c", ISPRINT(data[i]) ? data[i] : data[i] ? '?' : '.');
            else
                printf(" ");
        printf("|\n");

        addr += 16;
        left -= 16;
    }
}

void hexDiff(uint32_t tag,
             int32_t start1, uint32_t length1,
             int32_t start2, uint32_t length2) {
    uint32_t addr = 0;
    int32_t left;
    int32_t diffaddr = (-1);
    int noted = 0;

    if (length1 < length2)
        left = length1;
    else
        left = length2;

    SEEK_SURE(1, start1);
    SEEK_SURE(2, start2);

    while (left > 0) { /* Dump one line */
        int i;
        uint8_t data1[16];
        uint8_t data2[16];

        IN_BYTES(1, (left < 16) ? left : 16, data1);
        IN_BYTES(2, (left < 16) ? left : 16, data2);

        for (i = 0; i < ((left < 16) ? left : 16); i++) {
            if (data1[i] != data2[i]) {
                diffaddr = addr + i;
                DiffExists++;
                break;
            }
        }

        if (!noted && (diffaddr > 0)) {
            sdNote("'%c%c%c%c' differs.\n", TAG_ARG(tag));
            noted = 1;
        }

        if (diffaddr > 0) {
            if (level == 0) {
                return;
            } else if (level == 1) {
                sdNote("< %08lx+%08x=%08lx\n", start1, diffaddr, start1 + diffaddr);
                sdNote("> %08lx+%08x=%08lx\n", start2, diffaddr, start2 + diffaddr);
                return;
            } else if (level == 2) {
                /* Dump 8 hexadecimal words of data */
                printf("< %08x  ", addr);

                for (i = 0; i < 16; i++) {
                    if (i < left)
                        printf("%02x", data1[i]);
                    else
                        printf("  ");
                    if ((i + 1) % 2 == 0)
                        printf(" ");
                }
                /* Dump ascii interpretation of data */
                printf(" |");
                for (i = 0; i < 16; i++)
                    if (i < left)
                        printf("%c", ISPRINT(data1[i]) ? data1[i] : data1[i] ? '?' : '.');
                    else
                        printf(" ");
                printf("|\n");

                /* Dump 8 hexadecimal words of data */
                printf("> %08x  ", addr);

                for (i = 0; i < 16; i++) {
                    if (i < left)
                        printf("%02x", data2[i]);
                    else
                        printf("  ");
                    if ((i + 1) % 2 == 0)
                        printf(" ");
                }
                /* Dump ascii interpretation of data */
                printf(" |");
                for (i = 0; i < 16; i++)
                    if (i < left)
                        printf("%c", ISPRINT(data2[i]) ? data2[i] : data2[i] ? '?' : '.');
                    else
                        printf(" ");
                printf("|\n");

                printf("\n");
            }
        }
        addr += 16;
        left -= 16;
        diffaddr = (-1);
    }
}

/* Diff sfnt directory entries */
static void dirDiff3(int32_t start1, int32_t start2) {
    int i;

    if ((dir1.id > 0) && (dir2.id > 0))
        if (dir1.id != dir2.id) {
            DiffExists++;
            sdNote("< sfnt DirectoryID=%d\n", dir1.id);
            sdNote("> sfnt DirectoryID=%d\n", dir2.id);
        }

    if (sfnt1.version != sfnt2.version) {
        DiffExists++;
        sdNote("< sfnt ");
        if (sfnt1.version == OTTO_)
            DL(3, ("version=OTTO  [OpenType]"));
        else if (sfnt1.version == typ1_)
            DL(3, ("version=typ1  [Type 1]"));
        else if (sfnt1.version == true_)
            DL(3, ("version=true  [TrueType]"));
        else if (sfnt1.version == VERSION(1, 0))
            DL(3, ("version= 1.0  [TrueType]"));
        else
            DL(3, ("version=%c%c%c%c (%08x) [????]",
                   TAG_ARG(sfnt1.version), sfnt1.version));

        sdNote("> sfnt ");
        if (sfnt2.version == OTTO_)
            DL(3, ("version=OTTO  [OpenType]"));
        else if (sfnt2.version == typ1_)
            DL(3, ("version=typ1  [Type 1]"));
        else if (sfnt2.version == true_)
            DL(3, ("version=true  [TrueType]"));
        else if (sfnt2.version == VERSION(1, 0))
            DL(3, ("version= 1.0  [TrueType]"));
        else
            DL(3, ("version=%c%c%c%c (%08x) [????]",
                   TAG_ARG(sfnt2.version), sfnt2.version));
    }

    if (sfnt1.numTables != sfnt2.numTables) {
        DiffExists++;
        sdNote("< sfnt numtables=%hu\n", sfnt1.numTables);
        sdNote("> sfnt numtables=%hu\n", sfnt2.numTables);
    }

    /* cross-check to see if each table in sfnt1 exists in sfnt2 and vice-versa.
       if not, complain.
       if so, compare the checksum and length values. */

    for (i = 0; i < sfnt1.numTables; i++) {
        Entry *entry1 = &sfnt1.directory[i];
        int other = findDirTag2(entry1->tag);

        if (other < 0) {
            DiffExists++;
            sdNote("< 'sfnt' table has '%c%c%c%c'\n", TAG_ARG(entry1->tag));
            sdNote("> 'sfnt' table missing '%c%c%c%c'\n", TAG_ARG(entry1->tag));
        } else {
            Entry *entry2 = &sfnt2.directory[other];
            if (entry1->checksum != entry2->checksum) {
                DiffExists++;
                sdNote("< '%c%c%c%c' table checksum=%08x\n", TAG_ARG(entry1->tag), entry1->checksum);
                sdNote("> '%c%c%c%c' table checksum=%08x\n", TAG_ARG(entry2->tag), entry2->checksum);
            }
            if (entry1->length != entry2->length) {
                DiffExists++;
                sdNote("< '%c%c%c%c' table length=%08x\n", TAG_ARG(entry1->tag), entry1->length);
                sdNote("> '%c%c%c%c' table length=%08x\n", TAG_ARG(entry2->tag), entry2->length);
            }
        }
    }

    for (i = 0; i < sfnt2.numTables; i++) {
        Entry *entry2 = &sfnt2.directory[i];
        int other = findDirTag1(entry2->tag);

        if (other < 0) {
            DiffExists++;
            sdNote("< 'sfnt' table missing '%c%c%c%c'\n", TAG_ARG(entry2->tag));
            sdNote("> 'sfnt' table has '%c%c%c%c'\n", TAG_ARG(entry2->tag));
        }
    }

    if (sfnt1.searchRange != sfnt2.searchRange) {
        DiffExists++;
        sdNote("< sfnt searchRange=%hu\n", sfnt1.searchRange);
        sdNote("> sfnt searchRange=%hu\n", sfnt2.searchRange);
    }
    if (sfnt1.entrySelector != sfnt2.entrySelector) {
        DiffExists++;
        sdNote("< sfnt entrySelector=%hu\n", sfnt1.entrySelector);
        sdNote("> sfnt entrySelector=%hu\n", sfnt2.entrySelector);
    }
    if (sfnt1.rangeShift != sfnt2.rangeShift) {
        DiffExists++;
        sdNote("< sfnt rangeShift=%hu\n", sfnt1.rangeShift);
        sdNote("> sfnt rangeShift=%hu\n", sfnt2.rangeShift);
    }
}

static void dirDiff(uint32_t start1, uint32_t start2) {
    if (level > 2) {
        dirDiff3(start1, start2);
        return;
    } else {  /* else perform byte-by-byte comparison */
        uint32_t length1, length2;

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
static void freeTables(uint8_t which) {
    unsigned int i;
    if (which == 1) {
        if (sfnt1.directory == NULL)
            return; /* nothing loaded, so nothing to free */
    } else {
        if (sfnt2.directory == NULL)
            return; /* nothing loaded, so nothing to free */
    }

    for (i = 0; i < TABLE_LEN(function); i++)
        if (function[i].free != NULL)
            function[i].free(which);
}

/* Process tables */
static void doTables(int read) {
    int i;
    for (i = 0; i < dump.list.cnt; i++) {
        uint32_t tag = dump.list.array[i].tag;

        uint32_t start1, start2;
        uint32_t length1, length2;

        Function *func = findFunc(tag);
        Entry *entry1 = findEntry(1, tag);
        Entry *entry2 = findEntry(2, tag);

        if (!entry1 && !entry2) continue;
        if (!entry1 || !entry2) {
            continue;
        }

        if (dump.list.array[i].level == EXCLUDED)
            continue;

        switch (tag) {
            case 0:
                continue;
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

        if (read) {
            if (func != NULL && func->read != NULL) {
                func->read(1, start1, length1);
                func->read(2, start2, length2);
            }
        } else {
            if (func == NULL) {  // non-handled table
                hexDiff(tag, start1, length1, start2, length2);
            } else if (func->diff != NULL) {
                if (level < 3)
                    hexDiff(tag, start1, length1, start2, length2);
                else
                    func->diff(start1, start2);
            } else {
                if ((entry1->checksum != entry2->checksum) ||
                    (entry1->length != entry2->length))
                    hexDiff(tag, start1, length1, start2, length2);
            }
        }
    }
}

void sdSfntRead(int32_t start1, int id1, int32_t start2, int id2) {
    dir1.id = id1;
    dir2.id = id2;

    dirRead(start1, start2); /* Read sfnt directories */

    dirDiff(start1, start2);

    preMakeDump(); /* build dump list */
    makeDump();    /* Make dump list */
    doTables(1);   /* Read */
}

void sdSfntDump(void) {
    initDump();
    makeDump();
    doTables(1); /* Read */
    doTables(0); /* Dump/diff */
}

void sdSfntFree(void) {
    freeTables(1); /* Free */
    freeTables(2); /* Free */
    dirFree(1);
    dirFree(2);
}

int sdSfntReadATable(uint8_t which, uint32_t tag) {
    Entry *entry;

    if (which == 1)
        entry = findEntry(1, tag);
    else
        entry = findEntry(2, tag);

    if (entry == NULL) return 1; /* missing a table? */

    if (which == 1)
        findFunc(tag)->read(1, (ttc1.loaded ? ttc1.offset : dir1.offset) + entry->offset,
                            entry->length);
    else
        findFunc(tag)->read(2, (ttc2.loaded ? ttc2.offset : dir2.offset) + entry->offset,
                            entry->length);

    return 0;
}

/* Handle tag list argument */
int sdSfntTagScan(int argc, char *argv[], int argi, opt_Option *opt) {
    if (argi == 0)
        return 0; /* Initialization not required */

    if (argi == argc)
        opt_Error(opt_Missing, opt, NULL);
    else
        dump.arg = argv[argi++];
    return argi;
}
