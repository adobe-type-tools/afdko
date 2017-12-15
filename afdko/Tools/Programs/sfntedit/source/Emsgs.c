/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)Emsgs.c	1.1
 * Changed:    2/12/99 13:36:16
 ***********************************************************************/

#include "Emsgs.h"

const char *EnglishMessages[] =
{
"",
"file error <premature EOF> [%s]\n",/* SFED_MSG_EARLYEOF */
"bad input object size [%d]\n", 	/* SFED_MSG_BADREADSIZE */
"bad output object size [%d]\n", 	/* SFED_MSG_BADWRITESIZE */
"out of memory\n", 					/* SFED_MSG_NOMOREMEM */
"bad file [%s] (ignored)\n", 		/* SFED_MSG_BADFILE */
".", 								/* SFED_MSG_PROGRESS */
"I/O Error [%s]\n", /* SFED_MSG_sysIOERROR */
"EOF Error [%s]\n", /* SFED_MSG_sysEOFERROR */
"No such file? [%s]\n", /* SFED_MSG_sysNOSUCHF */
"file error (%d) [%s]\n", /* SFED_MSG_sysMACFERROR */
"file error <%s> [%s]\n", /* SFED_MSG_sysFERRORSTR */
"Empty script file? [%s]\n", /* SFED_MSG_BADSCRIPTFILE */
"Executing command-line: ", /* SFED_MSG_ECHOSCRIPTCMD */
"%s ", /* SFED_MSG_RAWSTRING */
"\n", /* SFED_MSG_EOLN */
"Please select OpenType file to use.\n", /* SFED_MSG_MACSELECTOTF */
"Please select script-file to execute.\n", /* SFED_MSG_MACSELECTSCRIPT */
"Missing/unrecognized font filename on commandline\n", /* SFED_MSG_MISSINGFILENAME */
"table limit (%d) exceeded\n", /* SFED_MSG_TABLELIMIT */
"bad filename (-%c)\n", /* SFED_MSG_BADFNAMOPT */
"bad tag '%s' (-%c)\n", /* SFED_MSG_BADTAGOPT */
"filename specified (-d) (ignored)\n", /* SFED_MSG_FILENIGNORED */
"filename missing (-a)\n", /* SFED_MSG_FILENMISSING */
"duplicate tag '%c%c%c%c' (-%c)\n", /* SFED_MSG_DUPTAGOPT */
"bad option (%s)\n", /* SFED_MSG_BADOPTION */
"no arg for option: -%c\n", /* SFED_MSG_NOARG */
"unrecognized option (%s)\n", /* SFED_MSG_UNRECOGOPT */
"option conflict\n", /* SFED_MSG_OPTCONFLICT */
"output file provided for non-edit option (ignored)\n", /* SFED_MSG_OUTFIGNORED */
"too many files provided\n", /* SFED_MSG_TOOMANYFILES */
"unrecognized/unsupported file type [%s]\n", /* SFED_MSG_UNRECFILE */
"table missing (%c%c%c%c)\n", /* SFED_MSG_TABLEMISSING */
"can't remove <%s> [%s]\n", /* SFED_MSG_REMOVEERR */
"file rename error <%s> [%s]\n", /* SFED_MSG_BADRENAME */
"temporary file [%s] already exists\n", /* SFED_MSG_TEMPFEXISTS */
"bad sfnt.searchRange: file=%hu, calc=%hu\n", /* SFED_MSG_BADSEARCH */
"bad sfnt.entrySelector: file=%hu, calc=%hu\n", /* SFED_MSG_BADSELECT */
"bad sfnt.rangeShift: file=%hu, calc=%hu\n", /* SFED_MSG_BADRSHIFT */
"'%c%c%c%c' bad checksum: file=%08lx, calc=%08lx\n", /* SFED_MSG_BADCHECKSUM */
"bad head.checkSumAdjustment: file=%08lx, calc=%08lx\n", /* SFED_MSG_BADCKSUMADJ */
"check failed [%s]\n", /* SFED_MSG_CHECKFAILED */
"check passed [%s]\n", /* SFED_MSG_CHECKPASSED */
"Done.\n", /* SFED_MSG_DONE */
};


const char *sfntedMsg(int msgId) 
{
	if ((msgId <= 0) || (msgId > SFED_MSG_ENDSENTINEL))
		return (0);
	return (EnglishMessages[msgId]);
}
