/* @(#)CM_VerSion except.c atm09 1.2 16563.eco sum= 27085 atm09.004 */
/* @(#)CM_VerSion except.c atm08 1.4 16267.eco sum= 51706 atm08.003 */
/* @(#)CM_VerSion except.c atm06 1.5 13928.eco sum= 08765 */
/* @(#)CM_VerSion except.c 2015 1.4 09735.eco sum= 00240 */
/* @(#)CM_VerSion except.c 2014 1.2 07057.eco sum= 03537 */
/* @(#)CM_VerSion except.c 2013 1.3 06822.eco sum= 12590 */
/* $Header$ */
/*
  except.c
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#include "supportenvironment.h"
#include "supportexcept.h"
#include "supportcanthappen.h"

PUBLIC _Exc_Buf *_Exc_Header;


PUBLIC procedure os_raise (int  code, char * msg)
{
#ifdef USE_CXX_EXCEPTION
	_Exc_Buf e(code, msg);
    throw e;
#else
  register _Exc_Buf *EBp;
  _Exc_Buf **PPExcept = &_Exc_Header;

  EBp = *PPExcept;

  DebugAssert((EBp != 0));
  EBp->Code = code;
  EBp->Message = msg;
  *PPExcept = EBp->Prev;

  longjmp(EBp->Environ, 1);
#endif
}
