
//This file exists to make to allow the windows version of this app to define exported functions.
//It exists under the Mac for the sake of consistency.


 
#ifdef OTFTABLEEDITORLIB_EXPORTS
#define OTFTABLEEDITORLIB_API __declspec(dllexport)
#else
#define OTFTABLEEDITORLIB_API  
#endif

#include "Python.h"

 void OTFTABLEEDITORLIB_API initteditlib(void);

 PyObject OTFTABLEEDITORLIB_API *  main_python(PyObject *self, PyObject *args);

