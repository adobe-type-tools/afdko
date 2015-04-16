/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the ACLIB_API
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// ACLIB_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
*/
#ifndef AC_C_LIB_H_
#define AC_C_LIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef AC_C_LIB_EXPORTS

#if WIN32
#define ACLIB_API __declspec(dllexport)
#else
	
#if __MWERKS__
#define ACLIB_API __declspec(export)
#elif __GNUC__ && __MACH__
#define ACLIB_API __attribute__((visibility("default")))
#endif
	
#endif

#else
#define ACLIB_API  
#endif

enum 
{
	AC_Success,
	AC_FontinfoParseFail,
	AC_FatalError,
	AC_MemoryError,
	AC_UnknownError,
	AC_DestBuffOfloError,
	AC_InvalidParameterError
};

ACLIB_API const char * AC_getVersion(void);

typedef void *(*AC_MEMMANAGEFUNCPTR)(void *ctxptr, void *old, unsigned long size);

/*
 * Function: AC_SetMemManager
 ****************************
If this is supplied, then the AC lib will call this function foor all memory allocations.
Else is it will use the std C lib alloc/malloc/free.
*/
ACLIB_API void  AC_SetMemManager(void *ctxptr, AC_MEMMANAGEFUNCPTR func);

/*
 * Function: AC_SetReportCB
 ****************************
If this is supplied and verbose is set to true, then the AC lib will write (many!) text status messages to this file.
If verbose is set false, then only error messages are written.
*/
typedef void (*AC_REPORTFUNCPTR)(char *msg);
extern AC_REPORTFUNCPTR libReportCB; /* global log function which is supplied by the following */
extern AC_REPORTFUNCPTR libErrorReportCB; /* global error log function which is supplied by the following */

ACLIB_API void  AC_SetReportCB(AC_REPORTFUNCPTR reportCB, int verbose);


/* Global for reporting log messages. */


/*
 * Function: AC_SetReportStemsCB
 ****************************
If this is called , then the AC lib will write all the stem widths it encounters.
Note that the callabcks should not dispose of the glyphName memory; that belongs to the AC lib.
It should be copied immediately - it may may last past the return of the calback.
*/

extern unsigned int allstems; /* if false, then stems defined by curves are xcluded from the reporting */
typedef void (*AC_REPORTSTEMPTR)(long int top, long int bottom, char* glyphName);

extern AC_REPORTSTEMPTR addHStemCB;
extern AC_REPORTSTEMPTR addVStemCB;
ACLIB_API void  AC_SetReportStemsCB(AC_REPORTSTEMPTR hstemCB, AC_REPORTSTEMPTR vstemCB, unsigned int allStems);

/*
 * Function: AC_SetReportZonesCB
 ****************************
If this is called , then the AC lib will write all the aligment zones it encounters.
Note that the callabcks should not dispose of the glyphName memory; that belongs to the AC lib.
It should be copied immediately - it may may last past the return of the calback.
*/
typedef void (*AC_REPORTZONEPTR)(long int top, long int bottom, char* glyphName);
extern AC_REPORTZONEPTR addCharExtremesCB;
extern AC_REPORTZONEPTR addStemExtremesCB;
ACLIB_API void  AC_SetReportZonesCB(AC_REPORTZONEPTR charCB, AC_REPORTZONEPTR stemCB);

typedef void (*AC_RETRYPTR)(void);
extern AC_RETRYPTR reportRetryCB;

/*
 * Function: AutoColorString
 ****************************
 * This function takes srcbezdata, a pointer to null terminated C-string containing data in the bez
 * format (see bez spec) and fontinfo, a pointer to null terminated C-string containing fontinfo
 * for the bez glyph. Hint information is added to the bez data and returned to the caller through the
 * buffer dstbezdata. dstbezdata must be allocated before the call and a pointer to its length passed
 * as *length. If the space allocated is insufficient for the target bezdata, an error will be returned
 * and *length will be set to the desired size.
 */
ACLIB_API int AutoColorString(const char *srcbezdata, const char *fontinfo, char *dstbezdata, int *length, int allowEdit, int allowHintSub, int roundCoords, int debug);

/*
 * Function: AC_initCallGlobals
 ****************************
 * This function must be called before calling AC_initCallGlobals in the case where
 * the program is switching between any of the auto-hinting and stem reportign modes
 * while running.
 */

ACLIB_API void AC_initCallGlobals(void);

#ifdef __cplusplus
}
#endif 
#endif /* AC_C_LIB_H_ */
