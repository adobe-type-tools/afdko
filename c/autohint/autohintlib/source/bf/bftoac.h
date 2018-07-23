/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* bftoac.h */

/* procedures in AC called from buildfont */

#ifndef BFTOAC_H
#define BFTOAC_H

#include "memcheck.h"

extern boolean AutoColor(boolean, boolean, boolean, boolean, boolean, short, char *[], boolean, boolean, boolean, boolean);

extern boolean CreateACTimes (void);
typedef void (*tConvertfunc)(const char *, const char *);
extern boolean ConvertCharFiles(char *, boolean, float, tConvertfunc);

extern void FindCurveBBox(Fixed, Fixed, Fixed, Fixed, Fixed, Fixed, Fixed, Fixed, Fixed *, Fixed *, Fixed *, Fixed *);

extern boolean GetInflectionPoint(Fixed, Fixed, Fixed, Fixed, Fixed, Fixed, Fixed, Fixed, Fixed *);

extern boolean ReadCharFileNames (char *, boolean *);

extern void setPrefix(char *);

extern void SetReadFileName(char *);

#endif /*BFTOAC_H*/
