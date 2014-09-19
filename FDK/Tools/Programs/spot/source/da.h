/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)da.h	1.3
 * Changed:    11/16/98 10:18:09
 ***********************************************************************/

#ifndef DA_H
#define DA_H

#include <stdlib.h>

/* Dynamic array support.

Overview
========
The da (dynamic array) library provides simple and flexible support for
homogeneous arrays that automatically grow to accomodate new elements. da's are
particularly useful in situations where the size of the array isn't known at
compile or run time until the last element has been stored and no suitable
default size can be determined. Such situations occur, for example, when data
is being read from a file and loaded into an array. 

The da object
=============
A da is implemented as a C stucture that contains a pointer to a dynamically
allocated array and a few fields to control (re)allocation, and optional
initialization, of elements of that array.

struct
    {
    <type> *array;
    long cnt;
    unsigned long size;
    unsigned long incr;
    int (*init)(<type> *element);
	} <name>;

Field							Description
-----							-----------
<type> *array					This is a pointer to the first element of a
								dynamically allocated array. Each element has
								type <type> which may be any C data type.

long cnt						This is a count of the number of elements of
								the array that are in use which is also the
								index of the next free element of the array.

unsigned long size				This is the total number of elements avilable
								in the array. 

unsigned long incr				This is the number of elements by which the
								array grows in order to accomodate a new index.

int (*init)(<type> *element)	This is the address of a client-supplied
								function that initializes new elements of the
								array.

[Note: <name> and <type> are supplied by the client program via declaration
macros.]

[This needs finishing--JH]

Error Handling
==============

Library Initialization
======================
The da library must be initialized before any da is accessed. This is

Declaration
===========
A client may
A da may be declared 

In order to use a da object a client program must perform 4 steps:

1. Initialize the da module.
2. Declare an initialize a da object.
3. Add data to the da object via one of the 4 access macros.
4. Use the data in the da object.

 */


/* Functions:
 *
 * da_Init			Only call via da_INIT or da_INIT_ONCE
 * da_Grow			Only call via da_GROW, da_INDEX, or da_NEXT
 * da_Free			Only call via da_FREE
 * da_SetMemFuncs	Set memory management functions (initializes da module)
 */
extern void da_Init(void *object, unsigned long intl, unsigned long incr);
extern void da_Grow(void *object, size_t element, unsigned long index);
extern void da_Free(void *object);
extern void da_SetMemFuncs(void *(*alloc)(size_t size),
						   void *(*resize)(void *old, size_t size),
						   void (*dealloc)(void *ptr));

/* Creation/destruction:
 *
 * da_DCL	Declare da object (initialize with da_INIT or da_INIT_ONCE)
 * da_DCLI	Declare and initialize da object
 * da_FREE	Free array
 */
#define da_DCL(type,da) \
struct \
    { \
	type *array; \
	long cnt; \
	long size; \
	long incr; \
	int (*init)(type *element); \
	} da
#define da_DCLI(type,da,intl,incr) da_DCL(type,da)={(type *)intl,0,0,incr,NULL}
#define da_FREE(da) da_Free(&(da))

/* Initialization:
 *
 * da_INIT		Unconditional initialization
 * da_INIT_ONCE	Conditional initialization
 */
#define da_INIT(da,intl,incr) da_Init((void *)(&(da)),intl,incr)
#define da_INIT_ONCE(da,intl,incr) \
    do{if((da).size==0)da_Init((void *)(&(da)),intl,incr);}while(0)

/* Access:
 *
 * da_GROW  	Grow da enough to accommodate index and return array
 * da_INDEX		Grow da, return pointer to indexed element
 * da_NEXT		Grow da, return pointer to next element and bump count
 * da_EXTEND	Grow da, return pointer to next element and extend count
 */
#define da_GROW(da,inx) ((inx)>=(da).size? \
    (da_Grow((void *)(&(da)),sizeof((da).array[0]),inx), \
	 (da).array):(da).array)

#define da_INDEX(da,inx) (&da_GROW(da,inx)[inx])

#define da_NEXT(da) (((da).cnt)>=(da).size? \
    (da_Grow((void *)(&(da)),sizeof((da).array[0]),(da).cnt),\
	 &(da).array[(da).cnt++]):&(da).array[(da).cnt++])

#define da_EXTEND(da,len) (((da).cnt+(len)-1)>=(da).size? \
    (da_Grow((void *)(&(da)),sizeof((da).array[0]),(da).cnt+(len)-1), \
	 &(da).array[((da).cnt+=(len),(da).cnt-(len))]): \
	 &(da).array[((da).cnt+=(len),(da).cnt-(len))])

#endif /* DA_H */
