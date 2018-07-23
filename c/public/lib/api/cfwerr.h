/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* cfwerr library error name to error string mapping. See ctlshare.h for a
   description of the CTL_DCL_ERR macro. */

CTL_DCL_ERR(cfwSuccess,             "no error")
CTL_DCL_ERR(cfwErrNoMemory,         "out of memory")
CTL_DCL_ERR(cfwErrDstStream,        "destination stream error")
CTL_DCL_ERR(cfwErrTmpStream,        "temporary stream error")
CTL_DCL_ERR(cfwErrBadDict,          "bad dictionary specification")
CTL_DCL_ERR(cfwErrNotImpl,          "unimplemented feature")
CTL_DCL_ERR(cfwErrNoNotdef,         "missing .notdef glyph")
CTL_DCL_ERR(cfwErrNoCID0,           "missing CID 0 glyph")
CTL_DCL_ERR(cfwErrGlyphType,        "glyph type doesn't match font type")
CTL_DCL_ERR(cfwErrGlyphPresent,     "identical charstring already present")
CTL_DCL_ERR(cfwErrGlyphDiffers,     "different charstring already present")
CTL_DCL_ERR(cfwErrBadFDArray,       "invalid FDArray")
CTL_DCL_ERR(cfwErrCstrTooLong,      "charstring too long")
CTL_DCL_ERR(cfwErrStackOverflow,     "stack overflow")
