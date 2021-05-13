/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_HHEA_H
#define HOTCONV_HHEA_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define hhea_ TAG('h', 'h', 'e', 'a')

/* Standard functions */
void hheaNew(hotCtx g);
int hheaFill(hotCtx g);
void hheaWrite(hotCtx g);
void hheaReuse(hotCtx g);
void hheaFree(hotCtx g);

/* Supplementary functions */
void hheaSetCaretOffset(hotCtx g, short caretOffset);

#ifdef __cplusplus
}
#endif

#endif /* HOTCONV_HHEA_H */
