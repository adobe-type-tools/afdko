/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* t1cstr library error name to error string mapping. See ctlshare.h for a
   description of the CTL_DCL_ERR macro. */

CTL_DCL_ERR(t1cSuccess,             "no error")
CTL_DCL_ERR(t1cErrSrcStream,        "source stream error")
CTL_DCL_ERR(t1cErrStackUnderflow,   "operand stack underflow")
CTL_DCL_ERR(t1cErrStackOverflow,    "operand stack overflow")
CTL_DCL_ERR(t1cErrInvalidOp,        "invalid operator")
CTL_DCL_ERR(t1cErrInvalidWV,        "invalid WV for blend operator")
CTL_DCL_ERR(t1cErrBadLenIV,         "invalid lenIV parameter")
CTL_DCL_ERR(t1cErrBadCntrCntl,      "invalid counter control operator")
CTL_DCL_ERR(t1cErrCallsubr,         "bounds check (callsubr)")
CTL_DCL_ERR(t1cErrStoreBounds,      "bounds check (store)")
CTL_DCL_ERR(t1cErrLoadBounds,       "bounds check (load)")
CTL_DCL_ERR(t1cErrPutBounds,        "bounds check (put)")
CTL_DCL_ERR(t1cErrGetBounds,        "bounds check (get)")
CTL_DCL_ERR(t1cErrIndexBounds,      "bounds check (index)")
CTL_DCL_ERR(t1cErrRollBounds,       "bounds check (roll)")
CTL_DCL_ERR(t1cErrSqrtDomain,       "domain error (sqrt)")
CTL_DCL_ERR(t1cErrStoreWVBounds,    "bounds check (storeWV)")
CTL_DCL_ERR(t1cErrBadSeacComp,      "invalid or missing seac component")
CTL_DCL_ERR(t1cErrFlex,             "invalid flex operator")
CTL_DCL_ERR(t1cErrCallgsubr,        "invalid subroutine index")
CTL_DCL_ERR(t1cErrSubrDepth,        "exceeded subroutine recursion level limit")


