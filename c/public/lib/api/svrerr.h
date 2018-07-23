/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* svwerr library error name to error string mapping. See ctlshare.h for a
   description of the CTL_DCL_ERR macro. */

CTL_DCL_ERR(svrSuccess,				"no error")
CTL_DCL_ERR(svrErrNoGlyph,      	"glyph does not exist")
CTL_DCL_ERR(svrErrNoMemory,			"out of memory")
CTL_DCL_ERR(svrErrSrcStream,		"source stream error")
CTL_DCL_ERR(svrErrStackUnderflow,	"stack underflow")
CTL_DCL_ERR(svrErrStackOverflow,	"stack overflow")
CTL_DCL_ERR(svrErrParse,			"parsing error")
CTL_DCL_ERR(svrErrParseQuit,		"parsing quit requested")
CTL_DCL_ERR(svrErrParseFail,		"parsing failed")
