/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* read.c */


#include "ac.h"
#include "bftoac.h"
#include "charpath.h"
#include "cryptdefs.h"
#include "fipublic.h"
#include "machinedep.h"
#include "opcodes.h"
#include "optable.h"
#define PRINT_READ (0)

#ifdef IS_LIB
extern const char *bezstring;
#endif

char bezGlyphName[64];

private Fixed currentx, currenty; /* used to calculate absolute coordinates */
private Fixed tempx, tempy; /* used to calculate relative coordinates */
#define STKMAX (20)
private Fixed stk[STKMAX];
private integer stkindex;
private boolean flex, startchar;
private boolean forMultiMaster, includeHints;
   /* Reading file for comparison of multiple master data and hint information.
      Reads into PCharPathElt structure instead of PPathElt. */

private real origEmSquare = 0.0;

public Fixed ScaleAbs(unscaled) Fixed unscaled; {
Fixed temp1;
  if (!scalinghints)
    return unscaled;
  if (origEmSquare == 0.0)
  {
    char *fistr;
    SetFntInfoFileName (SCALEDHINTSINFO);
    if ((fistr = GetFntInfo("OrigEmSqUnits", ACOPTIONAL)) == NULL)
      origEmSquare = 1000.0;
    else
    {
      sscanf(fistr, "%g", &origEmSquare);
#if DOMEMCHECK
	memck_free(fistr);
#else
   ACFREEMEM(fistr);
#endif
    }
    ResetFntInfoFileName ();
  }
  temp1 = (Fixed)(1000.0 / origEmSquare * ((real) unscaled));
  return temp1;
}

public Fixed UnScaleAbs(scaled) Fixed scaled; {
Fixed temp1;
  if (!scalinghints)
    return scaled;
  if (origEmSquare == 0.0)
  {
    char *fistr;
    SetFntInfoFileName (SCALEDHINTSINFO);
    if ((fistr = GetFntInfo("OrigEmSqUnits", ACOPTIONAL)) == NULL)
      origEmSquare = 1000.0;
    else
    {
      sscanf(fistr, "%g", &origEmSquare);
#if DOMEMCHECK
	memck_free(fistr);
#else
   ACFREEMEM(fistr);
#endif
    }
    ResetFntInfoFileName ();
  }
  temp1 = (Fixed)(origEmSquare / 1000.0 * ((real) scaled));
  temp1 = FRnd (temp1);
  return (temp1);
}

private Fixed Pop() {
  if (stkindex <= 0)
  {
	FlushLogFiles();
    sprintf (globmsg, "Stack underflow while reading %s file.\n", fileName);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
  stkindex--;
  return stk[stkindex];
  }

private procedure Push(r) Fixed r; {
  if (stkindex >= STKMAX)
  {
	FlushLogFiles();
    sprintf (globmsg, "Stack underflow while reading %s file.\n", fileName);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
  stk[stkindex] = r;
  stkindex++;
  }

private procedure Pop2() {
  (void) Pop(); (void) Pop();
  }

private procedure PopPCd(pcd) Cd *pcd; {
  pcd->y = Pop(); pcd->x = Pop(); }

#define DoDelta(dx,dy) currentx += (dx); currenty += (dy)

private PPathElt AppendElement(etype) integer etype; {
  PPathElt e;
  e = (PPathElt)Alloc(sizeof(PathElt));
  e->type = (short)etype;
  if (pathEnd != NULL) { pathEnd->next = e; e->prev = pathEnd; }
  else pathStart = e;
  pathEnd = e;
  return e;
  }

 
private procedure psDIV() {
  Fixed x, y;
  y = Pop(); x = Pop();
  if (y == FixInt(100)) x /= 100L; /* this will usually be the case */
  else x = (x * FixOne) / y;
  Push(x);
  }

private procedure RDcurveto(c1, c2, c3) Cd c1, c2, c3; {
  if (!forMultiMaster)
  {
    PPathElt new;
    new = AppendElement(CURVETO);
    new->x1 = tfmx(ScaleAbs(c1.x)); new->y1 = tfmy(ScaleAbs(c1.y));
    new->x2 = tfmx(ScaleAbs(c2.x)); new->y2 = tfmy(ScaleAbs(c2.y));
    new->x3 = tfmx(ScaleAbs(c3.x)); new->y3 = tfmy(ScaleAbs(c3.y));
  }
  else
  {
    PCharPathElt new;
    new = AppendCharPathElement(RCT);
    new->x = tempx; new->y = tempy;
    new->x1 = c1.x; new->y1 = c1.y;
    new->x2 = c2.x; new->y2 = c2.y;
    new->x3 = c3.x; new->y3 = c3.y;
    new->rx1 = c1.x - tempx; new->ry1 = c1.y - tempy;
    new->rx2 = c2.x - c1.x; new->ry2 = c2.y - c1.y;
    new->rx3 = c3.x - c2.x; new->ry3 = c3.y - c2.y;
    if (flex) new->isFlex = TRUE;
  }
  }

private procedure RDmtlt(etype) integer etype; {
  if (!forMultiMaster)
  {
    PPathElt new;
    new = AppendElement(etype);
    new->x = tfmx(ScaleAbs(currentx)); new->y = tfmy(ScaleAbs(currenty));
      return;
  }
  else
  {
    PCharPathElt new;
    new = AppendCharPathElement(etype == LINETO ? RDT : RMT);
    new->x = currentx; new->y = currenty;
    new->rx = tempx; new->ry = tempy;
  }
  }

#define RDlineto() RDmtlt(LINETO)
#define RDmoveto() RDmtlt(MOVETO)

private procedure psRDT() {
  Cd c;
  PopPCd(&c);
  tempx = c.x; tempy = c.y;
  DoDelta(c.x, c.y);
  RDlineto(); }

private procedure psHDT() {
  Fixed dx;
  tempy = 0;
  dx = tempx = Pop(); currentx += dx;
  RDlineto(); }

private procedure psVDT() {
  Fixed dy;
  tempx = 0;
  dy = tempy = Pop(); currenty += dy;
  RDlineto(); }

private procedure psRMT() {
  Cd c;
  PopPCd(&c);
  if (flex) return;
  tempx = c.x; tempy = c.y;
  DoDelta(c.x, c.y);
  RDmoveto(); }

private procedure psHMT() {
  Fixed dx;
  tempy = 0;
  dx = tempx = Pop(); currentx += dx;
  RDmoveto(); }

private procedure psVMT() {
  Fixed dy;
  tempx = 0;
  dy = tempy = Pop(); currenty += dy;
  RDmoveto(); }

private procedure Rct(c1, c2, c3) Cd c1, c2, c3; {
  tempx = currentx; tempy = currenty;
  DoDelta(c1.x,c1.y); c1.x = currentx; c1.y = currenty;
  DoDelta(c2.x,c2.y); c2.x = currentx; c2.y = currenty;
  DoDelta(c3.x,c3.y); c3.x = currentx; c3.y = currenty;
  RDcurveto(c1, c2, c3); }

private procedure psRCT() {
  Cd c1, c2, c3;
  PopPCd(&c3); PopPCd(&c2); PopPCd(&c1);
  Rct(c1, c2, c3); }

private procedure psVHCT() {
  Cd c1, c2, c3;
  c3.y = 0; c3.x = Pop();
  PopPCd(&c2);
  c1.y = Pop(); c1.x = 0;
  Rct(c1, c2, c3); }

private procedure psHVCT() {
  Cd c1, c2, c3;
  c3.y = Pop(); c3.x = 0;
  PopPCd(&c2);
  c1.y = 0; c1.x = Pop();
  Rct(c1, c2, c3); }

private procedure psCP() {
  if (!forMultiMaster)
    AppendElement(CLOSEPATH);
  else
    AppendCharPathElement(CP);
  }

private procedure psMT() {
  Cd c;
  c.y = Pop(); c.x = Pop();
  tempx = c.x - currentx; tempy = c.y - currenty;
  currenty = c.y; currentx = c.x;
  RDmoveto();
  }

private procedure psDT() {
  Cd c;
  c.y = Pop(); c.x = Pop();
  tempx = c.x - currentx; tempy = c.y - currenty;
  currenty = c.y; currentx = c.x;
  RDlineto();
  }

private procedure psCT() {
  Cd c1, c2, c3;
  tempx = currentx; tempy = currenty;
  PopPCd(&c3); PopPCd(&c2); PopPCd(&c1);
  RDcurveto(c1, c2, c3); }

private procedure psFLX() {
  Cd c0, c1, c2, c3, c4, c5;
  integer i;
  for (i = 0; i < 5; i++) (void) Pop();
  PopPCd(&c5); PopPCd(&c4); PopPCd(&c3);
  PopPCd(&c2); PopPCd(&c1); PopPCd(&c0);
  Rct(c0, c1, c2);
  Rct(c3, c4, c5);
  flex = FALSE;
  }

private procedure ReadHintInfo(nm, str) char nm;  const char *str; {
  Cd c0;
  short hinttype =
    nm == 'y' ? RY : nm == 'b' ? RB : nm == 'm' ? RM + ESCVAL : RV + ESCVAL;
  long elt1, elt2;
  PopPCd(&c0);
  c0.y += c0.x;  /* make absolute */
  /* Look for comment of path elements used to determine this band. */
  if (sscanf(str, " %% %ld %ld", &elt1, &elt2) != 2)
  {
	FlushLogFiles();
    sprintf(globmsg, "Extra hint information required for blended fonts is not in\n  character file: %s.  Please re-hint using the latest software.\n  Hints will not be included in this font.\n", fileName);
    LogMsg(globmsg, WARNING, NONFATALERROR, TRUE);
    SetNoHints();
    includeHints = FALSE;
  }
  else
    SetHintsElt(hinttype, &c0, elt1, elt2, (boolean)!startchar);
  }
  
private integer StrLen(s) register char *s; {
  register integer cnt = 0;
  while (*s++ != 0) cnt++;
  return cnt;
  }


/*Used instead of StringEqual to keep ac from cloberring source string*/
 
int isPrefix(const char *s, const char* pref)
{
	while(*pref)
	{
		if(*pref!=*s)
			return 0;
		pref++;
		s++;
	}
	return 1;
}

private procedure DoName(nm, buff, len) const char * nm, *buff; int len; {
  switch (len) {
    case 2:
      switch (nm[0]) {
        case 'c': /* ct, cp */
          switch (nm[1]) {
            case 't': psCT(); break;
	    case 'p': psCP(); break;
	    default: goto badFile;
	    }
          break;
	case 'm': /* mt */
          if (nm[1] != 't') goto badFile;
	  psMT();
	  break;
	case 'd': /* dt */
          if (nm[1] != 't') goto badFile;
	  psDT();
	  break;
	case 's': /* sc */
          if (nm[1] != 'c') goto badFile;
          startchar = TRUE;
	  break;
	case 'e': /* ed */
          if (nm[1] != 'd') goto badFile;
	  break;
	case 'r': /* rm, rv, ry, rb */
          if (includeHints)
            ReadHintInfo(nm[1], buff);
          else
            Pop2();
          break;
        case 'i': /* id */
          if (nm[1] != 'd') goto badFile;
          Pop();
          idInFile = TRUE;
          break;
        default: goto badFile;
        }
      break;
    case 3:
      switch (nm[0]) {
        case 'r': /* rdt, rmt, rct */
          if (nm[2] != 't') goto badFile;
          switch (nm[1]) {
            case 'd': psRDT(); break;
	    case 'm': psRMT(); break;
	    case 'c': psRCT(); break;
	    default: goto badFile;
            }
          break;
	case 'h': /* hdt, hmt */
          if (nm[2] != 't') goto badFile;
          switch (nm[1]) {
            case 'd': psHDT(); break;
	    case 'm': psHMT(); break;
	    default: goto badFile;
            }
          break;
	case 'v': /* vdt, vmt */
          if (nm[2] != 't') goto badFile;
          switch (nm[1]) {
            case 'd': psVDT(); break;
	    case 'm': psVMT(); break;
	    default: goto badFile;
            }
          break;
	case 's': /* sol, snc */
	case 'e': /* eol, enc */
          switch (nm[1]) {
            case 'o':
              if (nm[2] != 'l') goto badFile;
	      break;
	    case 'n':
              if (nm[2] != 'c') goto badFile;
	      break;
	    default: goto badFile;
	    }
          break;
	case 'f': /* flx */
          if (nm[1] == 'l' && nm[2] == 'x') psFLX();
	  else goto badFile;
	  break;
	case 'd': /* div */
          if (nm[1] == 'i' && nm[2] == 'v') psDIV();
	  else goto badFile;
	  break;
	default: goto badFile;
	}
      break;
    case 4:
      if (nm[2] != 'c' || nm[3] != 't') goto badFile;
      switch (nm[0]) {
        case 'v': /* vhct */
          if (nm[1] != 'h') goto badFile;
	  psVHCT();
	  break;
	case 'h': /* hvct */
          if (nm[1] != 'v') goto badFile;
	  psHVCT();
	  break;
        default: goto badFile;
        }
      break;
    case 7:
      switch (nm[6]) {
        case '1': /* preflx1 */
	case '2': /* preflx2 */
          if (nm[0] != 'p' || nm[1] != 'r' || nm[2] != 'e' ||
              nm[3] != 'f' || nm[4] != 'l' || nm[5] != 'x')
              goto badFile;
          flex = TRUE;
          break;
	case 'r': /* endsubr */
          if (!isPrefix(nm, "endsubr")) goto badFile;
	  break;
        default: goto badFile;
	}
      break;
    case 9:
      switch (nm[0]) {
        case 'b': /* beginsubr */
          if (!isPrefix(nm, "beginsubr")) goto badFile;
	  break;
	case 'n': /* newcolors */
          if (!isPrefix(nm, "newcolors")) goto badFile;
	  break;
        default: goto badFile;
	}
      break;
    default: goto badFile;
    }
  return;
  badFile:
  {
	char op[80];
	if (len > 79) len = 79;
	strncpy(op, nm, len);
	op[len]=0;

	FlushLogFiles();
    sprintf(globmsg, "Bad file format. Unknown operator: %s in %s character.\n", op, fileName);
    LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
  }
  }

private procedure ParseString(s) const char * s; {
  const char * s0;
    char c;
    char *c0;
   boolean neg;
    boolean isReal;
    float rval;
  integer val;
  Fixed r;
  pathStart = pathEnd = NULL;
  bezGlyphName[0] = 0;
  
  while (TRUE) {
    c = *s++;
    nxtChar:
    switch (c) {
      case '-': /* negative number */
        neg = TRUE; val = 0; goto rdnum;
      case '%': /* comment */
		if (bezGlyphName[0] == 0)
			{
			unsigned int end = 0;
			while (*s == ' ') 
				s++;

			while ((s[end] != ' ') && (s[end] != '\r') && (s[end] != '\n'))
				end++;
			
			strncpy(bezGlyphName, s, end);
			if (end < 64)
				bezGlyphName[end] = 0;
			else
				{
				bezGlyphName[63] = 0;
				sprintf (globmsg, 
					"Bad input file.  Glyph name  %s is greater than 32 chars.\n",
					bezGlyphName);
				  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
						}
				
			}
        while ((*s != '\n') && (*s != '\r')) {s++;}
        continue;
      case ' ': continue;
      case '\t': continue;
      case '\n': continue;
      case '\r': continue;
      case 0: /* end of file */
        if (stkindex != 0)
        {
		  FlushLogFiles();
          sprintf (globmsg, 
            "Bad input file.  Numbers left on stack at end of %s file.\n",
            fileName);
          LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
        }
        return;
      default:
        if (c >= '0' && c <= '9') {
          neg = FALSE; val = c - '0'; goto rdnum; }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
          s0 = s - 1;
          while (TRUE) {
            c = *s++;
			if ( (c == ' ') || (c == '\t') || (c == '\n') || (c =='\r') ||  (c == '\0'))
				break;
			if (c == 0)
				break;
            }
          DoName(s0, s, s-s0-1);
		if (c == '\0')
			s--;
         continue;
          }
		FlushLogFiles();
        sprintf (globmsg, "Unexpected character in %s file.\n", fileName);
        LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
      }
      rdnum:
      isReal = false;
      c0 = (char*)(s-1);
      while (TRUE) {
          c = *s++;
          if (c == '.')
              isReal = true;
          else if (c >= '0' && c <= '9')
              val = val * 10 + (c - '0');
          else if ((c == ' ') || (c == '\t'))
          {
            if (isReal)
            {
                sscanf(c0, "%f", &rval);
                rval = roundf(rval*100)/100;// Autohint can only support 2 digits of decimal precision.
                /* do not need to use 'neg' to negate the value, as c0 string includes the minus sign.*/
                 r = FixReal(rval); /* convert to Fixed */
            }
            else
            {
                if (neg)
                    val = -val;
                r = FixInt(val); /* convert to Fixed */
            }
              /* Push(r); */
              if (stkindex >= STKMAX) {
                  FlushLogFiles();
                  sprintf (globmsg, "Stack overflow while reading %s file.\n",
                           fileName);
                  LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
                  return;
              }
              stk[stkindex] = r;
              stkindex++;
              goto nxtChar;
          }
          else {
              FlushLogFiles();
              sprintf (globmsg,
                       "Illegal number terminator while reading %s file.\n",
                       fileName);
              LogMsg(globmsg, LOGERROR, NONFATALERROR, TRUE);
              return;
          }
      } /*end while true */
    }
  }

#define TESTING (FALSE)
#if TESTING
private unsigned char ibuff[MAXBYTES + 2];
private integer inputlen;
#endif /*TESTING*/

public procedure SetReadFileName(file_name)
char *file_name;
{
  fileName = file_name;
}

public boolean ReadCharFile(normal, forBlendData, readHints, prependprefix)
boolean normal, forBlendData, readHints, prependprefix; 
{
  char infile[MAXPATHLEN];
  FILE *fd;
  char *inputbuff;
  integer cc;
  unsigned long filelen;
  currentx = currenty = tempx = tempy = stkindex = 0;
  flex = idInFile = startchar = FALSE;
  forMultiMaster = forBlendData;
  includeHints = readHints;
  if (prependprefix
#ifdef IS_LIB
	&& !featurefiledata  
#endif
  ) {
    if (normal)
      sprintf(infile, "%s%s", inPrefix, fileName);
    else
      sprintf(infile, "%s%s", outPrefix, fileName);
  }
  else {
    strcpy(infile, fileName);
  }
#ifdef IS_LIB
  if (bezstring)
  {
    inputbuff=(char*)bezstring;
    cc=(integer)strlen(inputbuff);
  }
  else
  {
#endif
  fd = ACOpenFile(infile, "rb", OPENWARN);
  if (fd == NULL) return FALSE;
  filelen = ACGetFileSize(infile);
#if DOMEMCHECK
    inputbuff = (char *) memck_malloc(filelen+5);
#else
  inputbuff = (char *) ACNEWMEM(filelen+5);
#endif
  cc = ACReadFile((char *)inputbuff, fd, (char *)infile, filelen);
  inputbuff[cc] = '\0';
#ifdef IS_LIB
  }
#endif
  
#if PRINT_READ
  fprintf(OUTPUTBUFF, "%s", inputbuff);
#endif

#if TESTING
  inputlen = cc;
  for (i = 0; i <= cc; i++) ibuff[i] = inputbuff[i];
#endif
  ParseString(inputbuff);
#if !TESTING 
#ifndef IS_LIB
  { char *s;
    s = inputbuff;
    for (i = 0; i < cc; i++) *s++ = 0; /* erase the plain text version */
    }
#endif
#endif
#ifndef IS_LIB
  fclose(fd);
#if DOMEMCHECK
  memck_free(inputbuff);
#else
  ACFREEMEM(inputbuff);
#endif
#endif

#if TESTING
  if (normal) {
    fd = ACOpenFile("file.old", "wb", OPENWARN);
    if (fd == NULL) return FALSE;
    fprintf(fd, "%s", ibuff);
    fclose(fd);
    }
#endif
  return TRUE;
}

public procedure Test() {
#if TESTING
  unsigned char buff[MAXBYTES + 2], *s;
  char infile[MAXPATHLEN];
  FILE *fd;
  integer cc, i;

  sprintf(infile, "%s%s", outPrefix, fileName);
  fd = ACOpenFile(infile, "rb", OPENWARN);
  if (fd == NULL) return;
  cc = ReadDecFile(
     fd, (char *)infile, (char *)buff,
     TRUE, MAXBYTES, (unsigned long)MAXBYTES, OTHER);
  fclose(fd);
  if (cc >= MAXBYTES) { cc--; buff[cc] = 0; goto mismatch; }
  buff[cc] = 0;
  if (cc != inputlen) {
    LogMsg("Test: file length different.\n", WARNING, OK, TRUE);
    goto mismatch; }
  for (i = 0; i < cc; i++)
    if (buff[i] != ibuff[i]) {
      LogMsg("Test: file contents different.\n", WARNING, OK, TRUE);
      goto mismatch; }
/*  return; */
  mismatch:
  fd = ACOpenFile("file.new", "wb", OPENWARN);
  if (fd == NULL) return;
  fprintf(fd, "%s", buff);
  fclose(fd);
#endif
  }


