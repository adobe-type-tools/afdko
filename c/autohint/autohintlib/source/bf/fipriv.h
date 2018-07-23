/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* These procedures are private between filookup.c and fiblues.c. */

#include "fipublic.h"

#ifndef FIPRIVATE_H
#define FIPRIVATE_H

/* Builds the undershoot and overshoot band arrays. */
extern int build_bands(
  void
);

/* looks up specified keyword in fontinfo file. */
extern FIPTR filookup(
    char *, boolean
);

/* Frees the value string associated with fiptr. */
extern void fiptrfree(
    FIPTR
);

/* These procs check that band widths and band spacing are within limits. */
extern boolean checkbandwidths(
  float, boolean
);

extern boolean checkbandspacing(
    void
);

extern void WriteBlendedArray(
int *, int, int, char *
);

#endif /*FIPRIVATE_H*/
