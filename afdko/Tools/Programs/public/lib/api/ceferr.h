/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* cffread library error name to error string mapping. See ctlshare.h for
   description of CTL_DCL_ERR macro. */

CTL_DCL_ERR(cefSuccess,             "no error")
CTL_DCL_ERR(cefErrNotSVGMode,       "invalid call; not in SVG mode")
CTL_DCL_ERR(cefErrNoMemory,         "out of memory")
CTL_DCL_ERR(cefErrSrcStream,        "source stream error")
CTL_DCL_ERR(cefErrDstStream,        "destination stream error")
CTL_DCL_ERR(cefErrBadPOST,          "bad Macintosh POST resource")
CTL_DCL_ERR(cefErrNoPOST,           "no Macintosh POST resources")
CTL_DCL_ERR(cefErrBadPFB,           "bad Windows PFB segment")
CTL_DCL_ERR(cefErrBadSfnt,          "sfnt doesn't contain required table")
CTL_DCL_ERR(cefErrTTCNoSupport,     "TrueType Collections not supported")
CTL_DCL_ERR(cefErrUnrecFont,        "unrecognized font type")
CTL_DCL_ERR(cefErrBadSpec,          "bad subset specification")
CTL_DCL_ERR(cefErrCffwriteInit,     "can't init cffwrite lib")
CTL_DCL_ERR(cefErrT1readInit,       "can't init t1read lib")
CTL_DCL_ERR(cefErrCffreadInit,      "can't init cffread lib")
CTL_DCL_ERR(cefErrTtreadInit,       "can't init ttread lib")
CTL_DCL_ERR(cefErrSfntreadInit,     "can't init sfntread lib")
CTL_DCL_ERR(cefErrSvgwriteInit,     "can't init sgwrite lib")
CTL_DCL_ERR(cefErrT1Parse,          "Type 1 parse failed")
CTL_DCL_ERR(cefErrCFFParse,         "CFF parse failed")
CTL_DCL_ERR(cefErrTTParse,          "TrueType parse failed")
CTL_DCL_ERR(cefErrSfntParse,        "sfnt parse failed")
CTL_DCL_ERR(cefErrNoGlyph,          "requested glyph not in font")
CTL_DCL_ERR(cefErrNotImpl,          "uniplemented feature")
CTL_DCL_ERR(cefErrCffwriteFont,     "can't write CFF font")
CTL_DCL_ERR(cefErrCantRegister,     "can't register client table")
CTL_DCL_ERR(cefErrSfntwrite,        "can't write sfnt")
CTL_DCL_ERR(cefErrCantHappen,       "can't happen!")

