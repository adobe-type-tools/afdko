/* used by Metrowerks compiler to specify compile-time definitions */


#if __GNUC__

#if defined(__i386__) || defined(__x86_64__)
#define CT_LITTLE_ENDIAN
#endif

#endif

#if defined(__APPLE__)
#define OSX 1
#endif

#ifndef _MAX_PATH
#define _MAX_PATH	2048
#endif


#define AUTOSCRIPT 1
#define CFF_T13_SUPPORT 1
#define EXECUTABLE 1

  
#if EXECUTABLE
#define OUTPUTBUFF stdout
#else
#define OUTPUTBUFF PyOutFile
extern FILE* PyOutFile;
#endif




