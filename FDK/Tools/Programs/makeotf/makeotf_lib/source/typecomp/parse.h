/* @(#)CM_VerSion parse.h atm08 1.2 16245.eco sum= 30201 atm08.002 */
/* @(#)CM_VerSion parse.h atm07 1.2 16164.eco sum= 26270 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * PostScript font file parser.
 */

#ifndef PARSE_H
#define PARSE_H

#include "common.h"
#include "font.h"

void parseNew(tcCtx g);
void parseFree(tcCtx g);

void parseFont(tcCtx g, Font *font);
int parseAddComponent(tcCtx g, int code);
int parseGetComponent(tcCtx g, int code,
                      unsigned *length, unsigned char **cstr);
Font *parseGetFont(tcCtx g);
void CDECL parseNewGlyphReport(tcCtx g, char *fmt, ...);
void CDECL parseWarning(tcCtx g, char *fmt, ...);
void CDECL parseFatal(tcCtx g, char *fmt, ...);

#if TC_EURO_SUPPORT
double parseGetItalicAngle(tcCtx g);

#endif /* TC_EURO_SUPPORT */

#endif /* PARSE_H */