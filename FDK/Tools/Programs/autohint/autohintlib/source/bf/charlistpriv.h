/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* The sole purpose of this interface is to define the types that are
   needed by the two charlist*.c source files, but should not be
   used outside them.  The two source files were split so that just
   the procedure(s) needed for AC were in one of them. */

typedef struct cl_elem
{
  char *charname;
  char *filename;
  boolean composite:1, derived:1, transitional:1,
      inCharTable:1, inBezDir:1;
  short masters; /* number of masters for this char */
  short hintDir; /* which of the master dirs will be used for hints */
}CL_ELEM;


extern long clsize;
extern struct cl_elem *charlist;
