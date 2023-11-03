/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

/*
 * Handles encodings
 */

#ifndef ADDFEATURES_HOTCONV_HOTMAP_H_
#define ADDFEATURES_HOTCONV_HOTMAP_H_

#include "common.h"

/* Standard functions */
void mapNew(hotCtx g);
int mapFill(hotCtx g);
void mapReuse(hotCtx g);
void mapFree(hotCtx g);

/* Supplementary functions */
void mapApplyReencoding(hotCtx g, hotEncoding *comEnc, hotEncoding *macEnc);
void mapAddCMap(hotCtx g, hotCMapId id, hotCMapRefill refill);
void mapAddUVS(hotCtx g, const char *uvsName);
void mapMakeKern(hotCtx g);
void mapMakeVert(hotCtx g);
void mapPrintAFM(hotCtx g);

/* Conversion functions */

/* mapUV2Glyph() Caution: the uv requested could be in either the uv or the
   uvSup field of the returned hotGlyphInfo */
hotGlyphInfo *mapUV2Glyph(hotCtx g, UV uv);
GID mapUV2GID(hotCtx g, UV uv);

hotGlyphInfo *mapName2Glyph(hotCtx g, const char *gname, const char **useAliasDB);
GID mapName2GID(hotCtx g, const char *gname, const char **useAliasDB);

void mapGID2Name(hotCtx g, GID gid, char *msg, int msglen);

hotGlyphInfo *mapCID2Glyph(hotCtx g, CID cid);
GID mapCID2GID(hotCtx g, CID cid);

#define mapGID2CID(gid) (g->glyphs[gid].id)

UV mapWinANSI2UV(hotCtx g, int code);
hotGlyphInfo *mapWinANSI2Glyph(hotCtx g, int code);
GID mapWinANSI2GID(hotCtx g, int code);

hotGlyphInfo *mapPlatEnc2Glyph(hotCtx g, int code);
GID mapPlatEnc2GID(hotCtx g, int code);

#endif  // ADDFEATURES_HOTCONV_HOTMAP_H_