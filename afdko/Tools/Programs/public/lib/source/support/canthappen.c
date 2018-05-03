/* @(#)CM_VerSion canthappen.c atm09 1.2 16563.eco sum= 25698 atm09.004 */
/* @(#)CM_VerSion canthappen.c atm08 1.2 16248.eco sum= 10775 atm08.003 */
/* @(#)CM_VerSion canthappen.c atm06 1.2 13928.eco sum= 34659 */
/* $Header$ */
/*
  canthappen.c
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#include "supportenvironment.h"
#include "supportexcept.h"
#include "supportcanthappen.h"
#if STAGE == DEVELOP
#include <stdio.h>
#endif


#if STAGE == DEVELOP
/* both are used because support is compiled
   nonANSI for _asm fixmul & fixdiv and ANSI
   is used wherever possible */
PUBLIC procedure AssertForANSIDevelop 
                (Card32          errID,
		 readonly char*  pMessage,
		 readonly char*  pPgmFileName,
 		 IntX            pgmLineNum,
		 readonly char*  pAssertCond) 
{
   
  fprintf(stderr, "ErrID: %u; %s %s L:%d (%s)\n", errID, pMessage, pPgmFileName, pgmLineNum, pAssertCond);
  os_abort();

} 

PUBLIC procedure AssertForNonANSIDevelop 
                 (Card32          errID,
		  readonly char*  pMessage,
		  readonly char*  pPgmFileName,
		  IntX             pgmLineNum)

{
  fprintf(stderr, "ErrID: %u; %s %s L:%d \n", errID, pMessage, pPgmFileName, pgmLineNum);
  os_abort();
} 


#else /* STAGE == DEVELOP */
PUBLIC procedure AssertForExport (Card32  errID)
{
  os_abort();
}

#endif

PUBLIC procedure CantHappenForDevelop 
                 (Card32          errID,
		  readonly char*  pMessage,
		  readonly char*  pPgmFileName,
		  IntX  lineNum)
{
  fprintf(stderr, "ErrID: %u; %s %s L:%d \n", errID, pMessage, pPgmFileName, lineNum);
  os_abort();
}

PUBLIC procedure CantHappenForExport 
                 (Card32  errID)

{
  os_abort();
}
