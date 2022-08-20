/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_SUPPORTDEFINES_H_
#define SHARED_INCLUDE_SUPPORTDEFINES_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Inline Functions */

#ifndef ABS
#define ABS(x) ((x) < 0 ? -(x) : (x))
#endif
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifdef _MSC_VER
#define RAND_R rand_s
#define STRTOK_R strtok_s
#define FPRINTF_S fprintf_s
#define VFPRINTF_S vfprintf_s
#define SPRINTF_S sprintf_s
#define VSPRINTF_S vsprintf_s
#define SSCANF_S sscanf_s
#define STRCPY_S(d, ds, s) strcpy_s(d, ds, s)
#define STRNCPY_S(d, ds, s, n) strncpy_s(d, ds, s, n)
#define STRCAT_S(d, ds, s) strcat_s(d, ds, s)
#else
#ifndef RAND_R
#define RAND_R rand_r
#endif
#ifndef STRTOK_R
#define STRTOK_R strtok_r
#endif
#ifndef FPRINTF_S
#define FPRINTF_S fprintf
#endif
#ifndef VFPRINTF_S
#define VFPRINTF_S vfprintf
#endif
#ifndef SPRINTF_S
#define SPRINTF_S(b, l, f, ...) snprintf(b, l, f, ##__VA_ARGS__)
#endif
#ifndef VSPRINTF_S
#define VSPRINTF_S vsnprintf
#endif
#ifndef SSCANF_S
#define SSCANF_S sscanf
#endif
#ifndef STRCPY_S
#define STRCPY_S(d, ds, s) snprintf(d, ds, s)
#endif
#ifndef STRNCPY_S
#define STRNCPY_S(d, ds, s, n) strncpy(d, s, n)
#endif
#ifndef STRCAT_S
#define STRCAT_S(d, ds, s) strncat(d, s, ds-1)
#endif
#endif

#ifdef __cplusplus
}
#endif


#endif  // SHARED_INCLUDE_SUPPORTDEFINES_H_
