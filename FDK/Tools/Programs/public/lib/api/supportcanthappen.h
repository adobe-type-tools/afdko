/* @(#)CM_VerSion canthappen.h atm09 1.2 16563.eco sum= 51485 atm09.004 */
/* @(#)CM_VerSion canthappen.h atm08 1.6 16390.eco sum= 07013 atm08.007 */
/* @(#)CM_VerSion canthappen.h atm07 1.2 16009.eco sum= 26989 atm07.005 */
/* @(#)CM_VerSion canthappen.h atm06 1.7 14103.eco sum= 51592 */
/* $Header$ */
/*
  canthappen.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
  Charter -- Provide an abnormal terminiation facility, which can identify the
  source of the fatal error location by source file name and line number, and 
  which supports a programmer-generated termination message, incorporating
  formatted values of execution data.
*/
#ifndef CANTHAPPEN_H
#define CANTHAPPEN_H

#include "supportpublictypes.h"
#include "supportenvironment.h"

#ifndef WINATM
#define WINATM 0
#endif

#ifndef CDECL
#define CDECL
#endif

#ifndef os_abort
#if OS == os_sun || OS == os_mach
#define os_abort abort
#include "supportossyslib.h"
#elif OS == os_mac
#define os_abort() DebugStr("\pcanthappen")
#elif WINATM
#ifdef CODE32
#define os_abort() { char *n=NULL, m;                m = *n; }
#else
#define os_abort() { char *n=NULL, m; _asm{ int 3 }; m = *n; }
#endif
#else
#include <stdlib.h>
#define os_abort abort
#endif
#endif

/*----------------------------------------------------------------------*/
/*  USER INTERFACE -- Summary of Calls */
/*----------------------------------------------------------------------*/
/*                                CantHappen and Assert MACROS
				  ----------------------------
				  (For new development)

AssertMsg(cond,errID,msg);       Assert condition; if cond fail, quit w. msg.

DebugAssertMsg(cond,errID,msg);  Assert condition; if cond fail, quit w. msg.
                                  Note: DebugAssertMsg is compiled only for
				        DEVELOP stage. 

CantHappenMsg(errID,msg);               Quit with msg.
CantHappenMsg1(errID,format,v1)         Quit with msg and 1 formatted value.
CantHappenMsg2(errID,format,v1,v2)      Quit with msg and 2 formatted values.
CantHappenMsg3(errID,format,v1,v2,v3)   Quit with msg and 3 formatted values.

                                  CantHappen and Assert MACROS
				  ----------------------------
				  (For Backward Compatibility)

Assert(cond);                     Assert condition; if cond fails, quit.

DebugAssert(cond);                Assert condition; if cond fails, quit.
                                  Note: DebugAssert is compiled only for
				        DEVELOP stage. 
CantHappen();                     Quit


                                   examples:
				   ---------
*/

/*

				   Notes:
				   ------
1) CantHappen(), Assert(), DebugAssert() should not be used for new 
   development. Included only for backward compatibility.
2) errID is a 2-part error ID:
2a)  the most significant 8 bits forms the package ID number
     (see pkgid.h in util pkg for definitions) 
2b)  for a package listed in pkgid.h - the lower 24 bits forms the
     error ID within the package. For example, for the pslib pkg,
     see the psliberrid.h file in the pslib pkg sources.
2c)  for a package group listed in pkgid.h - the lower 24 bits forms the
     error ID within the package group. For example, for the PS Server
     package group, see the (for example) external interface
     PSSERVERERRID. 
2d)  for a product - the lower 24 bits forms the error ID within the
     product.  See the MUMBLEerrid.h file in the product sources,
     for example the refsyserrid.h file in the adobe/rxrefsys/sources.
3) msg is a C-language string, e.g. "Abort Message"
4) format is a format string legal for the C function printf, excluding
   floating point values.
5) string is buffer of sufficient size to hold formatted DebugSprintf value.
6) v1,v2,v3 are values to match format.
7) length of expanded formatted data must be less than 256 bytes.
8) For EXPORT stage, msg or formatting strings and formatted values are not
   compiled; they are stripped by the macro, to avoid taking up ROM space.
   Only the errID is compiled and used in the termination message.
*/

/*----------------------------------------------------------------------*/
/*  EXPORTED PROCEDURES -- Procedure called primarily by macros in this file */
/*----------------------------------------------------------------------*/
extern procedure CDECL CantHappenPrintf (
                  readonly char* pPgmFileName, 
		  IntX           pgmLineNum, 		 
		  Card32         errID,
		  readonly char* formatString, ...
		  /* 0-n vars, as per formatString .... */
		  ); 
/* 
  This is called by the CantHappenMsg1, CantHappenMsg2, CantHappenMsg3 macros, 
  when STAGE == DEVELOP.  It has the same syntax as the ANSI C printf
  statement, except that it always expects the filename and line number, and the
  errID as the first arguments.  The formatString describes Only the
  programmer-generated message and the formatting of the program
  variables which follow the formatString; it does Not deal with the
  filename and line number and errID, which are dealt with internal to the
  procedure.  
  Note:
  See "USER INTERFACE -- Summary of Calls" at start of file for errID descr.
  Note:
  No \n must appear in the message you code; this is added, together with other
  information, in the message generated by CantHappenPrintf.
  Example:
    CantHappenPrintf(__FILE__, __LINE__, PSLIB_MUMBLE_ERR,
                    "MyProc: Too many gorfs(%d)", numGorfs);
*/

/*----------------------------------------------------------------------*/
/*  EXPORTED PROCEDURES -- Procedures called only by macros in this file */
/*----------------------------------------------------------------------*/
extern procedure CantHappenForDevelop (
                  Card32         errID,
		  readonly char* pMessage,
		  readonly char* pPgmFileName,
		  IntX lineNum);
/* 
  This is called by the CantHappenMsg macro when STAGE == DEVELOP.  
  It is passed a programmer-generated errID and termination message, plus the
  filename and line number generated by the CantHappenMsg macro.  
  Note:
  See "USER INTERFACE -- Summary of Calls" at start of file for errID descr.
  Note:
  No \n need appear in the message you code; this is added, together with other
  information, in the message generated by CantHappenForDevelop.
  Example:
     CantHappenForDevelop(PSLIB_MUMBLE_ERR,
                         "MyProc: My abort msg", __FILE__,__LINE__);
*/

extern procedure CantHappenForExport (
		Card32 errID);

/* 
  This is called by the CantHappenMsg, CantHappenMsg1, CantHappenMsg2 and
  CantHappenMsg3 macros when STAGE == EXPORT. It is passed the programmer-
  generated errID.
p  Note:
  See "USER INTERFACE -- Summary of Calls" at start of file for errID descr.
  Example:
     CantHappenForExport(PSLIB_MUMBLE_ERR);
*/


#if (ANSI_C)
extern procedure AssertForANSIDevelop (
                 Card32         errID,
		 readonly char* pMessage,
		 readonly char* pPgmFileName,
 		 int            pgmLineNum,
		 readonly char* pAssertCond); 
/* 
  This is called by the Assert and DebugAssert macros, when STAGE == DEVELOP 
  and an ANSI compiler is used. It is passed a programmer-generated errID and
  termination message and Assert condition, plus the filename & line number
  generated by the Assert macro.
  Note:
  See "USER INTERFACE -- Summary of Calls" at start of file for errID descr.
  Note:
  No \n must appear in the message you code; this is added, together with other
  information, in the message generated by AssertForANSIDevelop.
*/
#else  /* (ANSI_C) */

extern procedure AssertForNonANSIDevelop (
                  Card32         errID,
		  readonly char* pMessage,
		  readonly char* pPgmFileName,
		  int            pgmLineNum);

/* 
  This is called by the Assert and DebugAssert macros, when STAGE == DEVELOP
  and a non-ANSI compiler is used. It is passed a programmer-generated errID and
  termination message, plus the filename & line number generated by the
  Assert macro.
  Note:
  See "USER INTERFACE -- Summary of Calls" at start of file for errID descr.
  Note:
  No \n must appear in the message you code; this is added, together with other
  information, in the message generated by AssertForNonANSIDevelop.
*/
#endif /* (ANSI_C) */

extern procedure AssertForExport (
                 Card32 errID);

/* 
  This is called by the Assert macro when STAGE == EXPORT, passing the
  programmer-generated errID.
*/

/*----------------------------------------------------------------------*/
/*  IN-LINE PROCEDURES -- for new CantHappenMsg, AssertMsg, DebugAssertMsg */
/*----------------------------------------------------------------------*/
				/* Use these for New Development */
/*-------------------------------------------------------------------------*/
				/* AssertMsg Macro */
#if (STAGE == DEVELOP)
#if (ANSI_C)
#define AssertMsg(cond,errID,msg) \
  ((void)((cond) || \
	   (AssertForANSIDevelop(errID,msg, __FILE__,__LINE__, #cond), 0) ))
 				/* ANSI DEVELOP AssertMsg */ 
#else /* not ANSI */ 
#define AssertMsg(cond,errID,msg) \
  ((void)((cond) || \
	   (AssertForNonANSIDevelop(errID,msg, __FILE__, __LINE__), 0) ))
				/* non_ANSI DEVELOP AssertMsg */ 
#endif /* (ANSI_C) */ 
#else /* (STAGE == DEVELOP) */
#define AssertMsg(cond,errID,msg) \
  ((void)((cond) || \
	   (AssertForExport(errID), 0) )) /* EXPORT AssertMsg */
#endif /* (STAGE == DEVELOP) */


/*-------------------------------------------------------------------------*/
				/* DebugAssertMsg Macro */
#if (STAGE == DEVELOP)
#if (ANSI_C)
#define DebugAssertMsg(cond,errID,msg) \
  ((void)((cond) || \
	   (AssertForANSIDevelop(errID,msg, __FILE__, __LINE__, #cond), 0) ))
				/* ANSI DEVELOP DebugAssertMsg */ 
#else /* not ANSI */ 
#define DebugAssertMsg(cond,errID,msg) \
  ((void)((cond) || \
	   (AssertForNonANSIDevelop(errID,msg, __FILE__,__LINE__), 0) ))
 				/* non_ANSI DEVELOP DebugAssertMsg */ 
#endif /* (ANSI_C) */ 
#else /* (STAGE == DEVELOP) */ 
#define DebugAssertMsg(cond,errID,msg)	/* No-op if not DEVELOP stage */ 
#endif /* (STAGE == DEVELOP) */
/*-------------------------------------------------------------------------*/
				/* CantHappenMsg Macro */

#if (STAGE == DEVELOP) 
#define CantHappenMsg(errID,msg) \
  CantHappenForDevelop(errID,msg, __FILE__,__LINE__)
				/* DEVELOP Cant Happen */
#else /* (STAGE == DEVELOP) */ 
#define CantHappenMsg(errID,msg) \
  CantHappenForExport(errID)	/* EXPORT Cant Happen */ 
#endif /* (STAGE == DEVELOP) */

/*-------------------------------------------------------------------------*/
				/* CantHappenMsg1 Macro */ 
#if (STAGE == DEVELOP) 
#define CantHappenMsg1(errID,format,v1) \
  CantHappenPrintf(__FILE__, __LINE__, errID, format, v1)
				/* DEVELOP Cant Happen */ 
#else /* (STAGE == DEVELOP) */ 
#define CantHappenMsg1(errID,format,v1) \
  CantHappenForExport(errID)	/* EXPORT Cant Happen */ 
#endif /* (STAGE == DEVELOP) */
/*-------------------------------------------------------------------------*/
				/* CantHappenMsg2 Macro */ 
#if (STAGE == DEVELOP) 
#define CantHappenMsg2(errID,format,v1,v2) \
  CantHappenPrintf(__FILE__, __LINE__, errID, format, v1,v2)
				/* DEVELOP Cant Happen */ 
#else /* (STAGE == DEVELOP) */ 
#define CantHappenMsg2(errID,format,v1,v2) \
  CantHappenForExport(errID)	/* EXPORT Cant Happen */ 
#endif /* (STAGE == DEVELOP) */
/*-------------------------------------------------------------------------*/
				/* CantHappenMsg3 Macro */ 
#if (STAGE == DEVELOP) 
#define CantHappenMsg3(errID,format,v1,v2,v3) \
  CantHappenPrintf(__FILE__, __LINE__, errID, format, v1,v2,v3)
				/* DEVELOP Cant Happen */ 
#else /* (STAGE == DEVELOP) */ 
#define CantHappenMsg3(errID,format,v1,v2,v3) \
  CantHappenForExport(errID)	/* EXPORT Cant Happen */ 
#endif /* (STAGE == DEVELOP) */
/*-------------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/*  IN-LINE PROCEDURES -- for old CantHappen, Assert, DebugAssert */
/*----------------------------------------------------------------------*/
				/* For Backward Compatibility;
				   These calls are planned to be eliminated,
				   after the conversion to the new facilities
				   (which support messages, error numbers and
				   which provide source filename and line
				   numbers) are in full use. */
/*-------------------------------------------------------------------------*/
				/* CantHappen() */
#if (STAGE == DEVELOP) 
#define CantHappen() \
  CantHappenForDevelop((Card32)0,NULL, __FILE__,__LINE__)
				/* DEVELOP Cant Happen */
#else /* (STAGE == DEVELOP) */ 
#define CantHappen() \
  CantHappenForExport((Card32)0)	/* EXPORT Cant Happen */ 
#endif /* (STAGE == DEVELOP) */
/*-------------------------------------------------------------------------*/
				/* Assert(cond) */
#if (STAGE == DEVELOP)
#if (ANSI_C)
#define Assert(cond) \
  ((void)((cond) || \
	   (AssertForANSIDevelop((Card32)0,NULL, __FILE__,__LINE__, #cond), 0) ))
 				/* ANSI DEVELOP Assert */ 
#else /* not ANSI */ 
#define Assert(cond) \
  ((void)((cond) || \
	   (AssertForNonANSIDevelop((Card32)0,NULL, __FILE__, __LINE__), 0) ))
				/* non_ANSI DEVELOP Assert */ 
#endif /* (ANSI_C) */ 
#else /* (STAGE == DEVELOP) */
#define Assert(cond) \
  ((void)((cond) || \
	   (CantHappenForExport((Card32)0), 0) )) /* EXPORT Assert */
#endif /* (STAGE == DEVELOP) */
/*-------------------------------------------------------------------------*/
				/* DebugAssert(cond) */
#if (STAGE == DEVELOP)
#if (ANSI_C)
#define DebugAssert(cond) \
  ((void)((cond) || \
	   (AssertForANSIDevelop((Card32)0,NULL, __FILE__, __LINE__, #cond), 0) ))
				/* ANSI DEVELOP DebugAssert */ 
#else /* not ANSI */ 
#define DebugAssert(cond) \
  ((void)((cond) || \
	   (AssertForNonANSIDevelop((Card32)0,NULL, __FILE__,__LINE__), 0) ))
 				/* non_ANSI DEVELOP DebugAssert */ 
#endif /* (ANSI_C) */ 
#else /* (STAGE == DEVELOP) */ 
#define DebugAssert(cond)	/* No-op if not DEVELOP stage */ 
#endif /* (STAGE == DEVELOP) */
/*-------------------------------------------------------------------------*/


#endif  /* CANTHAPPEN_H */




