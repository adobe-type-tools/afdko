/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* The following ifdef block is the standard way of creating macros which make exporting 
 from a DLL simpler. All files within this DLL are compiled with the MAKEOTFLIB_EXPORTS
 symbol defined on the command line. this symbol should not be defined on any project
 that uses this DLL. This way any other project whose source files include this file see 
 MAKEOTFLIB_API functions as being imported from a DLL, wheras this DLL sees symbols
 defined with this macro as being exported.
*/
#undef _DEBUG
#include "Python.h"

#ifdef MAKEOTFLIB_EXPORTS
#define MAKEOTFLIB_API __declspec(dllexport)
#else
#define MAKEOTFLIB_API  
#endif


void MAKEOTFLIB_API initmakeotflib();
void MAKEOTFLIB_API initmakeotflibDB();

PyObject MAKEOTFLIB_API *  main_python(PyObject *self, PyObject *args);


