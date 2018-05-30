/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)proof.c	1.34
 * Changed:    8/4/99
 ***********************************************************************/

#include <stdio.h>
#if OSX
	#include <sys/time.h>
#else
	#include <time.h>
#endif
#include <string.h>
#if MACINTOSH
#include <Carbon.h>
#include <Memory.h>
#include <Strings.h>
#include <Windows.h>
#elif WIN32
#include <windows.h>
#include <winspool.h>
#define TYPEDEF_boolean 1
#endif
#include "global.h"
#include "GLOB.h"
#include "glyf.h"
#include "BASE.h"
#include "hhea.h"
#include "head.h"
#include "CFF_.h"
#include "name.h"
#include "OS_2.h"
#include "sfnt.h"

#if SUNOS
extern int fputs(const char *s, FILE *stream);
#if AUTOSPOOL
extern int system(const char *command);
extern char *getenv(const char *name);
extern char *mktemp(char *template);

#endif
#endif

extern char * outputfilebase;

#include "proof.h"
#include "sys.h" /* sysOurtime() */

typedef struct _ProofContext 
	{
	proofOutputType kind; 
	double left, right, top, bottom; /* of imageable area */
	Card16 page; /* page number */
	Byte8 *title; /* page title */
	Byte8 *title2; /* page title2 */
	double currx, curry; /* absolute coords */
	double glyphSize; /* optimal glyph size: needed for newline */
	double thinspace; /* extra inter-glyph space: in points */
	double unitsPerEm; /* for scaling */
	Byte8 *psfilename;
	Byte8 onNewLine; /*To avoid unnecessary linefeeds*/
	FILE *psfileptr;
	Int16 bbLeft, bbBottom, bbRight, bbTop;
	
#if MACINTOSH
	PMPrintSession printSession;
 	PMPageFormat pageFormat;
    PMPrintSettings printSettings;
    GrafPtr printingPort;
    GrafPtr currPort;
#define MACPRINTBUFSIZE 256
	Handle psCommentBuf;	   /* Handle to output buffer (is always locked) */

#elif WIN32
	HDC winPrinterDC;
	DOCINFO docinfo;
#define PASSTHROUGHSIZE 256
	LPSTR lpPassThrough;
#endif
	} ProofContext;

#if MACINTOSH
enum picCommentsIDs
	  {
	  picAppComment = 100,
	  picPostScriptBegin = 190,
	  picPostScriptEnd = 191,
	  picPostScriptHandle = 192,
	  picTextIsPostScript = 194	/* NOTE: An Apple Tech Note describes this comment as "Obsolete". */
	  };


Int16 MacPrintOpen(ProofContext *ctx);
Int16 MacPrintClose(ProofContext *ctx);
Int16 MacPrintOpenPage(ProofContext *ctx);
Int16 MacPrintInitPage(ProofContext *ctx);
Int16 MacPrintClosePage(ProofContext *ctx);
#elif WIN32
Int16 WinPrintOpen(ProofContext *ctx);
Int16 WinPrintClose(ProofContext *ctx);
Int16 WinPrintOpenPage(ProofContext *ctx);
Int16 WinPrintInitPage(ProofContext *ctx);
Int16 WinPrintClosePage(ProofContext *ctx);
#endif


#define NUMERIC_LABEL_SIZE 5	/* Numeric label size (points) */
#define TITLE_LABEL_SIZE  12
#define LEADING 1.5
#define kMaxMessageLength 70


static IntX PolicyNoNames = 0;
static IntX PolicyNoNumericLabels = 0;
static IntX PolicyLines = 0;
static IntX PolicyKanjiEMbox = 0;
static IntX PolicyKanjiGlyphBBox = 0;
static IntX PolicyKanjiVertical = 0;
static IntX tempPolicyKanjiVertical = 0;
static IntX PolicyKanjiKernAltMetrics = 1;

static Byte8 PSProlog[]="\n%%BeginPageSetup\n/_MT{moveto}bind def /_LT{lineto}bind def\n/_CT{curveto}bind def /_CP{closepath}bind def\n/_SP{save /Courier-Bold findfont 8 scalefont setfont 20 20 moveto restore showpage}bind def\n%%EndPageSetup\n";

/* "showpage" is handled by Driver: */
static Byte8 PSPrologNS[]="\r%%BeginPageSetup\r/_MT{moveto}bind def /_LT{lineto}bind def\r/_CT{curveto}bind def /_CP{closepath}bind def\r/_SP{save /Courier-Bold findfont 8 scalefont setfont 20 20 moveto restore }bind def\r%%EndPageSetup\r";




#define RND(c)		   ((IntX)((c)<0?(c)-0.5:(c)+0.5))
#define FNT2STD(c)	   ((c)*1000.0/ctx->unitsPerEm)
#define STD2FNT(c)	   ((c)*ctx->unitsPerEm/1000.0)	
#define FNT2ABS(c)	 (((c)/ctx->unitsPerEm) * ctx->glyphSize)
#define ABS2FNT(c)	 (((c)*ctx->unitsPerEm) / ctx->glyphSize)

IntX proofIsVerticalMode(void);

static Byte8 str[256];

extern Byte8 *version;

static double proofGlyphSize = STDPAGE_GLYPH_PTSIZE;

char mMAX(int x, int y)
{
	if (x>y)
		return x;
	else
		return y;
}

double proofCurrentGlyphSize(void)
{
  return proofGlyphSize;
}
void proofSetGlyphSize(double siz)
{
  if (siz > 5.0)
	proofGlyphSize = siz;
}

void proofPSOUT(ProofContextPtr ctx, const Byte8 *cmd)
	{		 
	  if (ctx->kind == proofPS) {
		if (ctx->psfileptr != NULL)
		  fputs(cmd, ctx->psfileptr);
#if AUTOSPOOL
		else 
#if MACINTOSH
		{
		  Int16 length = 0;
		  OSStatus result = noErr;
		  length = strlen(cmd);
		  if (length >= (MACPRINTBUFSIZE - 1))
			fatal(SPOT_MSG_prufSTR2BIG);
		  memset(((char *)(*(ctx->psCommentBuf))), 32, MACPRINTBUFSIZE -1);
		  memcpy(((char *)(*(ctx->psCommentBuf))), cmd, length);
		  result = PMSessionPostScriptData(ctx->printSession, *(ctx->psCommentBuf), length);
		  if (result != noErr)
			fatal(SPOT_MSG_prufWRTFAIL);
		}
#elif WIN32
		{
			Int32 length;
			int n;
			length = lstrlen ((LPSTR)cmd);
			if (length >= (PASSTHROUGHSIZE - 1))
				fatal(SPOT_MSG_prufSTR2BIG);
			n = (int)length;
		    memset(((char *)(ctx->lpPassThrough)), 32, PASSTHROUGHSIZE -1);
			memcpy(ctx->lpPassThrough + 2, cmd, length);
			memcpy(ctx->lpPassThrough, &n, 2);

			n = Escape(ctx->winPrinterDC, PASSTHROUGH, (short)(2 + length), ctx->lpPassThrough, NULL);
		}
#else
		{
			fatal(SPOT_MSG_prufWRTFAIL);
		}
#endif
#endif
	  }
	}

static void proofPageProlog(ProofContextPtr ctx)
	{
	char* platformProlog;
	
	if (ctx->psfileptr != NULL) /* output to file */
		{
		platformProlog = PSProlog;
		}
	 else /* output to spooler */
		{
		platformProlog = PSPrologNS;
		}
		
		sprintf(str, "%%%%Page: body %d %s", ctx->page +1, platformProlog);
		proofPSOUT(ctx,str); 
	}



static void proofInitPage(ProofContextPtr ctx)
	{
	  sprintf(str,"/SYM /Symbol findfont %d scalefont def\n", RND(ctx->glyphSize));
	  proofPSOUT(ctx, str);
	  sprintf(str,"/LAB /Times-Roman findfont %g scalefont def\n", ABS2FNT(NUMERIC_LABEL_SIZE));
	  proofPSOUT(ctx, str);
	  sprintf(str,"/BLAB /Times-Bold findfont %g scalefont def\n", ABS2FNT(NUMERIC_LABEL_SIZE));
	  proofPSOUT(ctx, str);
	  sprintf(str,"/EMLAB /Times-BoldItalic findfont %g scalefont def\n", ABS2FNT(NUMERIC_LABEL_SIZE));
	  proofPSOUT(ctx, str);
	  sprintf(str,"/TITL /Times-Roman findfont %d scalefont def\n", TITLE_LABEL_SIZE);
	  proofPSOUT(ctx, str);

	  sprintf(str, "%% ================= %s ==============\n", ctx->title);
	  proofPSOUT(ctx, str);
	}


void proofNewPage(ProofContextPtr ctx)
	{
	  if (ctx->kind == proofPS) {
		if (ctx->page > 0) 
		  {
		  proofPSOUT(ctx, "_SP\n"); /* the showpage command*/
		  proofPageProlog(ctx); /* print page DSC and prolog for new page */

#if AUTOSPOOL
		  if (ctx->psfileptr)
			inform(SPOT_MSG_prufPROGRESS);
#if MACINTOSH
		  MacPrintClosePage(ctx);
		  MacPrintOpenPage(ctx);
		  MacPrintInitPage(ctx);
#elif WIN32
		  inform(SPOT_MSG_prufPROGRESS);
		  WinPrintClosePage(ctx);
		  WinPrintOpenPage(ctx);
		  WinPrintInitPage(ctx);
#endif
#endif
		  }
		 else
		 	{
		 	sprintf(str, "%%!PS-Adobe-3.0\n");
		  	proofPSOUT(ctx,str);
		  	
		    proofPageProlog(ctx); /* print page DSC and prolog for new page */
		 	}

		proofInitPage(ctx);
		ctx->page += 1;
   
		/* write title */
		proofPSOUT(ctx, str);
		sprintf(str,"gsave TITL setfont %g %g _MT (%s) show %g %g _MT (%s) show  %g %g _MT (page %d) show grestore\n",
			ctx->left,  ctx->top - (1 * TITLE_LABEL_SIZE),  ctx->title, 
			ctx->left, ctx->top - (2 * TITLE_LABEL_SIZE), ctx->title2,
			ctx->right -30, ctx->top - (2 * TITLE_LABEL_SIZE), ctx->page);
			
		proofPSOUT(ctx, str);

		/* set position for drawing glyphs.*/
		if(proofIsVerticalMode())
			{
	 		double amt = (ctx->glyphSize + 3*NUMERIC_LABEL_SIZE) * LEADING;
			ctx->currx = ctx->right - amt;
			/* ctx->currx = ctx->left; */
			}
		else
			ctx->currx = ctx->left;
			
		ctx->curry = ctx->top -  ((3 * TITLE_LABEL_SIZE) + ctx->glyphSize); /* check same seting in proofMessage.*/
			
		sprintf(str,"%g %g _MT\n", ctx->currx, ctx->curry);
		proofPSOUT(ctx, str);
		
	  }
   }

void proofOnlyNewPage(ProofContextPtr ctx)
	{
	  if (ctx->kind == proofPS) {
		if (ctx->page > 0) {
		  proofPSOUT(ctx, "_SP\n"); /* the showpage command*/
		  proofPageProlog(ctx); /* print page DSC and prolog for new page */

#if AUTOSPOOL
		  if (ctx->psfileptr)
			inform(SPOT_MSG_prufPROGRESS);
#if MACINTOSH
		  MacPrintClosePage(ctx);
		  MacPrintOpenPage(ctx);
		  MacPrintInitPage(ctx);
#elif WIN32
		  inform(SPOT_MSG_prufPROGRESS);
		  WinPrintClosePage(ctx);
		  WinPrintOpenPage(ctx);
		  WinPrintInitPage(ctx);
#endif
#endif
		  }
		else
			{
		 	sprintf(str, "%%!PS-Adobe-3.0\n");
		  	proofPSOUT(ctx,str);
		  	
		    proofPageProlog(ctx); /* print page DSC and prolog for new page */
		 	}
		 	
		proofInitPage(ctx);
		ctx->page += 1;
		
		/* set position for drawing glyphs.*/
		if(proofIsVerticalMode())
			{
	 		double amt = (ctx->glyphSize + 3*NUMERIC_LABEL_SIZE) * LEADING;
			ctx->currx = ctx->right - amt;
			/* ctx->currx = ctx->left; */
			}
		else
			ctx->currx = ctx->left;
			
		ctx->curry = ctx->top -  ((3 * TITLE_LABEL_SIZE) + ctx->glyphSize);
			
		sprintf(str,"%g %g _MT\n", ctx->currx, ctx->curry);
		proofPSOUT(ctx, str);
	  }
   }


IntX proofPageCount(ProofContextPtr ctx)
	{
	  return ctx->page;
	}

Byte8 *proofFileName(ProofContextPtr ctx)
	{
	  return ctx->psfilename;
	}

void proofNewline(ProofContextPtr ctx)
	{
	  double amt;

	  ctx->onNewLine=1;
	  amt = (ctx->glyphSize + 3*NUMERIC_LABEL_SIZE) * LEADING;

	  if (ctx->kind == proofPS) {
		if (proofIsVerticalMode()) 
		  {
			/* ctx->currx += amt + (ctx->glyphSize/2); */
			ctx->currx -= amt + (ctx->glyphSize/2);
			ctx->curry = ctx->top -  ((3 * TITLE_LABEL_SIZE) + ctx->glyphSize);
			sprintf(str,"\n%g %g _MT %% Vertical Newline\n", ctx->currx, ctx->curry);
			proofPSOUT(ctx, str);		
			/* if (ctx->currx > ctx->right) */
			if (ctx->currx < ctx->left)
			  proofNewPage(ctx);	
		  }
		else
		  {
			ctx->currx = ctx->left;
			ctx->curry -= amt;
			sprintf(str,"\n%g %g _MT %% Newline\n", ctx->currx, ctx->curry);
			proofPSOUT(ctx, str);		
			if (ctx->curry < ctx->bottom)
			  proofNewPage(ctx);	
		  }
	  }
   }

static void checkNewline(ProofContextPtr ctx)
	{
	  if (ctx->kind == proofPS) {
		if (proofIsVerticalMode())
		  {
			if (ctx->curry < ctx->bottom)
			  proofNewline(ctx);
		  }
		else
		  {
			if (ctx->currx > ctx->right)
			  proofNewline(ctx);
		  }
	  }
   }

static void advance(ProofContextPtr ctx, Int16 wx)
	{ /* in absolute Page-space coords*/
	  if (proofIsVerticalMode())
		{
		  Int16 abswx;
		  abswx = abs(wx);
		  ctx->curry -= FNT2ABS(abswx);
		}
	  else
		{
		  ctx->currx += FNT2ABS(wx);
		}
	  checkNewline(ctx);
	  sprintf(str,"%g %g _MT\n", ctx->currx, ctx->curry);
	  proofPSOUT(ctx, str);		
	}

void proofCheckAdvance(ProofContextPtr ctx, Int16 wx)
	{
	  if (ctx->kind == proofPS) {
		if (proofIsVerticalMode())
		  {
			Int16 abswx;
			abswx = abs(wx);
			if ((ctx->curry - FNT2ABS(abswx)) < ctx->bottom)
			  proofNewline(ctx);
		  }
		else
		  {
			if ((ctx->currx + FNT2ABS(wx)) > ctx->right)
			  proofNewline(ctx);
		  }
	  }
	}


static void advanceSYM(ProofContextPtr ctx, Int16 wx)
	{ /* in absolute Page-space coords*/
	  if (proofIsVerticalMode())
		ctx->curry -= (wx/1000.0)*ctx->glyphSize;
	  else
		ctx->currx += (wx/1000.0)*ctx->glyphSize;

	  checkNewline(ctx);
	  sprintf(str,"%g %g _MT\n", ctx->currx, ctx->curry);
	  proofPSOUT(ctx, str);		
	}


void proofThinspace(ProofContextPtr ctx, IntX count)
	{
	  IntX amt;

	  amt = count * (IntX)(proofGlyphSize / 2.4);

	  if (ctx->kind == proofPS) {
		if (proofIsVerticalMode())
		  {
			sprintf(str,"0 -%d rmoveto %%thin\n", amt);
			proofPSOUT(ctx, str);
			ctx->curry -= amt;
			checkNewline(ctx);
		  }
		else
		  {
			sprintf(str,"%d 0 rmoveto %%thin\n", amt);
			proofPSOUT(ctx, str);
			ctx->currx += amt;
			checkNewline(ctx);
		  }
	  }
	}

void proofMessage(ProofContextPtr ctx, Byte8 * str)
{
	char * psstr;
	#define kMaxPSCommandLen 80
	psstr=(char *)memNew( kMaxMessageLength + kMaxPSCommandLen);
	
	if (ctx->onNewLine==0)
		proofNewline(ctx);
	if(proofIsVerticalMode()){
	
		ctx->curry = ctx->top -  ((3 * TITLE_LABEL_SIZE) + ctx->glyphSize);
		sprintf(psstr,"%g %g _MT\n", ctx->currx, ctx->curry);
		proofPSOUT(ctx, psstr);

		if(strlen(str) < kMaxMessageLength)
		{
			sprintf(psstr, " gsave /Courier-Bold findfont 12 scalefont setfont -90 rotate (%s) show grestore ", str);
			proofPSOUT(ctx, psstr);
		}else{
			while (strlen(str) > kMaxMessageLength)
				{
				char temp=str[kMaxMessageLength-1];
				str[kMaxMessageLength-1]=0;

				sprintf(psstr, " gsave /Courier-Bold findfont 12 scalefont setfont  -90 rotate (%s) show grestore ", str);
				proofPSOUT(ctx, psstr);
				
				ctx->currx-=13;
				sprintf(psstr,"%g %g _MT %% Newline\n", ctx->currx, ctx->curry);
				proofPSOUT(ctx, psstr);
				
				str[kMaxMessageLength-1]=temp;
				str=str+(kMaxMessageLength-1);
				}
			sprintf(psstr, " gsave /Courier-Bold findfont 12 scalefont setfont -90 rotate (%s) show grestore ", str);
			proofPSOUT(ctx, psstr);
		}
	}else{
		ctx->currx = ctx->left;
		sprintf(psstr,"%g %g _MT\n", ctx->currx, ctx->curry);
		proofPSOUT(ctx, psstr);
		
		if (strlen(str) < kMaxMessageLength)
		{
			sprintf(psstr, " gsave /Courier-Bold findfont 12 scalefont setfont (%s) show grestore ", str);
			proofPSOUT(ctx, psstr);
		}else{
            char * temp =(char *)memNew( kMaxMessageLength+1);
            size_t nPos = 0;
            size_t nLen = strlen(str);
            size_t tLen = nLen;
            
 			while (tLen > 0)
				{
                if (tLen > kMaxMessageLength)
                {
                    strncpy(temp, &str[nPos], kMaxMessageLength);
                    nPos += kMaxMessageLength;
                    temp[nPos] = 0;
               }
                else
                {
                    strcpy(temp, &str[nPos]);
                    nPos = nLen;
                }
				sprintf(psstr, " gsave /Courier-Bold findfont 12 scalefont setfont (%s) show grestore ", temp);
				proofPSOUT(ctx, psstr);
				
				ctx->curry-=13;
				sprintf(psstr,"%g %g _MT %% Newline\n", ctx->currx, ctx->curry);
				proofPSOUT(ctx, psstr);
                tLen = nLen - nPos;
				}
		}	
	}
	proofNewline(ctx);	
	memFree(psstr);
}

void proofSymbol(ProofContextPtr ctx, IntX symbol)
	{
	  ctx->onNewLine=0;
	  if (ctx->kind == proofPS) {
		switch (symbol)
		  {
		  case PROOF_PLUS:
			proofThinspace(ctx, 1);
			proofPSOUT(ctx, "SYM setfont <2B> show %plus\n");
			advanceSYM(ctx, 549);
			proofThinspace(ctx, 1);
			break;
			
		  case PROOF_YIELDS:
			proofThinspace(ctx, 1);
			if (proofIsVerticalMode())
			  proofPSOUT(ctx, "SYM setfont <AF> show %yields\n");
			else
			  proofPSOUT(ctx, "SYM setfont <AE> show %yields\n");
			advanceSYM(ctx, 987);
			proofThinspace(ctx, 1);
			break;
			
		  case PROOF_DBLYIELDS:
			proofThinspace(ctx, 1);
			if (proofIsVerticalMode())
			  proofPSOUT(ctx, "SYM setfont <DF> show %dblyields\n");
			else
			  proofPSOUT(ctx, "SYM setfont <DE> show %dblyields\n");
			advanceSYM(ctx, 987);
			proofThinspace(ctx, 1);
			break;

		  case PROOF_COMMA:
			proofPSOUT(ctx, "SYM setfont <2C> show %comma\n");
			advanceSYM(ctx, 250);
			proofThinspace(ctx, 1);
			break;

		  case PROOF_COLON:
			proofPSOUT(ctx, "SYM setfont <3A> show %colon\n");
			advanceSYM(ctx, 278);
			proofThinspace(ctx, 1);
			break;

		  case PROOF_LPAREN:
			if (proofIsVerticalMode())
			  {
				proofPSOUT(ctx, "SYM setfont <C7> show %lparen\n");
				advanceSYM(ctx, 768);
			  }
			else
			  {
				proofPSOUT(ctx, "SYM setfont <28> show %lparen\n");
				advanceSYM(ctx, 333);
			  }

			break;

		  case PROOF_RPAREN:
			if (proofIsVerticalMode())
			  {
				proofPSOUT(ctx, "SYM setfont <C8> show %rparen\n");
				advanceSYM(ctx, 768);
			  }
			else
			  {
				proofPSOUT(ctx, "SYM setfont <29> show %rparen\n");
				advanceSYM(ctx, 333);
			  }

			break;

		  case PROOF_PRIME:
			proofPSOUT(ctx, "SYM setfont <A2> show %prime\n");
			advanceSYM(ctx, 247);
			proofThinspace(ctx, 1);
			break;

		  case PROOF_NOTELEM:
			proofPSOUT(ctx, "SYM setfont <CF> show %ignore\n");
			advanceSYM(ctx, 713);
			proofThinspace(ctx, 1);
			break;

		  default:
			break;
		  }
	  }
	}


static void proofGlyphStart(ProofContextPtr ctx, GlyphId glyphid, Byte8 *glyphname)
	{
	sprintf(str,"\ngsave %%glyph %d (%s)\ncurrentpoint translate %g dup scale\n",
			glyphid, glyphname,
			ctx->glyphSize / ctx->unitsPerEm);
	proofPSOUT(ctx, str);
	}

void proofGlyphMT(ProofContextPtr ctx, double x, double y)
	{
	sprintf(str,"%g %g _MT\n", x, y);
	proofPSOUT(ctx, str);
	}

void proofGlyphLT(ProofContextPtr ctx, double x, double y)
	{
	sprintf(str,"%g %g _LT\n", x, y);
	proofPSOUT(ctx, str);
	}

void proofGlyphCT(ProofContextPtr ctx, double x1, double y1, double x2, double y2, double x3, double y3)
	{
	sprintf(str,"%g %g %g %g %g %g _CT\n", x1, y1, x2, y2, x3, y3);
	proofPSOUT(ctx, str);
	}

void proofGlyphClosePath(ProofContextPtr ctx)
	{
	proofPSOUT(ctx, "_CP\n");
	}

static void proofGlyphFinish(ProofContextPtr ctx)
	{
	proofPSOUT(ctx, "grestore %glyph\n");
	}


void proofDestroyContext(ProofContextPtr *ctxptr)
{
  if (ctxptr == NULL)
	return;
  if (*ctxptr == NULL)
	return;

  sprintf(str,"_SP %% %d pages\n", proofPageCount(*ctxptr));
  proofPSOUT(*ctxptr, str);

  sprintf(str, "%%%%EOF\n");
  proofPSOUT(*ctxptr,str);
	

  if ((*ctxptr)->psfileptr) 
  {
	fflush((*ctxptr)->psfileptr);
	if ((*ctxptr)->psfileptr != stdout)
	  fclose((*ctxptr)->psfileptr);
  }
  
  if ((*ctxptr)->psfileptr != stdout)
  {		  
#if AUTOSPOOL
	if ((*ctxptr)->psfileptr)  /* if written to some file by us */
	  {
		inform(SPOT_MSG_prufNUMPAGES,  proofPageCount(*ctxptr));
		if (opt_Present("-l")
#if __CENTERLINE__
			|| (1) 
#endif
			)
		  {  /* just leave the file */
			inform(SPOT_MSG_prufOFILNAME, proofFileName(*ctxptr));
			inform(SPOT_MSG_prufPLSCLEAN);
		  }
		else 
		  { 
#if SUNOS				
			/*  spool the file to $PRINTER */
			char cmd[80];
			sprintf(cmd,"lpr -r %s \n", proofFileName(*ctxptr));
			inform(SPOT_MSG_prufSPOOLTO, getenv("PRINTER"));
			system(cmd);
#endif
		  }		  
	  }
	else
	  { /* written to a spooler */
#if	 MACINTOSH	
		MacPrintClose((*ctxptr));
		if ((*ctxptr)->psCommentBuf)
			{
		  	HUnlock((*ctxptr)->psCommentBuf);
		  	DisposeHandle((*ctxptr)->psCommentBuf);
		  	}

#elif WIN32
		inform(SPOT_MSG_prufNUMPAGES,  proofPageCount(*ctxptr));
		WinPrintClose((*ctxptr));
		if ((*ctxptr)->lpPassThrough)
		{
		  memFree((*ctxptr)->lpPassThrough);
		  (*ctxptr)->lpPassThrough = NULL;
		}
#endif					
	  }
#endif /* AUTOSPOOL */
  }
  
  if ((*ctxptr)->psfileptr && ((*ctxptr)->psfileptr != stdout) && (*ctxptr)->psfilename )
	memFree((*ctxptr)->psfilename);
	
  if ((*ctxptr)->title)
	memFree((*ctxptr)->title);
	
  if ((*ctxptr)->title2)
	memFree((*ctxptr)->title2);
  memFree(*ctxptr);
  *ctxptr = NULL;
}

void proofClearOptions(proofOptions *options)
{
  options->vorigin = 0;		   options->voriginflags= 0;
  options->baseline = 0;	   options->baselineflags= 0;
  options->vbaseline = 0;	   options->vbaselineflags= 0;
  options->lsb = 0;			   options->lsbflags= 0;
  options->rsb = 0;			   options->rsbflags= 0;
  options->tsb = 0;			   options->tsbflags= 0;
  options->bsb = 0;			   options->bsbflags= 0;
  options->vwidth = 0;		   options->vwidthflags= 0;
  options->neworigin = 0;	   options->neworiginflags= 0;
  options->newvorigin = 0;	   options->newvoriginflags= 0;
  options->newbaseline = 0;	   options->newbaselineflags= 0;
  options->newvbaseline = 0;   options->newvbaselineflags= 0;
  options->newlsb = 0;		   options->newlsbflags= 0;
  options->newrsb = 0;		   options->newrsbflags= 0;
  options->newwidth = 0;	   options->newwidthflags= 0;
  options->newtsb = 0;		   options->newtsbflags= 0;
  options->newbsb = 0;		   options->newbsbflags= 0;
  options->newvwidth = 0;	   options->newvwidthflags= 0;
}


int getEmbox(Int16 *Left, Int16 *Bottom, Int16 *Right, Int16 *Top)
{
	Card16 unitsPerEm;
	
	headGetUnitsPerEm(&unitsPerEm, BASE_);
	
	*Left = 0;
	{ 
		Int32 asc, des;
		OS_2GetTypocenders(&asc, &des);
		if(asc==0 && des==0)
			hheaGetTypocenders(&asc, &des);
		if(asc==0 && des==0)
			return 0;
		if ((asc - des) < unitsPerEm)
			{
			des -= unitsPerEm - (asc-des);
			asc = unitsPerEm + des;
			}
		*Top=(Int16)asc;
		*Bottom=(Int16)des;
		*Right=unitsPerEm;
	}
	return 1;
}

int getIdeoEmbox(Int16 *Left, Int16 *Bottom, Int16 *Right, Int16 *Top)
{
	Int16 value;
	Card16 unitsPerEm;
	
	headGetUnitsPerEm(&unitsPerEm, BASE_);
	
	*Left = 0;
	if(BASEgetValue(STR2TAG("ideo"), 'h', &value)){
		*Bottom=value;
		if(BASEgetValue(STR2TAG("idtp"), 'h', &value))
			*Top=value;
		else
			*Top=*Bottom+unitsPerEm;
		
		if(BASEgetValue(STR2TAG("idtp"), 'v', &value))
			*Right=value;
		else
			*Right=unitsPerEm;
		
		if((BASEgetValue(STR2TAG("ideo"), 'v', &value)) && value!=0)
			fprintf(OUTPUTBUFF, "OTFProof [WARNING]: Bad VertAxis.ideo value\n");
	}else { 
		Int32 asc, des;
		OS_2GetTypocenders(&asc, &des);
		if(asc==0 && des==0)
			hheaGetTypocenders(&asc, &des);
		if(asc==0 && des==0)
			return 0;
		*Top=(Int16)asc;
		*Bottom=(Int16)des;
		*Right=unitsPerEm;
	}
	return 1;
/*	From OT specs:
	ideoEmboxLeft=0
	If HorizAxis.ideo defined:
		ideoEmboxBottom = HorizAxis.ideo

		If HorizAxis.idtp defined:
			ideoEmboxTop = HorizAxis.idtp
		Else:
			ideoEmboxTop = HorizAxis.ideo +
			head.unitsPerEm

		If VertAxis.idtp defined:
			ideoEmboxRight = VertAxis.idtp
		Else:
			ideoEmboxRight = head.unitsPerEm

		If VertAxis.ideo defined and non-zero:
			Warning: Bad VertAxis.ideo value

	Else If this is a CJK font:
           ideoEmboxBottom = OS/2.sTypoDescender
           ideoEmboxTop = OS/2.sTypoAscender
           ideoEmboxRight = head.unitsPerEm
	Else:
           ideoEmbox cannot be determined for this font
*/
}

extern Byte8* getthedate(void);

ProofContextPtr proofInitContext(proofOutputType where,
								 Card32 leftposit, Card32 rightposit,
								 Card32 topposit, Card32 botposit,
								 Byte8 *pageTitle,
								 double glyphSize,
								 double thinspace,
								 double unitsPerEM,
								 IntX noFrills,
								 IntX isPatt,
								 Byte8 *PSFilenameorPATTorNULL)
	{
	 ProofContext *ctx;
	 IntX len;
	 Int16 bbLeft, bbBottom, bbRight, bbTop;
	 Byte8 *fullname;
	 Byte8 *psname;
	/* Byte8 *mydate=getthedate();*/
	 Byte8 *fmt1="[%s] OTF ver.%.3f";
	 Byte8 *fmt2="[%s \\(/%s\\)] head vers.%.3f";
	 /*Byte8 *fmt3="%s %s";* omit date */
	 Byte8 *fmt3="%s";
	 float fontRevision;
	 
	 fullname = NULL;
	 psname = NULL;
	 
	 ctx = memNew(sizeof(ProofContext));
	 if (!ctx) return NULL;
	 ctx->kind = where;
	 ctx->onNewLine=1;
	 ctx->left = leftposit;
	 ctx->right = rightposit;
	 ctx->top = topposit;
	 ctx->bottom = botposit;
	 ctx->page = 0;
	 ctx->title = NULL;
	 ctx->title2 = NULL;
	 
	 if (!opt_Present("-d"))
	 	{
		 /* Set up first line of title = font names + head fontRevision version number. */

		 /* The version string can be up to "65335.999" -> 9 chars,
		 while the number spec "%.3f" is 4 chars-> + 5 digits for each number + 4 for safety*/
		 
		 len = 9 + mMAX(strlen(fmt1), strlen(fmt2)) ;
		  
		 fullname = nameFontName();
		 if (fullname)
		 	len += strlen(fullname);
		 else
		 	fullname="";
		 
		 psname = namePostScriptName();
		 if (psname)
		 	len += strlen(psname);
		 else
		 	psname="";

		 headGetFontRevision(&fontRevision, 0);
		 
		 ctx->title = memNew(len * sizeof(Byte8));
		 if(strcmp(fullname, psname)==0)
		 	sprintf(ctx->title, fmt1, 
				 (fullname)?fullname : "", 
				 fontRevision);
		 else
		 	sprintf(ctx->title, fmt2, 
				 (fullname)?fullname : "", 
				 (psname)?psname : "", 
				 fontRevision);
		
		 if (fullname && fullname[0]!='\0')
		   memFree(fullname);
		 if (psname && psname[0]!='\0')
		   memFree(psname);
	 }

	/* Set up second line of title = date + title parameter */
	 len = strlen(fmt3);
	/* len +=strlen(mydate); */
	 if (pageTitle)
	   len += strlen(pageTitle);

	 ctx->title2 = memNew(len * sizeof(Byte8));
	/*  sprintf(ctx->title2, fmt3, mydate,  (pageTitle)?pageTitle : ""); */
	sprintf(ctx->title2, fmt3, (pageTitle)?pageTitle : "");
	 	
	 		
	 ctx->currx =  ctx->curry = 0.0;
	 ctx->glyphSize = glyphSize;
	 ctx->thinspace = thinspace;
	 ctx->unitsPerEm = unitsPerEM;
	 
	 /* IF it is a CJK font ( has vertical metrics) and neither (PolicyKanjiEMbox nor PolicyKanjiGlyphBBox
	 is set, then make the default policy be PolicyKanjiGlyphBBox. */
	 if  (  !(PolicyKanjiEMbox || PolicyKanjiGlyphBBox||sfntReadTable(vhea_)))
	 	PolicyKanjiGlyphBBox = 1;

	/* fprintf(stderr, "PolicyKanjiEMbox %d,  PolicyKanjiGlyphBBox %d. isVert %d\n", PolicyKanjiEMbox,  PolicyKanjiGlyphBBox, proofIsVerticalMode()); */
     if (proofIsVerticalMode() || PolicyKanjiEMbox || PolicyKanjiGlyphBBox)
     	/* use ideo base-line */
 		{
		 if(!getIdeoEmbox(&ctx->bbLeft, &ctx->bbBottom, &ctx->bbRight, &ctx->bbTop)){
			 GLOBUniBBox(&bbLeft, &bbBottom, &bbRight, &bbTop);
			 if (bbLeft || bbBottom || bbRight || bbTop)
			   {
				 ctx->bbLeft = bbLeft;
				 ctx->bbBottom = bbBottom;
				 ctx->bbRight = bbRight;
				 ctx->bbTop = bbTop;
			   }
			 else
			   {
				 getFontBBox(&bbLeft, &bbBottom, &bbRight, &bbTop);
				 if (bbLeft || bbBottom || bbRight || bbTop)
				   {
					 ctx->bbLeft = bbLeft;
					 ctx->bbBottom = bbBottom;
					 ctx->bbRight = bbRight;
					 ctx->bbTop = bbTop;
				   }
			   }
		  }
	  }
	  else
		   {
			if(!getEmbox(&ctx->bbLeft, &ctx->bbBottom, &ctx->bbRight, &ctx->bbTop))
				 {
				 getFontBBox(&bbLeft, &bbBottom, &bbRight, &bbTop);
				 if (bbLeft || bbBottom || bbRight || bbTop)
				   {
					 ctx->bbLeft = bbLeft;
					 ctx->bbBottom = bbBottom;
					 ctx->bbRight = bbRight;
					 ctx->bbTop = bbTop;
				   }
				}
			}
	  
	
	 if (where == proofPS) {
#if	 AUTOSPOOL
	   if (opt_Present("-O"))
		 {
		   ctx->psfileptr = stdout;
		 }
#if WIN32
	   else if (getenv("spotusestdout") != NULL)
		 {
		   ctx->psfileptr = stdout;
		 }
	   else if (opt_Present("-l"))
		 {
		   IntX retries = 0;
		   char tempname[255];
		   if (PSFilenameorPATTorNULL && PSFilenameorPATTorNULL[0] != '\0') 
			 {
			   if (isPatt) 
				 {
				   if(outputfilebase)
						sprintf(tempname,"%s_%s.ps", outputfilebase, PSFilenameorPATTorNULL);
					else
					   sprintf(tempname,"OTFproof_%s.ps", PSFilenameorPATTorNULL);
				   while (sysFileExists(tempname))
				   		{
				   		  
				   		  if(outputfilebase)
							sprintf(tempname,"%s_%s%d.ps", outputfilebase, PSFilenameorPATTorNULL, retries);
						  else
						   	sprintf(tempname,"OTFproof_%s%d.ps", PSFilenameorPATTorNULL, retries);
				   		  retries++;
				   		}
				   len = strlen(tempname);
				   ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
				   strcpy(ctx->psfilename, tempname);
				   if ((ctx->psfileptr = fopen(tempname, "w")) == (FILE *)NULL)
					 fatal(SPOT_MSG_prufNOOPENF);
				 }
			   else 
				 { /* whole thing is specified */
				 	strcpy(tempname, PSFilenameorPATTorNULL);
				 	while (sysFileExists(tempname))
				   		{
				   		  sprintf(tempname,"%s%d", PSFilenameorPATTorNULL, retries);
				   		  retries++;
				   		}
					len = strlen(tempname);
				 	ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
				 	strcpy(ctx->psfilename, tempname);
				 	if ((ctx->psfileptr = fopen(tempname, "w")) == (FILE *)NULL)
				   		fatal(SPOT_MSG_prufNOOPENF);
				 }
			 }
		   else 
			 {
			   	if(outputfilebase)
			   		sprintf(tempname, "%s.ps", outputfilebase);
			   	else
				   	strcpy(tempname,"OTFproof.ps");
				while (sysFileExists(tempname))
				   	{
				   	  if(outputfilebase)
			   				sprintf(tempname, "%s.ps%d", outputfilebase, retries);
			   		  else
				   			sprintf(tempname,"OTFproof.ps%d", retries);
					  retries++;
			   		}
			 	len = strlen(tempname);
			 	ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
			 	strcpy(ctx->psfilename, tempname);
			 	if ((ctx->psfileptr = fopen(tempname, "w")) == (FILE *)NULL)
			   		fatal(SPOT_MSG_prufNOOPENF);
			 }	
		   ctx->winPrinterDC = NULL; /* very important */
		   inform(SPOT_MSG_prufPREPPS);
		 }
	   else
		 {
		   static Byte8 tytl[256];
		   
		   ctx->psfileptr = (FILE *)NULL; /* very important */
		   
		   memset((char *)tytl, 32, 255);   
		   sprintf(tytl,"OTFproof %s [%s]", 
				   (psname)?psname : "OpenType Font", 
				   (pageTitle)?pageTitle : 
				   ((PSFilenameorPATTorNULL) ? PSFilenameorPATTorNULL : "proof"));
		   ctx->docinfo.cbSize = sizeof(DOCINFO);
		   ctx->docinfo.lpszDocName = tytl;
		   ctx->docinfo.lpszOutput = NULL;
		   			
		   if ((! WinPrintOpen (ctx)) || (ctx->winPrinterDC == NULL))
			 fatal(SPOT_MSG_prufCANTPS);
								
		   if ((ctx->lpPassThrough = (LPSTR)memNew(sizeof(Byte8) * (PASSTHROUGHSIZE + 6))) == NULL)
			 fatal(SPOT_MSG_prufNOBUFMEM);

		   inform(SPOT_MSG_prufPREPPS);
		   WinPrintInitPage(ctx);
		 }										
#elif OSX
	if (opt_Present("-O"))
	{
		ctx->psfileptr = stdout;
	}
	else if (getenv("spotusestdout") != NULL)
	{
		ctx->psfileptr = stdout;
	}
	else
	{
		IntX retries = 0;
		inform(SPOT_MSG_prufPREPPS);
		if (PSFilenameorPATTorNULL && PSFilenameorPATTorNULL[0] != '\0') 
		{
			if (isPatt) 
			{
				char tempname[MAX_NAME_LEN];
				if(outputfilebase)
					sprintf(tempname,"%s_%s.ps", outputfilebase, PSFilenameorPATTorNULL);
				else
					sprintf(tempname,"OTFproof_%s.ps", PSFilenameorPATTorNULL);
				
				while (sysFileExists(tempname))
				{
					if(outputfilebase)
						sprintf(tempname,"%s_%s%d.ps", outputfilebase, PSFilenameorPATTorNULL, retries);
					else
						sprintf(tempname,"OTFproof_%s%d.ps", PSFilenameorPATTorNULL, retries);
				 	retries++;
				} 
				len = strlen(tempname);
				ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
				strcpy(ctx->psfilename, tempname);
				if ((ctx->psfileptr = fopen(tempname, "w")) == (FILE *)NULL)
					fatal(SPOT_MSG_prufNOOPENF);
			}
			else 
			{ /* whole thing is specified */
				len = strlen(PSFilenameorPATTorNULL);
				ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
				strcpy(ctx->psfilename, PSFilenameorPATTorNULL);
				if ((ctx->psfileptr = fopen(PSFilenameorPATTorNULL, "w")) == (FILE *)NULL)
					fatal(SPOT_MSG_prufNOOPENF);
				 /*fprintf(stderr, "Proof file is %n", ctx->psfilename);*/
			}
		}
		else 
		{
			char tempname[MAX_NAME_LEN];
			if(outputfilebase)
				sprintf(tempname, "%s.ps", outputfilebase);
			else
				strcpy(tempname,"OTFproof.ps");
			while (sysFileExists(tempname))
			{
				if(outputfilebase)
					sprintf(tempname, "%s%d.ps", outputfilebase, retries);
				else
					sprintf(tempname,"OTFproof%d.ps", retries);
				retries++;
			}
			len = strlen(tempname);
			ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
			strcpy(ctx->psfilename, tempname);
			if ((ctx->psfileptr = fopen(tempname, "w")) == (FILE *)NULL)
				fatal(SPOT_MSG_prufNOOPENF);
			 /*fprintf(stderr, "Proof file is %n", ctx->psfilename);*/
		}
	}
#else /* UNIX */
	   if (opt_Present("-O"))
		 {
		   ctx->psfileptr = stdout;
		 }
	   else if (getenv("spotusestdout") != NULL)
		 {
		   ctx->psfileptr = stdout;
		 }
	   else
		 {
		   inform(SPOT_MSG_prufPREPPS);
		   if (PSFilenameorPATTorNULL && PSFilenameorPATTorNULL[0] != '\0') 
			 {
			 if (isPatt) 
			   {
				 char tempname[MAX_NAME_LEN];
				 sprintf(tempname,"/tmp/%s_%s_XXXXXX", global.progname, PSFilenameorPATTorNULL);
				 mktemp(tempname);
				 len = strlen(tempname);
				 ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
				 strcpy(ctx->psfilename, tempname);
				 if ((ctx->psfileptr = fopen(tempname, "w")) == (FILE *)NULL)
				   fatal(SPOT_MSG_prufNOOPENF);
				 /*fprintf(stderr, "Proof file is %n", tempname);*/
			   }
			 else 
			   { /* whole thing is specified */
				 len = strlen(PSFilenameorPATTorNULL);
				 ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
				 strcpy(ctx->psfilename, PSFilenameorPATTorNULL);
				 if ((ctx->psfileptr = fopen(PSFilenameorPATTorNULL, "w")) == (FILE *)NULL)
				   fatal(SPOT_MSG_prufNOOPENF);
				 /*fprintf(stderr, "Proof file is %n", ctx->psfilename);*/
			   }
		   }
		   else 
			 {
			 char tempname[MAX_NAME_LEN];
			 sprintf(tempname,"/tmp/%s.ps_XXXXXX", global.progname);
			 mktemp(tempname);
			 len = strlen(tempname);
			 ctx->psfilename = memNew(sizeof(Byte8) * (len+1));
			 strcpy(ctx->psfilename, tempname);
			 if ((ctx->psfileptr = fopen(tempname, "w")) == (FILE *)NULL)
			   fatal(SPOT_MSG_prufNOOPENF);
			 /*fprintf(stderr, "Proof file is %n", ctx->psfilename);*/
		   }
		 }
#endif /* MAC or NOT */
#else /* !AUTOSPOOL */
		ctx->psfileptr = stdout;
#endif

	   if (noFrills)
		 proofOnlyNewPage(ctx);
	   else
		 proofNewPage(ctx);
	 }
	 else {/* Not PS output */
	   ctx->psfileptr = (FILE *)NULL;
	 }
	 return (ctx);
   }


static void drawBox(ProofContextPtr ctx,
					FWord Left, FWord Bottom, FWord Right, FWord Top)
	{
	/* fprintf(stderr, "Box L %d B %d R %d T %d.\n", Left, Bottom, Right, Top); */
	  
	  if (Left || Bottom || Right || Top) {
		sprintf(str,"gsave newpath %d %d _MT %d %d _LT %d %d _LT %d %d _LT _CP ",
				Left, Bottom,
				Right, Bottom,
				Right, Top,
				Left, Top);
	   proofPSOUT(ctx, str);
	   proofPSOUT(ctx, "0 setlinewidth stroke grestore %%Box\n");
	 }
	}

static void drawKanjiBox(ProofContextPtr ctx, IntX isGlyphBBox, IntX width, IntX yOrigKanji)
	{
	
#define KANJI_TIC STD2FNT(60)

	  if (isGlyphBBox)
		{
		  if (proofIsVerticalMode())
			{
			  Int16 abswy;
			  abswy = abs(width);

			  /* Glyph widthbox */
			 sprintf(str,"gsave newpath %g %g _MT %g %g _LT %g %g _LT %g %g _LT _CP 0 setlinewidth stroke ",
					 STD2FNT(0), STD2FNT(yOrigKanji),
					 STD2FNT(0), STD2FNT(yOrigKanji) - abswy,
					 STD2FNT(1000), STD2FNT(yOrigKanji) - abswy,
					 STD2FNT(1000), STD2FNT(yOrigKanji));
			 proofPSOUT(ctx, str);

			 /* adornments */
			 sprintf(str," newpath %g %g _MT -%g 0 rlineto %g %g _MT %g 0 rlineto %g %g _MT 0 -%g rlineto %g %g _MT 0 %g rlineto _CP ",
					 STD2FNT(0), STD2FNT(yOrigKanji) - abswy/2, KANJI_TIC,
					 STD2FNT(1000), STD2FNT(yOrigKanji) - abswy/2, KANJI_TIC,
					 STD2FNT(500), STD2FNT(yOrigKanji) - abswy, KANJI_TIC,
					 STD2FNT(500), STD2FNT(yOrigKanji), KANJI_TIC);
			 proofPSOUT(ctx, str);
			 proofPSOUT(ctx, "0 setlinewidth stroke grestore %%KanjiGTics\n");
			}
		  else
			{
			  /* Glyph widthbox */
			float aw = (float)width;
			 sprintf(str,"gsave newpath %g %g _MT %g %g _LT %g %g _LT %g %g _LT _CP 0 setlinewidth stroke ",
					 STD2FNT(0), STD2FNT(yOrigKanji-1000),
					aw, STD2FNT(yOrigKanji-1000),
					aw, STD2FNT(yOrigKanji),
					 STD2FNT(0), STD2FNT(yOrigKanji));
			 proofPSOUT(ctx, str);

			 /* adornments */
			 sprintf(str," newpath %g %g _MT -%g 0 rlineto %g %g _MT %g 0 rlineto %g %g _MT 0 -%g rlineto %g %g _MT 0 %g rlineto _CP ",
					 STD2FNT(0), STD2FNT(yOrigKanji-500), KANJI_TIC,
					aw, STD2FNT(yOrigKanji-500), KANJI_TIC,
					 aw/2, STD2FNT(yOrigKanji-1000), KANJI_TIC,
					 aw/2, STD2FNT(yOrigKanji), KANJI_TIC);
			 proofPSOUT(ctx, str);
			 proofPSOUT(ctx, "0 setlinewidth stroke grestore %%KanjiGTics\n");
		   }
		}
	  else
		{
		  /* standard box */
		  sprintf(str,"gsave newpath %g %g _MT %g %g _LT %g %g _LT %g %g _LT _CP 0 setlinewidth stroke ",
				  STD2FNT(0), STD2FNT(yOrigKanji-1000),
				  STD2FNT(1000), STD2FNT(yOrigKanji-1000),
				  STD2FNT(1000), STD2FNT(yOrigKanji),
				  STD2FNT(0), STD2FNT(yOrigKanji));
		  proofPSOUT(ctx, str);
		  
		  /* adornments */
		  sprintf(str," newpath %g %g _MT -%g 0 rlineto %g %g _MT %g 0 rlineto %g %g _MT 0 -%g rlineto %g %g _MT 0 %g rlineto _CP ",
				  STD2FNT(0), STD2FNT(yOrigKanji-500), KANJI_TIC,
				  STD2FNT(1000), STD2FNT(yOrigKanji-500), KANJI_TIC,
				  STD2FNT(500), STD2FNT(yOrigKanji-1000), KANJI_TIC,
				  STD2FNT(500), STD2FNT(yOrigKanji), KANJI_TIC);
		  proofPSOUT(ctx, str);
		  proofPSOUT(ctx, "0 setlinewidth stroke grestore %%KanjiTics\n");
		}
	}

#define BELOWOFFSET	STD2FNT(-200)
#define TOPOFFSET	STD2FNT(1200)
#define LEFTOFFSET	STD2FNT(-200)
#define RIGHTOFFSET	STD2FNT(1200)

#define HASVERTANNOTATIONS(flags) (((flags) & ANNOT_ATRIGHTDOWN2) ||\
								   ((flags) & ANNOT_ATRIGHTDOWN1) ||\
								   ((flags) & ANNOT_ATRIGHT))

#define HASHORZANNOTATIONS(flags) (((flags) & ANNOT_ATBOTTOMDOWN2) ||\
								   ((flags) & ANNOT_ATBOTTOMDOWN1) ||\
								   ((flags) & ANNOT_ATBOTTOM))

static void drawVertLine(ProofContextPtr ctx, FWord Xpos, FWord oldXpos, Card16 flags)
	{
	  FWord labelvalue;
	  Byte8 *whichlabel = "LAB";
	  int LinesOK;
	  if (!flags) return;

	  LinesOK = (PolicyLines && (! (flags & ANNOT_NOLINE)));

	  if (LinesOK)
		{
		  sprintf(str,"gsave newpath %g %g _MT 0 %g rlineto ",
				  Xpos + 0.0, BELOWOFFSET, TOPOFFSET);
		  proofPSOUT(ctx, str);
		  if (flags & ANNOT_DASHEDLINE) {
			sprintf(str, "[%d %d] 0 setdash ", RND(ABS2FNT(3)),RND(ABS2FNT(1)) );
			proofPSOUT(ctx, str);
		  }
		  proofPSOUT(ctx, "0 setlinewidth stroke grestore %%vertline\n");
		}

	  if (PolicyNoNumericLabels) return;
	  
	  if (flags & ANNOT_RELVAL) 
		labelvalue = oldXpos - Xpos;
	  else
		labelvalue = Xpos;
	  
	  if (flags & ANNOT_BOLD) whichlabel = "BLAB";
	  else if (flags & ANNOT_EMPHASIS) whichlabel = "EMLAB";
		
	  if (LinesOK || HASHORZANNOTATIONS(flags))
		{
		 if (flags & ANNOT_ATBOTTOM) 
		   {
			 sprintf(str, "gsave %s setfont %g %g _MT (%d) show grestore\n",
					 whichlabel,
					 Xpos + 0.0, BELOWOFFSET - 2 * ABS2FNT(NUMERIC_LABEL_SIZE),
					 labelvalue);
			 proofPSOUT(ctx, str);
		   }
		 else if (flags & ANNOT_ATBOTTOMDOWN1) 
		   {
			 sprintf(str, "gsave %s setfont %g %g _MT (%d) show grestore\n",
					 whichlabel,
					 Xpos + 0.0, BELOWOFFSET - 3 * ABS2FNT(NUMERIC_LABEL_SIZE),
					 labelvalue);
			 proofPSOUT(ctx, str);
		   }
		 else if (flags & ANNOT_ATBOTTOMDOWN2) 
		   {
			 sprintf(str, "gsave %s setfont %g %g _MT (%d) show grestore\n",
					 whichlabel,
					 Xpos + 0.0, BELOWOFFSET - 4 * ABS2FNT(NUMERIC_LABEL_SIZE),
					 labelvalue);
			 proofPSOUT(ctx, str);
		   }
	   }
	  else	/* just vertical-mode labels */
		{
		  double at = 0.0;

		  if (flags & ANNOT_ATRIGHT) 
			  at =	RIGHTOFFSET + 2 * ABS2FNT(NUMERIC_LABEL_SIZE);
		  else if (flags & ANNOT_ATRIGHTDOWN1) 
			  at =	RIGHTOFFSET + 3 * ABS2FNT(NUMERIC_LABEL_SIZE);
		  else if (flags & ANNOT_ATRIGHTDOWN2) 
			  at =	RIGHTOFFSET + 4 * ABS2FNT(NUMERIC_LABEL_SIZE);

		  sprintf(str, "gsave %s setfont %g 0 _MT currentpoint gsave translate 90 rotate 0 0 _MT (%d) show grestore grestore\n",
				  whichlabel,
				  at,
				  labelvalue);
		  proofPSOUT(ctx, str);
		}
	}

static void drawHorzLine(ProofContextPtr ctx, FWord Ypos, FWord oldYpos, Card16 flags)
	{
	  FWord labelvalue;
	  Byte8 *whichlabel = "LAB";
	  int isVert = proofIsVerticalMode();
	  int LinesOK;

#define YCOORD(y) ((isVert)? (-(y) + 880) : (y))

	 if (!flags) return;
	  LinesOK = (PolicyLines && (! (flags & ANNOT_NOLINE)));
	  if (LinesOK)
		{
		  sprintf(str,"gsave newpath %g %d _MT %g 0 rlineto ",
				  LEFTOFFSET, 
				  YCOORD(Ypos),
				  RIGHTOFFSET);
		  proofPSOUT(ctx, str);
		  if (flags & ANNOT_DASHEDLINE) {
			sprintf(str, "[%d %d] 0 setdash ", RND(ABS2FNT(2)), RND(ABS2FNT(1)));
			proofPSOUT(ctx, str);
		  }
		  proofPSOUT(ctx, "0 setlinewidth stroke grestore %%horzline\n");
		}

	 if (PolicyNoNumericLabels) return;

	 if (flags & ANNOT_RELVAL) 
	   labelvalue = oldYpos - Ypos;
	 else
	   labelvalue = Ypos;

	  if (flags & ANNOT_BOLD) whichlabel = "BLAB";
	  else if (flags & ANNOT_EMPHASIS) whichlabel = "EMLAB";

	  if (LinesOK || HASVERTANNOTATIONS(flags))
		{
		  if (flags & ANNOT_ATRIGHT) 
			{
			  sprintf(str, "gsave %s setfont %g %d _MT (%d) show grestore\n",
					  whichlabel,
					  RIGHTOFFSET,
					  YCOORD(Ypos),
					  labelvalue);
			  proofPSOUT(ctx, str);
			}
		  else if (flags & ANNOT_ATRIGHTDOWN1) 
			{
			  sprintf(str, "gsave %s setfont %g %g _MT (%d) show grestore\n",
					  whichlabel,
					  RIGHTOFFSET, 
					  YCOORD(Ypos) - 2 * ABS2FNT(NUMERIC_LABEL_SIZE),
					  labelvalue);
			  proofPSOUT(ctx, str);
			}
		  else if (flags & ANNOT_ATRIGHTDOWN2) 
			{
			  sprintf(str, "gsave %s setfont %g %g _MT (%d) show grestore\n",
					  whichlabel,
					  RIGHTOFFSET, 
					  YCOORD(Ypos) - 3 * ABS2FNT(NUMERIC_LABEL_SIZE),
					  labelvalue);
			  proofPSOUT(ctx, str);
			}
		}
#undef YCOORD
	}

void proofDrawGlyph(ProofContextPtr ctx,	
					GlyphId glyphId,	Card16 glyphflags,
					Byte8 *glyphname,	Card16 glyphnameflags,
					Byte8 *altlabel,	Card16 altlabelflags,
					Int16 originDx,		Int16 originDy,
					Int16 origin,		Card16 originflags,
					Int16 width,		Card16 widthflags,
					proofOptions *options, Int16 yOrigKanji, char *message )
	{
	/* The idea is that we draw with respect to a base-line at y=0.
	Kanji em-boxes are drawn with their top at the STD positoon of top = 880, bottom at -120.
	
	In vertical mode, whcih is for Kanji only, the glyph is translated so that its origin
	aligns with the top of the kanji em-box at STD position y = 880.
	
	 */
	  int isVert = proofIsVerticalMode();

	/*  if ((glyphId==1572) || (glyphId==9619))
	   fprintf(stderr, "\n%d: ox %d oy %d w %d yok %d \n", glyphId, originDx, originDy, width, yOrigKanji, ctx->unitsPerEm, isVert);
	   */
	  if (!isVert)
	  	  yOrigKanji = DEFAULT_YORIG_KANJI;
	 else
	    yOrigKanji = (Int16)FNT2STD(yOrigKanji);
	  /*if (glyphId==758)
	  	fprintf(OUTPUTBUFF, "\n%d: ox %d oy %d w %d yok %d\n", glyphId, originDx, 
	  		originDy, width, yOrigKanji);
	 */
       /*if (glyphId==7904)
	  	fprintf(OUTPUTBUFF, "\n%d: ox %d oy %d w %d yok %d\n", glyphId, originDx, 
	  		originDy, width, yOrigKanji); */
	  ctx->onNewLine=0;
	  proofGlyphStart(ctx, glyphId, glyphname); /* This sets the current transform to scale from font coordinates straight to page coordinates */


	/* This moves the glyph up so its origin aligns with DEFAULT_YORIG_KANJI */
	/* If we are i horizontal mode, yOrigKanji is always passed through as DEFAULT_YORIG_KANJI, so this is nto rriggered. */
	
	  if (originDx || originDy || DEFAULT_YORIG_KANJI!=yOrigKanji)
	  {
		if ((originDx < 0) || (originDy < 0))
		  advance(ctx, 10);
		sprintf(str,"gsave %d %d translate %%originDelta\n", 
				originDx,
				((isVert ? (-1) : (1)) * (RND(originDy - STD2FNT(DEFAULT_YORIG_KANJI-yOrigKanji)))) );

	  	/*
	  if ((glyphId==1572) || (glyphId==9619))
	  	fprintf(stderr,"%s\n", str);
	  */
		proofPSOUT(ctx, str);
	  }

	  if (glyfLoaded()) 
	  {
		glyfProofGlyph(glyphId, (void *)ctx);
	  }
	  else if (CFF_Loaded()) 
	  {
		CFF_ProofGlyph(glyphId, (void *)ctx);
	  }

	  if (originDx || originDy || DEFAULT_YORIG_KANJI!=yOrigKanji)
	  {
		proofPSOUT(ctx, "fill grestore %%originDelta\n");
	  }
	  else
		proofPSOUT(ctx,"fill\n");

	  if (glyphflags & ADORN_WIDTHMARKS)
	  {
#define WIDTHPOS 20
#define WIDTHLEN 40

		if (isVert)
		  {
			Int16 abswy;
			abswy = abs(width);
			sprintf(str,"gsave newpath %g %g _MT 0 %g rlineto %g %g _MT %g 0 rlineto ",
					STD2FNT(500), STD2FNT(DEFAULT_YORIG_KANJI) ,
					STD2FNT(WIDTHLEN), 
					STD2FNT(500) - STD2FNT(WIDTHPOS), STD2FNT(DEFAULT_YORIG_KANJI),
					STD2FNT(WIDTHLEN));
			proofPSOUT(ctx,str);
			sprintf(str,"%g %g _MT 0 %g rlineto %g %g _MT %g 0 rlineto stroke grestore %%widthmarks\n",
					STD2FNT(500), STD2FNT(DEFAULT_YORIG_KANJI) - abswy, 
					STD2FNT(- WIDTHLEN),
					STD2FNT(500) - STD2FNT(WIDTHPOS), STD2FNT(DEFAULT_YORIG_KANJI) - abswy,
					STD2FNT(WIDTHLEN));
			proofPSOUT(ctx, str);
		  }
		else
		  {
			sprintf(str,"gsave newpath %d %g _MT 0 %g rlineto %d 0 _MT %g 0 rlineto ",
					0, STD2FNT(-WIDTHPOS),
					STD2FNT(WIDTHLEN), 
					0,
					STD2FNT(- WIDTHLEN));
			proofPSOUT(ctx,str);
			sprintf(str,"%d 0 _MT %g 0 rlineto %d %g _MT 0 %g rlineto stroke grestore %%widthmarks\n",
					width,
					STD2FNT(WIDTHLEN),
					width, STD2FNT(-WIDTHPOS),
					STD2FNT(WIDTHLEN) );
			proofPSOUT(ctx, str);
		  }
	  }

	  if ( PolicyKanjiEMbox || PolicyKanjiGlyphBBox)
	  {
		if (PolicyKanjiEMbox)
		  drawKanjiBox(ctx, 0, 0, DEFAULT_YORIG_KANJI);
		else /* glyph box */
		{
		  drawKanjiBox(ctx, 1, width, DEFAULT_YORIG_KANJI);
		  }
	  }
	  else if (glyphflags & ADORN_BBOXMARKS) 
	  {
		drawBox(ctx, ctx->bbLeft, ctx->bbBottom,width, ctx->bbTop);
	  }

	  if (!PolicyNoNames) {
		if (glyphnameflags & ANNOT_SHOWIT) 
		  {
			Byte8 *whichglyphlabel = "LAB";
			if (glyphnameflags & ANNOT_BOLD) whichglyphlabel = "BLAB";
			else if (glyphnameflags & ANNOT_EMPHASIS) whichglyphlabel = "EMLAB";

			if	(glyphnameflags & ANNOT_ATRIGHTDOWN2)
			  sprintf(str,"gsave %g 0 _MT currentpoint gsave translate 90 rotate 0 0 _MT %s setfont ", 
					  RIGHTOFFSET + 4 * ABS2FNT(NUMERIC_LABEL_SIZE),
					  whichglyphlabel);
			else if (glyphnameflags & ANNOT_ATBOTTOMDOWN2)
			  sprintf(str,"gsave 0 %g _MT %s setfont ", 
					  BELOWOFFSET - 4 * ABS2FNT(NUMERIC_LABEL_SIZE),
					  whichglyphlabel);
			else if (glyphnameflags & ANNOT_ATRIGHTDOWN1)
			  sprintf(str,"gsave %g 0 _MT currentpoint gsave translate 90 rotate 0 0 _MT %s setfont ", 
					  RIGHTOFFSET + 3 * ABS2FNT(NUMERIC_LABEL_SIZE),
					  whichglyphlabel);
			else if (glyphnameflags & ANNOT_ATBOTTOMDOWN1)
				  sprintf(str,"gsave 0 %g _MT %s setfont ", 
						  BELOWOFFSET - 3 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichglyphlabel);
			else if (glyphnameflags & ANNOT_ATRIGHT)
			  sprintf(str,"gsave %g 0 _MT currentpoint gsave translate 90 rotate 0 0 _MT %s setfont ", 
					  RIGHTOFFSET + 2 * ABS2FNT(NUMERIC_LABEL_SIZE),
					  whichglyphlabel);
			else if (glyphnameflags & ANNOT_ATBOTTOM)
				  sprintf(str,"gsave 0 %g _MT %s setfont ", 
					  BELOWOFFSET - 2 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichglyphlabel);

			proofPSOUT(ctx, str);
			if (glyfLoaded()) /* a proxy for " name was derived from post or camp table" */
			  sprintf(str,"(%s %s)\n", glyphname, message);
			else
			  sprintf(str,"(%s@%d %s)\n", glyphname, glyphId, message);
			strcat(str, " show grestore\n");

			if (HASVERTANNOTATIONS(glyphnameflags))
			  strcat(str, " grestore\n");
			proofPSOUT(ctx, str);
		  }
		
		if ((altlabelflags & ANNOT_SHOWIT) && 
			(altlabel && (altlabel[0] != '\0')))
		  {
			Byte8 *whichaltlabel = "LAB";
			if (altlabelflags & ANNOT_BOLD) whichaltlabel = "BLAB";
			else if (altlabelflags & ANNOT_EMPHASIS) whichaltlabel = "EMLAB";

			if (altlabelflags & ANNOT_ATBOTTOMDOWN1) 
			  {
				if (isVert)
				  sprintf(str,"gsave %g 0 _MT currentpoint gsave translate 90 rotate 0 0 _MT %s setfont ", 
						  RIGHTOFFSET + 3 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichaltlabel);
				else
				  sprintf(str,"gsave 0 %g _MT %s setfont ", 
						  BELOWOFFSET - 3 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichaltlabel);
			  }
			else if (altlabelflags & ANNOT_ATBOTTOMDOWN2) 
			  {
				if (isVert)
				  sprintf(str,"gsave %g 0 _MT currentpoint gsave translate 90 rotate 0 0 _MT %s setfont ", 
						  RIGHTOFFSET + 4 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichaltlabel);
				else
				  sprintf(str,"gsave 0 %g _MT %s setfont ", 
						  BELOWOFFSET - 4 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichaltlabel);
			  }
			else {
				if (isVert)
				  sprintf(str,"gsave %g 0 _MT currentpoint gsave translate 90 rotate 0 0 _MT %s setfont ", 
						  RIGHTOFFSET + 2 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichaltlabel);
				else
				  sprintf(str,"gsave 0 %g _MT %s setfont ", 
						  BELOWOFFSET - 2 * ABS2FNT(NUMERIC_LABEL_SIZE),
						  whichaltlabel);
			}
			proofPSOUT(ctx, str);
			sprintf(str,"(%s) show grestore\n", altlabel);
			if (isVert)
			  strcat(str, " grestore\n");
			proofPSOUT(ctx, str);
		  }
	  }


	  drawVertLine(ctx, width, width, widthflags);

	  drawVertLine(ctx, origin, origin, originflags);

	 if (options) 
	   {
		 drawHorzLine(ctx, options->vorigin, options->vorigin, options->voriginflags);
		 drawHorzLine(ctx, options->baseline, options->baseline, options->baselineflags);
		 drawVertLine(ctx, options->vbaseline, options->vbaseline, options->vbaselineflags);
		 drawVertLine(ctx, options->lsb, options->lsb, options->lsbflags);
		 drawVertLine(ctx, options->rsb, options->rsb, options->rsbflags);
		 drawHorzLine(ctx, options->tsb, options->tsb, options->tsbflags);
		 drawHorzLine(ctx, options->bsb, options->bsb, options->bsbflags);
		 drawHorzLine(ctx, options->vwidth, options->vwidth, options->vwidthflags);
		 drawVertLine(ctx, options->neworigin, origin, options->neworiginflags);
		 drawHorzLine(ctx, options->newvorigin, options->vorigin, options->newvoriginflags);
		 drawHorzLine(ctx, options->newbaseline, options->baseline, options->newbaselineflags);
		 drawVertLine(ctx, options->newvbaseline, options->vbaseline, options->newvbaselineflags);
		 drawVertLine(ctx, options->newlsb, options->lsb, options->newlsbflags);
		 drawVertLine(ctx, options->newrsb, options->rsb, options->newrsbflags);
		 drawVertLine(ctx, options->newwidth, width, options->newwidthflags);
		 drawHorzLine(ctx, options->newtsb, options->tsb, options->newtsbflags);
		 drawHorzLine(ctx, options->newbsb, options->bsb, options->newbsbflags);
		 drawHorzLine(ctx, options->newvwidth, options->vwidth, options->newvwidthflags);
	   }
	 proofGlyphFinish(ctx);
	 if (glyphflags & ADORN_BBOXMARKS) {
	   advance(ctx, (Int16)(STD2FNT(1000)));
	   proofThinspace(ctx, 1);
	 }
	 else {
	   if (isVert)
		 {
		   if (options && options->newvwidth)
			 advance(ctx, options->newvwidth);
		   else
			 advance(ctx, width);
		 }
  	   else
		 {
		   if (options && options->newwidth)
			 advance(ctx, options->newwidth);
		   else
			 advance(ctx, width);
		 }
	 }
	 if (ctx->thinspace) {
	   if (isVert)
		 {
		   sprintf(str,"0 -%g rmoveto\n", ctx->thinspace);
		   proofPSOUT(ctx, str);
		   ctx->curry -= ctx->thinspace;
		   checkNewline(ctx);
		 }
	   else
		 {
		   sprintf(str,"%g 0  rmoveto\n", ctx->thinspace);
		   proofPSOUT(ctx, str);
		   ctx->currx += ctx->thinspace;
		   checkNewline(ctx);
		 }
	 }
	}


void proofSetPolicy(IntX policyNum, IntX value)
	{
	  switch (policyNum)
		{
		case 1:
		  PolicyNoNames = value;
		  break;
		case 2:
		  PolicyNoNumericLabels = value;
		  break;
		case 3:
		  PolicyLines = value;
		  break;
		case 4:
		  PolicyKanjiEMbox = value;
		  break;
		case 5:
		  PolicyKanjiGlyphBBox = value;
		  break;
		case 6:
		  PolicyKanjiVertical = value;
		  break;
		case 7:
		  PolicyKanjiKernAltMetrics = value;
		  break;
		default:
		  break;
		}
	}

void proofSetVerticalMode(void)
{
  tempPolicyKanjiVertical = 1;
}

void proofUnSetVerticalMode(void)
{
  tempPolicyKanjiVertical = 0;
}


IntX proofIsVerticalMode(void)
{
  return ((PolicyKanjiVertical == 1) || /* Global policy */
		  (tempPolicyKanjiVertical == 1)); /* per-tag policy */
}

IntX proofIsAltKanjiKern(void)
{
  return (PolicyKanjiKernAltMetrics == 1);
}

void proofResetPolicies(void)
	{
	  PolicyNoNames = 0;
	  PolicyNoNumericLabels = 0;
	  PolicyLines = 0;
	  PolicyKanjiEMbox = 0;
	  PolicyKanjiGlyphBBox = 0;
	  PolicyKanjiVertical = 0;
	  PolicyKanjiKernAltMetrics = 1;
	}

IdList policies;

int proofPolicyScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	  IntX i;
	  if (argi == 0)
		return 0; /* No initialization required */
	  
	  if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	  else
		{
		  Byte8 *arg = argv[argi++];
		  
		  da_INIT_ONCE(policies, 5, 2);
		  policies.cnt = 0;
		  if (parseIdList(arg, &policies))
			opt_Error(opt_Format, opt, arg);
		}
	  if (policies.cnt > 0)
		{
		  for (i = 0; i < policies.cnt; i++) {
			IntX id = policies.array[i];
			if (id==7)
				proofSetPolicy(id, 0);		/*-p7 turns off KanjiKernAltMetrics*/
			else
				proofSetPolicy(id, 1);
		  }
		}
	  return argi;
	}


ProofContextPtr proofSynopsisInit(Byte8 *title, Card32 opt_tag)
{
  ProofContextPtr cp = NULL;
  cp = glyfSynopsisInit(title, opt_tag);
  if (cp == NULL) 
	{
	  cp = CFF_SynopsisInit(title, opt_tag);
	  if (cp == NULL)
		warning(SPOT_MSG_prufNOSHAPES);
	}
   return cp;
}

void proofSynopsisFinish(void)
{
  if (glyfLoaded()) 
	{
	  glyfSynopsisFinish();
	}
  else if (CFF_Loaded()) 
	{
	  CFF_SynopsisFinish();
	}
}

void proofDrawTile(GlyphId glyphId, Byte8 *code)
{
  if (glyfLoaded()) 
	{
	  glyfDrawTile(glyphId, code);
	}
  else if (CFF_Loaded()) 
	{
	  CFF_DrawTile(glyphId, code);
	}
  else
	warning(SPOT_MSG_prufNOSHAPES);
}





/* ============================================================================ */
#if MACINTOSH

static Boolean IncludePostScriptInSpoolFile(PMPrintSession printSession)
{
    Boolean	includePostScript = false;
    OSStatus	status;
    CFArrayRef	supportedFormats = NULL;
    SInt32	i, numSupportedFormats;

    /* Get the list of spool file formats supported by the current driver.
    // PMSessionGetDocumentFormatGeneration returns the list of formats which can be generated
    // by the spooler (client-side) AND converted by the despooler (server-side).
    // PMSessionGetDocumentFormatSupported only returns the list of formats which can be converted
    // by the despooler.
    */
    
    status = PMSessionGetDocumentFormatGeneration(printSession, &supportedFormats);
    if (status == noErr)
    {
        // Check if PICT w/ PS is in the list of supported formats.
        numSupportedFormats = CFArrayGetCount(supportedFormats);
        
        for (i=0; i < numSupportedFormats; i++)
        	{
            if ( CFStringCompare(CFArrayGetValueAtIndex(supportedFormats, i),
                				kPMDocumentFormatPICTPS, 
                				kCFCompareCaseInsensitive)
                 	== kCFCompareEqualTo )
            	{
                /* PICT w/ PS is supported, so tell the Printing Mgr to generate a PICT w/ PS spool file */
                
                /* Build an array of graphics contexts containing just one type, Quickdraw,
                 meaning that we will be using a QD port to image our pages in the print loop.
                 */
                CFStringRef	strings[1];
                CFArrayRef	arrayOfGraphicsContexts;
				
                strings[0] = kPMGraphicsContextQuickdraw;
                arrayOfGraphicsContexts = CFArrayCreate(kCFAllocatorDefault,
                        								(const void **)strings, 1, &kCFTypeArrayCallBacks);
										
                if (arrayOfGraphicsContexts != NULL)
	                {
	                /* Request a PICT w/ PS spool file */
	                status = PMSessionSetDocumentFormatGeneration(printSession, 
	                									kPMDocumentFormatPICTPS, 
	                    								arrayOfGraphicsContexts, 
	                    								NULL);
				
	                if (status == noErr)
	                    includePostScript = true;	/* Enable use of PS PicComments */

	                /* Deallocate the array used for the list of graphics contexts. */
	                    CFRelease(arrayOfGraphicsContexts);
	                }
				break;
            	}
        	}
                    
        /* Deallocate the array used for the list of supported spool file formats.*/
        CFRelease(supportedFormats);
    }
            
    return includePostScript;
}


Int16 MacPrintOpenPage(ProofContext *ctx)
{
	OSStatus status = noErr;
	status = PMSessionBeginPage(ctx->printSession, ctx->pageFormat, NULL);
	if (status) 
		return status;
	
	status = PMSessionPostScriptBegin(ctx->printSession);
	if (status) 
		return status;
		
	GetPort(&(ctx->currPort));
	status = PMSessionGetGraphicsContext(ctx->printSession, kPMGraphicsContextQuickdraw, (void**) &(ctx->printingPort));
	if (status == noErr)
		SetPort((GrafPtr)ctx->printingPort);
	return status;
}

Int16 MacPrintInitPage(ProofContext *ctx)
{	/* set-up coord system */
	proofPSOUT(ctx, "matrix defaultmatrix setmatrix initclip %% Macintosh-specific\n");	
	return 0;
}


Int16 MacPrintClosePage(ProofContext *ctx)
{
	OSStatus result = noErr;
	SetPort(ctx->currPort);
	result = PMSessionPostScriptEnd(ctx->printSession);
	if (result)
		return result;
	result = PMSessionEndPage(ctx->printSession);
	return result;
}

/* Open the print manager */
Int16 MacPrintOpen(ProofContext *ctx)
	{
	OSStatus result = noErr;
	Boolean ans = false;

	/* Open print driver */
	result = PMCreateSession(&(ctx->printSession));
	if (result)
		return result;
	
	/* default values */	
	ctx->pageFormat = kPMNoPageFormat;
	ctx->printSettings = kPMNoPrintSettings;	
	ctx->printingPort = NULL;

	result = PMCreatePrintSettings(&(ctx->printSettings));
	if (result)
		return result;
	result = PMSessionDefaultPrintSettings(ctx->printSession, ctx->printSettings);
	
	result = PMCreatePageFormat(&(ctx->pageFormat));
	if (result)
		return result;
	result = PMSessionDefaultPageFormat(ctx->printSession, ctx->pageFormat);	


	/* Validate */
	result = PMSessionValidatePrintSettings(ctx->printSession, ctx->printSettings, kPMDontWantBoolean);
	result |= PMSessionValidatePageFormat(ctx->printSession, ctx->pageFormat, kPMDontWantBoolean);
	
	if (result)
		return result;

#if INTERACTIVEDIALOGS		
    result = PMSessionPageSetupDialog(ctx->printSession, ctx->pageFormat, &ans);
    if (result == noErr && !ans)
    	result = kPMCancel;
	if (result)
		return result;
#endif
  
    PMSetPageRange(ctx->printSettings, 1, 99 /* ? */);
    
#if INTERACTIVEDIALOGS	    
    ans = false;
	result = PMSessionPrintDialog(ctx->printSession, ctx->printSettings, ctx->pageFormat, &ans);
    if (result == noErr && !ans)
    	result = kPMCancel;
    	
	if (result)
		return result;		
#endif
		

	ans = false;
	ans = IncludePostScriptInSpoolFile(ctx->printSession);
	if (!ans)
		{ /* failed */
		(void) MacPrintClose(ctx);
		return result;
		}
		
	
	/* Open document */
    result = PMSessionBeginDocument(ctx->printSession, ctx->printSettings, ctx->pageFormat);
    if (result)
    	return result;
	result = PMSessionError(ctx->printSession);

	if (!result)
		MacPrintOpenPage(ctx);
	
	return result;
	}

/* Close the print manager */
Int16 MacPrintClose(ProofContext *ctx)
	{
		OSStatus result;
	
		MacPrintClosePage(ctx);
		result = PMSessionEndDocument(ctx->printSession);		
		result = PMSessionError(ctx->printSession);	
		/* Spool document */

		PMRelease(ctx->pageFormat);
		PMRelease(ctx->printSettings);
		PMRelease(ctx->printSession);
			
		return result;
	}
/* ============================================================================ */
#elif WIN32

Int16 WinPrintOpenPage(ProofContext *ctx)
{
	if (ctx->winPrinterDC)
		return StartPage(ctx->winPrinterDC);
	else 
		return (-1);
}

Int16 WinPrintInitPage(ProofContext *ctx)
{	/* set-up coord system */
	proofPSOUT(ctx, "matrix defaultmatrix setmatrix initclip %% Macintosh-specific\n");	/* ??? */
	return 0;
}


Int16 WinPrintClosePage(ProofContext *ctx)
{
	if (ctx->winPrinterDC)
		return EndPage(ctx->winPrinterDC);
	else
		return (-1);
}

/* Open the print manager */
Int16 WinPrintOpen(ProofContext *ctx)
	{
	Int16 result = 0;
	PRINTER_INFO_2 *pinfo2;
	DWORD dwNeeded, dwReturned=0;
	BOOL	isPS = FALSE;
	BOOL bResult;
	DWORD bytesRequired, dwStructCount;
	int prCode = 0;
	int whichprinter;
	char defaultprintername[256];

	defaultprintername[0] = '\0';
	/* get name of default printer */
	if (GetProfileString("windows", "device", ",,,", defaultprintername, 255) == 0)
	{
		/* failed */
		warning(SPOT_MSG_prufNOTPSDEV);
		return (-1);
	}
	else
	{
		char *ptr;
		ptr = strchr(defaultprintername, ',');
		*ptr = '\0';
	}

	EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, NULL, 0, &bytesRequired, &dwStructCount);
	pinfo2 = (PRINTER_INFO_2 *) memNew(bytesRequired);
	bResult = EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 2, (LPBYTE) pinfo2,
					 bytesRequired, &dwNeeded, &dwReturned);
	if (bResult == 0)
	{
	  memFree(pinfo2);
	  ctx->winPrinterDC = NULL;
	  return (-1);
	}
	else
	{
		if (dwReturned < 1)
		{
			memFree(pinfo2);
			ctx->winPrinterDC = NULL;
			return (-1);
		}
	}
	
	for (whichprinter = 0; ((DWORD)whichprinter) < dwReturned ; whichprinter++)
	{
		if (lstrcmpi((LPSTR)pinfo2[whichprinter].pPrinterName, (LPSTR)defaultprintername) != 0)
			continue;

		ctx->winPrinterDC = CreateDC(NULL, pinfo2[whichprinter].pPrinterName, NULL, NULL);

		if (ctx->winPrinterDC == NULL) 
			continue;

	/* Check for PS */
#if WIN32
		prCode = POSTSCRIPT_PASSTHROUGH;
		isPS = Escape(ctx->winPrinterDC, QUERYESCSUPPORT, sizeof(int), (LPSTR)&prCode, NULL);
#else
		{
		char	cTechnology[128];
		prCode = PASSTHROUGH;
		isPS =  ( (Escape (ctx->winPrinterDC, GETTECHNOLOGY, 0, NULL, cTechnology) > 0)  &&
				  (lstrcmpi((LPSTR)cTechnology, (LPSTR)"PostScript") == 0) &&
				  (Escape(ctx->winPrinterDC, QUERYESCSUPPORT, sizeof(int), (LPSTR)&prCode, NULL)));
		}
#endif

		if (!isPS)
		{ 
			DeleteDC(ctx->winPrinterDC);
			ctx->winPrinterDC = NULL;
			continue;
		}
		else
			break;
	}

	memFree(pinfo2);

	if (ctx->winPrinterDC == NULL)
	{
		/* failed */
		warning(SPOT_MSG_prufNOTPSDEV);
		return (-1);
	}
	/* Open document */
	result = StartDoc(ctx->winPrinterDC, &(ctx->docinfo));
	if (result > 0)
		return WinPrintOpenPage(ctx);
	
	return result;
	}

/* Close the print manager */
Int16 WinPrintClose(ProofContext *ctx)
	{
		if (ctx->winPrinterDC)
		{		
			WinPrintClosePage(ctx);
			EndDoc(ctx->winPrinterDC);
			DeleteDC(ctx->winPrinterDC);
			ctx->winPrinterDC = NULL;
		}

		return 0;
	}

#endif /* WIN32 */


