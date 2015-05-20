/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//* AC_C_lib.c */


#include "setjmp.h"

#include "ac_C_lib.h"
#include "ac.h"
#include "bftoac.h"
#include "machinedep.h"


const char *libversion = "1.5.11";
const char *progname="AC_C_lib";
char editingResource=0;

FFEntry *featurefiledata;
int featurefilesize;

char *FL_glyphname=0;

const char *bezstring=0;
char *bezoutput=0;
int bezoutputalloc=0;
int bezoutputactual=0;

extern int compatiblemode; /*forces the definition of colors even before a color sub*/

jmp_buf aclibmark;   /* to handle exit() calls in the library version*/


#define skipblanks() while(*current=='\t' || *current=='\n' || *current==' ' || *current=='\r') current++
#define skipnonblanks() while(*current!='\t' && *current!='\n' && *current!=' ' && *current!='\r'&& *current!='\0') current++
#define skipmatrix() while(*current!='\0' && *current!=']') current++

static void skippsstring(const char **current)
{
	int parencount=0;
	
	do
	{
		if(**current=='(')
			parencount++;
		else if(**current==')')
			parencount--;
		else if(**current=='\0')
			return;
			
		(*current)++;
		
	}while(parencount>0);
}

static void FreeFontInfoArray(void)
{
	int i;
	
	for (i=0; i<featurefilesize; i++)
	{
		if (featurefiledata[i].value[0])
		{
#if DOMEMCHECK
			memck_free(featurefiledata[i].value);
#else
			ACFREEMEM(featurefiledata[i].value);
#endif
		}
	}
#if DOMEMCHECK
	memck_free(featurefiledata);
#else
	ACFREEMEM(featurefiledata);
#endif
	
}
static int ParseFontInfo(const char *fontinfo)
{
	const char *kwstart, *kwend, *tkstart, *current;
	int i;

	featurefilesize=34;
#if DOMEMCHECK
	featurefiledata= ( FFEntry *) memck_malloc(featurefilesize*sizeof(FFEntry));
#else
	featurefiledata= ( FFEntry *) ACNEWMEM(featurefilesize*sizeof(FFEntry));
#endif	
	featurefiledata[0].key="OrigEmSqUnits";
	featurefiledata[1].key="FontName";
	featurefiledata[2].key="FlexOK";
	/* Blue Values */
	featurefiledata[3].key="BaselineOvershoot";
	featurefiledata[4].key="BaselineYCoord";
	featurefiledata[5].key="CapHeight";
	featurefiledata[6].key="CapOvershoot";
	featurefiledata[7].key="LcHeight";
	featurefiledata[8].key="LcOvershoot";
	featurefiledata[9].key="AscenderHeight";
	featurefiledata[10].key="AscenderOvershoot";
	featurefiledata[11].key="FigHeight";
	featurefiledata[12].key="FigOvershoot";
	featurefiledata[13].key="Height5";
	featurefiledata[14].key="Height5Overshoot";
	featurefiledata[15].key="Height6";
	featurefiledata[16].key="Height6Overshoot";
	/* Other Values */
	featurefiledata[17].key="Baseline5Overshoot";
	featurefiledata[18].key="Baseline5";
	featurefiledata[19].key="Baseline6Overshoot";
	featurefiledata[20].key="Baseline6";
	featurefiledata[21].key="SuperiorOvershoot";
	featurefiledata[22].key="SuperiorBaseline";
	featurefiledata[23].key="OrdinalOvershoot";
	featurefiledata[24].key="OrdinalBaseline";
	featurefiledata[25].key="DescenderOvershoot";
	featurefiledata[26].key="DescenderHeight";
	
	featurefiledata[27].key="DominantV";
	featurefiledata[28].key="StemSnapV";
	featurefiledata[29].key="DominantH";
	featurefiledata[30].key="StemSnapH";
	featurefiledata[31].key="VCounterChars";
	featurefiledata[32].key="HCounterChars";
	/* later addenda */
	featurefiledata[33].key="BlueFuzz";
	
	
	for (i=0; i<featurefilesize; i++)
	{
		featurefiledata[i].value="";
	}
	
	if(!fontinfo)
		return AC_Success;

	current=fontinfo;
	while (*current)
	{
		size_t kwLen;
		skipblanks();
		kwstart=current;
		skipnonblanks();
		kwend = current;
		skipblanks();
		tkstart=current;
		if (*tkstart=='(')
		{
			skippsstring(&current);
			if (*tkstart) current++;
		}
		else if(*tkstart=='[')
		{
			skipmatrix();
			if (*tkstart) current++;
		}else
			skipnonblanks();
		
		kwLen = (int)(kwend - kwstart);
		for (i=0; i<featurefilesize; i++)
		{
			size_t matchLen = MAX(kwLen, strlen(featurefiledata[i].key));
			if(!strncmp(featurefiledata[i].key, kwstart, matchLen))
			{
#if DOMEMCHECK
				featurefiledata[i].value=(char *)memck_malloc(current-tkstart+1);
#else
				featurefiledata[i].value=(char *)ACNEWMEM( current-tkstart+1 );
#endif
				if (!featurefiledata[i].value)
					return AC_MemoryError;
				strncpy(featurefiledata[i].value, tkstart, current-tkstart);
				featurefiledata[i].value[current-tkstart]='\0';
				break;
			}
		}
		if (i==featurefilesize)
		{
			char *temp;
#if DOMEMCHECK
			temp=(char*)memck_malloc(tkstart-kwstart+1);
#else
			temp=(char*)ACNEWMEM(tkstart-kwstart+1);
#endif
			if(!temp)
				return AC_MemoryError;
			strncpy(temp, kwstart, tkstart-kwstart);
			temp[tkstart-kwstart]='\0';
			/*fprintf(stderr, "Ignoring fileinfo %s...\n", temp);*/
#if DOMEMCHECK
			memck_free(temp);
#else
			ACFREEMEM(temp);
#endif
		}
		skipblanks();
	}

	return AC_Success;
}
#if __MWERKS__
#pragma export on
#endif
ACLIB_API void  AC_SetMemManager(void *ctxptr, AC_MEMMANAGEFUNCPTR func)
{
	setAC_memoryManager(ctxptr, func);
}

ACLIB_API void  AC_SetReportCB(AC_REPORTFUNCPTR reportCB, int verbose)
{
	if (verbose)
		libReportCB = reportCB;
	else
		libReportCB = NULL;
	
	libErrorReportCB = reportCB;
}


ACLIB_API void  AC_SetReportStemsCB(AC_REPORTSTEMPTR hstemCB, AC_REPORTSTEMPTR vstemCB, unsigned int allStems)
{
	allstems = allStems;
	addHStemCB = hstemCB;
	addVStemCB = vstemCB;
	doStems = 1;

	addCharExtremesCB = NULL;
	addStemExtremesCB = NULL;
	doAligns = 0;
}

ACLIB_API void  AC_SetReportZonesCB(AC_REPORTZONEPTR charCB, AC_REPORTZONEPTR stemCB)
{
	addCharExtremesCB = charCB;
	addStemExtremesCB = stemCB;
	doAligns = 1;

	addHStemCB = NULL;
	addVStemCB = NULL;
	doStems = 0;
}


int cleanup(short code)
{
  closefiles();
  
	if (code==FATALERROR || code==NONFATALERROR)
		longjmp(aclibmark, -1);
	else
		longjmp(aclibmark, 1);

	return 0; /* we dont actually ever get here */
}


ACLIB_API int  AutoColorString(const char *srcbezdata, const char *fontinfo, char *dstbezdata, int *length, int allowEdit, int allowHintSub, int roundCoords, int debug)
{
	int value, result;
	char *names[]={""};
	
	if (!srcbezdata)
		return AC_InvalidParameterError;

	if (ParseFontInfo(fontinfo))
		return AC_FontinfoParseFail;

	set_errorproc(cleanup);
	value = setjmp(aclibmark);
	
	if(value==-1){
		/* a fatal error occurred soemwhere. */
		FreeFontInfoArray();
		return AC_FatalError;
		
	}else if(value==1){
		/* AutoColor was called succesfully */
		FreeFontInfoArray();
		if (bezoutputactual<*length)
		{
			strncpy(dstbezdata, bezoutput, bezoutputactual+1);
			*length=bezoutputactual+1;
#if DOMEMCHECK
			memck_free(bezoutput);
#else
			ACFREEMEM(bezoutput);
#endif
			bezoutputalloc=0;
			return AC_Success;
		}else{
			*length=bezoutputactual+1;
#if DOMEMCHECK
			memck_free(bezoutput);
#else
			ACFREEMEM(bezoutput);
#endif
			bezoutputalloc=0;
			return AC_DestBuffOfloError;
		}
	}
	
	bezstring=srcbezdata;
	
	bezoutputalloc=*length;
	bezoutputactual=0;
#if DOMEMCHECK
	bezoutput=(char*)memck_malloc(bezoutputalloc);
#else
	bezoutput=(char*)ACNEWMEM(bezoutputalloc);
#endif
	if(!bezoutput)
	{
		FreeFontInfoArray();
		return AC_MemoryError;
	}
	*bezoutput=0;
	
	result = AutoColor(
		       FALSE, /* whether any new coloring should cause error */
		       FALSE,  /*fixStems*/
			   (boolean)debug, /*debug*/
			   allowHintSub, /* extracolor*/ 
			   allowEdit, /*editChars*/ 
			   1,
		       names, 
			   FALSE, /*quiet*/ 
                FALSE, /* doAll*/
                roundCoords, /* doAll*/
                FALSE);/* do log */
	/* result == TRUE is good */
	/* The following call to cleanup() always returns control to just after the setjmp() function call above,,
	but with value set to 1 if success, or -1 if not */
	cleanup( (result == TRUE) ? OK: NONFATALERROR);
	
	
	return AC_UnknownError; /*Shouldn't get here*/
}

ACLIB_API void AC_initCallGlobals(void)
{
	libReportCB = NULL;
	libErrorReportCB = NULL;
	addCharExtremesCB = NULL;
	addStemExtremesCB = NULL;
	doAligns = 0;
	addHStemCB = NULL;
	addVStemCB = NULL;
	doStems = 0;
}

ACLIB_API const char *AC_getVersion(void)
{
	return libversion;
}


#if __MWERKS__
#pragma export off
#endif

