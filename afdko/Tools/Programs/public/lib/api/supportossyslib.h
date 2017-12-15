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

#elif OS == os_sun
/*
 * Sun compiles use Cygnus gcc, but the non-ANSI libc.a and libm.a libraries
 * provided by Sun.  Therefore, Sun include files should also be used to the
 * exclusion of Cygnus include files.  Currently, the Sun include files are
 * not complete, requiring the declarations you see below. 
 */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
/*
PUBLIC procedure printf (char * fmt,...);
PUBLIC procedure fprintf (FILE * fp, char * fmt,...);
*/
PUBLIC size_t	fread ( void *  ptr, size_t   size, size_t   nmemb, FILE *  f );
PUBLIC int	fclose ( FILE *  f );
PUBLIC int	fflush ( FILE *  f );
PUBLIC int	fscanf ( FILE * f, const char * format,...);
PUBLIC time_t   time ( time_t *  t);
/*
PUBLIC int	_flsbuf ( unsigned char  c, FILE *  fp);
PUBLIC char    *memset (void *  s, int  c, size_t  n);
*/
PUBLIC void    *memmove (void *  d, const void *  s, size_t  n);
PUBLIC char    *strchr (const char *  s, int  c);
/*
PUBLIC char    *strncpy (const char *  s1, const char *  s2, size_t  n);

PUBLIC int atoi (char *  s);
PUBLIC long atol (char *  s);
*/

/* PUBLIC procedure _flsbuf(); -- prevents sun cc from compiling putc() */
PUBLIC int _filbuf();
PUBLIC int fseek(FILE *stream, long offset, int whence);
/*
PUBLIC int vsprintf( char *str, char *format, va_list ap);
PUBLIC size_t fwrite( void *ptr, size_t size, size_t nmemb,  FILE*stream);
*/
PUBLIC int  vfprintf(  FILE  *stream, const char *format, va_list ap);
PUBLIC double strtod(const char *nptr, char **endptr);

#ifdef __GNUC__
/*PUBLIC volatile procedure exit (int  status);*/

#else /* __GNUC__ */
/* PUBLIC procedure exit (int  status); */

/* These definitions were moved out of the gcc path because they conflict */
PUBLIC char *malloc (unsigned long  size);
PUBLIC char *realloc (void *  p, unsigned long  size);
#endif /* __GNUC__ */

/*PUBLIC procedure abort ();*/
/*
PUBLIC long strtol (char *  s, char **  endp, int  radix);
*/

/* map PS world routines onto Sun environment */
PUBLIC void    bcopy ( const void *  s, void *  d, size_t  n);

#endif	/* OS == os_sun */

#endif /* TSD */

#endif  /* OSSYSLIB_H */
