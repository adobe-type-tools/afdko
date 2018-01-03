/* @(#)CM_VerSion ASZone.h atm09 1.2 16563.eco sum= 34263 atm09.004 */
/* @(#)CM_VerSion ASZone.h atm08 1.4 16340.eco sum= 30616 atm08.005 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* ASZone.h -- Storage Management Utility Objects (defs only)
 *
 *
 * BEWARE: Compilation parameter settings must be the same for all
 *         code that #includes this header file and will get linked
 *         into the same application, e.g. product code as well as
 *         all separately-built libraries.
 *
 * REQUIRED COMPILATION PARAMETERS:
 *         It is assumed that the following are defined when this is compiled:
 *           AS_ISP
 *           AS_OS
 *
 * OPTIONAL COMPILATION PARAMETERS:
 *         ASBASIC -- if this is defined it is included rather than "ASBasic.h"
 *
 * UNLESS YOUR APPLICATION IS AN ADOBE POSTSCRIPT PRODUCT, BE SURE TO
 * call ASZoneInitialize to establish sysASZone before attempting
 * to allocate any storage from it. DO NOT call ASZoneInitialize in an
 * Adobe PostScript product.
 *
 * NOTE: Some ASZone methods should be defined for every ASZone, and some are
 * optional, as specified below. The proposed convention for optional methods
 * is that NULL should be the value of the procedure pointer if the ASZone
 * does not provide the method.
 *
 * ASZone creation procedures should specify which of the optional methods
 * are provided (i.e., are non-NULL).
 *
 * Procedures that take an ASZone as a parameter should specify which of
 * the optional methods must be provided by the ASZone.
 *
 */

#ifndef _H_ASZONE
#define _H_ASZONE

#include "supportasbasic.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ASZone pointer type */
typedef struct _t_ASZoneRec *ASZone;


/* Concrete ASZone representation ... */

/* ASZone Methods */
typedef void * (*ASMallocFcn)(ASZone z, ASSize_t nBytes);
  /*  REQUIRED

      Allocates nBytes consecutive bytes of memory and returns a pointer
      to the first byte.

      The pointer returned is guaranteed to be aligned so that it may be
      used to store any C data type.

      The initial contents of the allocated memory are undefined.

      This returns NULL if z is unable to allocate the requested memory.
   */

typedef void * (*ASCallocFcn)(ASZone z, ASSize_t count, ASSize_t nBytes);
  /*  REQUIRED

      Allocates memory for an array of count elements, each of which occupies
      the specified number of bytes.
    
      The pointer returned is guaranteed to be aligned so that it may be
      used to store any C data type.
    
      The initial contents of the allocated memory are set to zero.
    
      This returns NULL if z is unable to allocate the requested memory.
   */

typedef void   (*ASFreeFcn)(ASZone z, void * ptr);
  /*  REQUIRED

      "ptr" addresses memory previously allocated by z. This frees the
      memory, making it available for subsequent re-use.
   */

typedef void * (*ASReallocFcn)(ASZone z, void * ptr, ASSize_t nBytes);
  /*  OPTIONAL

      Takes a pointer to memory previously allocated and changes
      its size, relocating it if necessary.
    
      Returns a pointer to the new memory, or NULL if the request could
      not be satisfied. If NULL is returned, ASZReallocFcn leaves the
      existing memory undisturbed.
    
      The memory contents within the minimum of the old and new sizes are
      preserved. If the new size is larger than the old, the extension has
      undefined contents. If the memory must be relocated, the old memory
      block is freed (hence the old pointer becomes invalid).
    
      Note: if the memory was originally allocated by ASZMemAlignFcn,
	  the original alignment is not necessarily preserved.
   */

typedef void * (*ASMemAlignFcn)(
	ASZone z, ASSize_t boundary, ASSize_t nBytes
	);
  /*  OPTIONAL

      Like ASMallocFcn, except that the pointer returned is aligned
      to a multiple of boundary, which must be a power of 2.
   */

/* ASZoneProcs Object (BEWARE: this may be subclassed!) */
typedef struct _t_ASZoneProcsRec *ASZoneProcs;
typedef struct _t_ASZoneProcsRec {
  ASMallocFcn   Malloc;       /* Failure (to allocate) returns NULL */
  ASCallocFcn   Calloc;       /* Failure returns NULL */
  ASFreeFcn     Free;

  ASMallocFcn   SureMalloc;   /* Failure raises ecLimitCheck */
  ASCallocFcn   SureCalloc;   /* Failure raises ecLimitCheck */

  ASReallocFcn  Realloc;      /* OPTIONAL (maybe NULL); Failure returns NULL */
  ASReallocFcn  SureRealloc;  /* OPTIONAL; Failure raises ecLimitCheck */

  ASMemAlignFcn MemAlign;     /* OPTIONAL; Failure returns NULL */
  ASMemAlignFcn SureMemAlign; /* OPTIONAL; Failure raises ecLimitCheck */
} ASZoneProcsRec;

/* ASZone Object (BEWARE: this may be subclassed!) */
typedef struct _t_ASZoneRec {
  ASZoneProcs procs;
} ASZoneRec;

/* Convenience definitions for invoking ASZone methods.
   BEWARE that in each the z argument is evaluated twice.
 */
#define ASZMalloc(z, nb) (*(z)->procs->Malloc)(z, nb)
#define ASZCalloc(z, ct, nb) (*(z)->procs->Calloc)(z, ct, nb)
#define ASZFree(z, p) (*(z)->procs->Free)(z, p)

#define ASZSureMalloc(z, nb) (*(z)->procs->SureMalloc)(z, nb)
#define ASZSureCalloc(z, ct, nb) (*(z)->procs->SureCalloc)(z, ct, nb)

#define ASZRealloc(z, p, nb) (*(z)->procs->Realloc)(z, p, nb)
#define ASZMemAlign(z, bd, nb) (*(z)->procs->MemAlign)(z, bd, nb)

#define ASZSureRealloc(z, p, nb) (*(z)->procs->SureRealloc)(z, p, nb)
#define ASZSureMemAlign(z, bd, nb) (*(z)->procs->SureMemAlign)(z, bd, nb)


/* Generic implementations of certain ASZone methods, usable by
   ASZone implementations that have no special needs. These may be
   assigned to the appropriate fields of the ASZoneProcs structure.
 */

extern void * ASStdCalloc(ASZone z, ASSize_t count, ASSize_t nBytes);
  /* This is a generic implementation of the "calloc" method.
     ASStdCalloc invokes the malloc method of z (for count*nBytes bytes)
     and clears the resulting memory. Returns NULL if the malloc method
     returns NULL.
   */

extern void * ASStdSureMalloc(ASZone z, ASSize_t nBytes);
  /* This is a generic implementation of the "suremalloc" method.
     ASStdSureMalloc invokes the malloc method of z and raises
     ecLimitCheck if it returns NULL.
   */

extern void * ASStdSureCalloc(ASZone z, ASSize_t count, ASSize_t nBytes);
  /* This is a generic implementation of the "surecalloc" method.
     ASStdSureCalloc invokes the malloc method of z (for count*nBytes bytes)
     and clears the resulting memory. Raises ecLimitCheck if the malloc method
     returns NULL.
   */

extern void * ASStdSureRealloc(ASZone z, void * ptr, ASSize_t nBytes);
  /* This is a generic implementation of the "surerealloc" method.
     ASStdSureRealloc invokes the realloc method of z.
     Raises ecLimitCheck if the realloc method is NULL or returns NULL.
   */

extern void * ASStdSureMemAlign(
	ASZone z, ASSize_t boundary, ASSize_t nBytes
	);
  /* This is a generic implementation of the "surememalign" method.
     ASStdSureMemAlign invokes the memalign method of z.
     Raises ecLimitCheck if the memalign method is NULL or returns NULL.
   */

/* A standard ASZone object */
extern ASZone sysASZone;
  /* Valid only after the call on ASZoneInitialize */

extern void ASZoneInitialize(ASZone zone);
  /* Call this to initialize the storage management package.
  
     zone becomes the value of sysASZone.
   */

#ifdef __cplusplus
}
#endif

#endif /* _H_ASZONE */
