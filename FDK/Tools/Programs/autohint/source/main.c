/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/* main.c */

#include <stdlib.h>
#include "ac.h"
#include "machinedep.h"

#ifdef WIN32
#include <sys\types.h>
#include <sys\stat.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

const char *C_ProgramVersion = "1.56040";
const char *reportExt = ".rpt";
const char *dfltExt = ".new";
char *bezName = NULL;
char *fileSuffix = NULL;
FILE *reportFile = NULL;

boolean verbose = TRUE; /* if TRUE don't number of characters processed. */

boolean scalinghints = FALSE;

/* EXECUTABLE defined means it is being compiled as a command-line tool;
if not defined, it is being compiled as a Python module.
*/
#ifndef EXECUTABLE
extern jmp_buf aclibmark;
#endif

#if ALLOWCSOUTPUT
boolean charstringoutput = FALSE;
static void read_char_widths(void);
#endif

#if !WIN32
extern int unlink(const char *);
#endif

static void openReportFile(char *name, char *fSuffix);

static void printVersions(void)
{
	fprintf(OUTPUTBUFF, "C program version %s. lib version %s.\n", C_ProgramVersion, AC_getVersion());
}

static void printUsage(void)
{
	fprintf(OUTPUTBUFF, "Usage: autohintexe [-u] [-h]\n");
	fprintf(OUTPUTBUFF, "       autohintexe  -f <font info name> [-e] [-n] [-q] [-s <suffix>] [-ra] [-rs] -a] [<file1> <file2> ... <filen>]\n");
	printVersions();
}

static void printHelp(void)
{
	printUsage();
	fprintf(OUTPUTBUFF, "   -u usage\n");
	fprintf(OUTPUTBUFF, "   -h help message\n");
	fprintf(OUTPUTBUFF, "   -e do not edit (change) the paths when hinting\n");
	fprintf(OUTPUTBUFF, "   -n no multiple layers of coloring\n");
	fprintf(OUTPUTBUFF, "   -q quiet\n");
	fprintf(OUTPUTBUFF, "   -f <name> path to font info file\n");
	fprintf(OUTPUTBUFF, "   <name1> [name2]..[nameN]  paths to glyph bez files\n");
	fprintf(OUTPUTBUFF, "   -s <suffix> Write output data to 'file name' + 'suffix', rather\n");
	fprintf(OUTPUTBUFF, "       than writing it to the same file name as the input file.\n");
	fprintf(OUTPUTBUFF, "   -ra Write alignment zones data. Does not hint or change glyph. Default extension is '.rpt'\n");
	fprintf(OUTPUTBUFF, "   -rs Write stem widths data. Does not hint or change glyph. Default extension is '.rpt'\n");
	fprintf(OUTPUTBUFF, "   -a Modifies -ra and -rs: Includes stems between curved lines: default is to omit these.\n");
	fprintf(OUTPUTBUFF, "    -v print versions.\n");
}

static int main_cleanup(short code)
{
	closefiles();
	if (code != AC_Success)
		exit(code);

	return 0;
}

static void charZoneCB(Fixed top, Fixed bottom, char *glyphName)
{
	if (reportFile)
		fprintf(reportFile, "charZone %s top %f bottom %f\n", glyphName, top / 256.0, bottom / 256.0);
}

static void stemZoneCB(Fixed top, Fixed bottom, char *glyphName)
{
	if (reportFile)
		fprintf(reportFile, "stemZone %s top %f bottom %f\n", glyphName, top / 256.0, bottom / 256.0);
}

static void hstemCB(Fixed top, Fixed bottom, char *glyphName)
{
	if (reportFile)
		fprintf(reportFile, "HStem %s top %f bottom %f\n", glyphName, top / 256.0, bottom / 256.0);
}

static void vstemCB(Fixed right, Fixed left, char *glyphName)
{
	if (reportFile)
		fprintf(reportFile, "VStem %s right %f left %f\n", glyphName, right / 256.0, left / 256.0);
}

static void reportCB(char *msg)
{
	fprintf(OUTPUTBUFF, "%s", msg);
	fprintf(OUTPUTBUFF, "\n");
}

static void reportRetry(void)
{
	if (reportFile != NULL) {
		fclose(reportFile);
		openReportFile(bezName, fileSuffix);
	}
}

static char *getFileData(char *name)
{
	char *data;

	struct stat filestat;
	if ((stat(name, &filestat)) < 0) {
		fprintf(OUTPUTBUFF, "Error. Could not open file '%s'. Please check that it exists and is not write-protected.\n", name);
		main_cleanup(FATALERROR);
	}

	if (filestat.st_size == 0) {
		fprintf(OUTPUTBUFF, "Error. File '%s' has zero size.\n", name);
		main_cleanup(FATALERROR);
	}

	data = malloc(filestat.st_size + 1);
	if (data == NULL) {
		fprintf(OUTPUTBUFF, "Error. Could not allcoate memory for contents of file %s.\n", name);
		main_cleanup(FATALERROR);
	}
	else {
		size_t fileSize = 0;
		FILE *fp = fopen(name, "r");
		if (fp == NULL) {
			fprintf(OUTPUTBUFF, "Error. Could not open file '%s'. Please check that it exists and is not write-protected.\n", name);
			main_cleanup(FATALERROR);
		}
		fileSize = fread(data, 1, filestat.st_size, fp);
		data[fileSize] = 0;
		fclose(fp);
	}
	return data;
}

static void writeFileData(char *name, char *output, char *fSuffix)
{
	size_t fileSize;
	FILE *fp;
	size_t nameSize = 1 + strlen(name);
	char *savedName;
	int usedNewName = 0;

	if ((fSuffix != NULL) && (fSuffix[0] != '\0')) {
		nameSize += strlen(fSuffix);
		savedName = malloc(nameSize);
		savedName[0] = '\0';
		strcat(savedName, name);
		strcat(savedName, fSuffix);
		usedNewName = 1;
	}
	else
		savedName = malloc(nameSize);

	fp = fopen(savedName, "w");
	fileSize = fwrite(output, 1, strlen(output), fp);
	fclose(fp);

	if (usedNewName)
		free(savedName);
}

static void openReportFile(char *name, char *fSuffix)
{
	size_t nameSize = 1 + strlen(name);
	char *savedName;
	int usedNewName = 0;

	if ((fSuffix != NULL) && (fSuffix[0] != '\0')) {
		nameSize += strlen(fSuffix);
		savedName = malloc(nameSize);
		savedName[0] = '\0';
		strcat(savedName, name);
		strcat(savedName, fSuffix);
		usedNewName = 1;
	}
	else
		savedName = malloc(nameSize);

	reportFile = fopen(savedName, "w");

	if (usedNewName)
		free(savedName);
}

static void closeReportFile(void)
{
	if (reportFile != NULL)
		fclose(reportFile);
}

int main(int argc, char *argv[])
{
	int allowEdit, allowHintSub, fixStems, debug, badParam;
	char *
		fontInfoFileName = NULL; /* font info file name, or suffix of environment variable holding
									the fontfino string. */
	char *fontinfo = NULL;		 /* the string of fontinfo data */
	int firstFileNameIndex = -1; /* arg index for first bez file name, or  suffix of environment
									variable holding the bez string. */
	register char *current_arg;
	short total_files = 0;
	int result, argi;

	badParam = fixStems = debug = doAligns = doStems = allstems = FALSE;
	allowEdit = allowHintSub = TRUE;
	fileSuffix = (char *)dfltExt;

	/* read in options */
	argi = 0;
	while (++argi < argc) {
		current_arg = argv[argi];

		if (current_arg[0] == '\0') {
			continue;
		}
		else if (current_arg[0] != '-') {
			if (firstFileNameIndex == -1) {
				firstFileNameIndex = argi;
			}
			total_files++;
			continue;
		}
		else if (firstFileNameIndex != -1) {
			fprintf(OUTPUTBUFF, "Error. Illegal command line. \"-\" option found after first file name.\n");
			exit(1);
		}

		switch (current_arg[1]) {
			case '\0':
				badParam = TRUE;
				break;
			case 'u':
				printUsage();
#ifdef EXECUTABLE
				exit(0);
#else
				longjmp(aclibmark, 1);
#endif
				break;
			case 'h':
				printHelp();
#ifdef EXECUTABLE
				exit(0);
#else
				longjmp(aclibmark, 1);
#endif
				break;
			case 'e':
				allowEdit = FALSE;
				break;
			case 'f':
				fontInfoFileName = argv[++argi];
				if ((fontInfoFileName[0] == '\0') || (fontInfoFileName[0] == '-')) {
					fprintf(OUTPUTBUFF, "Error. Illegal command line. \"-f\" option must be followed by a file name.\n");
					exit(1);
				}
				break;
			case 's':
				fileSuffix = argv[++argi];
				if ((fileSuffix[0] == '\0') || (fileSuffix[0] == '-')) {
					fprintf(OUTPUTBUFF, "Error. Illegal command line. \"-s\" option must be followed by a string, and the string must not begin with '-'.\n");
					exit(1);
				}
				break;
			case 'n':
				allowHintSub = FALSE;
				break;
			case 'q':
				verbose = FALSE;
				break;
			case 'D':
				debug = TRUE;
				break;
			case 'F':
				fixStems = TRUE;
				break;
			case 'a':
				allstems = 1;
				break;

			case 'r':
				allowEdit = allowHintSub = FALSE;
				fileSuffix = (char *)reportExt;
				switch (current_arg[2]) {
					case 'a':
						reportRetryCB = reportRetry;
						addCharExtremesCB = charZoneCB;
						addStemExtremesCB = stemZoneCB;
						doAligns = 1;

						addHStemCB = NULL;
						addVStemCB = NULL;
						doStems = 0;
						break;
					case 's':
						reportRetryCB = reportRetry;
						addHStemCB = hstemCB;
						addVStemCB = vstemCB;
						doStems = 1;

						addCharExtremesCB = NULL;
						addStemExtremesCB = NULL;
						doAligns = 0;
						break;
					default:
						fprintf(OUTPUTBUFF, "Error. %s is an invalid parameter.\n", current_arg);
						badParam = TRUE;
						break;
				}
				break;
			case 'v':
				printVersions();
				exit(0);
				break;
				break;
#if ALLOWCSOUTPUT
			case 'C':
				charstringoutput = TRUE;
				break;
#endif
			default:
				fprintf(OUTPUTBUFF, "Error. %s is an invalid parameter.\n", current_arg);
				badParam = TRUE;
				break;
		}
	}

	if (firstFileNameIndex == -1) {
		fprintf(OUTPUTBUFF, "Error. Illegal command line. Must provide bez file name.\n");
		badParam = TRUE;
	}
	if (fontInfoFileName == NULL) {
		fprintf(OUTPUTBUFF, "Error. Illegal command line. Must provide font info file name.\n");
		badParam = TRUE;
	}

	if (badParam)
#ifdef EXECUTABLE
		exit(NONFATALERROR);
#else
		longjmp(aclibmark, -1);
#endif

#if ALLOWCSOUTPUT
	if (charstringoutput)
		read_char_widths();
#endif

	AC_SetReportCB(reportCB, verbose);
	fontinfo = getFileData(fontInfoFileName);
	argi = firstFileNameIndex - 1;
	while (++argi < argc) {
		char *bezdata;
		char *output;
		size_t outputsize = 0;
		bezName = argv[argi];
		bezdata = getFileData(bezName);
		outputsize = 4 * strlen(bezdata);
		output = malloc(outputsize);

		if (doAligns || doStems)
			openReportFile(bezName, fileSuffix);

		result = AutoColorString(bezdata, fontinfo, output, (int *)&outputsize, allowEdit,
								 allowHintSub, debug);
		if (result == AC_DestBuffOfloError) {
			free(output);
			if (reportFile != NULL) {
				closeReportFile();
			}
			if (doAligns || doStems) {
				openReportFile(bezName, fileSuffix);
			}
			output = malloc(outputsize);
			/* printf("NOTE: trying again. Input size %d output size %d.\n", strlen(bezdata),
			 * outputsize); */
			AC_SetReportCB(reportCB, FALSE);
			result = AutoColorString(bezdata, fontinfo, output, (int *)&outputsize, allowEdit, allowHintSub, debug);
			AC_SetReportCB(reportCB, verbose);
		}
		if (reportFile != NULL) {
			closeReportFile();
		}
		else {
			if ((outputsize != 0) && (result == AC_Success)) {
				writeFileData(bezName, output, fileSuffix);
			}
		}
		free(output);
		main_cleanup((result == AC_Success) ? OK : FATALERROR);
	}

	return 0;
}
/* end of main */

#if ALLOWCSOUTPUT

typedef struct _wid {
	int wid;
	char nam[60];
} WidthElt;

static WidthElt *widthsarray = NULL;
static int numwidths = 0;
static int maxnumwidths = 0;
#define WIDTHINCR 50
#define STREQ(a, b) (((a)[0] == (b)[0]) && (strcmp((a), (b)) == 0))

static void init_widthsarray(void)
{
	if (widthsarray != NULL) {
#if DOMEMCHECK
		memck_free(widthsarray);
#else
		ACFREEMEM(widthsarray);
#endif
		widthsarray = NULL;
	}
	widthsarray = (WidthElt *)calloc(WIDTHINCR + 1, sizeof(WidthElt));
	if (widthsarray == NULL) {
		LogMsg("Could not allocate widths.\n", LOGERROR, OK, FALSE);
	}
	numwidths = 0;
	maxnumwidths = WIDTHINCR;
}

static void set_char_width(char *cname, int width)
{
	if (numwidths == maxnumwidths) {
		widthsarray = (WidthElt *)realloc(widthsarray, (maxnumwidths + WIDTHINCR + 1) * sizeof(WidthElt));
		if (widthsarray == NULL) {
			LogMsg("Could not re-allocate widths.\n", LOGERROR, OK, FALSE);
		}
		maxnumwidths += WIDTHINCR;
	}

	widthsarray[numwidths].wid = width;
	strcpy(widthsarray[numwidths].nam, cname);
	numwidths++;
}

int get_char_width(char *cname)
{
	int i;
	for (i = 0; i < numwidths; i++) {
		if (STREQ(widthsarray[i].nam, cname)) {
			return (widthsarray[i].wid);
		}
	}
	sprintf(globmsg, "Could not find width for '%s'. Assuming 1000.\n", cname);
	LogMsg(globmsg, INFO, OK, FALSE);
	return (1000);
}

/* Adds width info to char_table. */
static void read_char_widths(void)
{
	FILE *fwidth;
#define WIDTHMAXLINE 80
	char cname[50];
	char line[WIDTHMAXLINE + 1];
	int width;
	indx ix;

	init_widthsarray();
	fwidth = ACOpenFile("widths.ps", "r", OPENERROR);
	while (fgets(line, WIDTHMAXLINE, fwidth) != NULL) {
		if (((line[0] == '%') || (ix = strindex(line, "/")) == -1)) {
			continue;
		}
		if (sscanf(&line[ix + 1], " %s %d", cname, &width) != 2) {
			sprintf(globmsg, "%s file line: %s can't be parsed.\n  It should have the format: "
							 "/<char name> <width> WDef\n",
					"widths.ps", line);
			fclose(fwidth);
			LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
		}
		set_char_width(cname, width);
	}
	fclose(fwidth);
}
#endif /* ALLOWCSOUTPUT */
