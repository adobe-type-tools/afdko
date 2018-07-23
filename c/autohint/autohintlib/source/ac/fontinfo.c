/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* fontinfo.c */


#include "ac.h"
#include "fipublic.h"


#define UNDEFINED (MAXinteger)

public integer NumHColors, NumVColors;

private procedure ParseStems(kw, stems, pnum)
  char *kw; Fixed *stems; integer *pnum; 
{
  int istems[MAXSTEMS], i;
  ParseIntStems (kw, ACOPTIONAL, MAXSTEMS, istems, pnum, NULL);
  for (i = 0; i < *pnum; i++)
    stems[i] = FixInt (istems[i]);
}

private procedure GetKeyValue(keyword, optional, value)
  char *keyword; boolean optional; long int *value;
{
  char *fontinfostr;
  
 fontinfostr  = GetFntInfo(keyword, optional);

  if ((fontinfostr != NULL) && (fontinfostr[0] != 0))
  {
    *value = atol(fontinfostr);
#if DOMEMCHECK
	memck_free(fontinfostr);
#else
   ACFREEMEM(fontinfostr);
#endif
  }
  return;
}

private procedure GetKeyFixedValue(char* keyword, boolean optional, Fixed *value)
{
  char *fontinfostr;
  float tempValue;
  
 fontinfostr  = GetFntInfo(keyword, optional);

  if ((fontinfostr != NULL) && (fontinfostr[0] != 0))
  {
	sscanf(fontinfostr, "%g", &tempValue);
 	*value = (Fixed)tempValue*(1<< FixShift);
	#if DOMEMCHECK
	memck_free(fontinfostr);
#else
   ACFREEMEM(fontinfostr);
#endif
  }
  return;
}

public boolean ReadFontInfo() {
  char *fontinfostr;
  long 
    AscenderHeight,
    AscenderOvershoot,
    BaselineYCoord,
    BaselineOvershoot,
    Baseline5,
    Baseline5Overshoot,
    Baseline6,
    Baseline6Overshoot,
    CapHeight,
    CapOvershoot,
    DescenderHeight,
    DescenderOvershoot,
    FigHeight,
    FigOvershoot,
    Height5,
    Height5Overshoot,
    Height6,
    Height6Overshoot,
    LcHeight,
    LcOvershoot,
    OrdinalBaseline,
    OrdinalOvershoot,
    SuperiorBaseline,
    SuperiorOvershoot;
  boolean ORDINARYCOLORING = !scalinghints && writecoloredbez;

    AscenderHeight =
    AscenderOvershoot =
    BaselineYCoord =
    BaselineOvershoot =
    Baseline5 =
    Baseline5Overshoot =
    Baseline6 =
    Baseline6Overshoot =
    CapHeight =
    CapOvershoot =
    DescenderHeight =
    DescenderOvershoot =
    FigHeight =
    FigOvershoot =
    Height5 =
    Height5Overshoot =
    Height6 =
    Height6Overshoot =
    LcHeight =
    LcOvershoot =
    OrdinalBaseline =
    OrdinalOvershoot =
    SuperiorBaseline =
    SuperiorOvershoot = UNDEFINED; /* mark as undefined */
  NumHStems = NumVStems = 0;
  NumHColors = NumVColors = 0;
  lenBotBands = lenTopBands = 0;

  if (scalinghints)
  {
    SetFntInfoFileName (SCALEDHINTSINFO);
  }
  /* check for FlexOK, AuxHStems, AuxVStems */
  else  /* for intelligent scaling, it's too hard to check these */
  {
    ParseStems("StemSnapH", HStems, &NumHStems);
    ParseStems("StemSnapV", VStems, &NumVStems);
	if (NumHStems == 0)
	{
    ParseStems("DominantH", HStems, &NumHStems);
    ParseStems("DominantV", VStems, &NumVStems);
	}
  }
  fontinfostr = GetFntInfo("FlexOK", !ORDINARYCOLORING);
  flexOK = (fontinfostr != NULL) && (fontinfostr[0]!='\0') && strcmp(fontinfostr, "false");

#if DOMEMCHECK
	memck_free(fontinfostr);
#else
   ACFREEMEM(fontinfostr);
#endif
  fontinfostr = GetFntInfo("FlexStrict", TRUE);
if (fontinfostr != NULL) 
  flexStrict = strcmp(fontinfostr, "false");
#if DOMEMCHECK
	memck_free(fontinfostr);
#else
   ACFREEMEM(fontinfostr);
#endif

 	/* get bluefuzz. It is already set to its default value in ac.c::InitData().
	GetKeyFixedValue does nto change the value if it is not present in fontinfo.  */
   GetKeyFixedValue("BlueFuzz", ACOPTIONAL, &bluefuzz);

  /* Check for counter coloring characters. */
  if ((fontinfostr = GetFntInfo("VCounterChars", ACOPTIONAL)) != NULL)
  {
    NumVColors = AddCounterColorChars(fontinfostr, VColorList);
#if DOMEMCHECK
	memck_free(fontinfostr);
#else
   ACFREEMEM(fontinfostr);
#endif
  };
  if ((fontinfostr = GetFntInfo("HCounterChars", ACOPTIONAL)) != NULL)
  {
    NumHColors = AddCounterColorChars(fontinfostr, HColorList);
#if DOMEMCHECK
	memck_free(fontinfostr);
#else
   ACFREEMEM(fontinfostr);
#endif
  };
  GetKeyValue("AscenderHeight", ACOPTIONAL, &AscenderHeight);
  GetKeyValue("AscenderOvershoot", ACOPTIONAL, &AscenderOvershoot);
  GetKeyValue("BaselineYCoord", !ORDINARYCOLORING, &BaselineYCoord);
  GetKeyValue("BaselineOvershoot", !ORDINARYCOLORING, &BaselineOvershoot);
  GetKeyValue("Baseline5", ACOPTIONAL, &Baseline5);
  GetKeyValue("Baseline5Overshoot", ACOPTIONAL, &Baseline5Overshoot);
  GetKeyValue("Baseline6", ACOPTIONAL, &Baseline6);
  GetKeyValue("Baseline6Overshoot", ACOPTIONAL, &Baseline6Overshoot);
  GetKeyValue("CapHeight", !ORDINARYCOLORING, &CapHeight);
  GetKeyValue("CapOvershoot", !ORDINARYCOLORING, &CapOvershoot);
  GetKeyValue("DescenderHeight", ACOPTIONAL, &DescenderHeight);
  GetKeyValue("DescenderOvershoot", ACOPTIONAL, &DescenderOvershoot);
  GetKeyValue("FigHeight", ACOPTIONAL, &FigHeight);
  GetKeyValue("FigOvershoot", ACOPTIONAL, &FigOvershoot);
  GetKeyValue("Height5", ACOPTIONAL, &Height5);
  GetKeyValue("Height5Overshoot", ACOPTIONAL, &Height5Overshoot);
  GetKeyValue("Height6", ACOPTIONAL, &Height6);
  GetKeyValue("Height6Overshoot", ACOPTIONAL, &Height6Overshoot);
  GetKeyValue("LcHeight", ACOPTIONAL, &LcHeight);
  GetKeyValue("LcOvershoot", ACOPTIONAL, &LcOvershoot);
  GetKeyValue("OrdinalBaseline", ACOPTIONAL, &OrdinalBaseline);
  GetKeyValue("OrdinalOvershoot", ACOPTIONAL, &OrdinalOvershoot);
  GetKeyValue("SuperiorBaseline", ACOPTIONAL, &SuperiorBaseline);
  GetKeyValue("SuperiorOvershoot", ACOPTIONAL, &SuperiorOvershoot);

  lenBotBands = lenTopBands = 0;
  if (BaselineYCoord != UNDEFINED && BaselineOvershoot != UNDEFINED) {
    botBands[lenBotBands++] = ScaleAbs(FixInt(BaselineYCoord + BaselineOvershoot));
    botBands[lenBotBands++] = ScaleAbs(FixInt(BaselineYCoord));
    }
  if (Baseline5 != UNDEFINED && Baseline5Overshoot != UNDEFINED) {
    botBands[lenBotBands++] = ScaleAbs(FixInt(Baseline5 + Baseline5Overshoot));
    botBands[lenBotBands++] = ScaleAbs(FixInt(Baseline5));
    }
  if (Baseline6 != UNDEFINED && Baseline6Overshoot != UNDEFINED) {
    botBands[lenBotBands++] = ScaleAbs(FixInt(Baseline6 + Baseline6Overshoot));
    botBands[lenBotBands++] = ScaleAbs(FixInt(Baseline6));
    }
  if (SuperiorBaseline != UNDEFINED && SuperiorOvershoot != UNDEFINED) {
    botBands[lenBotBands++] = ScaleAbs(FixInt(SuperiorBaseline + SuperiorOvershoot));
    botBands[lenBotBands++] = ScaleAbs(FixInt(SuperiorBaseline));
    }
  if (OrdinalBaseline != UNDEFINED && OrdinalOvershoot != UNDEFINED) {
    botBands[lenBotBands++] = ScaleAbs(FixInt(OrdinalBaseline + OrdinalOvershoot));
    botBands[lenBotBands++] = ScaleAbs(FixInt(OrdinalBaseline));
    }
  if (DescenderHeight != UNDEFINED && DescenderOvershoot != UNDEFINED) {
    botBands[lenBotBands++] = ScaleAbs(FixInt(DescenderHeight + DescenderOvershoot));
    botBands[lenBotBands++] = ScaleAbs(FixInt(DescenderHeight));
    }
  if (CapHeight != UNDEFINED && CapOvershoot != UNDEFINED) {
    topBands[lenTopBands++] = ScaleAbs(FixInt(CapHeight));
    topBands[lenTopBands++] = ScaleAbs(FixInt(CapHeight + CapOvershoot));
    }
  if (LcHeight != UNDEFINED && LcOvershoot != UNDEFINED) {
    topBands[lenTopBands++] = ScaleAbs(FixInt(LcHeight));
    topBands[lenTopBands++] = ScaleAbs(FixInt(LcHeight + LcOvershoot));
    }
  if (AscenderHeight != UNDEFINED && AscenderOvershoot != UNDEFINED) {
    topBands[lenTopBands++] = ScaleAbs(FixInt(AscenderHeight));
    topBands[lenTopBands++] = ScaleAbs(FixInt(AscenderHeight + AscenderOvershoot));
    }
  if (FigHeight != UNDEFINED && FigOvershoot != UNDEFINED) {
    topBands[lenTopBands++] = ScaleAbs(FixInt(FigHeight));
    topBands[lenTopBands++] = ScaleAbs(FixInt(FigHeight + FigOvershoot));
    }
  if (Height5 != UNDEFINED && Height5Overshoot != UNDEFINED) {
    topBands[lenTopBands++] = ScaleAbs(FixInt(Height5));
    topBands[lenTopBands++] = ScaleAbs(FixInt(Height5 + Height5Overshoot));
    }
  if (Height6 != UNDEFINED && Height6Overshoot != UNDEFINED) {
    topBands[lenTopBands++] = ScaleAbs(FixInt(Height6));
    topBands[lenTopBands++] = ScaleAbs(FixInt(Height6 + Height6Overshoot));
    }
  if (scalinghints)
    ResetFntInfoFileName ();
  return TRUE;
  }
