/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Dictionary data.
 */

#ifndef DICT_H
#define DICT_H

#include "cffwrite_share.h"

/* DICT data buffer */
typedef dnaDCL (char, DICT);

/* Compute DICT op size */
#define DICT_OP_SIZE(op) (((op) & 0xff00) ? 2 : 1)

void cfwDictNew(cfwCtx g);
void cfwDictReuse(cfwCtx g);
void cfwDictFree(cfwCtx g);

void cfwDictCopyTop(cfwCtx g, abfTopDict *dst, abfTopDict *src);
void cfwDictCopyFont(cfwCtx g, abfFontDict *dst, abfFontDict *src);
void cfwDictCopyPrivate(cfwCtx g, abfPrivateDict *dst, abfPrivateDict *src);

void cfwDictFillTop(cfwCtx g, DICT *dst,
                    abfTopDict *top, abfFontDict *font, long iSyntheticBase);
void cfwDictFillFont(cfwCtx g, DICT *dst, abfFontDict *src);
void cfwDictFillPrivate(cfwCtx g, DICT *dst, abfPrivateDict *src);

void cfwDictSaveInt(DICT *dict, long i);
void cfwDictSaveReal(DICT *dict, float r);
void cfwDictSaveOp(DICT *dict, int op);
void cfwDictSaveIntOp(DICT *dict, long i, int op);
void cfwDictSaveRealOp(DICT *dict, float r, int op);

#endif /* DICT_H */
