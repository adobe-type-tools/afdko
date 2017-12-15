/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef DYNARR_H
#define DYNARR_H

#include "ctlshare.h"

#define DNA_VERSION CTL_MAKE_VERSION(2,0,3)

#ifdef __cplusplus
extern "C" {
#endif

/* Dynamic Array Library
   =====================
   The dynamic array library provides simple and flexible support for
   homogeneous arrays that automatically grow to accommodate new elements. 

   Dynamic arrays or da's are particularly useful in situations where the size
   of the array isn't known at compile or run time until the last element has
   been stored, and no suitable default size can be determined. Such situations
   occur, for example, when data is being read from a file and loaded into an
   array. The traditional way of handling this situation has been to make
   multiple passes over the data but this can be expensive, especially if
   parsing is complicated. Dynamic arrays can provide a cheaper alternative.

   da's are also useful in situations where the same array is used repeatedly
   with different data sets. The traditional way of handling this situation has
   been to allocate and deallocate memory for each data set but this can also
   be expensive. Dynamic arrays can simply be reused with each data set and no
   further allocation becomes necessary after the largest data set has been
   encountered.

   da's can also lead to a different programming style where programs no longer
   have to handle unusually large data sets (normally by fatal error) but
   rather "just work" irrespective of the data. This is particularly useful
   when working with poorly specified data sets.

   A da is implemented as a C structure that contains a pointer to a
   dynamically allocated array plus a few fields to control (re)allocation, and
   optional initialization, of elements of that array. da's are created by
   declaring them with the dnaDCL() macro described below.

   Once a da has been declared it must be initialized by the dnaINIT() or
   dnaINIT_FIRST() macros. This process associates values chosen by the
   client with the da that are used to control subsequent allocation.

   After initialization the da is accessed using a number of macros described
   below. These check if the desired index is within the array and, if not, the
   array is grown (reallocated) to accommodate it. The possibility of
   reallocation means that the array may be moved in memory outside of client
   control and it is therefore NOT permitted to save element POINTERS during
   the time that the da is growing. A client can, however, save element INDEXES
   reliably since these aren't dependent on memory addresses.

   After the da has been grown and loaded with data it may be accessed
   without restriction.

   Finally, when a client has finished with a da it is freed by a call to
   dnaFREE(). 
   
   The following describes the interface to the library. */

typedef struct dnaCtx_ *dnaCtx;

#define dnaDCL(type,da) \
struct \
    { \
    dnaCtx ctx; \
    type *array; \
    long cnt; \
    long size; \
    long incr; \
    void (*func)(void *ctx, long cnt, type *base); \
    } da

/* dnaDCL() declares a dynamic array object with elements of type "type". */

dnaCtx dnaNew(ctlMemoryCallbacks *mem_cb, CTL_CHECK_ARGS_DCL);

#define DNA_CHECK_ARGS CTL_CHECK_ARGS_CALL(DNA_VERSION)

/* dnaNew() initializes the library and returns an opaque context that is
   subsequently passed to dnaINIT() or dnaINIT_FIRST(). It must be the first
   function in the library to be called by the client. The client passes a set
   memory callback functions (described in ctlshare.h) to the library via the
   mem_cb parameters.

   The DNA_CHECK_ARGS macro is passed as the last parameter to dnaNew() and is
   concerned with checking client call compatibility with the library's version
   and interface structure sizes. If a mismatch is found dnaNew() returns NULL
   to indicate the initialization failed.

   A client uses the dynamic array library almost exclusively through the
   macros described below. Macros are necessary because the C language doesn't
   permit types to be manipulated flexibly enough for this implementation. 

   The macro interface to the library doesn't handle memory allocation
   failures, but since clients normally supply memory callback functions that
   handle allocation errors by setjmp/longjmp, and therefore never return NULL,
   this isn't a problem.

   However, sometimes the overhead of setjmp is too high for the task being
   performed. In such cases the client can choose to supply memory callback
   functions that don't handle allocation errors and can return NULL. The
   client must then use the dnaGrow() function to access the da (see below). */

void dnaInit(dnaCtx ctx, void *object, size_t init, size_t incr, int check);

#define dnaINIT(ctx,da,init,incr) \
    dnaInit(ctx,(void *)(&(da)),init,incr,0)

/* dnaINIT() initializes the dynamic array before its first use. The "init"
   parameter specifies the initial number of elements to be allocated to the
   array when first accessed. The "incr" parameter specifies the number of
   additional elements allocated to the array on subsequent accesses. Note
   that indexes that exceed the init or init+incr sizes are accomodated by
   allocating to the next multiple of the incr size that accomodates the index.

   Sometimes it is necessary to initialize the the newly created elements when
   they are first allocated. This can be achieved by attaching an element
   initializer function to the da by explicitly initializing the "func" field
   of the da object with the address of the initializer function after the
   calling dnaINIT() or dnaINIT_FIRST() and before first use of the da. 

   The initializer function's "cnt" parameter specifies the number of elements
   to be initialized and the "base" parameter (whose type must match the da
   declaration) specifies the address of the first element in the sub-array to
   be initialized. The client context is also made available via the "ctx"
   parameter and is simply a copy of the client context provided to the dna
   library in ctlMemoryCallbacks.ctx field. */

#define dnaINIT_FIRST(ctx,da,init,incr) \
    dnaInit(ctx,(void *)(&(da)),init,incr,1)

/* dnaINIT_FIRST() is identical to dnaINIT() except that it will only
   initialize the dynamic array on its first call. */

long dnaSetCnt(void *object, size_t elemsize, long cnt);
long dnaGrow(void *object, size_t elemsize, long index);
long dnaIndex(void *object, size_t elemsize, long index);
long dnaMax(void *object, size_t elemsize, long index);
long dnaNext(void *object, size_t elemsize);
long dnaExtend(void *object, size_t elemsize, long length);

/* These functions are used to grow a da according to different access schemes
   that suit a wide variety of usages. In all functions the object parameter is
   the address of the da to be accessed and the element parameter specifies the
   size (in bytes) of an element in the da.

   In normal operation these functions behave as follows.

   function         arg     action                          normal return
   ---------------  ------  ------------------------------  -----------------
   dnaSetCnt        cnt     da.cnt=cnt                      0
   dnaGrow          index   -                               0
   dnaIndex         index   -                               index
   dnaMax           index   if(index>=da.cnt)da.cnt=index+1 index
   dnaNext          -       da.cnt++                        da.cnt
   dnaExtend        length  da.cnt+=length                  da.cnt

   The fuctions return -1 if memory allocation failure occurs, that is if
   either the malloc or realloc functions return NULL.

   These function are normally used indirectly through the macros described
   below. Direct usage is only necessary when the memory callback functions can
   return NULL to indicate failure rather than handling the failure within the
   callback, for example by longjmp() or exit(). */

#define DNA_DA_ADDR_(da) \
	((void*)(&(da)))

#define DNA_ELEM_SIZE_(da) \
	(sizeof((da).array[0]))

#define DNA_ELEM_ADDR_(da,i) \
	(&(da).array[(i)])

#define dnaSET_CNT(da,n) \
    (void)dnaSetCnt(DNA_DA_ADDR_(da),DNA_ELEM_SIZE_(da),(n))

/* dnaSET_CNT() grows the array to accommodate a total of "n" elements and
   sets the da.cnt field to "n". */

#define dnaGROW(da,i) \
    ((void)dnaGrow(DNA_DA_ADDR_(da),DNA_ELEM_SIZE_(da),(i)),(da).array)

/* dnaGROW() grows the array to accommodate the "i"th element and returns
   the address of the first element of the array. The da.cnt field is
   unaltered. */

#define dnaINDEX(da,i) \
    ((void)dnaIndex(DNA_DA_ADDR_(da),DNA_ELEM_SIZE_(da),(i)),DNA_ELEM_ADDR_((da),(i)))

/* dnaINDEX() grow the array to accommodate the "i"th element and returns
   its address. The da.cnt field is unaltered. */

#define dnaMAX(da,i) \
    ((void)dnaMax(DNA_DA_ADDR_(da),DNA_ELEM_SIZE_(da),(i)),DNA_ELEM_ADDR_((da),(i)))

/* dnaMAX() is the same as dnaINDEX() except that the da.cnt field is set to
   be one greater than the maximum index accomodated. */

#define dnaNEXT(da) \
    ((void)dnaNext(DNA_DA_ADDR_(da),DNA_ELEM_SIZE_(da)),DNA_ELEM_ADDR_((da),(da).cnt-1))

/* dnaNEXT() grows the array to accommodate the next element (da.cnt) and
   returns its address. The da.cnt field is incremented by 1. */

#define dnaEXTEND(da,len) \
    ((void)dnaExtend(DNA_DA_ADDR_(da),DNA_ELEM_SIZE_(da),(len)),DNA_ELEM_ADDR_((da),(da).cnt-(len)))
	 
/* dnaEXTEND() extends the dynamic array by "len" elements and returns the
   address of the first element of the extension. The da.cnt fields is
   increment by "len". */

void dnaFreeObj(void *object);
#define dnaFREE(da) (dnaFreeObj)(DNA_DA_ADDR_(da))

/* dnaFREE() frees resources allocated to the dynamic array. It may be
   safely applied to dynamic array objects that have been declared and
   initialized but never used. */

void dnaSetDebug(dnaCtx h, int level);

void dnaFree(dnaCtx h);

/* dnaFree() destroys the library context and all the resources allocated to
   it. */

void dnaGetVersion(ctlVersionCallbacks *cb);

/* dnaGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). This*/

/* ------------------------ Discussion and Examples ------------------------

   Declaration
   ===========
   da's are declared with the dnaDCL() macro:

     dnaDCL(int, integers);
   
   Declares a da called "integers" with elements of type "int".

     dnaDCL(char *, strings);

   Declares a da called "strings" with elements of type "char *".

     typedef struct
         {
         float x
         float y
         } Coord;
     dnaDCL(Coord, coords);

   Declares a da called "coords" with elements of type "Coord".

     typedef dnaDCL(char, GrowBuf);
     GrowBuf tmp;
   
   Declares a type called "GrowBuf" that is a da with elements of type "char"
   and defines an object called "tmp" of this type. This could be used as a
   temporary char buffer for string manipulation, for example.

   Initialization
   ==============
   Once they have been declared, da's must be initialized before being used by
   calling the dnaINIT() or dnaINIT_FIRST() macros:

     dnaINIT(dactx, coords, 1000, 500);

   Initializes the "coords" da with an initial size of 1000 elements and an
   increment size of 500 elements. The "dactx" parameter is the pointer
   returned dnaNew() when the library was initialized.

   If the code fragment is executed multiple times, for example with multiple
   data sets, the initialization must only occur the first time otherwise the
   previously allocated array will be leaked. First time initialization can be
   conveniently handled by calling the dnaINIT_FIRST() macro in place of
   dnaINIT():

     dnaINIT_FIRST(dactx, coords, 1000, 500);

   Sometimes it is necessary to initialize the the newly created elements when
   they are first allocated. This can be achieved by attaching an element
   initializer function (of matching type) to the da:

     void initStrings(void *ctx, long cnt, char **base)
       {
       while (cnt--)
           *base++ = NULL;
       }
     ...
     dnaINIT(dactx, strings, 250, 50);
     strings.func = initStrings;

   Growing and Loading
   =================== 
   Once a da has been declared and initialized it may be accessed so that
   elements of the underlying array can be loaded. This is accomplished by a
   number of macros that are tailored to particular types of access.

   The simplest of these is dnaSET_CNT() which is used to grow the array to the
   specified size and set the cnt field to that size. This would normally be
   used for multiple data sets whose size was known in advance. The array is
   then simply reused for each data set.

   Next in complexity is dnaGROW() which simply grows the array to accommodate
   an index and returns the address of the first element of the array. This can
   be useful when inserting an element into the middle of a da, for example
   when implementing an insertion sort.

   Random elements in the da can be loaded using dnaINDEX(). This grows the
   array to accommodate the index and returns a pointer to the indexed element.
   This might be used when loading a sparse array, but in this case it is
   imperative that an element initializer is specified so that elements may be
   suitably initialized. If this isn't done there will be no reliable way of
   determining which elements were loaded.

   dnaMAX() is a variation on dnaINDEX() that records one greater than the
   highest index in the da.cnt field which may be used to limit the subsequent
   search for loaded elements.

   A common requirement is to load the array sequentially, starting at index 0
   and ending when the data is exhausted. This is easily achieved using
   dnaNEXT() which returns a pointer to the next element and increments da.cnt
   automatically. The client must initialize da.cnt to 0 (or any other suitable
   starting index) before calling dnaNEXT(). The following example fills the
   strings da with strings returned from the nextString() function which
   returns NULL when its supply of strings is exhausted.

     char *string;
     strings.cnt = 0;
     while ((string = nextString()) != NULL)
         *dnaNEXT(strings) = string;

   Finally, dnaEXTEND() is used to add a specified number of elements to the
   end of the array and returns the address of the first element of this
   extension. This function is useful for appending strings to a buffer
   as shown in the example below:

     char *string;
     tmp.cnt = 0;
     while ((string = nextString()) != NULL)
         {
         size_t len = strlen(string);
         memmove(dnaEXTEND(tmp, len), string, len);
         }

   Note: strings have null termination removed in this example.

   Access
   ======
   Once the da is loaded the elements in the array may be accessed using
   conventional techniques and even the addresses of elements can be used
   since the array will not be reallocated again. Generally, the da.cnt field
   records the number of elements in the array so that the following code may
   be used to find the maximum element in an array of integers. For brevity
   this example assumes that there is at least 1 element in the array.

     long i;
     long max = integers.array[0];
     for (i = 1; i < integers.cnt; i++)
         if (integers.array[i] > max)
             max = integers.array[i];

   The following example sorts the string da after it is loaded.

     int cmpStrings(const void *first, const void *second)
         {
         return strcmp(*(char**)first, *(char **)second);
         }
     ...
     qsort(strings.array, strings.cnt, sizeof(char *), cmpStrings);

 */

#ifdef __cplusplus
}
#endif

#endif /* DYNARR_H */
