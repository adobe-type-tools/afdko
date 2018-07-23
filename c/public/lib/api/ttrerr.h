/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* ttread library error name to error string mapping. See ctlshare.h for
   description of CTL_DCL_ERR macro. */

CTL_DCL_ERR(ttrSuccess,         "no error")
CTL_DCL_ERR(ttrErrCstrQuit,     "client quit glyph parse")
CTL_DCL_ERR(ttrErrCstrFail,     "client failed glyph parse")
CTL_DCL_ERR(ttrErrNoMemory,     "no memory")
CTL_DCL_ERR(ttrErrSrcStream,    "can't read source stream")
CTL_DCL_ERR(ttrErrBadCall,      "call not compatible with font data")
CTL_DCL_ERR(ttrErrVersion,      "unsupported version number")
CTL_DCL_ERR(ttrErrFontType,     "unrecognised font type")
CTL_DCL_ERR(ttrErrNoHead,       "head table missing")
CTL_DCL_ERR(ttrErrNoHhea,       "hhea table missing")
CTL_DCL_ERR(ttrErrNoMaxp,       "maxp table missing")
CTL_DCL_ERR(ttrErrNoLoca,       "loca table missing")
CTL_DCL_ERR(ttrErrLocaFormat,   "unknown loca table format")
CTL_DCL_ERR(ttrErrNoHmtx,       "hmtx table missing")
CTL_DCL_ERR(ttrErrNoGlyph,      "requested glyph not in font")
CTL_DCL_ERR(ttrErrNoComponent,  "component glyph not in font")
CTL_DCL_ERR(ttrErrBadGlyphData, "bad glyph data")
CTL_DCL_ERR(ttrErrNoPoints,     "invalid compound points")
CTL_DCL_ERR(ttrErrDynaLabFont,  "unsupported DynaLab font")
CTL_DCL_ERR(ttrErrSfntread,     "sfntread library error")
CTL_DCL_ERR(ttrErrTooManyPoints, "max points exceeded")
CTL_DCL_ERR(ttrErrTooManyContours, "max contours exceeded")
