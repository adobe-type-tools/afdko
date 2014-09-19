#ifndef MEMCHECK_H
#define MEMCHECK_H

#if DOMEMCHECK

#if defined(MACINTOSH) || defined(macintosh) || defined (OSX) || defined(__MWERKS__)
#define SINGEXPORT __declspec(export)
#define SINGSTDCALL 
#define SINGCALLBACK
#endif


#if WIN32
#define SINGEXPORT __declspec(dllexport)
#define SINGSTDCALL __stdcall
#define SINGCALLBACK __cdecl
#endif

#ifndef SINGLIB_CDECL
#define SINGLIB_CDECL SINGCALLBACK
#endif 

#ifdef __cplusplus
extern "C" {
#endif


typedef enum e_MemCheck_MemStore
{
    memck_store_default = 0,    /* default - calls OS-provided malloc() directly */
    memck_store_app,            /* for mem want to keep for the app's life */
    memck_store_maxMemStore     /* must be the last */
}MemCheck_MemStore;


SINGEXPORT void *memck_mallocinternal(
    int          size, 
    MemCheck_MemStore index,
	char    *filename,
	int     lineno
    );

SINGEXPORT void *memck_callocinternal(
    int          count, 
    int          size,
    MemCheck_MemStore index,
	char    *filename,
	int     lineno
    );

SINGEXPORT void memck_freeinternal(
    void    *ptr,
	char    *filename,
	int     lineno
    );

SINGEXPORT void *memck_reallocinternal(
    void    * ptr, 
    int     size,
	char    *filename,
	int     lineno
    );




/* PUBLIC */
SINGEXPORT int memck_PrintAllAllocFreeCalls;

SINGEXPORT int memck_Startup(char *logfilePathname); /* returns non-0 if failure */


/* PUBLIC
  Use these macros instead of calling the *alloc/free functions above */

#define memck_malloc(x)           memck_mallocinternal((x), memck_store_app, __FILE__, __LINE__)
#define memck_calloc(count, size) memck_callocinternal((count), (size), memck_store_app, __FILE__, __LINE__)


#define memck_free(x)             memck_freeinternal((x), __FILE__, __LINE__)
#define memck_realloc(ptr, size)  memck_reallocinternal((ptr), (size), __FILE__, __LINE__)


#define memck_os_malloc(x)            memck_mallocinternal((x), memck_store_default, __FILE__, __LINE__)
#define memck_os_calloc(count, size)  memck_callocinternal((count), (size), memck_store_default, __FILE__, __LINE__)

/* PUBLIC */
SINGEXPORT void memck_ResetMemLeakDetection(void);

/* PUBLIC */
SINGEXPORT void memck_CheckMemLeaks(void); /* print allocations, and call memck_ResetMemLeakDetection */

/* PUBLIC */
SINGEXPORT void memck_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* DOMEMCHECK */

#endif /* MEMCHECK_H */
