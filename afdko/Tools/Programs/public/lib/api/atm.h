/*
  atm.h
*/

/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
This file includes definitions for ATM compiler switches, operating
system, client, and environment dependent macros, and macros common to
buildch and other ATM modules (font parser). 
 */

#ifndef ATM_H
#define ATM_H

/* In the PostScript development enviroment PACKAGE_SPECS is defined as
   a header file that defines the path and file names of all the
   header files.  In non PostScript development environments
   PACKAGE_SPECS is defined as the file package.h. Package.h then
   defines the path and files names for the header files use in
   the non PostScript environment. It also has includes for system depedent
   header files.
  */

#if defined PSENVIRONMENT && PSENVIRONMENT
/* PostScript headers */
#include PACKAGE_SPECS
#include ENVIRONMENT
#include PUBLICTYPES
#include EXCEPT
#else  /* PSENVIRONMENT */
/* standard coretype environment */
#include "supportenvironment.h"
#include "supportpublictypes.h"
#include "supportexcept_bc.h"
#endif /* PSENVIRONMENT */

#if defined PSENVIRONMENT && PSENVIRONMENT
/* PostScript headers */
#include UTIL
#include CANTHAPPEN
#else
/* standard coretype environment */
#include "supportutil.h"
#include "supportcanthappen.h"
#endif

#ifndef DEVICE_CONSISTENT
#if ISP==isp_i80486 || ISP==isp_ppc || ISP==isp_x64 || ISP==isp_ia64
#define DEVICE_CONSISTENT 0
#else
#define DEVICE_CONSISTENT 1
#endif
#endif /*DEVICE_CONSISTENT*/

#ifndef GLOBALCOLORING
#define GLOBALCOLORING (1)    /* Turn on code for hinting Kanji Characters */
#endif

#ifndef DEBUG
#define DEBUG (0)
#endif

/*
 * Some of the PSENVIRONMENT configurations won't compile with ENABLE_ASSERT,
 * so we only turn it on for isp_sparc.  
 * This is enough to find machine-independent PSENVIRONMENT problems.
 */
#if STAGE==DEVELOP && ((defined PSENVIRONMENT && !PSENVIRONMENT) || ISP==isp_sparc)
#define ENABLE_ASSERT 1
#else
#define ENABLE_ASSERT 0
#endif

/*
 * A failed ASSERT aborts execution;
 * a failed PREFER generates a message on stderr.
 */
#if ENABLE_ASSERT
#if defined PSENVIRONMENT && PSENVIRONMENT
#define ASSERT(cond, str)     DebugAssertMsg(cond, BUILDCH_PKGID, str);
#define PREFER(cond, str)
#define ASSERT1(str)     DebugAssertMsg(false, BUILDCH_PKGID, str);

#else /* defined PSENVIRONMENT && PSENVIRONMENT */

#define ASSERT(cond, str) { \
    if (!(cond)) { \
      fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, str, #cond); \
      os_abort(); \
    } \
  }

#define ASSERT1(str) { \
	fprintf(stderr, "%s:%d: %s \n", __FILE__, __LINE__, str); \
	os_abort(); \
    }

#define PREFER(cond, str) { \
    if (!(cond)) { \
      fprintf(stderr, "%s:%d: %s (%s)\n", __FILE__, __LINE__, str, #cond); \
    } \
  }
#endif /* defined PSENVIRONMENT && PSENVIRONMENT */

#define return_or_abort( code, str)     ASSERT1(str)

#else  /* ENABLE_ASSERT */

#define ASSERT(cond, str)
#define PREFER(cond, str)

#define return_or_abort( code, str)     return (code)

#endif /* ENABLE_ASSERT */


#ifndef PSENVIRONMENT
#define PSENVIRONMENT 0
#endif

#if defined PSENVIRONMENT && !PSENVIRONMENT


/**********  System-dependent include files  *********/

#if OS==os_sun
/*  removed because of conflict with gcc built-in #include <memory.h> */
#define STRING_H /* added because of conflicts with gcc built-in */
#include <fcntl.h>              /* main.c needs this */
#include "supportossyslib.h"   /* EDIT FOR POSTSCRIPT@COMMENT@ */
#define labs(x) ((x)<0?-(x):(x))
#endif

#if OS==os_mach
#define MALLOC_H
#include <memory.h>
#include <fcntl.h>              /* main.c needs this */
#include <stdlib.h>
#include "supportossyslib.h"    /* EDIT FOR POSTSCRIPT@COMMENT@ */
#endif

#if OS==os_aix
#include <memory.h>
#include <fcntl.h>              /* main.c needs this */
#include <stdlib.h>
#include <stddef.h>             /* defines offsetof() macro */
#endif

#if OS==os_msdos || OS==os_os2 || OS==os_os2_32bit
#include <stdlib.h>
#include <memory.h>
#include <fcntl.h>              /* main.c needs this */
#endif

#if OS==os_windows3
/* need strtol, atoi, atol, ...? */
#include <stdlib.h>
#include "atmtypes.h"
/* #define labs ABS */
#else /* OS!=os_windows3 */
#define FARFUNC
#define FARVAR
#define NEARFUNC
#define NEARVAR
#endif

#if OS==os_mac
#define MALLOC_H
#define STRING_H
#include <string.h>
#include <stdlib.h>
#include <stddef.h>             /* supplies macro offset_of() */
#if ASMARITH
#define labs(x) asm_labs(x)
extern Int32 asm_labs ( Int32  i MW_D0);
#endif /* ASMARITH */
#endif

#if 0 /* was OS!=os_osx */
/* Defaults */
#ifndef MALLOC_H                /* Include malloc if we haven't already */
#include <malloc.h>
#endif
#ifndef STRING_H                /* Include strings if we haven't already */
#include <string.h>
#endif
#endif /* not os_osx */

/* The following definitions are used by the buildch package and the
   ATM parse code
*/

/* Inline Procedures */

/**********  System-dependent procs *********/

#ifndef os_free
#define os_free         free
#endif
#ifndef os_labs
#define os_labs         labs
#endif
#ifndef os_cos
#define os_cos          cos
#endif
#ifndef os_sin
#define os_sin          sin
#endif

#ifndef os_strcpy
#define os_strcpy	strcpy
#endif
#ifndef os_strncpy
#define os_strncpy	strncpy
#endif
#ifndef os_strcmp
#define os_strcmp	strcmp
#endif
#ifndef os_strncmp
#define os_strncmp	strncmp
#endif
#ifndef os_sprintf
#define os_sprintf	sprintf
#endif

/* MEMMOVE doesn't check for overlap so I use memcpy here */
#ifndef os_memcpy
#define os_memcpy	memcpy
#endif
#ifndef os_isspace
#define os_isspace	isspace
#endif

#if OS==os_msdos
#define os_malloc(x) malloc((size_t)x)
#define os_realloc(x,y) realloc(x,(size_t)y)
#else
#define os_malloc       malloc
#define os_realloc      realloc
#endif /* os_msdos */

#ifndef BCMAIN
#define BCMAIN (0)
#endif /* BCMAIN */

/* MEMMOVE(src, dest, len)  Copy a region of memory.  len in bytes. */
  /* WARNING: This does NOT check for overlapping memory areas */
/* MEMZERO(dest, len)  Zero out a region in memory.  len in bytes. */


#if OS==os_sun || OS==os_solaris || OS==os_mri
#define MEMMOVE(src, dest, len) memcpy(dest, src, len)
#define MEMZERO(dest, len) memset(dest, 0, len)
#endif

#if OS==os_msdos || OS==os_os2 || OS==os_windows3 || OS==os_os2_32bit
#define MEMMOVE(src, dest, len) memcpy(dest, src, (unsigned)(len))
#define MEMZERO(dest, len) memset(dest, 0, (unsigned)(len))
#endif

#if OS==os_thinkc
#define MEMMOVE(src, dest, len) \
  { register char *__s, *__d;  register long __l; \
    __s = (char *)(src); __d = (char *)(dest);  __l = (long)(len); \
    while (--__l >= 0) *__d++ = *__s++; \
  }
#define MEMZERO(dest, len) \
  { register char *__d;  register long __l; \
    __d = (char *)(dest);  __l = (long)(len); \
    while (--__l >= 0) *__d++ = 0; \
  }

/* If CScan cannot render the character with the initial cross buffer size,
   the client will get a callback to grow the buffer in increments of
   CSCAN_MIN_INCR to CSCAN_MAX_BUFSIZE.
   If the buffer is grown another attempt will be made to render a character.
   This will repeat for CSCAN_RETRIES number of times.*/

/* Define how to alloc memory during cscan.                             */
#define CSCAN_MAX_BUFSIZE MAXInt32   /* Grow until there is no more memory */
#define CSCAN_BUF_INCR (7000)
#define CSCAN_RETRIES   500     /* Ken - Really, I don't _care_ how many times it retries.... */

#endif

/* DEFAULTS */
#ifndef MEMMOVE
#define MEMMOVE(src, dest, len) os_bcopy( (const char *)src, (char *)dest, len)
#define MEMZERO(dest, len) os_bzero( (char *)dest, len)
#endif

/************* end non-postscript environment */

#else  /* PSENVIROMNEMT */

#define BCMAIN  (0) /* for 2ps environment */

/* If CScan cannot render the character with the initial cross buffer size,
   the client will get a callback to grow the buffer in increments of
   CSCAN_MIN_INCR to CSCAN_MAX_BUFSIZE.
   If the buffer is grown another attempt will be made to render a character.
   This will repeat for CSCAN_RETRIES number of times.
*/

#define CSCAN_MAX_BUFSIZE 65536   /* Grow to  64K bytes */
#define CSCAN_BUF_INCR 8192
#define CSCAN_RETRIES   6


/* MEMMOVE(src, dest, len)  Copy a region of memory.  len in bytes. */
  /* WARNING: This does NOT check for overlapping memory areas */
/* MEMZERO(dest, len)  Zero out a region in memory.  len in bytes. */

#define MEMMOVE(src, dest, len) os_bcopy(src, dest, len)
#define MEMZERO(dest, len) os_bzero(dest, len)

#endif /* PSENVIROMNEMT */

#ifndef CSCAN_BUF_INCR
#define CSCAN_MAX_BUFSIZE 65504	/* Back off from 64K because overhead */
				/* usually mean actual 64K cann't be reached */
#define CSCAN_BUF_INCR (8188)	/* 1/8th the max.  Works with value below */
#define CSCAN_RETRIES   8 /* After 8 tries it CAN'T get more guaranteed! */
#endif

#define FixedZero   (Fixed)0x0L
#define FixedHalf       (Fixed)0x00008000L
#define FixedOne        (Fixed)0x00010000L
#define FixedTwo        (Fixed)0x00020000L

#define FIXEDZERO   FixedZero
#define FIXEDHALF       FixedHalf
#define FIXEDONE        FixedOne
#define FIXEDTWO        FixedTwo


#define MAXFixed MAXInt32
#define MINFixed MINInt32

/* fixsqrt() cannot be defined in support/fp.h because PostScript's fp/fp.h
 * offers no such macro. */
#define fixsqrt(x) (((fracsqrt(x) + 0x40) >> 7) & 0x01FFFFFF)

/*
 SubtractPtr
   This macro is required for Microsoft C 6.x and 7.0 and possibly other Intel
   compilers that target the 80x86 64K segment architecture. When doing
   pointer arithmetic, only the segment offsets are used. The offsets
   are unsigned int16s, but the difference is represented as an int16.
   Therefore, when the pointers are more than 32767 bytes apart, the
   difference overflows. This happens even when the pointers point to
   structs that are larger than 2 bytes; the difference is calculated
   and put into an int16, no overflow check is performed, the int16 is
   extended to an int32 and divided by the struct size.

   Therefore, first get the byte difference between the pointers, cast
   the result to an unsigned, and then divide by the struct size.
   Nabeel Al-Shamma 9-Apr-1992
*/

/* strictly speaking, both SubtractPtr and offsetof are
   defined incorrectly. The difference between two pointers
   should be ptrdiff_t. Unfortunately that definition doesn't
   fix the code for the 64K segment case and there is no
   safe cast that works for (ptrdiff_t to long, int etc.)
   for all 32 and 64 bit platforms.

   The use of Int32 means that this code may get an incorrect
   result if we ever allocate more than 2GB at once.

   Note that it is always invalid to use any pointer arithmetic
   on pointers referring to different allocations. Only test
   for equality is defined in that case.
*/

#if ISP==isp_i80286 || SEGMENT_64K
#define SubtractPtr(p0, p1)     \
        ((unsigned)((char *)(p0)-(char *)(p1))/sizeof(*(p0)))
#else
#define SubtractPtr(p0, p1)     (Int32)((p0) - (p1))
#endif

/* offsetof should be in <stddef.h>, but not all machines have one. */
/* This is done with a subtraction to work around an apparent bug in the
 * Microsoft 16-bit 80x86 compiler.
 *
 * The AIX system generates an error if a macro is multiply-defined,
 * and oemtest.c needs to include "atm.h" before <stddef.h>.
 */
#if defined PSENVIRONMENT && PSENVIRONMENT
/* PostScript header */
#include ASBASIC
#else
/* standard coretype environment */
#include "supportasbasic.h"
#endif

#ifndef	offsetof
#define offsetof(type,field) \
	(Int32)((char *)&((type *)1)->field - (char *)1)
#endif /* offsetof */

#endif /* ATM_H */
