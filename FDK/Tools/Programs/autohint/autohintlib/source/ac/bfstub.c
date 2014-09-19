/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/* bfstub.c - to resolve otherwise unresolved external references.  These need to
   be defined because of AC's dependency on filookup, but not on fiblues. */
#include "ac.h"

/* Procedures defined in bf/fiblues.c - BuildFont */
public boolean checkbandspacing()
{
	return true;
}

public boolean checkbandwidths(overshootptsize, undershoot)
float overshootptsize;
boolean undershoot;
{
	return true;
}

public int build_bands()
{
	return true;
}

public void SetMasterDir(dirindx)
indx dirindx;
{
}

public void WriteBlendedArray(barray, totalvals, subarraysz, str, dirCount)
int *barray;
char *str;
int totalvals, subarraysz;
short dirCount;
{
}

boolean multiplemaster = 0; /* AC is never run at the MM level */
