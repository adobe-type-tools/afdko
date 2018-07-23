/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* uforead library error name to error string mapping. See ctlshare.h for a
   description of the CTL_DCL_ERR macro. */

CTL_DCL_ERR(ufoSuccess,				"no error")
CTL_DCL_ERR(ufoErrNoGlyph,      	"glyph does not exist")
CTL_DCL_ERR(ufoErrNoMemory,			"out of memory")
CTL_DCL_ERR(ufoErrSrcStream,		"source stream error")
CTL_DCL_ERR(ufoErrStackUnderflow,	"stack underflow")
CTL_DCL_ERR(ufoErrStackOverflow,	"stack overflow")
CTL_DCL_ERR(ufoErrParse,			"parsing error")
CTL_DCL_ERR(ufoErrParseQuit,		"parsing quit requested")
CTL_DCL_ERR(ufoErrParseFail,		"parsing failed")
