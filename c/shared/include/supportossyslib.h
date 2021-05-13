/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
  ossyslib.h
*/

/*
 * The purpose of ossyslib.h is to provide declarations for routines that
 * are provided by the system's libraries but, alas, not declared in system
 * header files.
 * XXX: The function of this file could be shifted to util.h, allowing a
 *      simplification of the atm->20xx source code transfer.
 */

/* Declarations for system library routines on Unix machines */
#ifndef OSSYSLIB_H
#define OSSYSLIB_H

#if 0 /* TSD */

#if OS == os_mach /* next machines */
#include <libc.h>
#define bcopy(from, to, len) ((void)memmove(to, from, len))
#define bzero(b, len) memset(b, 0, len)

#endif /* TSD */

#endif /* OSSYSLIB_H */
