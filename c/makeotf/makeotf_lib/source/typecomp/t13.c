/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Type 13 charstring support.
 *
 * The code may be compiled to support CJK protected fonts (via Type 13
 * charstrings) or to fail on such fonts. Support is enabled by defining the
 * macros TC_T13_SUPPORT to a non-zero value. Note, however, that Type 13 is
 * proprietary to Adobe and not all clients of this library will be supplied
 * with the module that supports Type 13. In such cases these clients should
 * not define the TC_T13_SUPPORT macro in order to avoid compiler complaints
 * about a missing t13supp.c file.
 */

#if TC_T13_SUPPORT
#include "t13supp.c"
#else
#include "t13fail.c"
#endif