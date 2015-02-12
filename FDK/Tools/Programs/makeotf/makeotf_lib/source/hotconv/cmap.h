/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef  CMAP_H
#define  CMAP_H

#include "common.h"

#define cmap_   TAG('c', 'm', 'a', 'p')

/* cmap platform/script codes */
/* Unicode */
#define cmap_UNI            0   /* Platform: Unicode */
#define cmap_UNI_UTF16_BMP  3   /* Plat-spec: Unicode 2.0 onwards semantics;
	                               UTF-16 BMP only */
#define cmap_UNI_UTF32      4   /* Plat-spec: Unicode 2.0 onwards semantics;
	                               UTF-32 */

#define cmap_UNI_IVS        5   /* Plat-spec: Unicode Variation Sequences */

#define  UVS_IS_DEFAULT 1 /* flag for UVS entry: indicates its base UV is the default */
#define UVS_IS_SUPPLEMENT 2 /* flag for UVS entry: indicates its base UV is in the supplemental range.*/

/* Macintosh */
#define cmap_MAC            1   /* Platform: Macintosh */

/* Microsoft */
#define cmap_MS             3   /* Platform: Microsoft */
#define cmap_MS_SYMBOL      0   /* Plat-spec enc: Undefined index scheme */
#define cmap_MS_UGL         1   /* Plat-spec enc: UGL */
#define cmap_MS_UCS4        10  /* Plat-spec enc: UCS-4 */

/* Custom */
#define cmap_CUSTOM         4   /* Platform: Custom */

/* Standard functions */
void cmapNew(hotCtx g);
int cmapFill(hotCtx g);
void cmapWrite(hotCtx g);
void cmapReuse(hotCtx g);
void cmapFree(hotCtx g);

/* Supplementary functions */
void cmapBeginEncoding(hotCtx g, unsigned platform, unsigned platformSpecific,
                       unsigned language);
void cmapAddMapping(hotCtx g, unsigned long code, unsigned glyphId,
                    int codeSize);
void cmapAddCodeSpaceRange(hotCtx g,    /* mixed-byte only */
                           unsigned lo, unsigned hi, int codeSize);
int cmapEndEncoding(hotCtx g);

void cmapAddUVSEntry(hotCtx g, unsigned int uvsFlags, unsigned long uv, unsigned long uvs, GID gid);
void cmapEndUVSEncoding(hotCtx g);

void cmapPointToPreviousEncoding(hotCtx g, unsigned platform,
                                 unsigned platformSpecific);

#endif /* CMAP_H */
