/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)spotmsgs.h	1.5
 * Changed:    5/19/99 11:20:13
 ***********************************************************************/
#ifndef	SPOTMSGS_H
#define	SPOTMSGS_H
#include "numtypes.h"

extern const Byte8 *spotMsg(IntX msgId);

/* Message IDs */
#define SPOT_MSG_BASEUNKCOORD 		1
#define SPOT_MSG_CFFPARSING   		2
#define SPOT_MSG_GIDTOOLARGE  		3
#define SPOT_MSG_BOPTION	  		4
#define SPOT_MSG_ENCOUNKENCOD 		5
#define SPOT_MSG_GPOSUNKSINGL 		6
#define SPOT_MSG_GPOSUNKPAIR  		7
#define SPOT_MSG_GPOSUNKANCH  		8
#define SPOT_MSG_GPOSUNKMARKB 		9
#define SPOT_MSG_GPOSUNSRLOOK  		10
#define SPOT_MSG_GPOSUNKRLOOK  		11
#define SPOT_MSG_GPOSNULFEAT  		12
#define SPOT_MSG_GPOSUNSDLOOK 		13
#define SPOT_MSG_GPOSUNKDLOOK 		14
#define SPOT_MSG_GSUBUNKSINGL 		15
#define SPOT_MSG_GSUBUNKMULT   		16
#define SPOT_MSG_GSUBUNKALT  		17
#define SPOT_MSG_GSUBUNKLIG   		18
#define SPOT_MSG_GSUBUNKCTX 		19
#define SPOT_MSG_GSUBUNKCCTX  		20
#define SPOT_MSG_GSUBUNKRLOOK  		21
#define SPOT_MSG_GSUBCTXDEF    		22
#define SPOT_MSG_GSUBCTXCNT    		23
#define SPOT_MSG_GSUBCTXNYI   		24
#define SPOT_MSG_GSUBCCTXCNT  		25
#define SPOT_MSG_GSUBINPUTCNT  		26
#define SPOT_MSG_GSUBNULFEAT  		27
#define SPOT_MSG_GSUBEVALCNT  		28
#define SPOT_MSG_GSUBNOCOVG   		29
#define SPOT_MSG_GSUBEUNKLOOK 		30
#define SPOT_MSG_GSUBESUBCNT  		31
#define SPOT_MSG_cmapBADMSFMT  		32
#define SPOT_MSG_EARLYEOF  			33
#define SPOT_MSG_BADREADSIZE  		34
#define SPOT_MSG_TABLEMISSING  		35
#define SPOT_MSG_NOMOREMEM  		36
#define SPOT_MSG_UNKNGLYPHS  		37
#define SPOT_MSG_glyfBADLOCA  		38
#define SPOT_MSG_glyfMAXCMP   		39
#define SPOT_MSG_glyfUNSCOMP  		40
#define SPOT_MSG_glyfUNSDCOMP 		41
#define SPOT_MSG_glyfUNSCANCH 		42
#define SPOT_MSG_glyfLSBXMIN  		43
#define SPOT_MSG_kernUNSTAB 		44
#define SPOT_MSG_locaBADFMT  		45
#define SPOT_MSG_BADFILE   			46
#define SPOT_MSG_pathNOPELT 		47
#define SPOT_MSG_pathNOSUBP 		48
#define SPOT_MSG_pathMTMT  			49
#define SPOT_MSG_pathNOOSA  		50
#define SPOT_MSG_postNONGLYPH		51
#define SPOT_MSG_postBADVERS  		52
#define SPOT_MSG_prufSTR2BIG  		53
#define SPOT_MSG_prufWRTFAIL  		54
#define SPOT_MSG_prufPROGRESS 		55
#define SPOT_MSG_prufNUMPAGES 		56
#define SPOT_MSG_prufOFILNAME 		57
#define SPOT_MSG_prufPLSCLEAN 		58
#define SPOT_MSG_prufSPOOLTO  		59
#define SPOT_MSG_prufNOOPENF  		60
#define SPOT_MSG_prufPREPPS   		61
#define SPOT_MSG_prufCANTPS  		62
#define SPOT_MSG_prufNOBUFMEM 		63
#define SPOT_MSG_prufNOSHAPES 		64
#define SPOT_MSG_prufNOTPSDEV 		65
#define SPOT_MSG_NOSUCHSFNT   		66
#define SPOT_MSG_BADTAGSTR  		67
#define SPOT_MSG_BADFEATSTR  		68
#define SPOT_MSG_sysIOERROR  		69
#define SPOT_MSG_sysEOFERROR 		70
#define SPOT_MSG_sysNOSUCHF  		71
#define SPOT_MSG_sysMACFERROR 		72
#define SPOT_MSG_sysFERRORSTR 		73
#define SPOT_MSG_BADSCRIPTFILE      74
#define SPOT_MSG_ECHOSCRIPTFILE     75
#define SPOT_MSG_ECHOSCRIPTCMD      76
#define SPOT_MSG_RAWSTRING			77
#define SPOT_MSG_EOLN				78
#define SPOT_MSG_MACSELECTOTF		79
#define SPOT_MSG_MACSELECTSCRIPT	80
#define SPOT_MSG_MISSINGFILENAME	81
#define SPOT_MSG_TABLETOOBIG		82
#define SPOT_MSG_BADUNKCOVERAGE		83
#define SPOT_MSG_BADUNKCLASS		84
#define SPOT_MSG_BADINDEX           85
#define SPOT_MSG_cmapBADTBL  		86
#define SPOT_MSG_DONE      			87
#define SPOT_MSG_GPOS_DUPKRN      	88
#define SPOT_MSG_FILEFERR      		89
#define SPOT_MSG_GPOSUNKCTX      	90
#define SPOT_MSG_GPOSUNKCCTX      	91
#define SPOT_MSG_GPOSCTXDEF      	92
#define SPOT_MSG_GPOSCTXCNT      	93
#define SPOT_MSG_GPOSCTXNYI      	94
#define SPOT_MSG_GPOSCCTXCNT      	95
#define SPOT_MSG_GPOSUFMTCTX		96
#define SPOT_MSG_GPOSUFMTCCTX		97
#define SPOT_MSG_GPOSUFMTCCTX3		98
#define SPOT_MSG_GPOSUFMTDEV		99
#define SPOT_MSG_GSUBUFMTCTX		100
#define SPOT_MSG_GSUBUFMTCCTX		101
#define SPOT_MSG_CNTX_RECURSION		102
#define SPOT_MSG_DUP_IN_COV	103
#define SPOT_MSG_GSUBMULTIPLEINPUTS	104




#define SPOT_MSG_ENDSENTINEL 		SPOT_MSG_GSUBMULTIPLEINPUTS
#endif
