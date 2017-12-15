
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MAKEOTFLIB_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MAKEOTFLIB_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

#ifdef OTFCOMPARELIB_EXPORTS
#define OTFCOMPARELIB_API __declspec(dllexport)
#else
#define OTFCOMPARELIB_API  
#endif

void OTFCOMPARELIB_API initotfcomparelibDB();
void OTFCOMPARELIB_API initotfcomparelib();

 PyObject OTFCOMPARELIB_API *  main_python(PyObject *self, PyObject *args);

