/*
 * A n t l r  T r a n s l a t i o n  H e a d e r
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * With AHPCRC, University of Minnesota
 * ANTLR Version 1.33MR33
 *
 *   pccts/bin/antlr -ga -o ../../source/hotconv -fe featerr.c -fl featpars.dlg -fm featmode.h -ft feattoks.h featgram.g
 *
 */

#define ANTLR_VERSION	13333
#include "pcctscfg.h"
#include "pccts_stdio.h"

/* --- This section will be included in all feat*.c files created. --- */

#include <stdlib.h>	/* For exit in err.h xxx */
#include <string.h>

#include <stdio.h>

#include "common.h"
#include "feat.h"
#include "MMFX.h"
#include "OS_2.h"
#include "hhea.h"
#include "vmtx.h"
#include "dynarr.h"

#define MAX_TOKEN	64

/* ------------------------------------------------------------------- */
/* Adobe patent application tracking #P295, entitled FONT FEATURE FILE */
/* PROCESSING, inventors: Patel, Hall.                                 */
/* ------------------------------------------------------------------- */
extern featCtx h;	/* Not reentrant; see featNew() comments */
extern hotCtx g;

typedef union
{
  long lval;
  unsigned long ulval;
  char text[MAX_TOKEN];
} Attrib;

void zzcr_attr(Attrib *attr, int type, char *text);

  
#define zzSET_SIZE 20
#include "antlr.h"
#include "feattoks.h"
#include "dlgdef.h"
#include "featmode.h"

/* MR23 In order to remove calls to PURIFY use the antlr -nopurify option */

#ifndef PCCTS_PURIFY
#define PCCTS_PURIFY(r,s) memset((char *) &(r),'\0',(s));
#endif

ANTLR_INFO

void featureFile(void);

#include "feat.c"

featCtx h;			/* Not reentrant; see featNew() comments */
hotCtx g;

GID
#ifdef __USE_PROTOS
glyph(char * tok,int allowNotdef)
#else
glyph(tok,allowNotdef)
 char *tok;
int allowNotdef ;
#endif
{
  GID   _retv;
  zzRULE;
  Attrib gname, cid;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GID  ))
  zzMake0;
  {
  _retv = 0; /* Suppress optimizer warning */
  if ( (LA(1)==T_GNAME) ) {
    zzmatch(T_GNAME);
    gname = zzaCur;

    _retv = featMapGName2GID(g,  gname.text, allowNotdef);
    if ( tok != NULL)
    strcpy( tok,  gname.text);
 zzCONSUME;

  }
  else {
    if ( (LA(1)==T_CID) ) {
      zzmatch(T_CID);
      cid = zzaCur;

      _retv = cid2gid(g, (CID)( cid).lval);
 zzCONSUME;

    }
    else {zzFAIL(1,zzerr1,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x1);
  return _retv;
  }
}

GNode *
#ifdef __USE_PROTOS
glyphClass(int named,char * gcname)
#else
glyphClass(named,gcname)
 int named;
char *gcname ;
#endif
{
  GNode *   _retv;
  zzRULE;
  Attrib a, b;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  
  _retv = NULL;	/* Suppress compiler warning */
  if ( named)
  gcDefine(gcname);	 /* GNode class assignment */
  if ( (LA(1)==T_GCLASS) ) {
    zzmatch(T_GCLASS);
    a = zzaCur;

    
    if ( named)
    {
      _retv = gcAddGlyphClass( a.text, 1);
      gcEnd(named);
    }
    else
    featGlyphClassCopy(g, &(_retv), gcLookup( a.text));
 zzCONSUME;

  }
  else {
    if ( (LA(1)==140) ) {
      {
        zzBLOCK(zztasp2);
        zzMake0;
        {
        
        GNode *anonHead;
        GNode *namedHead;
        if (!  named)
        h->gcInsert = &anonHead; /* Anon glyph class (inline) definition */
        zzmatch(140); zzCONSUME;
        {
          zzBLOCK(zztasp3);
          int zzcnt=1;
          zzMake0;
          {
          GID gid, endgid;
          char p[MAX_TOKEN], q[MAX_TOKEN];
          do {
            if ( (setwd1[LA(1)]&0x2) ) {
               gid  = glyph( p, TRUE );

              if (gid == 0) {
                /* it might be a range.*/
                zzBLOCK(zztasp4);
                zzMake0;
                if (strchr(p, '-')) {
                  char *secondPart = p;
                  char *firstPart = strsep(&secondPart, "-");
                  if (firstPart == NULL) {
                    featMsg(hotERROR, "Glyph \"%s\" not in font", p);
                  }
                  else {
                    gid = featMapGName2GID(g, firstPart, FALSE );
                    endgid  = featMapGName2GID(g, secondPart, FALSE );
                    if (gid != 0 && endgid != 0) {
                      gcAddRange(gid, endgid, firstPart, secondPart);
                    }
                    else {
                      zzFAIL(1,zzerr2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk);
                      goto fail;
                    }
                  }
                }
                else {
                  zzFAIL(1,zzerr2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk);
                  goto fail;
                }
                zzEXIT(zztasp4);
              }
              else
              {
                zzBLOCK(zztasp4);
                zzMake0;
                {
                if ( (LA(1)==141) ) {
                  zzmatch(141); zzCONSUME;
                   endgid  = glyph( q, FALSE );

                  gcAddRange(gid, endgid, p, q);
                }
                else {
                  if ( (setwd1[LA(1)]&0x4) ) {
                    gcAddGlyph(gid);
                  }
                  else {zzFAIL(1,zzerr2,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp4);
                }
              }
            }
            else {
              if ( (LA(1)==T_GCLASS) ) {
                zzmatch(T_GCLASS);
                b = zzaCur;

                gcAddGlyphClass( b.text, named);
 zzCONSUME;

              }
              /* MR10 ()+ */ else {
                if ( zzcnt > 1 ) break;
                else {zzFAIL(1,zzerr3,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
            zzcnt++; zzLOOP(zztasp3);
          } while ( 1 );
          zzEXIT(zztasp3);
          }
        }
        zzmatch(142);
        
        namedHead = gcEnd(named);
        _retv = ( named) ? namedHead : anonHead;
        (_retv)->flags |= FEAT_GCLASS;
 zzCONSUME;

        zzEXIT(zztasp2);
        }
      }
    }
    else {zzFAIL(1,zzerr4,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x8);
  return _retv;
  }
}

unsigned char
#ifdef __USE_PROTOS
numUInt8(void)
#else
numUInt8()
#endif
{
  unsigned char   _retv;
  zzRULE;
  Attrib n;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(unsigned char  ))
  zzMake0;
  {
  _retv = 0; /* Suppress optimizer warning */
  zzmatch(T_NUM);
  n = zzaCur;

  
  if ( n.lval < 0 ||  n.lval > 255)
  zzerr("not in range 0 .. 255");
  _retv = (unsigned char)( n).lval;
 zzCONSUME;

  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x10);
  return _retv;
  }
}

short
#ifdef __USE_PROTOS
numInt16(void)
#else
numInt16()
#endif
{
  short   _retv;
  zzRULE;
  Attrib n;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(short  ))
  zzMake0;
  {
  _retv = 0; /* Suppress optimizer warning */
  zzmatch(T_NUM);
  n = zzaCur;

  
  if ( n.lval < -32767 ||  n.lval > 32767)
  zzerr("not in range -32767 .. 32767");
  _retv = (short)( n).lval;
 zzCONSUME;

  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x20);
  return _retv;
  }
}

unsigned
#ifdef __USE_PROTOS
numUInt16(void)
#else
numUInt16()
#endif
{
  unsigned   _retv;
  zzRULE;
  Attrib n;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(unsigned  ))
  zzMake0;
  {
  _retv = 0; /* Suppress optimizer warning */
  zzmatch(T_NUM);
  n = zzaCur;

  
  if ( n.lval < 0 ||  n.lval > 65535)
  zzerr("not in range 0 .. 65535");
  _retv = (unsigned)( n).lval;
 zzCONSUME;

  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x40);
  return _retv;
  }
}

short
#ifdef __USE_PROTOS
parameterValue(void)
#else
parameterValue()
#endif
{
  short   _retv;
  zzRULE;
  Attrib d, n;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(short  ))
  zzMake0;
  {
  
  long retval = 0;
  _retv = 0; /* Suppress optimizer warning */
  if ( (LA(1)==T_FONTREV) ) {
    zzmatch(T_FONTREV);
    d = zzaCur;

    
    /* This is actually for picking up fractional point values - i use the T_FONTREV
    pattern as it works fine for fractional point values too. */
    retval = (long)(0.5 + 10*atof(d.text));
    if (retval < 0 || retval > 65535)
    zzerr("not in range 0 .. 65535");
    _retv = (short)retval;
 zzCONSUME;

  }
  else {
    if ( (LA(1)==T_NUM) ) {
      zzmatch(T_NUM);
      n = zzaCur;

      
      if ( n.lval < 0 ||  n.lval > 65535)
      zzerr("not in range 0 .. 65535");
      _retv = (short)( n).lval;
 zzCONSUME;

    }
    else {zzFAIL(1,zzerr5,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd1, 0x80);
  return _retv;
  }
}

unsigned
#ifdef __USE_PROTOS
numUInt16Ext(void)
#else
numUInt16Ext()
#endif
{
  unsigned   _retv;
  zzRULE;
  Attrib m, n;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(unsigned  ))
  zzMake0;
  {
  _retv = 0; /* Suppress optimizer warning */
  h->linenum = zzline;
  if ( (LA(1)==T_NUMEXT) ) {
    zzmatch(T_NUMEXT);
    m = zzaCur;

    
    if ( m.lval < 0 ||  m.lval > 65535)
    zzerr("not in range 0 .. 65535");
    _retv = (unsigned)( m).lval;
 zzCONSUME;

  }
  else {
    if ( (LA(1)==T_NUM) ) {
      zzmatch(T_NUM);
      n = zzaCur;

      
      if ( n.lval < 0 ||  n.lval > 65535)
      zzerr("not in range 0 .. 65535");
      _retv = (unsigned)( n).lval;
 zzCONSUME;

    }
    else {zzFAIL(1,zzerr6,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x1);
  return _retv;
  }
}

unsigned
#ifdef __USE_PROTOS
numUInt32Ext(void)
#else
numUInt32Ext()
#endif
{
  unsigned   _retv;
  zzRULE;
  Attrib m, n;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(unsigned  ))
  zzMake0;
  {
  _retv = 0; /* Suppress optimizer warning */
  h->linenum = zzline;
  if ( (LA(1)==T_NUMEXT) ) {
    zzmatch(T_NUMEXT);
    m = zzaCur;

    
    if (  m.ulval > ((unsigned long)0xFFFFFFFF))
    zzerr("not in range 0 .. ((1<<32) -1)");
    _retv = (unsigned)( m).ulval;
 zzCONSUME;

  }
  else {
    if ( (LA(1)==T_NUM) ) {
      zzmatch(T_NUM);
      n = zzaCur;

      
      if ( n.ulval > ((unsigned long)0xFFFFFFFF))
      zzerr("not in range 0 .. ((1<<32) -1)");
      _retv = (unsigned)( n).ulval;
 zzCONSUME;

    }
    else {zzFAIL(1,zzerr7,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x2);
  return _retv;
  }
}

short
#ifdef __USE_PROTOS
metric(void)
#else
metric()
#endif
{
  short   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(short  ))
  zzMake0;
  {
   _retv  = numInt16();

  if(0)goto fail;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x4);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
deviceEntry(void)
#else
deviceEntry()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  unsigned short ppem;
  short delta;
   ppem  = numUInt16();

   delta  = numInt16();

  deviceAddEntry(ppem, delta);
  if(0)goto fail;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x8);
  }
}

unsigned short
#ifdef __USE_PROTOS
contourpoint(void)
#else
contourpoint()
#endif
{
  unsigned short   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(unsigned short  ))
  zzMake0;
  {
  _retv = 0;
  zzmatch(K_contourpoint); zzCONSUME;
   _retv  = numUInt16();

  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x10);
  return _retv;
  }
}

unsigned short
#ifdef __USE_PROTOS
device(void)
#else
device()
#endif
{
  unsigned short   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(unsigned short  ))
  zzMake0;
  {
  _retv = 0;
  zzmatch(K_device); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==143) ) {
      zzmatch(143);
      _retv = 0;
 zzCONSUME;

    }
    else {
      if ( (LA(1)==T_NUM) ) {
        deviceBeg();
        deviceEntry();
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          while ( (LA(1)==144) ) {
            zzmatch(144); zzCONSUME;
            deviceEntry();
            zzLOOP(zztasp3);
          }
          zzEXIT(zztasp3);
          }
        }
        _retv = deviceEnd();
      }
      else {zzFAIL(1,zzerr8,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x20);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
caret(void)
#else
caret()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  short val;
  unsigned short contPt;
  Offset dev;
  zzmatch(K_caret); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==T_NUM) ) {
       val  = metric();

    }
    else {
      if ( (LA(1)==K_BeginValue) ) {
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          zzmatch(K_BeginValue); zzCONSUME;
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (LA(1)==K_contourpoint) ) {
               contPt  = contourpoint();

            }
            else {
              if ( (LA(1)==K_device) ) {
                 dev  = device();

              }
              else {zzFAIL(1,zzerr9,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          zzmatch(K_EndValue); zzCONSUME;
          zzEXIT(zztasp3);
          }
        }
      }
      else {zzFAIL(1,zzerr10,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x40);
  }
}

void
#ifdef __USE_PROTOS
valueRecordDef(void)
#else
valueRecordDef()
#endif
{
  zzRULE;
  Attrib valName;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  short metrics[4];
  zzmatch(K_valueRecordDef); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_BeginValue) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        zzmatch(K_BeginValue); zzCONSUME;
         metrics[0]  = metric();

         metrics[1]  = metric();

         metrics[2]  = metric();

         metrics[3]  = metric();

        zzmatch(K_EndValue); zzCONSUME;
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (LA(1)==T_NUM) ) {
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
           metrics[2]  = metric();

          
          metrics[0] = 0;
          metrics[1] = 0;
          metrics[3] = 0;
          zzEXIT(zztasp3);
          }
        }
      }
      else {zzFAIL(1,zzerr11,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(T_GNAME);
  valName = zzaCur;

  
  featAddValRecDef(metrics,  valName.text);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd2, 0x80);
  }
}

void
#ifdef __USE_PROTOS
valueRecord(GNode* gnode)
#else
valueRecord(gnode)
 GNode* gnode ;
#endif
{
  zzRULE;
  Attrib valueDefName;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  short metrics[4];
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    zzmatch(K_BeginValue); zzCONSUME;
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (LA(1)==T_GNAME) ) {
        zzmatch(T_GNAME);
        valueDefName = zzaCur;

        fillMetricsFromValueDef( valueDefName.text, metrics);
 zzCONSUME;

      }
      else {
        if ( (LA(1)==T_NUM) ) {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
             metrics[0]  = metric();

             metrics[1]  = metric();

             metrics[2]  = metric();

             metrics[3]  = metric();

            zzEXIT(zztasp4);
            }
          }
        }
        else {zzFAIL(1,zzerr12,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    zzmatch(K_EndValue);
    if (gnode == NULL)
    zzerr("Glyph or glyph class must precede a value record.");
    gnode->metricsInfo = getNextMetricsInfoRec();
    gnode->metricsInfo->cnt = 4;
    gnode->metricsInfo->metrics[0] = metrics[0];
    gnode->metricsInfo->metrics[1] = metrics[1];
    gnode->metricsInfo->metrics[2] = metrics[2];
    gnode->metricsInfo->metrics[3] = metrics[3];
    if(0)goto fail;  /* Suppress compiler warning (xxx fit antlr) */
 zzCONSUME;

    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x1);
  }
}

void
#ifdef __USE_PROTOS
valueRecord3(GNode* gnode)
#else
valueRecord3(gnode)
 GNode* gnode ;
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int val;
   val  = metric();

  if (gnode == NULL)
  zzerr("Glyph or glyph class must precede a value record.");
  gnode->metricsInfo = getNextMetricsInfoRec();
  gnode->metricsInfo->cnt = 1;
  gnode->metricsInfo->metrics[0] = val;
  if(0)goto fail;  /* Suppress compiler warning (xxx fit antlr) */
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x2);
  }
}

void
#ifdef __USE_PROTOS
anchorDef(void)
#else
anchorDef()
#endif
{
  zzRULE;
  Attrib anchor;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  short xVal, yVal;
  unsigned short contourIndex;
  int hasContour = 0;
  zzmatch(K_anchordef); zzCONSUME;
   xVal  = metric();

   yVal  = metric();

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==K_contourpoint) ) {
       contourIndex  = contourpoint();

      hasContour = 1;
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(T_GNAME);
  anchor = zzaCur;

  
  featAddAnchorDef( xVal, yVal, contourIndex,  hasContour,  anchor.text);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x4);
  }
}

int
#ifdef __USE_PROTOS
anchor(int componentIndex)
#else
anchor(componentIndex)
 int componentIndex ;
#endif
{
  int   _retv;
  zzRULE;
  Attrib anchorDefName;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(int  ))
  zzMake0;
  {
  
  short xVal, yVal;
  unsigned short contourIndex = 0;
  int isNULL = 0;
  int hasContour = 0;
  char* anchorName = NULL;
  _retv = 0;
  zzmatch(K_BeginValue); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    zzmatch(K_anchor);
    
    xVal = 0;
    yVal = 0;
    isNULL = 0;
 zzCONSUME;

    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (LA(1)==T_NUM) ) {
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
           xVal  = metric();

           yVal  = metric();

          {
            zzBLOCK(zztasp5);
            zzMake0;
            {
            if ( (LA(1)==K_contourpoint) ) {
              {
                zzBLOCK(zztasp6);
                zzMake0;
                {
                 contourIndex  = contourpoint();

                hasContour = 1;
                zzEXIT(zztasp6);
                }
              }
            }
            else {
              if ( (LA(1)==K_EndValue) ) {
              }
              else {zzFAIL(1,zzerr13,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp5);
            }
          }
          zzEXIT(zztasp4);
          }
        }
      }
      else {
        if ( (LA(1)==K_NULL) ) {
          zzmatch(K_NULL);
          _retv = 1;
          isNULL = 1;
 zzCONSUME;

        }
        else {
          if ( (LA(1)==T_GNAME) ) {
            zzmatch(T_GNAME);
            anchorDefName = zzaCur;

            anchorName =  anchorDefName.text;
 zzCONSUME;

          }
          else {zzFAIL(1,zzerr14,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      zzEXIT(zztasp3);
      }
    }
    zzmatch(K_EndValue);
    featAddAnchor(xVal, yVal, contourIndex, isNULL, hasContour, anchorName,  componentIndex);
 zzCONSUME;

    zzEXIT(zztasp2);
    }
  }
  if(0)goto fail;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd3, 0x8);
  return _retv;
  }
}

GNode *
#ifdef __USE_PROTOS
pattern(int markedOK)
#else
pattern(markedOK)
 int markedOK ;
#endif
{
  GNode *   _retv;
  zzRULE;
  Attrib t;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  GNode **insert = &_retv;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (setwd3[LA(1)]&0x10) ) {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            GID gid;
             gid  = glyph( NULL, FALSE );

            
            *insert = newNode(h);
            (*insert)->gid = gid;
            (*insert)->nextCl = NULL;
            zzEXIT(zztasp4);
            }
          }
        }
        else {
          if ( (setwd3[LA(1)]&0x20) ) {
            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              GNode *gc;
               gc  = glyphClass( 0, NULL );

              
              *insert = gc;
              zzEXIT(zztasp4);
              }
            }
          }
          else {zzFAIL(1,zzerr15,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==145) ) {
          zzmatch(145);
          
          if ( markedOK)
          {
            /* Mark this node: */
            (*insert)->flags |= FEAT_MARKED;
          }
          else
          zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

        }
        else {
          if ( (setwd3[LA(1)]&0x40) ) {
          }
          else {zzFAIL(1,zzerr16,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==K_lookup) ) {
          zzmatch(K_lookup); zzCONSUME;
          zzmatch(T_LABEL);
          t = zzaCur;

          
          {
            int labelIndex;
            if ((*insert) == NULL)
            zzerr("Glyph or glyph class must precede a lookup reference in a contextual rule.");
            
					labelIndex = featGetLabelIndex( t.text);
            (*insert)->lookupLabel = labelIndex;
            _retv->flags |= FEAT_LOOKUP_NODE; /* used to flag that lookup key was used.  */
          }
 zzCONSUME;

        }
        else {
          if ( (setwd3[LA(1)]&0x80) ) {
          }
          else {zzFAIL(1,zzerr17,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      insert = &(*insert)->nextSeq;
      zzLOOP(zztasp2);
    } while ( (setwd4[LA(1)]&0x1) );
    zzEXIT(zztasp2);
    }
  }
  (*insert) = NULL;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd4, 0x2);
  return _retv;
  }
}

GNode *
#ifdef __USE_PROTOS
pattern2(int markedOK,GNode** headP)
#else
pattern2(markedOK,headP)
 int markedOK;
GNode** headP ;
#endif
{
  GNode *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  
  GNode **insert;
  insert =  headP;
  if (*insert != NULL)
  insert = &(*insert)->nextSeq;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (setwd4[LA(1)]&0x4) ) {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            GID gid;
             gid  = glyph( NULL, FALSE );

            
            *insert = newNode(h);
            (*insert)->gid = gid;
            (*insert)->nextCl = NULL;
            zzEXIT(zztasp4);
            }
          }
        }
        else {
          if ( (setwd4[LA(1)]&0x8) ) {
            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              GNode *gc;
               gc  = glyphClass( 0, NULL );

              
              *insert = gc;
              zzEXIT(zztasp4);
              }
            }
          }
          else {zzFAIL(1,zzerr18,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==145) ) {
          zzmatch(145);
          
          if ( markedOK)
          {
            /* Mark this node: */
            (*insert)->flags |= FEAT_MARKED;
          }
          else
          zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

        }
        else {
          if ( (setwd4[LA(1)]&0x10) ) {
          }
          else {zzFAIL(1,zzerr19,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      
      {
        GNode **lastPP = &_retv;
        *lastPP = *insert;
        insert = &(*insert)->nextSeq;
      }
      zzLOOP(zztasp2);
    } while ( (setwd4[LA(1)]&0x20) );
    zzEXIT(zztasp2);
    }
  }
  (*insert) = NULL;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd4, 0x40);
  return _retv;
  }
}

GNode *
#ifdef __USE_PROTOS
pattern3(int markedOK,GNode** headP)
#else
pattern3(markedOK,headP)
 int markedOK;
GNode** headP ;
#endif
{
  GNode *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  
  GNode **insert;
  insert =  headP;
  if (*insert != NULL)
  insert = &(*insert)->nextSeq;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (setwd4[LA(1)]&0x80) ) {
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          GID gid;
           gid  = glyph( NULL, FALSE );

          
          *insert = newNode(h);
          (*insert)->gid = gid;
          (*insert)->nextCl = NULL;
          zzEXIT(zztasp4);
          }
        }
      }
      else {
        if ( (setwd5[LA(1)]&0x1) ) {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            GNode *gc;
             gc  = glyphClass( 0, NULL );

            
            *insert = gc;
            zzEXIT(zztasp4);
            }
          }
        }
        else {zzFAIL(1,zzerr20,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (LA(1)==145) ) {
        zzmatch(145);
        
        if ( markedOK)
        {
          /* Mark this node: */
          (*insert)->flags |= FEAT_MARKED;
        }
        else
        zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

      }
      else {
        if ( (setwd5[LA(1)]&0x2) ) {
        }
        else {zzFAIL(1,zzerr21,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    
    {
      GNode **lastPP = &_retv;
      *lastPP = *insert;
      insert = &(*insert)->nextSeq;
    }
    zzEXIT(zztasp2);
    }
  }
  (*insert) = NULL;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd5, 0x4);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
ignoresub_or_pos(void)
#else
ignoresub_or_pos()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  GNode *targ;
  int doSub = 1;
  unsigned int typeGSUB = GSUBChain;
  h->metricsInfo.cnt = 0;
  zzmatch(K_ignore); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_substitute) ) {
      zzmatch(K_substitute); zzCONSUME;
    }
    else {
      if ( (LA(1)==K_reverse) ) {
        zzmatch(K_reverse);
        typeGSUB=GSUBReverse;
 zzCONSUME;

      }
      else {
        if ( (LA(1)==K_position) ) {
          zzmatch(K_position);
          doSub = 0;
 zzCONSUME;

        }
        else {zzFAIL(1,zzerr22,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
    }
    zzEXIT(zztasp2);
    }
  }
   targ  = pattern( 1 );

  targ->flags |= FEAT_IGNORE_CLAUSE;
  if (doSub) addSub(targ, NULL, typeGSUB, zzline);
  else       addPos(targ, 0, 0);
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==144) ) {
      zzmatch(144); zzCONSUME;
       targ  = pattern( 1 );

      targ->flags |= FEAT_IGNORE_CLAUSE;
      if (doSub) addSub(targ, NULL, GSUBChain, zzline);
      else       addPos(targ, 0, 0);
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd5, 0x8);
  }
}

void
#ifdef __USE_PROTOS
substitute(void)
#else
substitute()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  GNode *targ;
  GNode *repl = NULL;
  int targLine;
  int type = 0;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_except) ) {
      zzmatch(K_except);
      type = GSUBChain; h->syntax.numExcept++;
 zzCONSUME;

       targ  = pattern( 1 );

      targ->flags |= FEAT_IGNORE_CLAUSE;
      addSub(targ, NULL, type, zzline);
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        while ( (LA(1)==144) ) {
          zzmatch(144); zzCONSUME;
           targ  = pattern( 1 );

          targ->flags |= FEAT_IGNORE_CLAUSE;
          addSub(targ, NULL, type, zzline);
          zzLOOP(zztasp3);
        }
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (setwd5[LA(1)]&0x10) ) {
      }
      else {zzFAIL(1,zzerr23,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_reverse) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        zzmatch(K_reverse);
        type = GSUBReverse;
 zzCONSUME;

         targ  = pattern( 1 );

        targLine = zzline;
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          if ( (LA(1)==K_by) ) {
            zzmatch(K_by); zzCONSUME;
            {
              zzBLOCK(zztasp5);
              zzMake0;
              {
              if ( (LA(1)==K_NULL) ) {
                zzmatch(K_NULL);
                addSub(targ, NULL, type, targLine);
 zzCONSUME;

              }
              else {
                if ( (setwd5[LA(1)]&0x20) ) {
                   repl  = pattern( 0 );

                  addSub(targ, repl, type, targLine);
                }
                else {zzFAIL(1,zzerr24,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
              zzEXIT(zztasp5);
              }
            }
          }
          else {
            if ( (LA(1)==146) ) {
            }
            else {zzFAIL(1,zzerr25,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp4);
          }
        }
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (LA(1)==K_substitute) ) {
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          zzmatch(K_substitute); zzCONSUME;
           targ  = pattern( 1 );

          targLine = zzline;
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (setwd5[LA(1)]&0x40) ) {
              {
                zzBLOCK(zztasp5);
                zzMake0;
                {
                if ( (LA(1)==K_by) ) {
                  zzmatch(K_by); zzCONSUME;
                }
                else {
                  if ( (LA(1)==K_from) ) {
                    zzmatch(K_from);
                    type = GSUBAlternate;
 zzCONSUME;

                  }
                  else {zzFAIL(1,zzerr26,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp5);
                }
              }
              {
                zzBLOCK(zztasp5);
                zzMake0;
                {
                if ( (LA(1)==K_NULL) ) {
                  zzmatch(K_NULL); zzCONSUME;
                }
                else {
                  if ( (setwd5[LA(1)]&0x80) ) {
                     repl  = pattern( 0 );

                  }
                  else {zzFAIL(1,zzerr27,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp5);
                }
              }
            }
            else {
              if ( (LA(1)==146) ) {
              }
              else {zzFAIL(1,zzerr28,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          addSub(targ, repl, type, targLine);
          zzEXIT(zztasp3);
          }
        }
      }
      else {zzFAIL(1,zzerr29,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd6, 0x1);
  }
}

void
#ifdef __USE_PROTOS
mark_statement(void)
#else
mark_statement()
#endif
{
  zzRULE;
  Attrib a;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  GNode *targ;
  int isNull =0;
  targ = NULL;
  h->anchorMarkInfo.cnt = 0;
  zzmatch(K_markClass); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd6[LA(1)]&0x2) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        GID gid;
         gid  = glyph( NULL, FALSE );

        
        targ = newNode(h);
        targ->gid = gid;
        targ->nextCl = NULL;
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (setwd6[LA(1)]&0x4) ) {
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          GNode *gc;
           gc  = glyphClass( 0, NULL );

          
          targ = gc;
          zzEXIT(zztasp3);
          }
        }
      }
      else {zzFAIL(1,zzerr30,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
   isNull  = anchor( 0 );

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    zzmatch(T_GCLASS);
    a = zzaCur;

    
    featAddMark(targ,  a.text);
 zzCONSUME;

    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd6, 0x8);
  }
}

void
#ifdef __USE_PROTOS
position(void)
#else
position()
#endif
{
  zzRULE;
  Attrib t, t2;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  /* This need to work for both single and pair pos lookups, and mark to base lookups. */
  GNode *lastNodeP = NULL;
  GNode *targ = NULL;
  GNode *ruleHead = NULL;
  GNode *temp = NULL;
  int enumerate = 0;
  int markedOK = 1;
  int labelIndex;
  int type = 0;
  int labelLine;
  h->metricsInfo.cnt = 0;
  h->anchorMarkInfo.cnt = 0;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_enumerate) ) {
      zzmatch(K_enumerate);
      enumerate = 1;
 zzCONSUME;

    }
    else {
      if ( (LA(1)==K_position) ) {
      }
      else {zzFAIL(1,zzerr31,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(K_position); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd6[LA(1)]&0x10) ) {
       lastNodeP  = pattern2( markedOK, &targ );

      ruleHead = lastNodeP;
    }
    else {
      if ( (setwd6[LA(1)]&0x20) ) {
      }
      else {zzFAIL(1,zzerr32,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd6[LA(1)]&0x40) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          if ( (LA(1)==T_NUM) ) {
            valueRecord3( ruleHead );
          }
          else {
            if ( (LA(1)==K_BeginValue) ) {
              valueRecord( ruleHead );
            }
            else {zzFAIL(1,zzerr33,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp4);
          }
        }
        type = GPOSSingle;
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          while ( (setwd6[LA(1)]&0x80) ) {
             lastNodeP  = pattern3( markedOK, &ruleHead );

            ruleHead = lastNodeP;
            {
              zzBLOCK(zztasp5);
              zzMake0;
              {
              if ( (LA(1)==T_NUM) ) {
                valueRecord3( ruleHead );
              }
              else {
                if ( (LA(1)==K_BeginValue) ) {
                  valueRecord( ruleHead );
                }
                else {
                  if ( (setwd7[LA(1)]&0x1) ) {
                  }
                  else {zzFAIL(1,zzerr34,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
              zzEXIT(zztasp5);
              }
            }
            zzLOOP(zztasp4);
          }
          zzEXIT(zztasp4);
          }
        }
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (LA(1)==K_lookup) ) {
        {
          zzBLOCK(zztasp3);
          zzMake0;
          {
          zzmatch(K_lookup);
          labelLine = zzline;
 zzCONSUME;

          zzmatch(T_LABEL);
          t = zzaCur;

          
          if (ruleHead == NULL)
          zzerr("Glyph or glyph class must precede a lookup reference in a contextual rule.");
          
					labelIndex = featGetLabelIndex( t.text);
          ruleHead->lookupLabel = labelIndex;
          type = 0;
          ruleHead = lastNodeP;
          type = GPOSChain;
 zzCONSUME;

          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            while ( (setwd7[LA(1)]&0x2) ) {
               lastNodeP  = pattern3( markedOK, &ruleHead );

              ruleHead = lastNodeP;
              {
                zzBLOCK(zztasp5);
                zzMake0;
                {
                if ( (LA(1)==K_lookup) ) {
                  zzmatch(K_lookup);
                  labelLine = zzline;
 zzCONSUME;

                  zzmatch(T_LABEL);
                  t2 = zzaCur;

                  
                  if (ruleHead == NULL)
                  zzerr("Glyph or glyph class must precede a lookup reference in a contextual rule.");
                  
							labelIndex = featGetLabelIndex( t2.text);
                  ruleHead->lookupLabel = labelIndex;
                  ruleHead = lastNodeP;
 zzCONSUME;

                }
                else {
                  if ( (setwd7[LA(1)]&0x4) ) {
                  }
                  else {zzFAIL(1,zzerr35,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp5);
                }
              }
              zzLOOP(zztasp4);
            }
            zzEXIT(zztasp4);
            }
          }
          zzEXIT(zztasp3);
          }
        }
      }
      else {
        if ( (LA(1)==K_cursive) ) {
          {
            zzBLOCK(zztasp3);
            zzMake0;
            {
            zzmatch(K_cursive); zzCONSUME;
             lastNodeP  = cursive( markedOK, &ruleHead );

            
            type = GPOSCursive;
            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              if ( (setwd7[LA(1)]&0x8) ) {
                 lastNodeP  = pattern2( markedOK, &lastNodeP );

              }
              else {
                if ( (LA(1)==146) ) {
                }
                else {zzFAIL(1,zzerr36,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
              zzEXIT(zztasp4);
              }
            }
            zzEXIT(zztasp3);
            }
          }
        }
        else {
          if ( (LA(1)==K_markBase) ) {
            {
              zzBLOCK(zztasp3);
              zzMake0;
              {
              zzmatch(K_markBase); zzCONSUME;
               lastNodeP  = baseToMark( markedOK, &ruleHead );

              type = GPOSMarkToBase;
              {
                zzBLOCK(zztasp4);
                zzMake0;
                {
                if ( (setwd7[LA(1)]&0x10) ) {
                   lastNodeP  = pattern2( markedOK, &lastNodeP );

                }
                else {
                  if ( (LA(1)==146) ) {
                  }
                  else {zzFAIL(1,zzerr37,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp4);
                }
              }
              zzEXIT(zztasp3);
              }
            }
          }
          else {
            if ( (LA(1)==K_markLigature) ) {
              {
                zzBLOCK(zztasp3);
                zzMake0;
                {
                zzmatch(K_markLigature); zzCONSUME;
                 lastNodeP  = ligatureMark( markedOK, &ruleHead );

                type = GPOSMarkToLigature;
                {
                  zzBLOCK(zztasp4);
                  zzMake0;
                  {
                  if ( (setwd7[LA(1)]&0x20) ) {
                     lastNodeP  = pattern2( markedOK, &lastNodeP );

                  }
                  else {
                    if ( (LA(1)==146) ) {
                    }
                    else {zzFAIL(1,zzerr38,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                  zzEXIT(zztasp4);
                  }
                }
                zzEXIT(zztasp3);
                }
              }
            }
            else {
              if ( (LA(1)==K_mark) ) {
                {
                  zzBLOCK(zztasp3);
                  zzMake0;
                  {
                  zzmatch(K_mark); zzCONSUME;
                   lastNodeP  = baseToMark( markedOK, &ruleHead );

                  type = GPOSMarkToMark;
                  {
                    zzBLOCK(zztasp4);
                    zzMake0;
                    {
                    if ( (setwd7[LA(1)]&0x40) ) {
                       lastNodeP  = pattern2( markedOK, &lastNodeP );

                    }
                    else {
                      if ( (LA(1)==146) ) {
                      }
                      else {zzFAIL(1,zzerr39,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                    }
                    zzEXIT(zztasp4);
                    }
                  }
                  zzEXIT(zztasp3);
                  }
                }
              }
              else {zzFAIL(1,zzerr40,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
    }
    zzEXIT(zztasp2);
    }
  }
  
  if (targ ==  NULL) /* there was no contextual look-ahead */
  targ = ruleHead;
  addPos(targ, type, enumerate);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd7, 0x80);
  }
}

void
#ifdef __USE_PROTOS
parameters(void)
#else
parameters()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  short params[MAX_FEAT_PARAM_NUM]; /* allow for feature param size up to MAX_FEAT_PARAM_NUM params. */
  short value;
  short index = 0;
  zzmatch(K_parameters); zzCONSUME;
   value  = parameterValue();

  params[index] = value;
  index++;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
       value  = parameterValue();

      if (index == MAX_FEAT_PARAM_NUM)
      zzerr("Too many size parameter values");
      params[index] = value;
      index++;
      zzLOOP(zztasp2);
    } while ( (setwd8[LA(1)]&0x1) );
    zzEXIT(zztasp2);
    }
  }
  addFeatureParam(g, params, index);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd8, 0x2);
  }
}

void
#ifdef __USE_PROTOS
featureNameEntry(void)
#else
featureNameEntry()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  long plat = -1;		/* Suppress optimizer warning */
  long spec = -1;		/* Suppress optimizer warning */
  long lang = -1;		/* Suppress optimizer warning */
  
		h->nameString.cnt = 0;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    zzmatch(K_name); zzCONSUME;
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (setwd8[LA(1)]&0x4) ) {
         plat  = numUInt16Ext();

        
        if (plat != HOT_NAME_MS_PLATFORM &&
        plat != HOT_NAME_MAC_PLATFORM)
        hotMsg(g, hotFATAL,
        "platform id must be %d or %d [%s %d]",
        HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM,
        INCL.file, h->linenum);
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          if ( (setwd8[LA(1)]&0x8) ) {
             spec  = numUInt16Ext();

             lang  = numUInt16Ext();

          }
          else {
            if ( (LA(1)==T_STRING) ) {
            }
            else {zzFAIL(1,zzerr41,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp4);
          }
        }
      }
      else {
        if ( (LA(1)==T_STRING) ) {
        }
        else {zzFAIL(1,zzerr42,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    zzmatch(T_STRING); zzCONSUME;
    zzmatch(146); zzCONSUME;
    zzEXIT(zztasp2);
    }
  }
  addFeatureNameString(plat, spec, lang);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd8, 0x10);
  }
}

void
#ifdef __USE_PROTOS
featureNames(void)
#else
featureNames()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  h->featNameID = 0;
  zzmatch(K_feat_names); zzCONSUME;
  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      int zzcnt=1;
      zzMake0;
      {
      do {
        featureNameEntry();
        zzLOOP(zztasp3);
      } while ( (LA(1)==K_name) );
      zzEXIT(zztasp3);
      }
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  
  addFeatureNameParam(h->g, h->featNameID);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd8, 0x20);
  }
}

void
#ifdef __USE_PROTOS
cvParameterBlock(void)
#else
cvParameterBlock()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  h->cvParameters.FeatUILabelNameID = 0;
  h->cvParameters.FeatUITooltipTextNameID = 0;
  h->cvParameters.SampleTextNameID = 0;
  h->cvParameters.NumNamedParameters = 0;
  h->cvParameters.FirstParamUILabelNameID = 0;
  h->cvParameters.charValues.cnt = 0;
  h->featNameID = 0;
  zzmatch(K_cv_params); zzCONSUME;
  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_cvUILabel) ) {
      zzmatch(K_cvUILabel); zzCONSUME;
      zzmatch(147); zzCONSUME;
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        {
          zzBLOCK(zztasp4);
          int zzcnt=1;
          zzMake0;
          {
          do {
            featureNameEntry();
            zzLOOP(zztasp4);
          } while ( (LA(1)==K_name) );
          zzEXIT(zztasp4);
          }
        }
        
        addCVNameID(h->featNameID, cvUILabelEnum);
        zzEXIT(zztasp3);
        }
      }
      zzmatch(148); zzCONSUME;
      zzmatch(146); zzCONSUME;
    }
    else {
      if ( (setwd8[LA(1)]&0x40) ) {
      }
      else {zzFAIL(1,zzerr43,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_cvToolTip) ) {
      zzmatch(K_cvToolTip); zzCONSUME;
      zzmatch(147); zzCONSUME;
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        {
          zzBLOCK(zztasp4);
          int zzcnt=1;
          zzMake0;
          {
          do {
            featureNameEntry();
            zzLOOP(zztasp4);
          } while ( (LA(1)==K_name) );
          zzEXIT(zztasp4);
          }
        }
        
        addCVNameID(h->featNameID, cvToolTipEnum);
        zzEXIT(zztasp3);
        }
      }
      zzmatch(148); zzCONSUME;
      zzmatch(146); zzCONSUME;
    }
    else {
      if ( (setwd8[LA(1)]&0x80) ) {
      }
      else {zzFAIL(1,zzerr44,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_cvSampletext) ) {
      zzmatch(K_cvSampletext); zzCONSUME;
      zzmatch(147); zzCONSUME;
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        {
          zzBLOCK(zztasp4);
          int zzcnt=1;
          zzMake0;
          {
          do {
            featureNameEntry();
            zzLOOP(zztasp4);
          } while ( (LA(1)==K_name) );
          zzEXIT(zztasp4);
          }
        }
        
        addCVNameID(h->featNameID, cvSampletextEnum);
        zzEXIT(zztasp3);
        }
      }
      zzmatch(148); zzCONSUME;
      zzmatch(146); zzCONSUME;
    }
    else {
      if ( (setwd9[LA(1)]&0x1) ) {
      }
      else {zzFAIL(1,zzerr45,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_cvParameterLabel) ) {
      {
        zzBLOCK(zztasp3);
        int zzcnt=1;
        zzMake0;
        {
        do {
          zzmatch(K_cvParameterLabel); zzCONSUME;
          zzmatch(147); zzCONSUME;
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            {
              zzBLOCK(zztasp5);
              int zzcnt=1;
              zzMake0;
              {
              do {
                featureNameEntry();
                zzLOOP(zztasp5);
              } while ( (LA(1)==K_name) );
              zzEXIT(zztasp5);
              }
            }
            
            addCVNameID(h->featNameID, kCVParameterLabelEnum);
            zzEXIT(zztasp4);
            }
          }
          zzmatch(148); zzCONSUME;
          zzmatch(146); zzCONSUME;
          zzLOOP(zztasp3);
        } while ( (LA(1)==K_cvParameterLabel) );
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (setwd9[LA(1)]&0x2) ) {
      }
      else {zzFAIL(1,zzerr46,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_cvCharacter) ) {
      {
        zzBLOCK(zztasp3);
        int zzcnt=1;
        zzMake0;
        {
        do {
          zzmatch(K_cvCharacter); zzCONSUME;
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            
            unsigned long uv = 0;
             uv  = numUInt32Ext();

            
            addCVParametersCharValue(uv);
            zzEXIT(zztasp4);
            }
          }
          zzmatch(146); zzCONSUME;
          zzLOOP(zztasp3);
        } while ( (LA(1)==K_cvCharacter) );
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (LA(1)==148) ) {
      }
      else {zzFAIL(1,zzerr47,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  
  addCVParam(h->g);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd9, 0x4);
  }
}

GNode *
#ifdef __USE_PROTOS
cursive(int markedOK,GNode** headP)
#else
cursive(markedOK,headP)
 int markedOK;
GNode** headP ;
#endif
{
  GNode *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  
  GNode **insert;
  int isNull = 0;
  insert =  headP;
  if (*insert != NULL)
  insert = &(*insert)->nextSeq;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (setwd9[LA(1)]&0x8) ) {
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          GID gid;
           gid  = glyph( NULL, FALSE );

          
          *insert = newNode(h);
          (*insert)->gid = gid;
          (*insert)->nextCl = NULL;
          zzEXIT(zztasp4);
          }
        }
      }
      else {
        if ( (setwd9[LA(1)]&0x10) ) {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            GNode *gc;
             gc  = glyphClass( 0, NULL );

            
            *insert = gc;
            zzEXIT(zztasp4);
            }
          }
        }
        else {zzFAIL(1,zzerr48,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (LA(1)==145) ) {
        zzmatch(145);
        
        if ( markedOK)
        {
          /* Mark this node: */
          (*insert)->flags |= FEAT_MARKED;
        }
        else
        zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

      }
      else {
        if ( (LA(1)==K_BeginValue) ) {
        }
        else {zzFAIL(1,zzerr49,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    
    {
      GNode **lastPP = &_retv;
      *lastPP = *insert;
      (*insert)->flags |= FEAT_IS_BASE_NODE;
      insert = &(*insert)->nextSeq;
    }
    zzEXIT(zztasp2);
    }
  }
   isNull  = anchor( 0 );

   isNull  = anchor( 0 );

  (*insert) = NULL;
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd9, 0x20);
  return _retv;
  }
}

GNode *
#ifdef __USE_PROTOS
baseToMark(int markedOK,GNode** headP)
#else
baseToMark(markedOK,headP)
 int markedOK;
GNode** headP ;
#endif
{
  GNode *   _retv;
  zzRULE;
  Attrib a;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  
  GNode **insert;
  GNode **markInsert = NULL; /* This is used to hold the union of the mark classes, for use in contextual lookups */
  int isNull;
  insert =  headP;
  if (*insert != NULL)
  insert = &(*insert)->nextSeq;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (setwd9[LA(1)]&0x40) ) {
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          GID gid;
           gid  = glyph( NULL, FALSE );

          
          *insert = newNode(h);
          (*insert)->gid = gid;
          (*insert)->nextCl = NULL;
          zzEXIT(zztasp4);
          }
        }
      }
      else {
        if ( (setwd9[LA(1)]&0x80) ) {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            GNode *gc;
             gc  = glyphClass( 0, NULL );

            
            *insert = gc;
            zzEXIT(zztasp4);
            }
          }
        }
        else {zzFAIL(1,zzerr50,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (LA(1)==145) ) {
        zzmatch(145);
        
        if ( markedOK)
        {
          /* Mark this node: */
          (*insert)->flags |= FEAT_MARKED;
        }
        else
        zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

      }
      else {
        if ( (setwd10[LA(1)]&0x1) ) {
        }
        else {zzFAIL(1,zzerr51,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    
    {
      GNode **lastPP = &_retv;
      *lastPP = *insert;
      (*insert)->flags |= FEAT_IS_BASE_NODE;
      insert = &(*insert)->nextSeq;
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd10[LA(1)]&0x2) ) {
      {
        zzBLOCK(zztasp3);
        int zzcnt=1;
        zzMake0;
        {
        do {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (setwd10[LA(1)]&0x4) ) {
              {
                zzBLOCK(zztasp5);
                zzMake0;
                {
                GID gid;
                 gid  = glyph( NULL, FALSE );

                
                *insert = newNode(h);
                (*insert)->gid = gid;
                (*insert)->nextCl = NULL;
                zzEXIT(zztasp5);
                }
              }
            }
            else {
              if ( (setwd10[LA(1)]&0x8) ) {
                {
                  zzBLOCK(zztasp5);
                  zzMake0;
                  {
                  GNode *gc;
                   gc  = glyphClass( 0, NULL );

                  
                  *insert = gc;
                  zzEXIT(zztasp5);
                  }
                }
              }
              else {zzFAIL(1,zzerr52,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (LA(1)==145) ) {
              zzmatch(145);
              
              if ( markedOK)
              {
                /* Mark this node: */
                (*insert)->flags |= FEAT_MARKED;
              }
              else
              zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

            }
            else {
              if ( (setwd10[LA(1)]&0x10) ) {
              }
              else {zzFAIL(1,zzerr53,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          
          {
            GNode **lastPP = &_retv;
            *lastPP = *insert;
            insert = &(*insert)->nextSeq;
          }
          zzLOOP(zztasp3);
        } while ( (setwd10[LA(1)]&0x20) );
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (LA(1)==K_BeginValue) ) {
      }
      else {zzFAIL(1,zzerr54,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      int zzcnt=1;
      zzMake0;
      {
      do {
         isNull  = anchor( 0 );

        zzmatch(K_mark); zzCONSUME;
        zzmatch(T_GCLASS);
        a = zzaCur;

        
        addMarkClass( a.text); /* add mark class reference to current AnchorMarkInfo for this rule */
 zzCONSUME;

        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          if ( (LA(1)==145) ) {
            zzmatch(145);
            
            if (markInsert == NULL)
            {
              markInsert = featGlyphClassCopy(g,insert, gcLookup( a.text));
              (*insert)->flags |= FEAT_MARKED;
            }
            else
            markInsert = featGlyphClassCopy(g, markInsert, gcLookup( a.text));
            /* Mark this node: */
 zzCONSUME;

          }
          else {
            if ( (setwd10[LA(1)]&0x40) ) {
            }
            else {zzFAIL(1,zzerr55,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp4);
          }
        }
        zzLOOP(zztasp3);
      } while ( (LA(1)==K_BeginValue) );
      zzEXIT(zztasp3);
      }
    }
    
    if (markInsert != NULL)
    {
      GNode **lastPP = &_retv;
      *lastPP = *insert;
      (*insert)->flags |= FEAT_IS_MARK_NODE;
      insert = &(*insert)->nextSeq;
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd10, 0x80);
  return _retv;
  }
}

GNode *
#ifdef __USE_PROTOS
ligatureMark(int markedOK,GNode** headP)
#else
ligatureMark(markedOK,headP)
 int markedOK;
GNode** headP ;
#endif
{
  GNode *   _retv;
  zzRULE;
  Attrib a;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  
  GNode **insert;
  GNode **markInsert = NULL; /* This is used to hold the union of the mark classes, for use in contextual lookups */
  int componentIndex = 0;
  insert =  headP;
  if (*insert != NULL)
  insert = &(*insert)->nextSeq;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (setwd11[LA(1)]&0x1) ) {
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          GID gid;
           gid  = glyph( NULL, FALSE );

          
          *insert = newNode(h);
          (*insert)->gid = gid;
          (*insert)->nextCl = NULL;
          zzEXIT(zztasp4);
          }
        }
      }
      else {
        if ( (setwd11[LA(1)]&0x2) ) {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            GNode *gc;
             gc  = glyphClass( 0, NULL );

            
            *insert = gc;
            zzEXIT(zztasp4);
            }
          }
        }
        else {zzFAIL(1,zzerr56,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (LA(1)==145) ) {
        zzmatch(145);
        
        if ( markedOK)
        {
          /* Mark this node: */
          (*insert)->flags |= FEAT_MARKED;
        }
        else
        zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

      }
      else {
        if ( (setwd11[LA(1)]&0x4) ) {
        }
        else {zzFAIL(1,zzerr57,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    
    {
      GNode **lastPP = &_retv;
      *lastPP = *insert;
      (*insert)->flags |= FEAT_IS_BASE_NODE;
      insert = &(*insert)->nextSeq;
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd11[LA(1)]&0x8) ) {
      {
        zzBLOCK(zztasp3);
        int zzcnt=1;
        zzMake0;
        {
        do {
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (setwd11[LA(1)]&0x10) ) {
              {
                zzBLOCK(zztasp5);
                zzMake0;
                {
                GID gid;
                 gid  = glyph( NULL, FALSE );

                
                *insert = newNode(h);
                (*insert)->gid = gid;
                (*insert)->nextCl = NULL;
                zzEXIT(zztasp5);
                }
              }
            }
            else {
              if ( (setwd11[LA(1)]&0x20) ) {
                {
                  zzBLOCK(zztasp5);
                  zzMake0;
                  {
                  GNode *gc;
                   gc  = glyphClass( 0, NULL );

                  
                  *insert = gc;
                  zzEXIT(zztasp5);
                  }
                }
              }
              else {zzFAIL(1,zzerr58,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (LA(1)==145) ) {
              zzmatch(145);
              
              if ( markedOK)
              {
                /* Mark this node: */
                (*insert)->flags |= FEAT_MARKED;
              }
              else
              zzerr("cannot mark a replacement glyph pattern");
 zzCONSUME;

            }
            else {
              if ( (setwd11[LA(1)]&0x40) ) {
              }
              else {zzFAIL(1,zzerr59,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          
          {
            GNode **lastPP = &_retv;
            *lastPP = *insert;
            insert = &(*insert)->nextSeq;
          }
          zzLOOP(zztasp3);
        } while ( (setwd11[LA(1)]&0x80) );
        zzEXIT(zztasp3);
        }
      }
    }
    else {
      if ( (LA(1)==K_BeginValue) ) {
      }
      else {zzFAIL(1,zzerr60,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      int zzcnt=1;
      zzMake0;
      {
      do {
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          
          int isNULL = 0; 
          int seenMark = 0;
           isNULL  = anchor( componentIndex );

          {
            zzBLOCK(zztasp5);
            zzMake0;
            {
            if ( (LA(1)==K_mark) ) {
              zzmatch(K_mark); zzCONSUME;
              zzmatch(T_GCLASS);
              a = zzaCur;

              
              addMarkClass( a.text); /* add mark class reference to current AnchorMarkInfo for this rule */
              seenMark = 1;
 zzCONSUME;

            }
            else {
              if ( (setwd12[LA(1)]&0x1) ) {
              }
              else {zzFAIL(1,zzerr61,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp5);
            }
          }
          
          if (!seenMark && !isNULL)
          zzerr("In mark to ligature, non-null anchor must be followed by a mark class.");
          zzEXIT(zztasp4);
          }
        }
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          if ( (LA(1)==K_LigatureComponent) ) {
            zzmatch(K_LigatureComponent);
            componentIndex++;
 zzCONSUME;

          }
          else {
            if ( (setwd12[LA(1)]&0x2) ) {
            }
            else {zzFAIL(1,zzerr62,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp4);
          }
        }
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          if ( (LA(1)==145) ) {
            zzmatch(145);
            
            /* For contextual rules, we need the union of all the mark glyphs as a glyph node in the context */
            if (markInsert == NULL)
            {
              markInsert = featGlyphClassCopy(g,insert, gcLookup( a.text));
              (*insert)->flags |= FEAT_MARKED;
            }
            else
            markInsert = featGlyphClassCopy(g, markInsert, gcLookup( a.text));
 zzCONSUME;

          }
          else {
            if ( (setwd12[LA(1)]&0x4) ) {
            }
            else {zzFAIL(1,zzerr63,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp4);
          }
        }
        zzLOOP(zztasp3);
      } while ( (LA(1)==K_BeginValue) );
      zzEXIT(zztasp3);
      }
    }
    
    if (markInsert != NULL)
    {
      GNode **lastPP = &_retv;
      *lastPP = *insert;
      (*insert)->flags |= FEAT_IS_MARK_NODE;
      insert = &(*insert)->nextSeq;
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd12, 0x8);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
glyphClassAssign(void)
#else
glyphClassAssign()
#endif
{
  zzRULE;
  Attrib a;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  GNode *tmp1;
  zzmatch(T_GCLASS);
  a = zzaCur;
 zzCONSUME;
  zzmatch(149); zzCONSUME;
   tmp1  = glyphClass( 1,  a.text );

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd12, 0x10);
  }
}

void
#ifdef __USE_PROTOS
scriptAssign(void)
#else
scriptAssign()
#endif
{
  zzRULE;
  Attrib t;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_script); zzCONSUME;
  zzmatch(T_TAG);
  t = zzaCur;

  checkTag( t.ulval, scriptTag, 1);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd12, 0x20);
  }
}

void
#ifdef __USE_PROTOS
languageAssign(void)
#else
languageAssign()
#endif
{
  zzRULE;
  Attrib t;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  int langChange;
  int incDFLT = 1;
  int oldFormatSeen = 0;
  zzmatch(K_language); zzCONSUME;
  zzmatch(T_TAG);
  t = zzaCur;

  langChange = checkTag( t.ulval, languageTag,1);
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_excludeDFLT) ) {
      zzmatch(K_excludeDFLT);
      incDFLT = 0; oldFormatSeen = 1;
 zzCONSUME;

    }
    else {
      if ( (LA(1)==K_includeDFLT) ) {
        zzmatch(K_includeDFLT);
        incDFLT = 1; oldFormatSeen = 1;
 zzCONSUME;

      }
      else {
        if ( (LA(1)==K_exclude_dflt) ) {
          zzmatch(K_exclude_dflt);
          incDFLT = 0;
 zzCONSUME;

        }
        else {
          if ( (LA(1)==K_include_dflt) ) {
            zzmatch(K_include_dflt);
            incDFLT = 1;
 zzCONSUME;

          }
          else {
            if ( (LA(1)==146) ) {
            }
            else {zzFAIL(1,zzerr64,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
    }
    zzEXIT(zztasp2);
    }
  }
  
  if (langChange != -1)
  includeDFLT(incDFLT, langChange, oldFormatSeen);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd12, 0x40);
  }
}

void
#ifdef __USE_PROTOS
namedLookupFlagValue(unsigned short * val)
#else
namedLookupFlagValue(val)
 unsigned short *val ;
#endif
{
  zzRULE;
  Attrib umfClass, matClass;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  unsigned short umfIndex = 0;
  unsigned short gdef_markclass_index = 0;
  if ( (LA(1)==K_RightToLeft) ) {
    zzmatch(K_RightToLeft);
    setLkpFlagAttribute(val,otlRightToLeft,0);
 zzCONSUME;

  }
  else {
    if ( (LA(1)==K_IgnoreBaseGlyphs) ) {
      zzmatch(K_IgnoreBaseGlyphs);
      setLkpFlagAttribute(val,otlIgnoreBaseGlyphs,0);
 zzCONSUME;

    }
    else {
      if ( (LA(1)==K_IgnoreLigatures) ) {
        zzmatch(K_IgnoreLigatures);
        setLkpFlagAttribute(val,otlIgnoreLigatures,0);
 zzCONSUME;

      }
      else {
        if ( (LA(1)==K_IgnoreMarks) ) {
          zzmatch(K_IgnoreMarks);
          setLkpFlagAttribute(val,otlIgnoreMarks,0);
 zzCONSUME;

        }
        else {
          if ( (LA(1)==K_UseMarkFilteringSet) ) {
            zzmatch(K_UseMarkFilteringSet); zzCONSUME;
            {
              zzBLOCK(zztasp2);
              zzMake0;
              {
              zzmatch(T_GCLASS);
              umfClass = zzaCur;

              
              getMarkSetIndex( umfClass.text, &umfIndex);
              setLkpFlagAttribute(val,otlUseMarkFilteringSet, umfIndex);
 zzCONSUME;

              zzEXIT(zztasp2);
              }
            }
          }
          else {
            if ( (LA(1)==K_MarkAttachmentType) ) {
              {
                zzBLOCK(zztasp2);
                zzMake0;
                {
                zzmatch(K_MarkAttachmentType); zzCONSUME;
                {
                  zzBLOCK(zztasp3);
                  zzMake0;
                  {
                  if ( (LA(1)==T_NUM) ) {
                     gdef_markclass_index  = numUInt8();

                  }
                  else {
                    if ( (LA(1)==T_GCLASS) ) {
                      {
                        zzBLOCK(zztasp4);
                        zzMake0;
                        {
                        zzmatch(T_GCLASS);
                        matClass = zzaCur;

                        
                        getGDEFMarkClassIndex( matClass.text, &gdef_markclass_index);
 zzCONSUME;

                        zzEXIT(zztasp4);
                        }
                      }
                    }
                    else {zzFAIL(1,zzerr65,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                  }
                  zzEXIT(zztasp3);
                  }
                }
                setLkpFlagAttribute(val, otlMarkAttachmentType, gdef_markclass_index);
                zzEXIT(zztasp2);
                }
              }
            }
            else {zzFAIL(1,zzerr66,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd12, 0x80);
  }
}

void
#ifdef __USE_PROTOS
lookupflagAssign(void)
#else
lookupflagAssign()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  unsigned short val = 0;
  zzmatch(K_lookupflag); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      if ( (LA(1)==T_NUM) ) {
         val  = numUInt16();

      }
      else {
        if ( (setwd13[LA(1)]&0x1) ) {
          namedLookupFlagValue( &val );
        }
        /* MR10 ()+ */ else {
          if ( zzcnt > 1 ) break;
          else {zzFAIL(1,zzerr67,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
      }
      zzcnt++; zzLOOP(zztasp2);
    } while ( 1 );
    zzEXIT(zztasp2);
    }
  }
  setLkpFlag(val);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd13, 0x2);
  }
}

void
#ifdef __USE_PROTOS
featureUse(void)
#else
featureUse()
#endif
{
  zzRULE;
  Attrib t;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_feature); zzCONSUME;
  zzmatch(T_TAG);
  t = zzaCur;

  aaltAddFeatureTag( t.ulval);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd13, 0x4);
  }
}

void
#ifdef __USE_PROTOS
subtable(void)
#else
subtable()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_subtable);
  subtableBreak();
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd13, 0x8);
  }
}

void
#ifdef __USE_PROTOS
sizemenuname(void)
#else
sizemenuname()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  long plat = -1;		/* Suppress optimizer warning */
  long spec = -1;		/* Suppress optimizer warning */
  long lang = -1;		/* Suppress optimizer warning */
  
			h->nameString.cnt = 0;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    zzmatch(K_sizemenuname); zzCONSUME;
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (setwd13[LA(1)]&0x10) ) {
         plat  = numUInt16Ext();

        
        if (plat != HOT_NAME_MS_PLATFORM &&
        plat != HOT_NAME_MAC_PLATFORM)
        hotMsg(g, hotFATAL,
        "platform id must be %d or %d [%s %d]",
        HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM,
        INCL.file, h->linenum);
        {
          zzBLOCK(zztasp4);
          zzMake0;
          {
          if ( (setwd13[LA(1)]&0x20) ) {
             spec  = numUInt16Ext();

             lang  = numUInt16Ext();

          }
          else {
            if ( (LA(1)==T_STRING) ) {
            }
            else {zzFAIL(1,zzerr68,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
          zzEXIT(zztasp4);
          }
        }
      }
      else {
        if ( (LA(1)==T_STRING) ) {
        }
        else {zzFAIL(1,zzerr69,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    zzmatch(T_STRING); zzCONSUME;
    zzEXIT(zztasp2);
    }
  }
  addSizeNameString(plat, spec, lang);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd13, 0x40);
  }
}

void
#ifdef __USE_PROTOS
statement(void)
#else
statement()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_feature) ) {
      featureUse();
    }
    else {
      if ( (LA(1)==K_script) ) {
        scriptAssign();
      }
      else {
        if ( (LA(1)==K_language) ) {
          languageAssign();
        }
        else {
          if ( (LA(1)==K_lookupflag) ) {
            lookupflagAssign();
          }
          else {
            if ( (LA(1)==T_GCLASS) ) {
              glyphClassAssign();
            }
            else {
              if ( (LA(1)==K_ignore) ) {
                ignoresub_or_pos();
              }
              else {
                if ( (setwd13[LA(1)]&0x80) ) {
                  substitute();
                }
                else {
                  if ( (LA(1)==K_markClass) ) {
                    mark_statement();
                  }
                  else {
                    if ( (setwd14[LA(1)]&0x1) ) {
                      position();
                    }
                    else {
                      if ( (LA(1)==K_parameters) ) {
                        parameters();
                      }
                      else {
                        if ( (LA(1)==K_sizemenuname) ) {
                          sizemenuname();
                        }
                        else {
                          if ( (LA(1)==K_feat_names) ) {
                            featureNames();
                          }
                          else {
                            if ( (LA(1)==K_subtable) ) {
                              subtable();
                            }
                            else {
                              if ( (LA(1)==146) ) {
                              }
                              else {zzFAIL(1,zzerr70,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd14, 0x2);
  }
}

void
#ifdef __USE_PROTOS
lookupBlockOrUse(void)
#else
lookupBlockOrUse()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int labelLine; int useExtension = 0;
  zzmatch(K_lookup); zzCONSUME;
  zzmatch(T_LABEL);
  t = zzaCur;

  labelLine = zzline;
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd14[LA(1)]&0x4) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==K_useExtension) ) {
          zzmatch(K_useExtension);
          useExtension = 1;
 zzCONSUME;

        }
        else {
          if ( (LA(1)==147) ) {
          }
          else {zzFAIL(1,zzerr71,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(147);
      
      checkLkpName( t.text, labelLine, 1, 0);
      if (useExtension)
      flagExtension(1);
 zzCONSUME;

      {
        zzBLOCK(zztasp3);
        int zzcnt=1;
        zzMake0;
        {
        do {
          statement();
          zzLOOP(zztasp3);
        } while ( (setwd14[LA(1)]&0x8) );
        zzEXIT(zztasp3);
        }
      }
      zzmatch(148);
      zzmode(LABEL_MODE);
 zzCONSUME;

      zzmatch(T_LABEL);
      u = zzaCur;

      checkLkpName( u.text, 0, 0, 0);
 zzCONSUME;

    }
    else {
      if ( (LA(1)==146) ) {
        useLkp( t.text);
      }
      else {zzFAIL(1,zzerr72,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd14, 0x10);
  }
}

void
#ifdef __USE_PROTOS
lookupBlockStandAlone(void)
#else
lookupBlockStandAlone()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  int labelLine; int useExtension = 0;
  zzmatch(K_lookup); zzCONSUME;
  zzmatch(T_LABEL);
  t = zzaCur;

  labelLine = zzline;
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    {
      zzBLOCK(zztasp3);
      zzMake0;
      {
      if ( (LA(1)==K_useExtension) ) {
        zzmatch(K_useExtension);
        useExtension = 1;
 zzCONSUME;

      }
      else {
        if ( (LA(1)==147) ) {
        }
        else {zzFAIL(1,zzerr73,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
      }
      zzEXIT(zztasp3);
      }
    }
    zzmatch(147);
    
    checkLkpName( t.text, labelLine, 1,1 );
    if (useExtension)
    flagExtension(1);
 zzCONSUME;

    {
      zzBLOCK(zztasp3);
      int zzcnt=1;
      zzMake0;
      {
      do {
        statement();
        zzLOOP(zztasp3);
      } while ( (setwd14[LA(1)]&0x20) );
      zzEXIT(zztasp3);
      }
    }
    zzmatch(148);
    zzmode(LABEL_MODE);
 zzCONSUME;

    zzmatch(T_LABEL);
    u = zzaCur;

    checkLkpName( u.text, 0, 0, 1);
 zzCONSUME;

    zzEXIT(zztasp2);
    }
  }
  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd14, 0x40);
  }
}

void
#ifdef __USE_PROTOS
featureBlock(void)
#else
featureBlock()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_feature); zzCONSUME;
  zzmatch(T_TAG);
  t = zzaCur;

  checkTag( t.ulval, featureTag, 1);
 zzCONSUME;

  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_useExtension) ) {
      zzmatch(K_useExtension);
      flagExtension(0);
 zzCONSUME;

    }
    else {
      if ( (LA(1)==147) ) {
      }
      else {zzFAIL(1,zzerr74,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      if ( (setwd14[LA(1)]&0x80) ) {
        statement();
      }
      else {
        if ( (LA(1)==K_lookup) ) {
          lookupBlockOrUse();
        }
        else {
          if ( (LA(1)==K_cv_params) ) {
            cvParameterBlock();
          }
          /* MR10 ()+ */ else {
            if ( zzcnt > 1 ) break;
            else {zzFAIL(1,zzerr75,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
      }
      zzcnt++; zzLOOP(zztasp2);
    } while ( 1 );
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, featureTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd15, 0x1);
  }
}

void
#ifdef __USE_PROTOS
baseScript(int vert,long nTag)
#else
baseScript(vert,nTag)
 int vert;
long nTag ;
#endif
{
  zzRULE;
  Attrib s, d;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  int num;
  dnaDCL(short, coord);
  dnaINIT(g->dnaCtx, coord, 5, 5);
  zzmatch(T_TAG);
  s = zzaCur;

  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  d = zzaCur;
 zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
       num  = numInt16();

      *dnaNEXT(coord) = num;
      zzLOOP(zztasp2);
    } while ( (LA(1)==T_NUM) );
    zzEXIT(zztasp2);
    }
  }
  
  if (coord.cnt !=  nTag)
  zzerr("number of coordinates not equal to number of "
  "baseline tags");
  BASEAddScript(g,  vert,  s.ulval,  d.ulval, coord.array);
  dnaFREE(coord);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd15, 0x2);
  }
}

void
#ifdef __USE_PROTOS
axisSpecs(void)
#else
axisSpecs()
#endif
{
  zzRULE;
  Attrib t;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  
  int vert = 0;
  dnaDCL(Tag, tagList);
  dnaINIT(g->dnaCtx, tagList, 5, 5);
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_HorizAxis_BaseTagList) ) {
      zzmatch(K_HorizAxis_BaseTagList); zzCONSUME;
    }
    else {
      if ( (LA(1)==K_VertAxis_BaseTagList) ) {
        zzmatch(K_VertAxis_BaseTagList);
        vert = 1;
 zzCONSUME;

      }
      else {zzFAIL(1,zzerr76,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      zzmatch(T_TAG);
      t = zzaCur;

      *dnaNEXT(tagList) =  t.ulval;
      /* printf("<%c%c%c%c> ", TAG_ARG(t.ulval)); */
      zzmode(TAG_MODE);
 zzCONSUME;

      zzLOOP(zztasp2);
    } while ( (LA(1)==T_TAG) );
    zzEXIT(zztasp2);
    }
  }
  BASESetBaselineTags(g, vert, tagList.cnt, tagList.array);
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_HorizAxis_BaseScriptList) ) {
      zzmatch(K_HorizAxis_BaseScriptList);
      if (vert == 1) zzerr("expecting \"VertAxis.BaseScriptList\"");
 zzCONSUME;

    }
    else {
      if ( (LA(1)==K_VertAxis_BaseScriptList) ) {
        zzmatch(K_VertAxis_BaseScriptList);
        if (vert == 0) zzerr("expecting \"HorizAxis.BaseScriptList\"");
 zzCONSUME;

      }
      else {zzFAIL(1,zzerr77,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  baseScript( vert, tagList.cnt );
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (LA(1)==144) ) {
      zzmatch(144);
      zzmode(TAG_MODE);
 zzCONSUME;

      baseScript( vert, tagList.cnt );
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(146);
  dnaFREE(tagList);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd15, 0x4);
  }
}

void
#ifdef __USE_PROTOS
table_BASE(void)
#else
table_BASE()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_BASE);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  axisSpecs();
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (setwd15[LA(1)]&0x8) ) {
      axisSpecs();
    }
    else {
      if ( (LA(1)==148) ) {
      }
      else {zzFAIL(1,zzerr78,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd15, 0x10);
  }
}

void
#ifdef __USE_PROTOS
table_OS_2(void)
#else
table_OS_2()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_OS_2);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        
        char panose[10];
        short valInt16;
        unsigned short arrayIndex;
        unsigned short valUInt16;
        short codePageList[kLenCodePageList];
        short unicodeRangeList[kLenUnicodeList];
        h->nameString.cnt = 0;
        if ( (LA(1)==K_TypoAscender) ) {
          zzmatch(K_TypoAscender); zzCONSUME;
           valInt16  = numInt16();

          g->font.TypoAscender = valInt16;
        }
        else {
          if ( (LA(1)==K_TypoDescender) ) {
            zzmatch(K_TypoDescender); zzCONSUME;
             valInt16  = numInt16();

            g->font.TypoDescender = valInt16;
          }
          else {
            if ( (LA(1)==K_TypoLineGap) ) {
              zzmatch(K_TypoLineGap); zzCONSUME;
               valInt16  = numInt16();

              g->font.TypoLineGap = valInt16;
            }
            else {
              if ( (LA(1)==K_winAscent) ) {
                zzmatch(K_winAscent); zzCONSUME;
                 valInt16  = numInt16();

                g->font.winAscent = valInt16;
              }
              else {
                if ( (LA(1)==K_winDescent) ) {
                  zzmatch(K_winDescent); zzCONSUME;
                   valInt16  = numInt16();

                  g->font.winDescent = valInt16;
                }
                else {
                  if ( (LA(1)==K_XHeight) ) {
                    zzmatch(K_XHeight); zzCONSUME;
                     valInt16  = metric();

                    g->font.win.XHeight = valInt16;
                  }
                  else {
                    if ( (LA(1)==K_CapHeight) ) {
                      zzmatch(K_CapHeight); zzCONSUME;
                       valInt16  = metric();

                      g->font.win.CapHeight = valInt16;
                    }
                    else {
                      if ( (LA(1)==K_Panose) ) {
                        zzmatch(K_Panose); zzCONSUME;
                         panose[0]  = numUInt8();

                         panose[1]  = numUInt8();

                         panose[2]  = numUInt8();

                         panose[3]  = numUInt8();

                         panose[4]  = numUInt8();

                         panose[5]  = numUInt8();

                         panose[6]  = numUInt8();

                         panose[7]  = numUInt8();

                         panose[8]  = numUInt8();

                         panose[9]  = numUInt8();

                        OS_2SetPanose(g, panose);
                      }
                      else {
                        if ( (LA(1)==K_FSType) ) {
                          zzmatch(K_FSType); zzCONSUME;
                           valUInt16  = numUInt16();

                          OS_2SetFSType(g, valUInt16);
                        }
                        else {
                          if ( (LA(1)==K_FSType2) ) {
                            zzmatch(K_FSType2); zzCONSUME;
                             valUInt16  = numUInt16();

                            OS_2SetFSType(g, valUInt16);
                          }
                          else {
                            if ( (LA(1)==K_UnicodeRange) ) {
                              zzmatch(K_UnicodeRange); zzCONSUME;
                              {
                                zzBLOCK(zztasp4);
                                zzMake0;
                                {
                                for (arrayIndex = 0; arrayIndex < kLenUnicodeList; arrayIndex++) unicodeRangeList[arrayIndex] = kCodePageUnSet; arrayIndex = 0;
                                {
                                  zzBLOCK(zztasp5);
                                  int zzcnt=1;
                                  zzMake0;
                                  {
                                  do {
                                     valUInt16  = numUInt16();

                                    unicodeRangeList[arrayIndex] = valUInt16; arrayIndex++;
                                    zzLOOP(zztasp5);
                                  } while ( (LA(1)==T_NUM) );
                                  zzEXIT(zztasp5);
                                  }
                                }
                                featSetUnicodeRange(g, unicodeRangeList);
                                zzEXIT(zztasp4);
                                }
                              }
                            }
                            else {
                              if ( (LA(1)==K_CodePageRange) ) {
                                zzmatch(K_CodePageRange); zzCONSUME;
                                {
                                  zzBLOCK(zztasp4);
                                  zzMake0;
                                  {
                                  for (arrayIndex = 0; arrayIndex < kLenCodePageList; arrayIndex++) codePageList[arrayIndex] = kCodePageUnSet; arrayIndex = 0;
                                  {
                                    zzBLOCK(zztasp5);
                                    int zzcnt=1;
                                    zzMake0;
                                    {
                                    do {
                                       valUInt16  = numUInt16();

                                      codePageList[arrayIndex] = valUInt16; arrayIndex++;
                                      zzLOOP(zztasp5);
                                    } while ( (LA(1)==T_NUM) );
                                    zzEXIT(zztasp5);
                                    }
                                  }
                                  featSetCodePageRange(g, codePageList);
                                  zzEXIT(zztasp4);
                                  }
                                }
                              }
                              else {
                                if ( (LA(1)==K_WeightClass) ) {
                                  zzmatch(K_WeightClass); zzCONSUME;
                                   valUInt16  = numUInt16();

                                  OS_2SetWeightClass(g, valUInt16);
                                }
                                else {
                                  if ( (LA(1)==K_WidthClass) ) {
                                    zzmatch(K_WidthClass); zzCONSUME;
                                     valUInt16  = numUInt16();

                                    OS_2SetWidthClass(g, valUInt16);
                                  }
                                  else {
                                    if ( (LA(1)==K_LowerOpticalPointSize) ) {
                                      zzmatch(K_LowerOpticalPointSize); zzCONSUME;
                                       valUInt16  = numUInt16();

                                      OS_2LowerOpticalPointSize(g, valUInt16);
                                    }
                                    else {
                                      if ( (LA(1)==K_UpperOpticalPointSize) ) {
                                        zzmatch(K_UpperOpticalPointSize); zzCONSUME;
                                         valUInt16  = numUInt16();

                                        OS_2UpperOpticalPointSize(g, valUInt16);
                                      }
                                      else {
                                        if ( (LA(1)==K_Vendor) ) {
                                          {
                                            zzBLOCK(zztasp4);
                                            zzMake0;
                                            {
                                            zzmatch(K_Vendor); zzCONSUME;
                                            zzmatch(T_STRING); zzCONSUME;
                                            zzEXIT(zztasp4);
                                            }
                                          }
                                          addVendorString(g);
                                        }
                                        else {
                                          if ( (LA(1)==146) ) {
                                          }
                                          else {zzFAIL(1,zzerr79,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(146); zzCONSUME;
      zzLOOP(zztasp2);
    } while ( (setwd15[LA(1)]&0x20) );
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd15, 0x40);
  }
}

GNode *
#ifdef __USE_PROTOS
glyphClassOptional(void)
#else
glyphClassOptional()
#endif
{
  GNode *   _retv;
  zzRULE;
  zzBLOCK(zztasp1);
  PCCTS_PURIFY(_retv,sizeof(GNode *  ))
  zzMake0;
  {
  _retv = NULL;
  if ( (setwd15[LA(1)]&0x80) ) {
     _retv  = glyphClass( 0, NULL );

  }
  else {
    if ( (setwd16[LA(1)]&0x1) ) {
      _retv = NULL;
    }
    else {zzFAIL(1,zzerr80,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
  }
  zzEXIT(zztasp1);
  return _retv;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd16, 0x2);
  return _retv;
  }
}

void
#ifdef __USE_PROTOS
table_GDEF(void)
#else
table_GDEF()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_GDEF);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    while ( (setwd16[LA(1)]&0x4) ) {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        GNode *gc[4];
        short val;
        if ( (LA(1)==K_GlyphClassDef) ) {
          zzmatch(K_GlyphClassDef); zzCONSUME;
           gc[0]  = glyphClassOptional();

          zzmatch(144); zzCONSUME;
           gc[1]  = glyphClassOptional();

          zzmatch(144); zzCONSUME;
           gc[2]  = glyphClassOptional();

          zzmatch(144); zzCONSUME;
           gc[3]  = glyphClassOptional();

          setGDEFGlyphClassDef(gc[0],gc[1],gc[2],gc[3]);
        }
        else {
          if ( (LA(1)==K_Attach) ) {
            {
              zzBLOCK(zztasp4);
              zzMake0;
              {
              zzmatch(K_Attach); zzCONSUME;
               gc[0]  = pattern( 0 );

              {
                zzBLOCK(zztasp5);
                int zzcnt=1;
                zzMake0;
                {
                do {
                   val  = metric();

                  addGDEFAttach(gc[0], val);
                  zzLOOP(zztasp5);
                } while ( (LA(1)==T_NUM) );
                zzEXIT(zztasp5);
                }
              }
              zzEXIT(zztasp4);
              }
            }
          }
          else {
            if ( (LA(1)==K_LigatureCaret1) ) {
              {
                zzBLOCK(zztasp4);
                zzMake0;
                {
                zzmatch(K_LigatureCaret1); zzCONSUME;
                 gc[0]  = pattern( 0 );

                {
                  zzBLOCK(zztasp5);
                  int zzcnt=1;
                  zzMake0;
                  {
                  do {
                     val  = metric();

                    setGDEFLigatureCaret(gc[0], val, 1);
                    zzLOOP(zztasp5);
                  } while ( (LA(1)==T_NUM) );
                  zzEXIT(zztasp5);
                  }
                }
                zzEXIT(zztasp4);
                }
              }
            }
            else {
              if ( (LA(1)==K_LigatureCaret2) ) {
                {
                  zzBLOCK(zztasp4);
                  zzMake0;
                  {
                  zzmatch(K_LigatureCaret2); zzCONSUME;
                   gc[0]  = pattern( 0 );

                  {
                    zzBLOCK(zztasp5);
                    int zzcnt=1;
                    zzMake0;
                    {
                    do {
                       val  = metric();

                      setGDEFLigatureCaret(gc[0], val, 2);
                      zzLOOP(zztasp5);
                    } while ( (LA(1)==T_NUM) );
                    zzEXIT(zztasp5);
                    }
                  }
                  zzEXIT(zztasp4);
                  }
                }
              }
              else {
                if ( (LA(1)==146) ) {
                }
                else {zzFAIL(1,zzerr81,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(146); zzCONSUME;
      zzLOOP(zztasp2);
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd16, 0x8);
  }
}

void
#ifdef __USE_PROTOS
table_head(void)
#else
table_head()
#endif
{
  zzRULE;
  Attrib t, r, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_head);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_FontRevision) ) {
      zzmatch(K_FontRevision); zzCONSUME;
      zzmatch(T_FONTREV);
      r = zzaCur;

      setFontRev( r.text);
 zzCONSUME;

    }
    else {
      if ( (LA(1)==146) ) {
      }
      else {zzFAIL(1,zzerr82,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(146); zzCONSUME;
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd16, 0x10);
  }
}

void
#ifdef __USE_PROTOS
table_hhea(void)
#else
table_hhea()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_hhea);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        short value;
        if ( (LA(1)==K_CaretOffset) ) {
          zzmatch(K_CaretOffset); zzCONSUME;
           value  = numInt16();

          hheaSetCaretOffset(g, value);
        }
        else {
          if ( (LA(1)==K_Ascender) ) {
            zzmatch(K_Ascender); zzCONSUME;
             value  = numInt16();

            g->font.hheaAscender = value;
          }
          else {
            if ( (LA(1)==K_Descender) ) {
              zzmatch(K_Descender); zzCONSUME;
               value  = numInt16();

              g->font.hheaDescender = value;
            }
            else {
              if ( (LA(1)==K_LineGap) ) {
                zzmatch(K_LineGap); zzCONSUME;
                 value  = numInt16();

                g->font.hheaLineGap = value;
              }
              else {
                if ( (LA(1)==146) ) {
                }
                else {zzFAIL(1,zzerr83,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(146); zzCONSUME;
      zzLOOP(zztasp2);
    } while ( (setwd16[LA(1)]&0x20) );
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd16, 0x40);
  }
}

void
#ifdef __USE_PROTOS
table_name(void)
#else
table_name()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_name);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    
    int ignoreRec = 0;	/* Suppress optimizer warning */
    long id = 0;		/* Suppress optimizer warning */
    long plat = 0;		/* Suppress optimizer warning */
    long spec = 0;		/* Suppress optimizer warning */
    long lang = 0;		/* Suppress optimizer warning */
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        if ( (LA(1)==K_nameid) ) {
          zzmatch(K_nameid);
          
          h->nameString.cnt = 0;
          plat = -1;
          spec = -1;
          lang = -1;
          ignoreRec = 0;
 zzCONSUME;

           id  = numUInt16Ext();

          {
            zzBLOCK(zztasp4);
            zzMake0;
            {
            if ( (setwd16[LA(1)]&0x80) ) {
               plat  = numUInt16Ext();

              
              if (plat != HOT_NAME_MS_PLATFORM &&
              plat != HOT_NAME_MAC_PLATFORM)
              hotMsg(g, hotFATAL,
              "platform id must be %d or %d [%s %d]",
              HOT_NAME_MS_PLATFORM, HOT_NAME_MAC_PLATFORM,
              INCL.file, h->linenum);
              {
                zzBLOCK(zztasp5);
                zzMake0;
                {
                if ( (setwd17[LA(1)]&0x1) ) {
                   spec  = numUInt16Ext();

                   lang  = numUInt16Ext();

                }
                else {
                  if ( (LA(1)==T_STRING) ) {
                  }
                  else {zzFAIL(1,zzerr84,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
                zzEXIT(zztasp5);
                }
              }
            }
            else {
              if ( (LA(1)==T_STRING) ) {
              }
              else {zzFAIL(1,zzerr85,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
            zzEXIT(zztasp4);
            }
          }
          zzmatch(T_STRING); zzCONSUME;
        }
        else {
          if ( (LA(1)==146) ) {
          }
          else {zzFAIL(1,zzerr86,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(146);
      if (!ignoreRec) addNameString(plat, spec, lang, id);
 zzCONSUME;

      zzLOOP(zztasp2);
    } while ( (setwd17[LA(1)]&0x2) );
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd17, 0x4);
  }
}

void
#ifdef __USE_PROTOS
table_vhea(void)
#else
table_vhea()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_vhea);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        short value;
        if ( (LA(1)==K_VertTypoAscender) ) {
          zzmatch(K_VertTypoAscender); zzCONSUME;
           value  = numInt16();

          g->font.VertTypoAscender = value;
        }
        else {
          if ( (LA(1)==K_VertTypoDescender) ) {
            zzmatch(K_VertTypoDescender); zzCONSUME;
             value  = numInt16();

            g->font.VertTypoDescender = value;
          }
          else {
            if ( (LA(1)==K_VertTypoLineGap) ) {
              zzmatch(K_VertTypoLineGap); zzCONSUME;
               value  = numInt16();

              g->font.VertTypoLineGap = value;
            }
            else {
              if ( (LA(1)==146) ) {
              }
              else {zzFAIL(1,zzerr87,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(146); zzCONSUME;
      zzLOOP(zztasp2);
    } while ( (setwd17[LA(1)]&0x8) );
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd17, 0x10);
  }
}

void
#ifdef __USE_PROTOS
table_vmtx(void)
#else
table_vmtx()
#endif
{
  zzRULE;
  Attrib t, u;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_vmtx);
  t = zzaCur;

  checkTag( t.ulval, tableTag, 1);
 zzCONSUME;

  zzmatch(147); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      {
        zzBLOCK(zztasp3);
        zzMake0;
        {
        GID gid; short value;
        if ( (LA(1)==K_VertOriginY) ) {
          zzmatch(K_VertOriginY); zzCONSUME;
           gid  = glyph( NULL, FALSE );

           value  = numInt16();

          
          hotAddVertOriginY(g, gid, value, INCL.file, zzline);
        }
        else {
          if ( (LA(1)==K_VertAdvanceY) ) {
            zzmatch(K_VertAdvanceY); zzCONSUME;
             gid  = glyph( NULL, FALSE );

             value  = numInt16();

            
            hotAddVertAdvanceY(g, gid, value, INCL.file, zzline);
          }
          else {
            if ( (LA(1)==146) ) {
            }
            else {zzFAIL(1,zzerr88,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
          }
        }
        zzEXIT(zztasp3);
        }
      }
      zzmatch(146); zzCONSUME;
      zzLOOP(zztasp2);
    } while ( (setwd17[LA(1)]&0x20) );
    zzEXIT(zztasp2);
    }
  }
  zzmatch(148);
  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  u = zzaCur;

  checkTag( u.ulval, tableTag, 0);
 zzCONSUME;

  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd17, 0x40);
  }
}

void
#ifdef __USE_PROTOS
tableBlock(void)
#else
tableBlock()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_table); zzCONSUME;
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==K_BASE) ) {
      table_BASE();
    }
    else {
      if ( (LA(1)==K_GDEF) ) {
        table_GDEF();
      }
      else {
        if ( (LA(1)==K_head) ) {
          table_head();
        }
        else {
          if ( (LA(1)==K_hhea) ) {
            table_hhea();
          }
          else {
            if ( (LA(1)==K_name) ) {
              table_name();
            }
            else {
              if ( (LA(1)==K_OS_2) ) {
                table_OS_2();
              }
              else {
                if ( (LA(1)==K_vhea) ) {
                  table_vhea();
                }
                else {
                  if ( (LA(1)==K_vmtx) ) {
                    table_vmtx();
                  }
                  else {zzFAIL(1,zzerr89,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
                }
              }
            }
          }
        }
      }
    }
    zzEXIT(zztasp2);
    }
  }
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd17, 0x80);
  }
}

void
#ifdef __USE_PROTOS
languagesystemAssign(void)
#else
languagesystemAssign()
#endif
{
  zzRULE;
  Attrib s, t;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_languagesystem); zzCONSUME;
  zzmatch(T_TAG);
  s = zzaCur;

  zzmode(TAG_MODE);
 zzCONSUME;

  zzmatch(T_TAG);
  t = zzaCur;

  addLangSys( s.ulval,  t.ulval, 1);
 zzCONSUME;

  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd18, 0x1);
  }
}

void
#ifdef __USE_PROTOS
topLevelStatement(void)
#else
topLevelStatement()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  {
    zzBLOCK(zztasp2);
    zzMake0;
    {
    if ( (LA(1)==T_GCLASS) ) {
      glyphClassAssign();
    }
    else {
      if ( (LA(1)==K_languagesystem) ) {
        languagesystemAssign();
      }
      else {
        if ( (LA(1)==K_markClass) ) {
          mark_statement();
        }
        else {
          if ( (LA(1)==K_anchordef) ) {
            anchorDef();
          }
          else {
            if ( (LA(1)==K_valueRecordDef) ) {
              valueRecordDef();
            }
            else {
              if ( (LA(1)==146) ) {
              }
              else {zzFAIL(1,zzerr90,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
            }
          }
        }
      }
    }
    zzEXIT(zztasp2);
    }
  }
  zzmatch(146); zzCONSUME;
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd18, 0x2);
  }
}

void
#ifdef __USE_PROTOS
anonBlock(void)
#else
anonBlock()
#endif
{
  zzRULE;
  Attrib t;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  zzmatch(K_anon); zzCONSUME;
  zzmatch(T_TAG);
  t = zzaCur;

  h->anonData.tag =  t.ulval;
 zzCONSUME;

  featAddAnonData(); featSetTagReturnMode(START);
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd18, 0x4);
  }
}

void
#ifdef __USE_PROTOS
featureFile(void)
#else
featureFile()
#endif
{
  zzRULE;
  zzBLOCK(zztasp1);
  zzMake0;
  {
  {
    zzBLOCK(zztasp2);
    int zzcnt=1;
    zzMake0;
    {
    do {
      if ( (setwd18[LA(1)]&0x8) ) {
        topLevelStatement();
      }
      else {
        if ( (LA(1)==K_feature) ) {
          featureBlock();
        }
        else {
          if ( (LA(1)==K_table) ) {
            tableBlock();
          }
          else {
            if ( (LA(1)==K_anon) ) {
              anonBlock();
            }
            else {
              if ( (LA(1)==K_lookup) ) {
                lookupBlockStandAlone();
              }
              /* MR10 ()+ */ else {
                if ( zzcnt > 1 ) break;
                else {zzFAIL(1,zzerr91,&zzMissSet,&zzMissText,&zzBadTok,&zzBadText,&zzErrk); goto fail;}
              }
            }
          }
        }
      }
      zzcnt++; zzLOOP(zztasp2);
    } while ( 1 );
    zzEXIT(zztasp2);
    }
  }
  if (zzchar != EOF) zzerrCustom("expecting EOF");
  zzEXIT(zztasp1);
  return;
fail:
  zzEXIT(zztasp1);
  zzsyn(zzMissText, zzBadTok, (ANTLRChar *)"", zzMissSet, zzMissTok, zzErrk, zzBadText);
  zzresynch(setwd18, 0x10);
  }
}
