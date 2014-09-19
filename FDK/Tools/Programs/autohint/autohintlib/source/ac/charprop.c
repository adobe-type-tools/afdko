/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* charprop.c */


#include "ac.h"
#include "machinedep.h"
#if PYTHONLIB
extern char *FL_glyphname;
#endif

public char *VColorList[] = {
  "m", "M", "T", "ellipsis", NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
public char *HColorList[] = {
  "element", "equivalence", "notelement", "divide", NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL };

private char * UpperSpecialChars[] = {
  "questiondown", "exclamdown", "semicolon", NULL };

private char * LowerSpecialChars[] = {
  "question", "exclam", "colon", NULL };

private char * NoBlueList[] = {
   "at", "bullet", "copyright", "currency", "registered", NULL };

private char * SolEol0List[] = {
  "asciitilde", "asterisk", "bullet", "period", "periodcentered",
  "colon", "dieresis",
  "divide", "ellipsis", "exclam", "exclamdown", "guillemotleft",
  "guillemotright", "guilsinglleft", "guilsinglright", "quotesingle", 
  "quotedbl", "quotedblbase", "quotesinglbase", "quoteleft",
  "quoteright", "quotedblleft", "quotedblright", "tilde", NULL };

private char * SolEol1List[] = {
  "i", "j", "questiondown", "semicolon", NULL };

private char * SolEolNeg1List[] = { "question", NULL };

public boolean StrEqual(s1, s2) register char *s1, *s2; {
  register unsigned char c1, c2;
  while (TRUE) {
    c1 = *s1++; c2 = *s2++;
    if (c1 != c2) return FALSE;
    if (c1 == 0 && c2 == 0) return TRUE;
    if (c1 == 0 || c2 == 0) return FALSE;
    }
  }

extern boolean FindNameInList(nm, lst) char *nm, **lst; {
  char **l, *lnm;
  l = lst;
  while (TRUE) {
    lnm = *l;
    if (lnm == NULL) return FALSE;
    if (StrEqual(lnm, nm)) return TRUE;
    l++;
    }
  }

/* Adds specified characters to CounterColorList array. */
extern int AddCounterColorChars(charlist, ColorList)
char *charlist, *ColorList[];
{
  const char* setList = "(), \t\n\r";
  char *token;
  short length;
  boolean firstTime = TRUE;
  short ListEntries = COUNTERDEFAULTENTRIES;

  while (TRUE)
  {
    if (firstTime)
    {
      token = (char *)strtok(charlist, setList);
      firstTime = FALSE;
    }
    else token = (char *)strtok(NULL, setList);
    if (token == NULL) break;
    if (FindNameInList(token, ColorList))
      continue;
    /* Currently, ColorList must end with a NULL pointer. */
    if (ListEntries == (COUNTERLISTSIZE-1)) 
    {
	  FlushLogFiles();
      sprintf(globmsg, "Exceeded counter hints list size. (maximum is %d.)\n  Cannot add %s or subsequent characters.\n", (int) COUNTERLISTSIZE, token);
      LogMsg(globmsg, WARNING, OK, TRUE);
      break;
    }
    length = (short)strlen(token);
#if DOMEMCHECK
	ColorList[ListEntries] = memck_malloc(length+1);
#else
    ColorList[ListEntries] = AllocateMem(1, length+1, "counter hints list");
#endif
    strcpy(ColorList[ListEntries++], token);
  }
  return (ListEntries - COUNTERDEFAULTENTRIES);
}

public integer SpecialCharType() {
  /* 1 = upper; -1 = lower; 0 = neither */
#if PYTHONLIB
  if(featurefiledata)
  {
	if (FindNameInList(FL_glyphname, UpperSpecialChars)) return 1;
    if (FindNameInList(FL_glyphname, LowerSpecialChars)) return -1;
  }
#endif
  if (FindNameInList(bezGlyphName, UpperSpecialChars)) return 1;
  if (FindNameInList(bezGlyphName, LowerSpecialChars)) return -1;
  return 0;
  }

public boolean HColorChar() {
#if PYTHONLIB
  if(featurefiledata)
  {
	return FindNameInList(FL_glyphname, HColorList);
  }
#endif
	  return FindNameInList(bezGlyphName, HColorList);
  }

public boolean VColorChar() {
#if PYTHONLIB
  if(featurefiledata)
  {
	return FindNameInList(FL_glyphname, VColorList);
  }
#endif
  return FindNameInList(bezGlyphName, VColorList);
  }

public boolean NoBlueChar() {
#if PYTHONLIB
  if(featurefiledata)
  {
	return FindNameInList(FL_glyphname, NoBlueList);
  }
#endif
  return FindNameInList(bezGlyphName, NoBlueList);
  }

public integer SolEolCharCode() {
#if PYTHONLIB
  if(featurefiledata)
  {
	if (FindNameInList(FL_glyphname, SolEol0List)) return 0;
    if (FindNameInList(FL_glyphname, SolEol1List)) return 1;
    if (FindNameInList(FL_glyphname, SolEolNeg1List)) return -1;
  }
#endif
  if (FindNameInList(bezGlyphName, SolEol0List)) return 0;
  if (FindNameInList(bezGlyphName, SolEol1List)) return 1;
  if (FindNameInList(bezGlyphName, SolEolNeg1List)) return -1;
  return 2;
  }

/* This change was made to prevent bogus sol-eol's.  And to prevent
   adding sol-eol if there are a lot of subpaths. */
public boolean SpecialSolEol() {
  integer code = SolEolCharCode();
  integer count;
  if (code == 2) return FALSE;
  count = CountSubPaths();
  if (code != 0 && count != 2) return FALSE;
  if (code == 0 && count > 3) return FALSE;
  return TRUE;
  }

private PPathElt SubpathEnd(e) PPathElt e; {
  while (TRUE) {
    e = e->next;
    if (e == NULL) return pathEnd;
    if (e->type == MOVETO) return e->prev;
    }
  }

private PPathElt SubpathStart(e) PPathElt e; {
  while (e != pathStart) {
    if (e->type == MOVETO) break;
    e = e->prev;
    }
  return e;
  }

private PPathElt SolEol(e) PPathElt e; {
  e = SubpathStart(e);
  e->sol = TRUE;
  e = SubpathEnd(e);
  e->eol = TRUE;
  return e;
  }

private procedure SolEolAll() {
  PPathElt e;
  e = pathStart->next;
  while (e != NULL) {
    e = SolEol(e);
    e = e->next;
    }
  }

private procedure SolEolUpperOrLower(upper) boolean upper; {
  PPathElt e, s1, s2;
  Fixed x1, y1, s1y, s2y;
  boolean s1Upper;
  if (pathStart == NULL) return;
  e = s1 = pathStart->next;
  GetEndPoint(e, &x1, &y1);
  s1y = itfmy(y1);
  e = SubpathEnd(e);
  e = e->next;
  if (e == NULL) return;
  s2 = e;
  GetEndPoint(e, &x1, &y1); 
  s2y = itfmy(y1);
  s1Upper = (s1y > s2y);
  if ((upper && s1Upper) || (!upper && !s1Upper)) (void)SolEol(s1);
  else (void)SolEol(s2);
  }

/* This change was made to prevent bogus sol-eol's.  And to prevent
   adding sol-eol if there are a lot of subpaths. */
public procedure AddSolEol() {
  if (pathStart == NULL) return;
  if (!SpecialSolEol()) return;
  switch (SolEolCharCode()) {
    /* 1 means upper, -1 means lower, 0 means all */
    case 0: SolEolAll(); break;
    case 1: SolEolUpperOrLower(TRUE); break;
    case -1: SolEolUpperOrLower(FALSE); break;
    }
  }

public boolean MoveToNewClrs() {
#if PYTHONLIB
  if(featurefiledata)
  {
	  return StrEqual(FL_glyphname, "percent") || StrEqual(FL_glyphname, "perthousand");
  }
#endif
  return StrEqual(fileName, "percent") || StrEqual(fileName, "perthousand");
  }

