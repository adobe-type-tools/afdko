/* Copyright 2012 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <stdlib.h>
#include <string.h>

#include "smem.h"
#include "slogger.h"

/* Allocate memory */
void *sMemNew(size_t size) {
    void *ptr;
    if (size == 0) size = 4;
    ptr = malloc(size);
    if (ptr == NULL)
        sLogMsg(sFATAL, "out of memory");
    else
        memset((char *)ptr, 0, size);
    return ptr;
}

/* Resize allocated memory */
void *sMemResize(void *old, size_t size) {
    void *new = realloc(old, size);
    if (new == NULL)
        sLogMsg(sFATAL, "out of memory");
    return new;
}

/* Free memory */
void sMemFree(void *ptr) {
    free(ptr);
}
