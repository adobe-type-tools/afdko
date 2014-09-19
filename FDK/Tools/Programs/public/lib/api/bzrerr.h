/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* bezread library error name to error string mapping. See ctlshare.h for a
   description of the CTL_DCL_ERR macro. */

CTL_DCL_ERR(bzrSuccess,				"no error")
CTL_DCL_ERR(bzrErrNoGlyph,      	"glyph does not exist")
CTL_DCL_ERR(bzrErrNoMemory,			"out of memory")
CTL_DCL_ERR(bzrErrSrcStream,		"source stream error")
CTL_DCL_ERR(bzrErrStackUnderflow,	"stack underflow")
CTL_DCL_ERR(bzrErrStackOverflow,	"stack overflow")
CTL_DCL_ERR(bzrErrParse,			"parsing error")
CTL_DCL_ERR(bzrErrParseQuit,		"parsing quit requested")
CTL_DCL_ERR(bzrErrParseFail,		"parsing failed")
