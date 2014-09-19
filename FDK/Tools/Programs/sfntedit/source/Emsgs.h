/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)Emsgs.h	1.1
 * Changed:    2/12/99 13:36:16
 ***********************************************************************/
#ifndef	EMSGS_H
#define	EMSGS_H

extern const char *sfntedMsg(int msgId);

/* Message IDs */
#define SFED_MSG_EARLYEOF  			1
#define SFED_MSG_BADREADSIZE  		2
#define SFED_MSG_BADWRITESIZE  		3
#define SFED_MSG_NOMOREMEM  		4
#define SFED_MSG_BADFILE   			5
#define SFED_MSG_PROGRESS 			6
#define SFED_MSG_sysIOERROR  		7
#define SFED_MSG_sysEOFERROR 		8
#define SFED_MSG_sysNOSUCHF  		9
#define SFED_MSG_sysMACFERROR 		10
#define SFED_MSG_sysFERRORSTR 		11
#define SFED_MSG_BADSCRIPTFILE      12
#define SFED_MSG_ECHOSCRIPTCMD      13
#define SFED_MSG_RAWSTRING			14
#define SFED_MSG_EOLN				15
#define SFED_MSG_MACSELECTOTF		16
#define SFED_MSG_MACSELECTSCRIPT	17
#define SFED_MSG_MISSINGFILENAME	18
#define SFED_MSG_TABLELIMIT			19
#define SFED_MSG_BADFNAMOPT			20
#define SFED_MSG_BADTAGOPT			21
#define SFED_MSG_FILENIGNORED		22
#define SFED_MSG_FILENMISSING		23
#define SFED_MSG_DUPTAGOPT			24
#define SFED_MSG_BADOPTION			25
#define SFED_MSG_NOARG				26
#define SFED_MSG_UNRECOGOPT			27
#define SFED_MSG_OPTCONFLICT		28
#define SFED_MSG_OUTFIGNORED		29
#define SFED_MSG_TOOMANYFILES		30
#define SFED_MSG_UNRECFILE			31
#define SFED_MSG_TABLEMISSING		32
#define SFED_MSG_REMOVEERR			33
#define SFED_MSG_BADRENAME			34
#define SFED_MSG_TEMPFEXISTS		35
#define SFED_MSG_BADSEARCH			36
#define SFED_MSG_BADSELECT			37
#define SFED_MSG_BADRSHIFT			38
#define SFED_MSG_BADCHECKSUM		39
#define SFED_MSG_BADCKSUMADJ		40
#define SFED_MSG_CHECKFAILED		41
#define SFED_MSG_CHECKPASSED		42
#define SFED_MSG_DONE      			43

#define SFED_MSG_ENDSENTINEL 		SFED_MSG_DONE
#endif
