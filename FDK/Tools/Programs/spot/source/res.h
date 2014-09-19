/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)res.h	1.1
 * Changed:    7/19/95 13:35:04
 ***********************************************************************/

/*
 * Macintosh resource reading support.
 */

#ifndef RES_H
#define RES_H

#include "global.h"
#include "opt.h"

extern void resRead(long origin);
extern opt_Scanner resIdScan;

#endif /* RES_H */



