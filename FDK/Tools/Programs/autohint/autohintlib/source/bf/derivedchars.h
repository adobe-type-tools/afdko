/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* derivedchars.h */

#ifndef DERIVEDCHARS_H
#define DERIVEDCHARS_H

#define DERIVEDCHARFILENAME "derivedchars"

extern void free_derivedtables(
boolean, indx
);

extern char **make_derivedchars(
boolean, short *, indx
);

extern void readderived (
boolean, indx
);

extern void CheckDerivedFileSizes(
int, int
);

#endif 
