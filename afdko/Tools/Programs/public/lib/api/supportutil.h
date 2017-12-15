/* @(#)CM_VerSion util.h atm09 1.3 16563.eco sum= 40816 atm09.004 */
/* @(#)CM_VerSion util.h atm08 1.5 16364.eco sum= 28141 atm08.006 */
/* @(#)CM_VerSion util.h atm07 1.7 16179.eco sum= 43496 atm07.013 */
/* @(#)CM_VerSion util.h atm06 1.7 14418.eco sum= 02241 */
/* $Header$ */
/*
  util.h
*/
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


#ifndef	UTIL_H
#define	UTIL_H

#include "supportpublictypes.h"

#ifndef WINATM
#define WINATM 0
#endif

#ifndef CDECL
#define CDECL
#endif

/* Exported Procedures */

/*
 * Byte Array Interface  
 */
#if !PSENVIRONMENT

#if (OS == os_sun || OS == os_mach)
#include "supportossyslib.h"               /* include declarations not available from a standard place */

/* the following definition is a compromise. There is a conflict between the
   sun string.h definition of strlen and the built-in definition of strlen.
   define strncmp here rather than refer to the include file.

   string.h

*/

extern int strncmp();

/* end of compromise. */

#elif WINATM
#ifndef CODE32 /* Microsoft Compiler */
#include <windows.h>
#include <string.h> 
#else /* Metaware Compiler */
#include <string.h> 
#endif /* Microsoft Compiler */
#else /* ! ((OS == os_sun || OS == os_mach) || WINATM) */
#include <stdio.h>
#include <string.h>

#endif /* (OS == os_sun || OS == os_mach) */

#define os_strlen       strlen
#if OS == os_solaris || OS==os_msdos || OS==os_os2 || OS==os_windows3 || OS==os_os2_32bit || OS==os_mac || OS==os_windowsNT || OS==os_windows95
#define os_bcopy(from,to,cnt)  	memmove(to,from, (unsigned)(cnt&0x0FFFFL))
#else /* OS */
#define os_bcopy    	memmove
#endif
#if OS==os_sun || OS==os_solaris || OS==os_windowsNT || OS==os_mac || OS==os_windows3 || OS==os_windows95
#define os_bzero(b, len)		memset(b, 0, len)
#else
#define os_bzero    	bzero
#endif

#if OS==os_sun
#include <stdarg.h>
/*
extern int vprintf(char *, va_list);
*/
#endif

#define os_strcat	strcat
#define os_strchr       strchr
#define os_strcpy	strcpy
#define os_strncpy	strncpy
#define os_strcmp	strcmp
#define os_strncmp	strncmp
#define os_sprintf	sprintf
#define os_vsprintf	vsprintf
#define os_vprintf	vprintf
#define os_min(a,b)	(a<b)?a:b
#else /* !PSENVIRONMENT */
/*
 * In the PostScript environment, these protos would not be seen,
 * so they are for documentation purposes only.
 */

extern procedure os_bcopy ( const void *  from, void *  to, long int  cnt);
/* Copies count consecutive bytes from a source block to a destination
   block. This procedure works correctly for all positive values of
   the count (even if greater than 2**16); it does nothing if count
   is zero or negative. This procedure works correctly even if the
   source and destination blocks overlap; that is, the result is
   as if all the source bytes are read before any destination bytes
   are written. */

extern procedure os_bzero ( void *  to, long int  cnt);
/* Sets count consecutive bytes to zero starting at the specified
   destination address. This procedure works correctly for all positive
   values of the count (even if greater than 2**16); it does nothing
   if count is zero or negative. */

extern procedure os_bvalue ( register void *  to, register long int  cnt, register char  value);
/* Sets count consecutive bytes to value starting at the specified
   destination address. This procedure works correctly for all positive
   values of the count (even if greater than 2**16); it does nothing
   if count is zero or negative. */

/*
 * String interface
 *
 * Note: in all the following procedures, a "string" is an indefinitely
 * long array of characters terminated by '\0' (a null character).
 */

/* Exported Procedures */

extern char *os_index ( register char *  str, register char  ch);
/* Returns a pointer to the first occurrence of the character ch
   in the string str, or zero if ch does not occur in str. */
/* Should be name os_strchr() to match ANSI function strchr() (otherwise
   this function is identical, including parameter and return types */
#define os_strchr(s, c) (os_index(s, c))

extern char *os_rindex ( register char *  str, register char  ch);
/* Returns a pointer to the last occurrence of the character ch
   in the string str, or zero if ch does not occur in str. */
/* Should be name os_strrchr() to match ANSI function strrchr() (otherwise
   this function is identical, including parameter and return types */

extern Card32 os_strlen ( const char *  str);
/* Returns the number of characters in the string, not counting
   the terminating null byte. */

extern int os_strcmp ( register const char *  s1, register const char *  s2);
/* Compares two strings lexicographically and returns a negative
   result if s1 is less than s2, zero if they are equal, and
   a positive result if s1 is greater than s2. s1 is
   lexicographically less than s2 if it is a substring or if
   the first non-matching character of s1 is less than the
   corresponding character of s2. */

extern int os_strncmp ( register const char *  s1, register const char *  s2, register int  n);
/* Compares two strings lexicographically up to length n
   and returns a negative
   result if s1 is less than s2, zero if they are equal, and
   a positive result if s1 is greater than s2. s1 is
   lexicographically less than s2 if it is a substring or if
   the first non-matching character of s1 is less than the
   corresponding character of s2. */

extern char *os_strcpy ( char *  to, register const char *  from);
/* Copies the contents of the second string (including the terminating
   null character) into the first string, overwriting its former
   contents. The number of characters is determined entirely by
   the position of the null byte in the source string and is not
   influenced by the former length of the destination string.
   The source and destination strings must not overlap. Returns
   the destination string pointer. */

extern char *os_strncpy ( char *  to, register const char *  from, register int  cnt);
/* Copies cnt bytes from the contents of the second string to the first
   string, truncating or zero padding the first string as necessary.  The
   first string will be null terminated only if cnt is greater than the
   length of the second string. */

extern char *os_strcat ( char *  to, register char *  from);
/* Appends a copy of the second string (including its terminating
   null character) to the end of the first string, overwriting
   the first string's terminating null. It is the caller's
   responsibility to ensure adequate space beyond the end of
   the first string. The source and destination strings must not
   overlap. Returns the destination string pointer. */

/*
 * Miscellaneous Numerical Operations Interface
 */

extern long int os_max ( long int  a, long int  b);
extern long int os_min ( long int  a, long int  b);
/* Returns the maximum or minimum, respectively, of two long integer
   arguments. Unlike the macros MAX and MIN, these are pure procedures;
   thus, it's OK for their arguments to be expressions with side-effects,
   and the calling sequence, though less effecient, is usually shorter. */

/*
 * Signal Interface
 */

extern os_SigFunc ( int  signum);
/* Generates the signal specified by signum.  This routine is provided for
   use by exception handlers and similar routines that must generate a signal
   in response to some condition. */

/* Initialization Interface */

extern procedure os_signalinit ();
/* Provides default handler routines for each type of signal.  The SIGINT
   and SIGFPE default handler routines ignore the signal and return.  All
   others call CantHappen. */

/* Debug Interface */

extern int CDECL os_vsprintf (
			char * s,
			readonly char * format,...
			);
/* Writes a formatted sequence of characters to the string s.
  For simplicity, floating-point numbers
  and formats are not implemented.
  This is the va_list form of sprintf.  */ 
#endif /* !PSENVIRONMENT */
#endif	/* UTIL_H */
