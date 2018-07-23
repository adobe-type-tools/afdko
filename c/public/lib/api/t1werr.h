/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* cfwerr library error name to error string mapping. See ctlshare.h for a
   description of the CTL_DCL_ERR macro. */

CTL_DCL_ERR(t1wSuccess,             "no error")
CTL_DCL_ERR(t1wErrNoMemory,         "out of memory")
CTL_DCL_ERR(t1wErrDstStream,        "destination stream error")
CTL_DCL_ERR(t1wErrTmpStream,        "temporary stream error")
CTL_DCL_ERR(t1wErrSINGStream,       "SING stream error")
CTL_DCL_ERR(t1wErrBadDict,          "bad font dictionary. Missing the PostScript FontName.")
CTL_DCL_ERR(t1wErrBadCIDDict,       "bad font dictionary. Missing either the CID font name, or the Registry-Order-Supplement names.")
CTL_DCL_ERR(t1wErrGlyphType,        "glyph type doesn't match font type")
CTL_DCL_ERR(t1wErrBadCall,          "inappropriate function call or arguments")
CTL_DCL_ERR(t1wErrDupEnc,           "multiple glyphs with the same encoding")
CTL_DCL_ERR(t1wErrNoCID0,           "missing CID 0 glyph")
CTL_DCL_ERR(t1wErrNoGlyphs,         "no glyphs in addition font")
CTL_DCL_ERR(t1wErrBadFDArray,       "invalid FDArray")
