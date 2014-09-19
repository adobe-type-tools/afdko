/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/* SCCS Id:     @(#)bez_parse.y	1.5
/* Changed:     3/14/97 19:52:22
/*********************************************************************/
%start bezFile

%{
#include <tfio_lib.h>

#define BAD_COMMENT	{ parseError = tfio_SYNTAX; return 1; /* Quit parsing */ }

int tfio_ignoreComment;

static void (*inHandler[tfio_INHANDLERCNT])(); /* Input handler functions */
#define CALL_HANDLER(handler, args) \
	if (inHandler[handler]) \
		inHandler[handler]args

#define HINT_PARAMS 6
static int paramCnt;
static double hintParam[HINT_PARAMS];
static int validElems;
static int elemCnt;
static int elemParam[HINT_PARAMS];
static int cx, cy;
static int dx, dy, dx1, dy1, dx2, dy2, dx3, dy3;
static int adx1, ady1, adx2, ady2, adx3, ady3;
static int bdx1, bdy1, bdx2, bdy2, bdx3, bdy3;
static int parseError;
static void *clientPtr;	/* Client data pointer */
%}

%union
	{
	int LONG;
	double DOUBLE;
	char *TEXT;
	}

%token K_rmt K_hmt K_vmt K_mt
%token K_rdt K_hdt K_vdt K_dt
%token K_rct K_hvct K_vhct K_ct
%token K_preflx1 K_preflx2 K_flx
%token K_id K_cp
%token K_rb K_ry K_rm K_rv
%token K_sc K_ed
%token K_sol K_eol
%token K_beginsubr K_snc K_endsubr K_enc K_newcolors
%token K_div NUMBER COMMENT
%token K_unknown

%type <TEXT> COMMENT
%type <DOUBLE> hintParam signedNumber expression
%type <LONG> NUMBER

%%

bezFile		: hintList path
			| path
			;
hintList	: hintList hintCmdAction
			| hintCmdAction
			;
hintCmdAction: hintCmd
				{ paramCnt = elemCnt = 0; validElems = 0; }
			;		  
hintCmd		: rm3
			| rv3
			| hintParam hintParam K_rb comment
				{
				CALL_HANDLER(tfio_INHSTEM,
							 (clientPtr,
							  $1, $2, validElems, elemParam[0], elemParam[1]));
				}
			| hintParam hintParam K_ry comment
				{
				CALL_HANDLER(tfio_INVSTEM,
							 (clientPtr,
							  $1, $2, validElems, elemParam[0], elemParam[1]));
				}
			;
rm3			: rm rm rm
				{
				if (elemCnt != 0 && elemCnt != 6)
					BAD_COMMENT;
				CALL_HANDLER(tfio_INHSTEM3,
							 (clientPtr, 
							  hintParam[0], hintParam[1],
							  hintParam[2], hintParam[3],
							  hintParam[4], hintParam[5],
							  validElems,
							  elemParam[0], elemParam[1],
							  elemParam[2], elemParam[3],
							  elemParam[4], elemParam[5]));
				}
			;
rm			: hintParam hintParam K_rm comment
				{ hintParam[paramCnt++] = $1; hintParam[paramCnt++] = $2; }
			;
hintParam	: signedNumber
				{ $$ = $1; }
			;
signedNumber: NUMBER
				{ $$ = $1; }
			| expression
				{ $$ = $1; }
			;
expression	: hintParam hintParam K_div
				{ $$ = $1 / $2; }
			;
rv3			: rv rv rv
				{
				if (elemCnt != 0 && elemCnt != 6)
					BAD_COMMENT;
				CALL_HANDLER(tfio_INVSTEM3,
							 (clientPtr,
							  hintParam[0], hintParam[1],
							  hintParam[2], hintParam[3],
							  hintParam[4], hintParam[5],
							  validElems,
							  elemParam[0], elemParam[1],
							  elemParam[2], elemParam[3],
							  elemParam[4], elemParam[5]));
				}
			;
rv			: hintParam hintParam K_rv comment
				{ hintParam[paramCnt++] = $1; hintParam[paramCnt++] = $2; }
			;
comment		: 	{ tfio_ignoreComment = 0; }
			  commentRule
				{ tfio_ignoreComment = 1; }
			;
commentRule	: COMMENT
				{
				if (sscanf($1, "%d %d", &elemParam[elemCnt],
					&elemParam[elemCnt + 1]) != 2)
					BAD_COMMENT;
				elemCnt += 2;
				validElems = 1;
				}
			| empty
			;
empty		:
			;
path		: K_sc
				{ CALL_HANDLER(tfio_INSTARTPATH, (clientPtr)); }
			  subPathList K_ed
				{ CALL_HANDLER(tfio_INENDPATH, (clientPtr)); }
			| K_sc K_ed
			;
subPathList	: subPathList subPath
			| subPath
			;
subPath		: subPathBody K_cp
				{ CALL_HANDLER(tfio_INCLOSEPATH, (clientPtr, cx, cy)); }
			| subPathBody
			| eol
			;
subPathBody	: movetoAction
			| movetoAction subPathTail
			| movetoAction hintSubs subPathTail
			| hintSubs movetoAction subPathTail
			| hintSubs movetoAction hintSubs subPathTail
			;
subPathTail	: pathCmdList
			| pathHintPairList
			| pathHintPairList pathCmdList
			;
pathHintPairList: pathHintPairList pathCmdList hintSubs
			| pathCmdList hintSubs
			;

movetoAction: moveto
				{
				CALL_HANDLER(tfio_INRMOVETO, (clientPtr, cx, cy, dx, dy));
				cx += dx;
				cy += dy;
				}
			|
			;

moveto		: NUMBER NUMBER K_rmt
				{
				dx = $1;
				dy = $2;
				}
			| NUMBER K_hmt
				{
				dx = $1;
				dy = 0;
				}
			| NUMBER K_vmt
				{
				dx = 0;
				dy = $1;
				}
			| NUMBER NUMBER K_mt
				{
				dx = $1 - cx;
				dy = $2 - cy;
				}
			;
pathCmdList	: pathCmdList pathCmd
			| pathCmd
			;
pathCmd		: lineto
				{
				CALL_HANDLER(tfio_INRLINETO, (clientPtr, cx, cy, dx, dy));
				cx += dx;
				cy += dy;
				}
			| curveto
				{
				CALL_HANDLER(tfio_INRCURVETO,
							 (clientPtr,
							  cx, cy, dx1, dy1, dx2, dy2, dx3, dy3));
				cx += dx3;
				cy += dy3;
				}
			| flex
			| NUMBER K_id
				{
				CALL_HANDLER(tfio_INID, (clientPtr, $1));
				}
			| K_sol
				{
				CALL_HANDLER(tfio_INBEGINDOTSECTION, (clientPtr));
				}
			| eol
			;
lineto		: NUMBER NUMBER K_rdt
				{
				dx = $1;
				dy = $2;
				}
			| NUMBER K_hdt
				{
				dx = $1;
				dy = 0;
				}
			| NUMBER K_vdt
				{
				dx = 0;
				dy = $1;
				}
			| NUMBER NUMBER K_dt
				{
				dx = $1 - cx;
				dy = $2 - cy;
				}
			;
curveto		: NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER K_rct
				{
				dx1 = $1;
				dy1 = $2;
				dx2 = dx1 + $3;
				dy2 = dy1 + $4;
				dx3 = dx2 + $5;
				dy3 = dy2 + $6;
				}
			| NUMBER NUMBER NUMBER NUMBER K_hvct
				{
				dx1 = $1;
				dy1 = 0;
				dx2 = dx1 + $2;
				dy2 = $3;
				dx3 = dx2;
				dy3 = dy2 + $4;
				}
			| NUMBER NUMBER NUMBER NUMBER K_vhct
				{
				dx1 = 0;
				dy1 = $1;
				dx2 = $2;
				dy2 = dy1 + $3;
				dx3 = dx2 + $4;
				dy3 = dy2;
				}
			| NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER K_ct
				{
				dx1 = $1 - cx;
				dy1 = $2 - cy;
				dx2 = $3 - cx;
				dy2 = $4 - cy;
				dx3 = $5 - cx;
				dy3 = $6 - cy;
				}
			;
flex		: K_preflx1 moveto
			  K_preflx2 moveto K_preflx2 moveto K_preflx2 moveto
			  K_preflx2 moveto K_preflx2 moveto K_preflx2 moveto K_preflx2
			  NUMBER NUMBER NUMBER NUMBER NUMBER
			  NUMBER NUMBER NUMBER NUMBER NUMBER
			  NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER NUMBER K_flx
				{
				adx1 = $16;
				ady1 = $17;
				adx2 = adx1 + $18;
				ady2 = ady1 + $19;
				adx3 = adx2 + $20;
				ady3 = ady2 + $21;
				bdx1 = $22;
				bdy1 = $23;
				bdx2 = bdx1 + $24;
				bdy2 = bdy1 + $25;
				bdx3 = bdx2 + $26;
				bdy3 = bdy2 + $27;
				CALL_HANDLER(tfio_INRFLEX,
							 (clientPtr, cx, cy,
							  adx1, ady1, adx2, ady2, adx3, ady3,
							  bdx1, bdy1, bdx2, bdy2, bdx3, bdy3,
							  $28));
				cx = $31;
				cy = $32;
				}
			;
eol			: K_eol
				{
				CALL_HANDLER(tfio_INENDDOTSECTION, (clientPtr));
				}
hintSubs	: beginHintSubs hintList endHintSubs
			;
beginHintSubs: K_beginsubr K_snc
			;
endHintSubs	: K_endsubr K_enc K_newcolors
			;

%%
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define LEN_IV 4	/* Length of initial random byte sequence */

void tfio_RHin(tfio_inHId id, void (*hfn)())
	{
	static int init = 1;
	if (init)
		{
		int i;
		for (i = 0; i < tfio_INHANDLERCNT; i++)
			inHandler[i] = NULL;
		init = 0;
		}
	if (id >= 0 && id < tfio_INHANDLERCNT)
		inHandler[id] = hfn;
	}

static int isEncrypted(FILE *fp)
	{	
	int ch;
	int n = 0;
	int encrypted = 1;

	/* If the first hundred bytes are hex digits and spaces then
	 * assume file is encrypted.
	 */
	while ((ch = getc(fp)) != EOF && n++ < 100)
		if (!isxdigit(ch) && !isspace(ch))
			{
			encrypted = 0;
			break;
			}

	rewind(fp);
	return encrypted;
	}

static int decryptFile(FILE *fp, char *buf)
	{
	unsigned long r = 11586L;
	char *p = buf;
	int byteCnt = 0;

	for (;;)
		{
		int cipher = 0;
		int plain;
		int i = 2;

		while (i--)
			{
			int ch;

			do
				{
				ch = getc(fp);
				if (ch == EOF)
					return byteCnt - LEN_IV;
				}
			while (isspace(ch));

			if (isupper(ch))
				ch = tolower(ch);
			cipher = (cipher << 4) | (isdigit(ch) ? ch - '0' : ch - 'a' + 10);
			}

		plain = cipher ^ (r >> 8);
		r = (cipher + r) * 902381661L + 341529579L;

		if (++byteCnt > LEN_IV)
			*p++ = plain;
		}
	}
	
int tfio_inChar(FILE *fp, void *client)
	{
	extern int tfio_bezparse();
	extern void *tfio_bezUseBuffer();
	char *buf;
	int decrypt;
	struct stat s;
	int bufLen;
	int plainLen;

	if (fp == NULL)
		return tfio_FILE;

	if (fstat(fileno(fp), &s) != 0)
	  return tfio_SYSTEM;
	
	decrypt = isEncrypted(fp);
	if (decrypt) {
	  bufLen = (s.st_size - (LEN_IV * 2)) / 2;
	}
	else {
	  bufLen = s.st_size;
	}

	if (bufLen < 1)
	  return tfio_SYNTAX;
	
	buf = calloc(bufLen+2, sizeof(char));
	if (buf == NULL)
	  return tfio_SYSTEM;

	if (decrypt) {
	  plainLen = decryptFile(fp, buf);
	}
	else { /* Read from file */
	  fread( buf, sizeof(char), bufLen, fp);
	}

	/* Initialise parser */
	tfio_ignoreComment = 1;
	paramCnt = elemCnt = cx = cy = 0;
	validElems = 0;
	parseError = tfio_OK;
	clientPtr = client;
	
	tfio_bezUseBuffer (buf, (decrypt) ? plainLen : bufLen);
	tfio_bezparse();
	
	if (decrypt)
		free(buf);

	return parseError;
	}

void
tfio_bezerror(char *s)
	{
	parseError = (strcmp(s, "syntax error") == 0) ? tfio_SYNTAX : tfio_YACC;
	}
