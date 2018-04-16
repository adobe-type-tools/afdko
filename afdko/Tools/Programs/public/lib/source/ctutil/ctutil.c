/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * CoreType Utilities Library
 */


#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>
#include "ctutil.h"

/* Exchange 2 values of size "s" pointed to by "a" and "b". */
#define MAX_BUF 256
#define EXCH(a, b, s) \
	do { \
		char tmp[MAX_BUF]; \
		size_t n; \
		char *p = (a); \
		char *q = (b); \
		for (n = (s); n > MAX_BUF; n -= MAX_BUF) {\
			memcpy(tmp, p, MAX_BUF); \
			memcpy(p, q, MAX_BUF); \
			memcpy(q, tmp, MAX_BUF); \
			p += MAX_BUF; \
			q += MAX_BUF; \
		} \
		memcpy(tmp, p, n); \
		memcpy(p, q, n); \
		memcpy(q, tmp, n); \
	} while (0)

/* Sort array using quicksort algorithm. Algorithm adapted from the Quicksort
   chapter of "Algorithms in C" by Robert Sedgewick. */
static void quicksort(char *pl, char *pr, int size,
                      int (CTL_CDECL *cmp)(const void *first,
                                           const void *second, void *ctx),
                      void *ctx) {
	while (pr - pl > 0) {
		char *pi = pl - size;
		char *pj = pr;

		for (;; ) {
			while (cmp(pi += size, pr, ctx) < 0) {
			}
			while (cmp(pj -= size, pr, ctx) > 0) {
				if (pj == pl) {
					break;  /* Avoid overrun */
				}
			}
			if (pi >= pj) {
				break; /* Pointers crossed */
			}
			EXCH(pi, pj, size);
		}

		/* Move partition value to final position */
		if (pi != pr) {
			EXCH(pi, pr, size);
		}

		/* Sort subfiles l..i-1 and i+1..r; recurse on smallest subfile */
		pj = pi - size;
		pi += size;
		if (pj - pl < pr - pi) {
			if (pj - pl > 0) {
				quicksort(pl, pj, size, cmp, ctx);
			}
			pl = pi;
		}
		else {
			if (pr - pi > 0) {
				quicksort(pi, pr, size, cmp, ctx);
			}
			pr = pj;
		}
	}
}

/* Sort array. Replacement for ANSI C qsort() when compare function requires
   access to context but can't use global variables for reentrancy reasons. */
void ctuQSort(void *base, size_t count, size_t size,
              int (CTL_CDECL *cmp)(const void *first, const void *second,
                                   void *ctx),
              void *ctx) {
	/* 64-bit warning fixed by cast here */
	quicksort((char *)base, (char *)base + (count - 1) * size, (int)size, cmp, ctx);
}

/* Binary search for key. Returns 1 if found else 0. Found or insertion
   position returned via "index" parameter. */
int ctuLookup(const void *key, const void *base, size_t count, size_t size,
              int (CTL_CDECL *cmp)(const void *key, const void *value,
                                   void *ctx),
              size_t *index, void *ctx) {
	long lo = 0;
	/* 64-bit warning fixed by cast here */
	long hi = (long)(count - 1);

	while (lo <= hi) {
		long mid = (lo + hi) / 2;
		int match = cmp(key, (char *)base + mid * size, ctx);
		if (match > 0) {
			lo = mid + 1;
		}
		else if (match < 0) {
			hi = mid - 1;
		}
		else {
			/* Found */
			*index = mid;
			return 1;
		}
	}

	/* Not found */
	*index = lo;
	return 0;
}

/* Convert ANSI standard date/time format to Apple LongDateTime format.
   Algorithm adapted from standard Julian Day calculation. */
void ctuANSITime2LongDateTime(struct tm *ansi, ctuLongDateTime ldt) {
	unsigned long elapsed;
	int month = ansi->tm_mon + 1;
	int year = ansi->tm_year;

	if (month < 3) {
		/* Jan and Feb become months 13 and 14 of the previous year */
		month += 12;
		year--;
	}

	/* Calculate elapsed seconds since 1 Jan 1904 00:00:00 */
	elapsed = (((year - 4L) * 365 +             /* Add years (as days) */
	            year / 4 +                      /* Add leap year days */
	            (306 * (month + 1)) / 10 - 64 + /* Add month days */
	            ansi->tm_mday) *                /* Add days */
	           24 * 60 * 60 +                   /* Convert days to secs */
	           ansi->tm_hour * 60 * 60 +        /* Add hours (as secs) */
	           ansi->tm_min * 60 +              /* Add minutes (as secs) */
	           ansi->tm_sec);                   /* Add seconds */

	/* Set value */
	ldt[0] = ldt[1] = ldt[2] = ldt[3] = 0;
	ldt[4] = (char)(elapsed >> 24);
	ldt[5] = (char)(elapsed >> 16);
	ldt[6] = (char)(elapsed >> 8);
	ldt[7] = (char)elapsed;
}

/* Convert Apple LongDateTime format to ANSI standard date/time format.
   Algorithm adapted from standard Julian Day calculation. */
void ctuLongDateTime2ANSITime(struct tm *ansi, ctuLongDateTime ldt) {
	unsigned long elapsed = ((unsigned long)ldt[4] << 24 |
	                         (unsigned long)ldt[5] << 16 |
	                         ldt[6] << 8 |
	                         ldt[7]);
	long A = elapsed / (24 * 60 * 60L);
	long B = A + 1524;
	long C = (long)((B - 122.1) / 365.25);
	long D = (long)(C * 365.25);
	long E = (long)((B - D) / 30.6001);
	long F = (long)(E * 30.6001);
	long secs = elapsed - A * (24 * 60 * 60L);
	if (E > 13) {
		ansi->tm_year = C + 1;
		ansi->tm_mon = E - 14;
		ansi->tm_yday = B - D - 429;
	}
	else {
		ansi->tm_year = C;
		ansi->tm_mon = E - 2;
		ansi->tm_yday = B - D - 64;
	}
	ansi->tm_mday = B - D - F;
	ansi->tm_hour = secs / (60 * 60);
	secs -= ansi->tm_hour * (60 * 60);
	ansi->tm_min = secs / 60;
	ansi->tm_sec = secs - ansi->tm_min * 60;
	ansi->tm_wday = (A + 5) % 7;
	ansi->tm_isdst = 0;
}

/* Count bits in a long word */
int ctuCountBits(long value) {
	int count;
	for (count = 0; value; count++) {
		value &= value - 1;
	}
	return count;
}

/* Convert string to double in thread safe and locale-independant manner. */
double ctuStrtod(const char *s, char **endptr) {
	char *ep;
	double result = strtod(s, &ep);

	if (*ep == '.') {
		/* Parse stopped on period; may be using non-C locale */
		char *p;
		size_t length;

		/* Assume the period was the decimal point and parse rest of number.
		   Note, it is possible that the period terminated a number tha didn't
		   contain a decimal point, e.g. "1E2.". In this rare case the number
		   was already parsed correctly, the following repeated conversions
		   will have no effect, and the correct value will be returned. */
		(void)strtod(ep + 1, &p);
		length = p - s;

		if (length < 50) {
			int i;
			char tmp[50];

			/* Make modifiable copy of string */
			memcpy(tmp, s, length);
			tmp[length] = '\0';
			p = ep - s + tmp;

			/* Try conversion serveral times in case locale switched on
			   another thread */
			for (i = 0; i < 6; i++) {
				/* Cycle between period and comma */
				*p = (*p == '.') ? ',' : '.';
				result = strtod(tmp, &ep);
				if (ep == tmp + length) {
					/* Reached end of number; conversion succeeded */
					ep = (char *)(s + length);
					break;
				}
			}
		}
	}

	if (endptr != NULL) {
		*endptr = ep;
	}

	return result;
}

/* Convert double in a locale-independant manner. */
void ctuDtostr(char *buf, size_t bufLen, double value, int width, int precision) {
	char *p;
	if (width == 0 && precision == 0) {
		SPRINTF_S(buf, bufLen, "%.12lf", value);
	}
	else if (width == 0 && precision > 0) {
		SPRINTF_S(buf, bufLen, "%.*lf", precision, value);
	}
	else if (width > 0 && precision == 0) {
		SPRINTF_S(buf, bufLen, "%*lf", width, value);
	}
	else {
		SPRINTF_S(buf, bufLen, "%*.*lf", width, precision, value);
	}
	p = strchr(buf, ',');
	if (p != NULL) {
		/* Non-C locale in use; convert to C locale convention */
		*p = '.';
	}
    p = strchr(buf, '.');
    if (p != NULL) {
        /* Have decimal point. Remove trailing zeroes.*/
        int l = (int)strlen(p);
        p += l-1;
        while(*p == '0')
        {
           *p = '\0';
            p--;
        }
        if (*p == '.') {
          *p = '\0';
        }
    }
}

/* Get version numbers of libraries. */
void ctuGetVersion(ctlVersionCallbacks *cb) {
	if (cb->called & 1 << CTU_LIB_ID) {
		return; /* Already enumerated */
	}
	cb->getversion(cb, CTU_VERSION, "ctutil");

	/* Record this call */
	cb->called |= 1 << CTU_LIB_ID;
}

char* CTL_SPLIT_VERSION(char* version_buf, unsigned int version) {
	sprintf(version_buf, "%d.%d.%d", (int)((version)>>16&0xff),
			(int)((version)>>8&0xff),
			(int)((version)&0xff));
	return version_buf;
}


#if !defined(_UCRT)
float roundf(float x)
{
    float val =  (float)((x < 0) ? (ceil((x)-0.5)) : (floor((x)+0.5)));
    return val;
}
#endif

