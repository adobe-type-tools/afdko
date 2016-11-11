/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* cffcstr library error name to error string mapping. See ctlshare.h for a
   description of the CTL_DCL_ERR macro. */

CTL_DCL_ERR(t2cSuccess,             "no error")
CTL_DCL_ERR(t2cErrSrcStream,        "source stream error")
CTL_DCL_ERR(t2cErrStackUnderflow,   "operand stack underflow")
CTL_DCL_ERR(t2cErrStackOverflow,    "operand stack overflow")
CTL_DCL_ERR(t2cErrInvalidOp,        "invalid operator")
CTL_DCL_ERR(t2cErrCallsubr,         "invalid local subroutine call")
CTL_DCL_ERR(t2cErrCallgsubr,        "invalid global subroutine call")
CTL_DCL_ERR(t2cErrStemOverflow,     "stem hint overflow")
CTL_DCL_ERR(t2cErrHintmask,         "invalid hint/cntrmask operator")
CTL_DCL_ERR(t2cErrBadSeacComp,      "invalid or missing seac component")
CTL_DCL_ERR(t2cErrPutBounds,        "bounds check (put)")
CTL_DCL_ERR(t2cErrGetBounds,        "bounds check (get)")
CTL_DCL_ERR(t2cErrIndexBounds,      "bounds check (index)")
CTL_DCL_ERR(t2cErrRollBounds,       "bounds check (roll)")
CTL_DCL_ERR(t2cErrSqrtDomain,       "domain error (sqrt)")
CTL_DCL_ERR(t2cErrInvalidWV,       "Invalid cube weight vector")
CTL_DCL_ERR(t2cErrSubrDepth,       "subr recursion level limit exceeded")
CTL_DCL_ERR(t2cErrMemory,          "memory error")
