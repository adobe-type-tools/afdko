/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
  truetype.h
*/

#ifndef TRUETYPE_H
#define TRUETYPE_H

/* #include OTHERINTERFACE */

/* Data Structures */

/* Exported Procedures */
global IntX TTInitBC(PBCRenderProcs rprocs, PBCProcs bcprocs, void* pClientHook);

/* TTInitBC is used to initialize the TrueType renderer. 

rprocs is a pointer to the clients BCRenderProcs structure. This routine
assigns the TrueType rendering procedure pointers to this structure. 
For details on how to call the rendering procedures please see the buildch.h
file in the interface directory of the buildch package. 

bcprocs is a pointer to the client's callback routines. TTInitBC will call
GetMemory to allocate memory used by the TrueType renderer. The
routine rprocs->Terminate will call Free to release this memory.
The memory must not be moved will using the TrueType renderer. If it is 
necessary to move the memory rprocs->Terminate must be called and then
TTInitBC must be called again.

*/
/* Inline Procedures */

/* Exported Data */

#endif /* TRUETYPE_H */
