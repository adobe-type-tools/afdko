/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* SCCS Id:    @(#)timecheck.c	1.5
/* Changed:    4/22/98 13:00:28
/***********************************************************************/
/* timecheck.c */

/* File modification time recording and cross-checking module. This
 * module maintains the files .ACtimes and .Converttimes. The format
 * of these files has been completely changed from binary to ASCII
 * because of porting problems of an alignment and byte ordering
 * nature. However, the old format files, version 10 and 11, are
 * still read and silently updated to the new ASCII format.
 *
 * Jerry Hall - 22 Apr 1991
 * Judy Lee  17 Sep 1991 - call standard FileExists in machinedep.c
 */

#include "ac.h"
#include "bftoac.h"
#include "fipublic.h"
#include "machinedep.h"
#include "buildfont.h"

#ifdef __MWERKS__
/*#include "types.h"*/
/*#include <sys/file.h>*/
/*#include <sys/dir.h>*/
#else
#include <sys/types.h>
#if WIN32
#include <file.h>
#include <dir.h>
#else
#include <sys/file.h>
#include <sys/dir.h>
#endif
#endif

#if !IS_LIB
#include <sys/stat.h>
#endif

/*#include <search.h>*/
#include <sys/time.h>
/*#include <strings.h>*/

#define VERSION10 1010 /* Mon Jan  9 10:25:53 1989 */
/* version 11 includes the ability to specify {h,v}counter hinting
   characters.  It also includes a conversion time if starting
   from pschars or ill. */
#define VERSION11 1011 /* Wed Mar 29 1989 */
#define VERSION_1_0 "1.0" /* JAH, 04/22/91 */
#define AC_NAME ".ACtimes"
#define CVT_NAME ".Converttimes"

/* The counter list is COUNTERLISTSIZE = 20 entries long. It turns out
 * that this means that it actually accomodates 19 entries plus a
 * terminating NULL.
 */
#define COUNTER_EXTRA_ENTRIES (COUNTERLISTSIZE - COUNTERDEFAULTENTRIES - 1)

/* The longest line that can appear in a timestamp file is for the
 * H or V counter chars and is calculated as:
 * 14 + (MAXCHARNAME * COUNTER_EXTRA_ENTRIES - 1) + 1 + 1 = 570.
 * The following constant can comfortably accomodate this.
 */
#define MAX_TEXT_LINE 768

/* Modification time struct from each file */
typedef struct {
	time_t modTime;
	short nameIndx;
} fileTime;

int newFileCnt = 0; /* Number of files and their ... */
fileTime* newFileTimes; /* ... timestamps read from charset or directory */

int oldFileCnt = 0; /* Number of files and their ... */
fileTime* oldFileTimes; /* ... timestamps read from the timestamp file */

/* Lists of character names/filenames are accumulated in the buffer below
 * which is allocated dynamically and grown if it becomes too small. The
 * buffer is managed by the routine storeName. A slot of MAXCHARNAME
 * characters is reserved at index 0 and is used by the binary name search
 * routine to store the search name. The initial buffer size is easily big
 * enough to hold 2 name lists for an ISO Adobe character set. These require
 * 1095 characters each, including terminating NULLs.
 */
char* nameBuf = 0; /* Character name/filename buffer */
int bufSize = 0; /* Name buffer size */
char* nextFree = (char*)MAXCHARNAME; /* Pointer to next free char in buffer */
#define BUF_INCR 4096 /* Buffer allocation increment */
#define INDX_PTR(i) ((i) + nameBuf) /* Converts name index to a char pointer */

/*char* bsearch();*/
time_t time();

private char* allocate();
private unsigned short calcChksum();
private void newTimesFile(const char*, boolean);
private void readInputDir(const char*, boolean);
private short storeName();
private void formatTime();
private void writeTimesFile(boolean convert, float scale);
private void printArray();
private void printCounterList();
private void sortFileTimes();
private fileTime* findName();
private void copyTimes();
private void updateTime();
private fileTime* checkFile();
private void readCharList();
private boolean readTimesFile(boolean convert, float scale);
private boolean readCharSet();
private boolean readOldTimesFile(FILE* fp, boolean convert, float scale);
private int get16();
private int get32();
private boolean compareData();
private boolean readFileTimesData();
private boolean compareCounterListData();
private boolean compareArrays();
private boolean readFileTimes();
private boolean compareCounterLists();
private boolean compareLists();
private boolean compareTimes(boolean, boolean, boolean*, boolean);
private boolean compareCvtTimes(char *inputDir, boolean release, tConvertfunc convertFunc);

/* Free up space used for storing file times and filenames.
 */
private void freeFileTimes(void)
{
#if DOMEMCHECK
	memck_free(oldFileTimes);
	memck_free(newFileTimes);
	memck_free(nameBuf);
#else
	ACFREEMEM(oldFileTimes);
	ACFREEMEM(newFileTimes);
	ACFREEMEM(nameBuf);
#endif
	oldFileTimes = newFileTimes = NULL;
	oldFileCnt = newFileCnt = bufSize = 0;
	nextFree = (char*)MAXCHARNAME;
	nameBuf = NULL;
}

/* Creates a new .ACtimes file.  A .Converttimes file is also
 * created using the current timestamps if a pschars or ill
 * directory exists. Called by the -t option to BuildFont.
 */
public boolean CreateTimesFile(void)
{
#if !IS_LIB
	if (DirExists(ILLDIR, FALSE, FALSE, FALSE))
		newTimesFile(ILLDIR, TRUE);
	else if (DirExists(RAWPSDIR, FALSE, FALSE, FALSE))
		newTimesFile(RAWPSDIR, TRUE);

	newTimesFile(BEZDIR, FALSE);

	freeFileTimes();
#endif	
	return TRUE; /* This maintains compatibility with the old version */
}

/* Warns about and renames any existing .ACtimes or .Converttimes files
 * then creates a new .ACtimes or .Converttimes file using the character
 * files' current timestamps.
 */  
private void newTimesFile(const char *dirName, 
						  boolean convert)/* Flags converting from ill/ or pschars/ directory */
{
#if IS_LIB
#pragma unused(dirName)
#pragma unused(convert)
#else
	float scale;
	char* timesFileName = (convert) ? CVT_NAME : AC_NAME;
	
	readInputDir(dirName, TRUE);
	
	if (CFileExists(timesFileName, FALSE)) {
		/* Rename the old timestamp file */
		char newName[50];
		
		sprintf(globmsg, "Renaming existing %s file to %s.old.\n",
				timesFileName, timesFileName);
		LogMsg(globmsg, WARNING, OK, TRUE);
		sprintf(newName, "%s.old", timesFileName);
		RenameFile(timesFileName, newName);
	}

	set_scale(&scale);
	writeTimesFile(convert, scale);
#endif
}

/* Reads in file names from input directory instead of
 * character set.  Ignores .BAK files.
 */
private void readInputDir(const char *dirName, 
						  boolean creating)/* Flags creation (not updating) of timestamp file */ 
{
#if IS_LIB
#pragma unused(dirName)
#pragma unused(creating)
#else
	
	extern int scandir();
	extern int bf_alphasort();
	int fileCnt;
	struct direct** nameList;
		
	fileCnt = BFscandir(dirName, &nameList, IncludeFile, bf_alphasort);
	if (fileCnt == -1) {
		sprintf(globmsg, "Can't read the %s directory.\n", dirName);
		LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
	}
	else {
		int i;
		fileTime* p;
		char charFile[MAXPATHLEN];
		struct stat s;

		/* explicitly look for .notdef bez file */
#ifdef __MWERKS__
		sprintf(charFile, ":%s:.notdef", dirName);
#else
		sprintf(charFile, "%s\\.notdef", dirName);
#endif
		if (FileExists(charFile, FALSE) &&
			!stat(charFile, &s))
		  {
			newFileTimes =(fileTime*)allocate(fileCnt+1, sizeof(fileTime));
		  }
		else
		  {
			newFileTimes = (fileTime*)allocate(fileCnt, sizeof(fileTime));
		  }

		p = newFileTimes;
		
		for (i = 0; i < fileCnt; i++) {
#ifdef __MWERKS__
			sprintf(charFile, ":%s:%s", dirName, nameList[i]->d_name);
#else
			sprintf(charFile, "%s\\%s", dirName, nameList[i]->d_name);
#endif		
			if (FileExists(charFile, FALSE) &&
				!stat(charFile, &s)) {
				p->nameIndx = storeName(nameList[i]->d_name);
				p->modTime = (creating) ? s.st_mtime : 0;
				p++;
			}
		}
		
		/* explicitly handle for .notdef bez file */
#ifdef __MWERKS__
			sprintf(charFile, ":%s:.notdef", dirName);
#else
			sprintf(charFile, "%s\\.notdef", dirName);
#endif

		if (FileExists(charFile, FALSE) &&
			!stat(charFile, &s))
		  {
			p->nameIndx = storeName(".notdef");
			p->modTime = (creating) ? s.st_mtime : 0;
			p++;
		  }

		newFileCnt = p - newFileTimes;	/* Actual number of valid files */
#if DOMEMCHECK
		memck_free(nameList);
#else
		ACFREEMEM(nameList);
#endif
	}
#endif
}

/* Dynamic space allocator.
 */
private char* allocate(int n, int size)
{
#if IS_LIB
#if DOMEMCHECK
	char *p = (char *)memck_calloc(n, size);
#else
	char *p = (char *)ACNEWMEM(n * size);
	if (NULL != p)
		memset((void *)p, 0x0, n * size);
#endif
#else
	char* p = (char *)calloc(n,size);
#endif	
	if (!p) {
		sprintf(globmsg, "Can't allocate space for file times.\n");
		LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
	}
	
	return p;
}


/* Store a name in the name buffer. Return the index to the
 * name, not a pointer. This is so the buffer can be moved (by
 * realloc) without screwing up all the references that have
 * already been made to it.
 */
private short storeName(const char *name)
{
	short tmp = nextFree - nameBuf;

	if (strlen(name) + 1 + nextFree - nameBuf > bufSize) {
		/* Not enough space so (re)allocate it */
		
		bufSize += BUF_INCR;
#if DOMEMCHECK
		if (nameBuf)
			nameBuf = memck_realloc(nameBuf, bufSize);
		else
			nameBuf = memck_malloc(bufSize);
#else
		if (nameBuf)
			nameBuf = ACREALLOCMEM(nameBuf, bufSize);
		else
			nameBuf = ACNEWMEM(bufSize);
#endif
		if (!nameBuf) {
			sprintf(globmsg, "Can't allocate space for file names.\n");
			LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}

		nextFree = tmp + nameBuf; /* Realloc can move the block */
	}

	/* Copy name */
	while (*nextFree++ = *name++)
		;
	
	return tmp;
}

/* Write out the timestamp file.
 */
private void writeTimesFile(boolean convert, /* Flags if writing .Converttimes */
							float scale)/* Coversion scale factor */
{
#if IS_LIB
#pragma unused(convert)
#pragma unused(scale)
#else
	
	int i;
	char s[25];
	unsigned short chksum;
	char* timesFileName = (convert) ? CVT_NAME : AC_NAME;
	int lines = 4 + 1 + ((convert) ? 1 : 7) + 1 + newFileCnt;
	FILE* fp = ACOpenFile(timesFileName, "w+", OPENWARN); /* Valid for Mac? */

	if (fp == NULL)
		return;
	
	fprintf(fp, "%%%% File %s\n", timesFileName);
	fprintf(fp, "%%%% Version %s\n", VERSION_1_0);
	formatTime(time((time_t*)0), s);
	fprintf(fp, "%%%% Created %s\n", s);
	fprintf(fp, "%%%% Length %d\n", lines);
	fprintf(fp, "%%%% Header:\n");

	if (convert)
		/* .Converttimes header */
		fprintf(fp, "Scale %d.%d\n", (int)scale,
				(int)((scale - (int)scale) * 1000.0));
	else {
		/* .ACtimes header */
		fprintf(fp, "FlexOK %s\n", (flexOK) ? "true" : "false");
		printArray(fp, "HStems", NumHStems, HStems);
		printArray(fp, "VStems", NumVStems, VStems);
		printArray(fp, "BotBands", lenBotBands, botBands);
		printArray(fp, "TopBands", lenTopBands, topBands);
		printCounterList(fp, "HCounterChars", NumHColors,
						 &HColorList[COUNTERDEFAULTENTRIES]);
		printCounterList(fp, "VCounterChars", NumVColors,
						 &VColorList[COUNTERDEFAULTENTRIES]);
	}

	fprintf(fp, "%%%% File Timestamps:\n");
	for (i = 0; i < newFileCnt; i++) {
		formatTime(newFileTimes[i].modTime, s);
		fprintf(fp, "(%s) %s\n", s, INDX_PTR(newFileTimes[i].nameIndx));
	}		

	chksum = calcChksum(fp, lines);

	/* Must do this when changing from read to write on a file opened
	 * for update, see fopen(3S).
	 */
	fseek(fp, 0L, L_XTND);

	fprintf(fp, "%%%% Checksum %04x\n", chksum);
	
	if (ferror(fp)) {
		sprintf(globmsg, "Error writing the %s file.\n", timesFileName);
		LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
	}
	fclose(fp);
#endif
	return;
}

/* Format string with date and time.
 */
private void formatTime(time_t t, char* s)
{
	struct tm* tmp;

	tmp = localtime(&t);
	sprintf(s, "%02d/%02d/%02d %02d:%02d:%02d",
			tmp->tm_mon + 1, tmp->tm_mday, tmp->tm_year,
			tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
}

/* Prints an array of numbers inside square brackets.
 */
private void printArray(
			FILE* fp, /* Timestamp file pointer */
			char* name, /* Array name */
			long len, /* Array length */
			Fixed* a /* Array */)
{
	int i;
	
	fprintf(fp, "%s [", name);
	for (i = 0; i < len; i++)
		if (i + 1 == len)
			fprintf(fp, "%d", FTrunc(a[i])); /* Last in array, no space */
		else
			fprintf(fp, "%d ", FTrunc(a[i]));
	fprintf(fp, "]\n");
}

/* Prints a list of strings inside round brackets.
 */
private void printCounterList(
			FILE* fp, /* Timestamp file pointer */
			char* name, /* List name */
			long len, /* List length */
			char** list /* The list */)
{
	int i;

	fprintf(fp, "%s (", name);
	for (i = 0; i < len; i++)
		if (i + 1 == len)
			fprintf(fp, "%s", list[i]); /* Last in list, no separating space */
		else
			fprintf(fp, "%s ", list[i]);
	fprintf(fp, ")\n");
}

/* Computes the timestamp file checksum by reading the specified
 * number of lines from the beginning of the file and accumulating
 * checksum as a 16-bit rotated shifted exor.
 */
private unsigned short calcChksum(FILE *fp, int lines)
{
	char buf[MAX_TEXT_LINE];
	register unsigned short chksum = 0;
	
	rewind(fp);
	while (lines-- && fgets(buf, MAX_TEXT_LINE, fp)) {
		register char* p = buf;

		while (*p)
			chksum = ((chksum << 1) + ((chksum & 0x8000) ? 1 : 0)) ^ *p++;
	}
	
	return chksum;
}


/* Hint the specified files and update their time entries in the
 * timestamp file. Called by AC only when hinting a list of characters.
 */
public boolean DoArgs(
			int cnt, /* Number of characters to process */
			char* names[], /* Names of characters to process */
			boolean extraColor,
			boolean* renameLog,
			boolean release /* Flags release version */)
{
#ifdef IS_GGL
	return FALSE;
#else
	int i;
	boolean result = TRUE;
	
#ifdef IS_LIB
	if (!featurefiledata){
#endif
	
	if (release) {
		if (!readCharSet()) {
			freeFileTimes();
			return FALSE;
		}
	}
	else
		readInputDir(BEZDIR, FALSE);

	
	if (readTimesFile(FALSE, 0.0))
		copyTimes();

#ifdef IS_LIB
	}
#endif
	
	for (i = 0; i < cnt; i++) {
		if (!DoFile(names[i], extraColor)) {
			result = FALSE;
			continue;
		}
		else
			*renameLog = TRUE;
#if !IS_LIB		
		updateTime(findName(names[i], newFileCnt, newFileTimes), BEZDIR);
#endif
    }

#if !IS_LIB
	writeTimesFile(FALSE, 0.0);

	freeFileTimes();
#endif

	return result;
#endif
}

/* Reads through the charset file and builds
 * a list of character names/filenames.
 */
private boolean readCharSet()
{
#if !IS_LIB
	FILE *fp;
	char cName[MAXCHARNAME+4];
	char csFileName[MAXPATHLEN];
	char fName[MAXFILENAME+4];
	int i;
	long masters, hintDir;
	short nextName = nextFree - nameBuf; /* Remember first name position */
	
	getcharsetname(csFileName);
	fp = ACOpenFile(csFileName, "r", OPENWARN);
	if (fp == NULL)
		return FALSE;

	for (newFileCnt = 0; ReadNames(cName, fName, &masters, &hintDir, fp); newFileCnt++)
		(void)storeName(fName);
	fclose(fp);

	newFileTimes = (fileTime*)allocate(newFileCnt, sizeof(fileTime));

	for (i = 0; i < newFileCnt; i++) {
		newFileTimes[i].nameIndx = nextName;
		nextName += strlen(INDX_PTR(nextName)) + 1;
	}   

	sortFileTimes(newFileCnt, newFileTimes);
#endif	
	return TRUE;
}

/* Compare function for qsort and bsearch */
private int cmpNames(const void *va, const void *vb)

{
	fileTime* a = (fileTime *)va;
	fileTime* b = (fileTime *)vb;
	return strcmp(INDX_PTR(a->nameIndx), INDX_PTR(b->nameIndx));
}
/* Sorts a file times list by name into alphabetical order.
 */
private void sortFileTimes(
			int len, /* Length of table */
			fileTime* table /* Table to be sorted */)
{
	qsort((char*)table, len, sizeof(fileTime), cmpNames);
}



/* Reads timestamp file. Checks checksum. Compares values read
 * with values read from the fontinfo file (in memory). If any
 * header values are different then the file times are discarded
 * and all the characters will be rehinted.
 */
private boolean readTimesFile(
				boolean convert, /* Flags if reading .Converttimes */
				float scale /* Conversion scale */)
{
#if IS_LIB
#pragma unused(scale)
#pragma unused(convert)
	return TRUE;
#else
	char tmp[MAXFILENAME];
	int lines;
	unsigned short chksum;
	int linesRead = 0;
	char HCounterLine[MAX_TEXT_LINE];
	char VCounterLine[MAX_TEXT_LINE];
	char* timesFileName = (convert) ? CVT_NAME : AC_NAME;
	FILE* fp = ACOpenFile(timesFileName, "r", OPENOK);
	
	if (fp == NULL)
		return FALSE;

	/* Read and check the file preamble */
	if (fscanf(fp, "%%%% File %s\n", tmp) != 1) {
		/* Try to read in old format */
		if (!readOldTimesFile(fp, convert, scale))
			goto error;
		fclose(fp);
		return TRUE;
	}
	
	if (strcmp(tmp, timesFileName) ||
		fscanf(fp, "%%%% Version %s\n", tmp) != 1 ||
		strcmp(tmp, VERSION_1_0) ||
		fscanf(fp, "%%%% Created %*s %*s\n") != 0 ||
		fscanf(fp, "%%%% Length %d\n", &lines) != 1 ||
		fscanf(fp, "%%%% Header:\n") != 0)
		goto error;
	linesRead += 5;

	if (convert) {
		/* Read and check .Converttimes header */
		int whole;
		int frac;

		if (fscanf(fp, "Scale %d.%d\n", &whole, &frac) != 2 ||
			whole != (int)scale ||
			frac != (int)((scale - (int)scale) * 1000.0))
			goto error;
		linesRead += 1;
	}
	else {	/* Read and check .ACtimes header */
		
		if (fscanf(fp, "FlexOK %s\n", tmp) != 1 ||
			strcmp(tmp, (flexOK) ? "true" : "false") ||
			!compareArrays(fp, "HStems", NumHStems, HStems) ||
			!compareArrays(fp, "VStems", NumVStems, VStems) ||
			!compareArrays(fp, "BotBands", lenBotBands, botBands) ||
			!compareArrays(fp, "TopBands", lenTopBands, topBands) ||
			!fgets(HCounterLine, MAX_TEXT_LINE, fp) ||
			!fgets(VCounterLine, MAX_TEXT_LINE, fp))
			goto error;
		linesRead += 7;
	}

	if (fscanf(fp, "%%%% File Timestamps:\n") != 0)
		goto error;
	linesRead += 1;

	/* Read all of the file times */
	if (!readFileTimes(fp, lines - linesRead))
		goto error;

	/* Now check the checksum */
	if (fscanf(fp, "%%%% Checksum %hx\n", &chksum) != 1 ||
		chksum != calcChksum(fp, lines))
		goto error;

	if (!convert &&
		(!compareCounterLists("HCounterChars", HCounterLine, NumHColors,
							  &HColorList[COUNTERDEFAULTENTRIES]) ||
		 !compareCounterLists("VCounterChars", VCounterLine, NumVColors,
							  &VColorList[COUNTERDEFAULTENTRIES])))
		goto error;

	fclose(fp);
	return TRUE;

error: /* Just tidy up and return. Yes I've used a goto! I think this is
		* one of the few legitimate uses.
		*/
	fclose(fp);
#if DOMEMCHECK
	memck_free(oldFileTimes);
#else
	UnallocateMem(oldFileTimes);
#endif
	oldFileCnt = 0;
	return FALSE;
#endif
}

/* Try to read old format, version 10 or 11, timestamp file. This
 * function is very careful about how the numbers in the old file
 * are read so as not to introduce byte swapping errors since we
 * may not be running on a 680X0 machine. It does assume that
 * the file was generated on a 680X0 processor and that the
 * objects are therefore 2-byte aligned. This is not an unreasonable
 * assumption since BuildFont has not be available on any other
 * platform previously and this ASCII timestamp version will be
 * the first ported version.
 */
private boolean readOldTimesFile(
			FILE* fp, /* Timestamp file pointer */
			boolean convert, /* Flags if reading .Converttimes */
			float scale /* Conversion scale */)
{
#if IS_LIB
#pragma unused(fp)
#pragma unused(convert)
#pragma unused(scale)
#else
	
	long version;
	char buf[MAX_TEXT_LINE];
	long offset;
	int counterListLen;
	int HCounterCnt;
	int VCounterCnt;
	float s = (float)scale;

	rewind(fp);

	version = get32(fp);
	if (version != VERSION10 && version != VERSION11)
		return FALSE; /* Not a valid file */

	offset = get16(fp); /* File times offset */

	if (convert)
		/* Read and check .Converttimes file */
		return version == VERSION11 && get32(fp) == acpflttofix(&s) &&
			readFileTimesData(fp, offset);

	/* Read and check .ACtimes file */
	if (get32(fp) != flexOK ||
		get16(fp) != NumHStems || get16(fp) != NumVStems ||
		get16(fp) != lenBotBands || get16(fp) != lenTopBands)
		return FALSE;
	if (version == VERSION11) {
		VCounterCnt = get16(fp); /* V before H for some reason */
		HCounterCnt = get16(fp);
	}
	if (!compareData(fp, NumHStems, HStems) ||
		!compareData(fp, NumVStems, VStems) ||
		!compareData(fp, lenBotBands, botBands) ||
		!compareData(fp, lenTopBands, topBands))
		return FALSE;

	counterListLen = offset - ftell(fp);
	if (version == VERSION11 && counterListLen)
		if (fread(buf, counterListLen, 1, fp) != 1)
			return FALSE;
	
	if (!readFileTimesData(fp, offset))
		return FALSE;

	if (version == VERSION11) {
		char* cp = buf;
		
		return compareCounterListData(VCounterCnt, &cp, NumVColors,
									  &VColorList[COUNTERDEFAULTENTRIES]) &&
			compareCounterListData(HCounterCnt, &cp, NumHColors,
								   &HColorList[COUNTERDEFAULTENTRIES]);
	}
	else
#endif
		return TRUE;
}

/* Get 16-bit number from stream, assumes hi-byte lo-address ordering.
 */
private int get16(FILE* fp)
{
	int hi = getc(fp) & 0xff;
	int lo = getc(fp) & 0xff;

	return (hi << 8) | lo;
}

/* Get 32-bit number from stream, assumes hi-byte lo-address ordering.
 */
private int get32(FILE *fp)
{
	int b3 = getc(fp) & 0xff;
	int b2 = getc(fp) & 0xff;
	int b1 = getc(fp) & 0xff;
	int b0 = getc(fp) & 0xff;

	return (b3 << 24) | (b2 << 16) | (b1 << 8) | b0;
}

/* Reads an array of numbers in old format from the timestamp file
 * and compares them with an array derived from the fontinfo file (in memory).
 */
private boolean compareData(
		FILE* fp, /* Timestamp file pointer */
		long len, /* Array length */
		Fixed a[] /* Array */)
{
	int i;

	for (i = 0; i < len; i++)
		if (get32(fp) != a[i])
			return FALSE;

	return TRUE;
}

/* Read the old format file times data.
 */
private boolean readFileTimesData(
			FILE* fp, /* Timestamp file pointer */
			long offset /* File times offset */)
{
#if IS_LIB
#pragma unused(fp)
#pragma unused(offset)
#else	
	int i;
	int len;
	short nextName = nextFree - nameBuf; /* Remember first name position */

	fseek(fp, offset, L_SET); /* Seek to file times */
	for (oldFileCnt = 0; (len = get16(fp)) && !feof(fp); oldFileCnt++) {
		int nameLen;
		char name[MAXCHARNAME];
		char* p = name;
		
		(void)get32(fp); /* Discard modification time */

		while (*p++ = getc(fp)) /* Read the name */
			;

		nameLen = p - name;
		if (nameLen & 1) {
			(void)getc(fp); /* Read the odd byte */
			nameLen++;
		}

		if (len != 2 + 4 + nameLen)
			return FALSE;	/* Format error */

		(void)storeName(name);
	}
		
	oldFileTimes = (fileTime*)allocate(oldFileCnt, sizeof(fileTime));
	
	fseek(fp, offset, L_SET); /* Re-seek to file times */
	for (i = 0; i < oldFileCnt; i++) {
		len = get16(fp);

		oldFileTimes[i].modTime = get32(fp);
		oldFileTimes[i].nameIndx = nextName;
		nextName += strlen(INDX_PTR(nextName)) + 1;

		fseek(fp, len - (2 + 4), L_INCR); /* Skip over name */
	}
#endif
	return TRUE;
}

/* Scans the old format counter list input data (which was read earlier)
 * and marks the character names. These are then compared with a list
 * derived from the fontinfo file (in memory). Names not in the
 * intersection of these lists are marked for rehinting.
 */
private boolean compareCounterListData(
				int nameCnt, /* Number of names in the counter list  */
				char** cp, /* Counter list */
				long len, /* List length */
				char* list[] /* The list */)
{
	char* names[COUNTER_EXTRA_ENTRIES];
	int i;

	for (i = 0; i < nameCnt; i++) {
		names[i] = *cp;
		*cp += strlen(*cp) + 1;
	}

	return compareLists(nameCnt, names, len, list);
}

/* Reads an array of numbers inside square brackets from the
 * timestamp file and compares them with an array derived from
 * the fontinfo file (in memory).
 */
private boolean compareArrays(
				FILE* fp, /* Timestamp file pointer */
				char* name, /* Array name */
				long len, /* Array length */
				Fixed a[] /* Array */)
{
	char fmt[MAX_TEXT_LINE];
	int i;
	
	sprintf(fmt, "%s [", name); /* Make format string with array name */
	if (fscanf(fp, fmt)) /* Read the beginning of the line */
		return FALSE;
	
	for (i = 0; i < len; i++) {
		int val;
		
		if (fscanf(fp, "%d ", &val) != 1 || FixInt(val) != a[i])
			return FALSE;
	}

	return !fscanf(fp, "]\n"); /* Read the rest of the line */
}

/* Reads the file times from the timestamp file.
 */
private boolean readFileTimes(
				FILE* fp, /* Timestamp file pointer */
				int timeCnt /* Number of times to read */)
{
	int i;

	oldFileTimes = (fileTime*)allocate(timeCnt, sizeof(fileTime));

	for (i = 0; i < timeCnt; i++) {
		char name[MAXFILENAME];
		struct tm tm;
		
		if (fscanf(fp, "(%d/%d/%d %d:%d:%d) %s\n",
				   &tm.tm_mon, &tm.tm_mday, &tm.tm_year,
				   &tm.tm_hour, &tm.tm_min, &tm.tm_sec, name) != 7)
			return FALSE;

		tm.tm_mon--; /* Because it's stored 0-11!, see ctime(3) */
		oldFileTimes[i].nameIndx = storeName(name);
		oldFileTimes[i].modTime = mktime(&tm);
	}

	oldFileCnt = timeCnt;

	return TRUE;
}

/* Scans the counter list input line (which was read earlier) and marks
 * and counts the character names. These are then compared with a list
 * derived from the fontinfo file (in memory). Names not in the
 * intersection of these lists are marked for rehinting.
 */
private boolean compareCounterLists(
			char* listName, /* List name */
			char line[], /* Input line */
			long len, /* List length */
			char* list[] /* The list */)
{
	char label[64];
	char* names[COUNTER_EXTRA_ENTRIES];
	char* startp;
	int nameCnt;
	int labelLen;
	
	sprintf(label, "%s (", listName); /* Make label string with list name */
	labelLen = strlen(label);
	if (strncmp(line, label, labelLen))
		return FALSE;

	/* Locate all the names */
	startp = line + labelLen;
	for (nameCnt = 0; nameCnt < COUNTER_EXTRA_ENTRIES;
		 nameCnt++, startp = NULL)
		if (!(names[nameCnt] = (char *)strtok(startp, " )\n")))
			break;

	return compareLists(nameCnt, names, len, list);
}

/* Actually does the work of comparing the counter lists together.
 */
private boolean compareLists(
				int nameCnt, /* Items in names */
				char* names[], /* Name list */
				long len, /* List length */
				char* list[] /* The list */)
{
	int i;
	
	if (nameCnt == 0 && len == 0)
		return TRUE; /* Nothing to do */
	
	for (i = nameCnt; i < COUNTER_EXTRA_ENTRIES; i++)
		names[i] = NULL; /* Clear unused entries */

	for (i = 0; i < len; i++) {
		int j;
		
		for (j = 0; j < nameCnt; j++)
			if (!names[j])
				continue;
			else if (!strcmp(names[j], list[i])) {
				/* Character name in both lists so remove it */

				names[j] = NULL;
				break;
			}
		
		if (j == nameCnt) {
			/* Character name not found so force rehinting */
			fileTime* p = findName(list[i], oldFileCnt, oldFileTimes);

			if (p)
				p->modTime = 0;
			else {
				sprintf(globmsg, "Cannot find counter hints character \"%s\" in current %s file.\n",
						list[i], AC_NAME);
				LogMsg(globmsg, WARNING, OK, TRUE);
			}
		}
	}

	/* Force remaining characters to be rehinted */
	for (i = 0; i < nameCnt; i++)
		if (names[i]) {
			fileTime* p = findName(names[i], oldFileCnt, oldFileTimes);

			if (p)
				p->modTime = 0;
			/* OK if character not found in this situation */
		}

	return TRUE;
}
		
/* Copies old times list (read from timestamp file) to new
 * times list if the name can be found in the new list.
 */
private void copyTimes()
{
	int i;
	fileTime* op = oldFileTimes;

	for (i = 0; i < oldFileCnt; i++, op++) {
		fileTime* np = findName(INDX_PTR(op->nameIndx),
								newFileCnt, newFileTimes);
		if (np)
			np->modTime = op->modTime;
	}
}

/* Searches the specified table for the specified name.
 */
private fileTime* findName(
			char* name, /* Search name */
			int cnt, /* Number of names in table */
			fileTime* table /* Table to search */)
{
	fileTime ft;

	if (!cnt)
		return NULL;

	/* Copy the string to reserved area */
	ft.nameIndx = 0;
	strncpy(INDX_PTR(ft.nameIndx), name, MAXCHARNAME - 1);
	
	return (fileTime*)bsearch((char*)&ft, (char*)table, cnt,
							  sizeof(fileTime), cmpNames);
}

/* Updates a file's time by reading the new time from
 * the file on disk.
 */
private void updateTime(
			fileTime* p, /* Record to update */
			const char* dirName /* Directory name */)
{
#if IS_LIB
#pragma unused(p)
#pragma unused(dirName)
#else
	if (p) {
		char charFile[MAXPATHLEN];
		struct stat s;
#ifdef __MWERKS__
		sprintf(charFile, ":%s:%s", dirName, INDX_PTR(p->nameIndx));
#else
		sprintf(charFile, "%s\\%s", dirName, INDX_PTR(p->nameIndx));
#endif
		if (!stat(charFile, &s))
			p->modTime = s.st_mtime;
	}
#endif	
}

/* Scan all the files and hints the bez files that have changed.
 * Called by AC and BuildFont for rehinting whole font.
 */
public boolean DoAll(
			boolean extraColor,
			boolean release, /* Flags release version */
			boolean *renameLog,
			boolean quiet)
{
#ifdef IS_GGL
	return FALSE;
#else
	boolean result = TRUE;
	
	if (release) {
		if (!readCharSet()) {
			freeFileTimes();
			return FALSE;
		}
	}
	else
		readInputDir(BEZDIR, FALSE);
	
	readTimesFile(FALSE, 0.0);
	result = compareTimes(extraColor, release, renameLog, quiet);
	writeTimesFile(FALSE, 0.0);

	freeFileTimes();
	
	return result;
#endif
}

/* Converts characters that have changed since the last run
 * of BuildFont in the bez directory.
 */
private boolean compareTimes(
			boolean extraColor,
			boolean release, /* Flags release version */
			boolean *renameLog,
			boolean quiet)
{
	boolean result = TRUE;
	boolean printStatus = TRUE;
	int charCnt = 0;
	fileTime* newp;

	for (newp = newFileTimes; newp - newFileTimes < newFileCnt; newp++) {
		fileTime* oldp;
		
		newp = checkFile(&oldp, newp, BEZDIR);
		if (newp == NULL)
			break;
		else if (release) {
			sprintf(globmsg, "The %s character has out of date hints.\n  Add hints again before creating a release version.\n", INDX_PTR(newp->nameIndx));
			LogMsg(globmsg, LOGERROR, OK, TRUE);
			
			if (oldp)
				newp->modTime = oldp->modTime;
			
			result = FALSE;
		}
		else if (DoFile(INDX_PTR(newp->nameIndx), extraColor)) {
			if (printStatus) {
				if (!quiet) {
					sprintf(globmsg, "Adding hints %s... ",
							(extraColor) ? "" : "(single layer) ");
					LogMsg(globmsg, INFO, OK, FALSE);
				}
				printStatus = FALSE;
			}
			*renameLog = TRUE;
			charCnt++;
			updateTime(newp, BEZDIR);
		}
	}
	
	if (!quiet && !release && charCnt > 0) {
		LogMsg(globmsg, INFO, OK, FALSE);
	}
	
	return result;
}

/* The character names/filenames in the name list are checked. If
 * the circumstances have changed since the last timestamp file was
 * generated the function returns so that the character can be rehinted.
 * If the circumstances have stayed the same the file time is simply copied
 * and the scan continues. Returns NULL when the list is exhausted.
 */
private fileTime* checkFile(
				fileTime** oldp, /* Old file time */
				fileTime* newp, /* File time to check from */
				const char* inputDir /* Conversion input directory */)
{
#if IS_LIB
#pragma unused(oldp)
#pragma unused(newp)
#pragma unused(inputDir)
#else
	for (;newp - newFileTimes < newFileCnt; newp++) {
		char inFile[MAXPATHLEN];
		char* name = INDX_PTR(newp->nameIndx);
		struct stat s;
#ifdef __MWERKS__
		sprintf(inFile, ":%s:%s", inputDir, name);	
#else
		sprintf(inFile, "%s\\%s", inputDir, name);
#endif
		if (!stat(inFile, &s)) {
			char bezFile[MAXPATHLEN];
			time_t newTime = s.st_mtime;
			
			*oldp = findName(name, oldFileCnt, oldFileTimes);

#ifdef __MWERKS__
			sprintf(bezFile, ":%s:%s", BEZDIR, name);
#else
			sprintf(bezFile, "%s\\%s", BEZDIR, name);
#endif		
			if ((*oldp == NULL) || 
				((newTime != (*oldp)->modTime) && (strcmp(name,".notdef")!=0)) ||
				!FileExists(bezFile, FALSE))
				/* The name wasn't in the old timestamp file or the
				 * file time has changed or the bez file doesn't
				 * exist or has the wrong permissions.
				 */
				return newp;
			else
				newp->modTime = (*oldp)->modTime;
		}
	}
#endif	
	return NULL;
}

/* Converts characters that have changed since the last run of
 * BuildFont from the specified input directory using
 * the specified conversion function. Called by BuildFont -o when
 * ill/ or pschars/ characters exist and may need converting.
 */
public boolean ConvertCharFiles(char *inputDir,	/* Coversion input directory */
				boolean release,				/* Flags release version */
				float scale,				/* Conversion scale */
				tConvertfunc convertFunc	/* Conversion function */
				)
{
	boolean result = FALSE;
	
	if (release)
		readCharList();
	else
		readInputDir(inputDir, TRUE);
	
	readTimesFile(TRUE, scale);
	result = compareCvtTimes(inputDir, release, convertFunc);
	writeTimesFile(TRUE, scale);

	freeFileTimes();
	
	return result;
}

/* Reads through the charset file and builds a list of character/filenames.
 */
private void readCharList()
{
    char fName[MAXFILENAME+4];
	int i;
	boolean start = TRUE;
	short nextName = nextFree - nameBuf; /* Remember first name position */
	
	for (newFileCnt = 0; ReadCharFileNames(fName, &start); newFileCnt++)
		(void)storeName(fName);

	newFileTimes = (fileTime*)allocate(newFileCnt, sizeof(fileTime));

	for (i = 0; i < newFileCnt; i++) {
		newFileTimes[i].nameIndx = nextName;
		nextName += strlen(INDX_PTR(nextName)) + 1;
	}

	sortFileTimes(newFileCnt, newFileTimes);
}


/* Converts characters that have changed since the last run
 * of BuildFont from the input to the bez directory.
 */
private boolean compareCvtTimes(char *inputDir, boolean release, tConvertfunc convertFunc)
{
	boolean result = TRUE;
	fileTime* newp;
	fileTime* oldp;
	
	for (newp = newFileTimes; newp - newFileTimes < newFileCnt; newp++) {
		
		newp = checkFile(&oldp, newp, inputDir);
		if (!newp)
			break;
		else if (release) {
			sprintf(globmsg, "The %s character needs to be converted.\n  Convert before creating a release version.\n", INDX_PTR(newp->nameIndx));
			LogMsg(globmsg, LOGERROR, OK, TRUE);
			
			if (oldp)
				newp->modTime = oldp->modTime;
			
			result = FALSE;
		}
		else {
			(*convertFunc)(INDX_PTR(newp->nameIndx), INDX_PTR(newp->nameIndx));
			updateTime(newp, inputDir);
		}
	}
	return result;
}

