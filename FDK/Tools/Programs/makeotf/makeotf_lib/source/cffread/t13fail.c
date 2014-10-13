/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Dummy Type 13 module for implementations that don't support Morisawa fonts
 * and Type 13 charstrings.
 */

static void t13Read(cffCtx h, Offset offset, int init) {
}

static int t13Support(cffCtx h) {
	return 0;
}