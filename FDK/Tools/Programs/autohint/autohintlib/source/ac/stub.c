/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* stub.c - to resolve otherwise unresolved external references. */
#include "ac.h"

/* Procedures defined in stems/stemreport.c - StemHist */
public procedure AddVStem(top, bottom, curved)
Fixed top;
Fixed bottom;
boolean curved;
{
  if (curved && !allstems)
    return;
  
  if (addVStemCB != NULL)
  	{
  	addVStemCB(top, bottom, bezGlyphName);
  	}
}

public procedure AddHStem(right, left, curved)
Fixed right;
Fixed left;
boolean curved;
{
  if (curved && !allstems)
    return;
  
  if (addHStemCB != NULL)
  	{
  	addHStemCB(right, left, bezGlyphName);
  	}
}

public procedure AddCharExtremes(bot, top)
Fixed bot, top;
{
  if (addCharExtremesCB != NULL)
  	{
  	addCharExtremesCB(top, bot, bezGlyphName);
  	}
}

public procedure AddStemExtremes(bot, top)
Fixed bot, top;
{
  if (addStemExtremesCB != NULL)
  	{
  	addStemExtremesCB(top, bot, bezGlyphName);
  	}
}
