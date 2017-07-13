/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef CTUTIL_H
#define CTUTIL_H

#include "ctlshare.h"

#define CTU_VERSION CTL_MAKE_VERSION(2,0,2)

#include <stddef.h> /* For size_t */
#include <time.h>   /* For struct tm */

#ifdef __cplusplus
extern "C" {
#endif

/* CoreType Utilities Library
   ==========================
   This library contains miscellaneous utility functions that are applicable
   across many applications and are therefore separated out into a library. */

typedef int 
    (CTL_CDECL *ctuCmpFunc)(const void *first, const void *second, void *ctx);
void ctuQSort(void *base, size_t count, size_t size, ctuCmpFunc cmp,void *ctx);

/* Sort array. 

   CTUQSort() is modeled on the ANSI C library qsort() function and all
   arguments and semantics, except the "ctx" arguments, match those of the
   standard function. The "ctx" arguments provides the facility for the client
   to pass a context containing auxiliary data to the comparison function.

   This function is intended to be used in situations where auxiliary data is
   required by the comparison function but can't be provided by global
   variables for reentrancy reasons. */

typedef int 
    (CTL_CDECL *ctuMatchFunc)(const void *key, const void *value, void *ctx);
int ctuLookup(const void *key, const void *base, size_t count, size_t size,
              ctuMatchFunc match, size_t *index, void *ctx);

/* Search sorted array for key.

   CTULookup() is modeled on the ANSI C library bsearch() function. However, it
   differs from the standard function in that it returns an element index, via
   the "index" parameter, and not an element pointer. It also returns the
   result of the search. If an element in the array matches the key, the
   function returns 1 and the "index" parameter is set to the matching element.
   If no matching element is found, the function returns 0 and the "index"
   parameter is set to the element where the key should be inserted. The "ctx"
   arguments provides the facility for the client to pass a context containing
   auxiliary data to the comparison function.

   This function is intended to be used for inserting new elements into a
   sorted list without duplicating identical elements. */

typedef unsigned char ctuLongDateTime[8];

/* Apple's LongDateTime is a 64-bit number representing the number of seconds
   since 1 Jan 1904 00:00:00 local time. This value is stored in an 8 byte
   array, most significant byte first. 

   The conversions performed by the functions in this library are use the long
   data type, which is assumed to hold at least 32-bits, and do not check for
   overflow. This means that the conversions are only valid for dates up to 6
   Feb 2040 06:28:15; 32-bit overflow occurs on the next second.
*/

void ctuANSITime2LongDateTime(struct tm *ansi, ctuLongDateTime ldt);

/* ctuANSITime2LongDateTime() converts ANSI standard date/time format to Apple
   LongDateTime format.

   The struct tm fields: tm_wday, tm_yday, and tm_dst are ignored by the
   conversion process. No validation is performed on the input data. */

void ctuLongDateTime2ANSITime(struct tm *ansi, ctuLongDateTime ldt);

/* ctuLongDateTime2ANSITime() converts Apple LongDateTime format to ANSI
   standard date/time format. */

int ctuCountBits(long value);

/* ctuCountBits() counts the number of bits set in the value argument. */

double ctuStrtod(const char *s, char **endptr);

/* ctuStrtod() operates like the standard library function strtod() except that
   it behaves as though it was using the C locale and thus the decimal point
   character is always a period and not comma. */

void ctuDtostr(char *buf, size_t bufLen, double value, int width, int precision);

/* ctuDtostr() converts the "value" parameter into a string and stores the
   result into the buffer pointed to by the "buf" parameter. The conversion is
   performed using the "%g" print format and if the "width" or "percision"
   parameters are non-0 they are used to control the minimum field width and
   precision, respectively. This function behaves as though it was using the C
   locale and thus the decimal point character is always a period and not
   comma. */

void ctuGetVersion(ctlVersionCallbacks *cb);

/* ctuGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#if !defined(_UCRT)  
    float roundf(float x);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CTUTIL_H */
