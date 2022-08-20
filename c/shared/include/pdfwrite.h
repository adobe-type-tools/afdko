/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_PDFWRITE_H_
#define SHARED_INCLUDE_PDFWRITE_H_

#include "ctlshare.h"

#define PDW_VERSION CTL_MAKE_VERSION(1, 0, 7)

#include "absfont.h"

#if defined(__cplusplus) && !defined(STRIP_EXTERN_C)
extern "C" {
#endif

typedef struct pdwCtx_ *pdwCtx;
pdwCtx pdwNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
              CTL_CHECK_ARGS_DCL);

#define PDW_CHECK_ARGS CTL_CHECK_ARGS_CALL(PDW_VERSION)

int pdwBegFont(pdwCtx h, long flags, long level, abfTopDict *top);

enum {
    PDW_FLIP_TICS = 1 << 0
};

int pdwEndFont(pdwCtx h);

extern const abfGlyphCallbacks pdwGlyphCallbacks;

int pdwFree(pdwCtx h);

enum {
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name, string) name,
#include "pdwerr.h"
    pdwErrCount
};

/* Library functions return either zero (pdwSuccess) to indicate success or a
   positive non-zero error code that is defined in the above enumeration that
   is built from pdwerr.h. */

const char *pdwErrStr(int err_code);

/* pdwErrStr() maps the "err_code" parameter to a null-terminated error 
   string. */

void pdwGetVersion(ctlVersionCallbacks *cb);

/* pdwGetVersion() returns the library version number and name via the client
   callbacks passed with the "cb" parameter (see ctlshare.h). */

#if defined(__cplusplus) && !defined(STRIP_EXTERN_C)
}
#endif

#endif  // SHARED_INCLUDE_PDFWRITE_H_
