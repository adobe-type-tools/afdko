/* @(#)CM_VerSion ossyslib.h atm09 1.2 16563.eco sum= 26489 atm09.004 */
/* @(#)CM_VerSion ossyslib.h atm08 1.3 16314.eco sum= 61190 atm08.004 */
/* @(#)CM_VerSion ossyslib.h atm07 1.3 16042.eco sum= 32875 atm07.009 */
/* @(#)CM_VerSion ossyslib.h atm05 1.10 12455.eco sum= 59595 */
/* @(#)CM_VerSion ossyslib.h atm04 1.6 07558.eco sum= 48090 */
/*
  ossyslib.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 * The purpose of ossyslib.h is to provide declarations for routines that
 * are provided by the system's libraries but, alas, not declared in system
 * header files.  
 * XXX: The function of this file could be shifted to util.h, allowing a
 *      simplification of the atm->20xx source code transfer.
 */

/* Declarations for system library routines on unix machines */
#ifndef OSSYSLIB_H
#define OSSYSLIB_H

#if 0 /* TSD */

#if OS == os_mach /* next machines */
#include <libc.h>
#define bcopy(from,to,len) ((void)memmove(to,from,len))
#define bzero(b,len) memset(b,0,len)

#endif /* TSD */

#endif  /* OSSYSLIB_H */
