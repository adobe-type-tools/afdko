/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. 
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Encoding support.
 */

#ifndef ENCODING_H
#define ENCODING_H

#include "common.h"
#include "font.h"

void encodingNew(tcCtx g);
void encodingFree(tcCtx g);
SID *encodingGetStd(void);
char **encodingGetStdNames(void);
char *encodingGetStdName(int code);
int encodingCheckPredef(SID *enc);
int encodingAdd(tcCtx g, int nCodes, unsigned char *code,
                int nMultiple, CodeMap *supplement);
long encodingFill(tcCtx g);
void encodingWrite(tcCtx g);
Offset encodingGetOffset(tcCtx g, int iEncoding, Offset base);

#endif /* ENCODING_H */
