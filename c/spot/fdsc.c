/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "fdsc.h"
#include "sfnt_fdsc.h"

static fdscTbl *fdsc = NULL;
static int loaded = 0;

void fdscRead(int32_t start, uint32_t length) {
    int i;

    if (loaded)
        return;

    fdsc = (fdscTbl *)sMemNew(sizeof(fdscTbl));
    SEEK_ABS(start);

    /* Read header */
    IN1(fdsc->version);
    IN1(fdsc->nDescriptors);

    /* Read descriptors */
    fdsc->descriptor = sMemNew(sizeof(FontDescriptor) * fdsc->nDescriptors);
    for (i = 0; i < (int)fdsc->nDescriptors; i++) {
        FontDescriptor *desc = &fdsc->descriptor[i];
        IN1(desc->tag);
        IN1(desc->value);
    }

    loaded = 1;
}

void fdscDump(int level, int32_t start) {
    int i;

    DL(1, (OUTPUTBUFF, "### [fdsc] (%08lx)\n", start));

    /* Dump header */
    DLV(2, "version     =", fdsc->version);
    DL(2, (OUTPUTBUFF, "nDescriptors=%u\n", fdsc->nDescriptors));

    /* Dump descriptors */
    DL(2, (OUTPUTBUFF, "--- descriptor[index]={tag,value}\n"));
    for (i = 0; i < (int)fdsc->nDescriptors; i++) {
        FontDescriptor *desc = &fdsc->descriptor[i];
        DL(2, (OUTPUTBUFF, "[%d]={%c%c%c%c,%1.3f (%08x)}\n", i,
               TAG_ARG(desc->tag), FIXED_ARG(desc->value)));
    }
}

void fdscFree(void) {
    if (!loaded)
        return;

    sMemFree(fdsc->descriptor);
    sMemFree(fdsc);
    fdsc = NULL;
    loaded = 0;
}
