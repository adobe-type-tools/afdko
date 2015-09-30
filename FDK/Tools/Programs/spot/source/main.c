/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
#include <string.h>
#include <stdlib.h>

#include "global.h"
#include "sfnt.h"
#include "sfnt_sfnt.h"
#include "CFF_.h"
#include "res.h"
#include "glyf.h"
#include "proof.h"
#include "da.h"
#include "sys.h"
#if MEMCHECK
	#include "memcheck.h"
#endif
#include "cmap.h"
#include <ctype.h>
#include "setjmp.h"
#include "map.h"

jmp_buf mark;
#define MAX_ARGS 200


Byte8 *version = "3.5.64775";	/* Program version */
Byte8 *libversion = "3.5.64775";	/* Library version */
char * sourcepath;
char * outputfilebase = NULL;
char *infilename=NULL;

extern GlyphComplementReportT gcr; /*Used for Glyph Complement Reporter*/
extern char aliasfromfileinit;

char * glyphaliasfilename = NULL;

/*VORGfound: set in CFF_getMetrics to indicate whether a VORG entry was found for the glyph.*/
char VORGfound=0;

#if AUTOSCRIPT
static Byte8 scriptfilename[512];

typedef struct _cmdlinetype
{
  da_DCL(char *, args); /* arg list */  
} cmdlinetype;

static struct
{
  Byte8 *buf; /* input buffer */
  da_DCL(cmdlinetype, cmdline);
} script;

#endif

static char * MakeFullPath(char *source)
{
	char * dest;
		
	dest = (char *) memNew(256);
	if(sourcepath[0]=='\0' || strchr(source, '\\')!=NULL)
		sprintf(dest, "%s", source);
	else
		sprintf(dest, "%s\\%s", sourcepath, source);
	return dest;
}


/* File signatures */

typedef unsigned long ctlTag;
#define CTL_TAG(a,b,c,d) \
    ((ctlTag)(a)<<24|(ctlTag)(b)<<16|(ctlTag)(c)<<8|(ctlTag)(d))

#define sig_PostScript0	CTL_TAG('%',  '!',0x00,0x00)
#define sig_PostScript1	CTL_TAG('%',  'A',0x00,0x00)	/* %ADO... */
#define sig_PostScript2	CTL_TAG('%',  '%',0x00,0x00)	/* %%... */
#define sig_PFB   	   	((ctlTag)0x80010000)
#define sig_CFF		   	((ctlTag)0x01000000)
#define sig_MacResource	((ctlTag)0x00000100)
#define sig_AppleSingle	((ctlTag)0x00051600)
#define sig_AppleDouble	((ctlTag)0x00051607)
typedef struct				/* AppleSingle/Double entry descriptor */
	{
	unsigned long id;
	long offset;
	unsigned long length;
} EntryDesc;


/* Process AppleSingle/Double format data. */
static void doASDFormats(ctlTag magic)
	{
	long junk;
	long i;
	Card16 entryCount = 0;
	struct					/* AppleSingle/Double data */
		{
		unsigned long magic;/* Magic #, 00051600-single, 00051607-double */
		unsigned long version;/* Format version */
		da_DCL(EntryDesc, entries);/* Entry descriptors */
	} asd;

	asd.magic = magic;
	IN1(asd.version);

	/* Skip filler of 16 bytes*/
	IN1(junk);
	IN1(junk);
	IN1(junk);
	IN1(junk);

	/* Read number of entries */
	IN1(entryCount);
	da_INIT(asd.entries, entryCount,10);
	
	/* Read entry descriptors */
	for (i = 0; i < entryCount; i++)
		{
		EntryDesc *entry = da_INDEX(asd.entries,i);
	 	IN1(entry->id);
	 	IN1(entry->offset);
	 	IN1(entry->length);
		}

		for (i = 0; i < entryCount; i++)
			{
			EntryDesc  *entry = da_INDEX(asd.entries,i);
			if (entry->length > 0)
				switch (entry->id)
					{
				case 1:
					/* Data fork (AppleSingle); see if it's an sfnt */
		  			sfntRead(entry->offset + 4, -1);	/* Read plain sfnt file */
					  sfntDump();
					  sfntFree(1);		  
					break;
				case 2:
					/* Resource fork (AppleSingle/Double) */
					fileSeek(entry->offset, 0);
		 			resRead(entry->offset);			/* Read and dump Macintosh resource file */
					break;
					}
			}
	}

static int readFile(char * filename)
{
	  Card32 value;

	/* See if we can recognize the file type */
	value = fileSniff();	
	switch (value)
	{
		case bits_:
		case typ1_:		
		case true_:
		case mor0_:
		case OTTO_:
		case VERSION(1,0):
		  sfntRead(0, -1);	/* Read plain sfnt file */
		  sfntDump();
		  sfntFree(1);		  
		  break;
		case ttcf_:
		  sfntTTCRead(0);		/* Read TTC and dump file */
		  break;
		case 256:
		  SEEK_ABS(0); /* Raw resource file as a data fork */ 
		  resRead(0);			/* Read and dump  Macintosh resource file */
		  break;
		case sig_AppleSingle:
		case sig_AppleDouble:
			doASDFormats((ctlTag)value);
			break;
		default:
 			warning(SPOT_MSG_BADFILE, filename);
 			return 1;
		}
	return 0;
}
	
	
	
	
/* Print usage information */
static void printUsage(void)
	{
	fprintf(OUTPUTBUFF,  "Usage: %s [-u|-h|-ht|-r] [-n|-nc|-G|-T|-F] [-f] [-V] [-m] [-d]"
#if AUTOSPOOL
		   "[-l|-O] "
#endif
		   "[-i<ids>] [-o<offs>] [-t<tags>|-P<featuretags>] [-p<policy>] [-@ <ptsize>]  "
		   "<fontfile>+\n\n"
#if AUTOSCRIPT
			"OR: %s  -X <scriptfile>\n\n"
#endif
		   "Options:\n"
		   "    -u  print usage information\n"
		   "    -h  print usage and help information\n"
		   "    -ht print table-specific usage information\n"
		   "    -r  dump Macintosh resource map\n"
		   "    -n  dump glyph id/name mapping (also see '-m' below)\n"
		   "    -nc dump glyph id/name mapping, one per line (also see '-m' below)\n"
		   "    -ngid Suppress terminal gid on glyph names from TTF fonts.\n"
		   "    -T  list table-directory in sfnt table\n"
		   "    -F  list features in GPOS,GSUB tables\n"
		   "    -G  proof glyph synopsis\n"
		   "    -f  proof GPOS features in font order instead of GID order\n"
		   "    -V  proof glyphs in Vertical writing mode (same as -p6 below)\n"
		   "    -m  map glyph names into Adobe 'friendly' names, not AGL/Unicode names\n"
		   "    -d  suppress header info from proof\n"
		   "    -br proof glyph synopsis one per page\n"
#if AUTOSPOOL
		   "    -l  leave proofing output files; do not spool them to printer\n"
		   "    -O  proof data to 'stdout' (e.g., for sending to printer or saving to some file)\n"
#endif
		   "    -i  sfnt resource id list (see help)\n"
		   "    -o  TTC directory offsets list (see help)\n"
		   "    -t  table dump list (see help)\n"
		   "    -P  <list of feature tags from GSUB or GPOS>, e.g 'P cswh,frac,kern'  (use '-Proof' for all)\n"
		   "    -p  set proofing policies: \n"
		   "        1=No glyph name labels\n"
		   "        2=No glyph numeric labels\n"
/*		   "        3=Show glyph lines (lsb, rsb, etc.)\n" */
		   "        4=Show KanjiStandardEMbox on glyph\n"
		   "        5=Show GlyphBBox on glyph\n"
   		   "        6=Show Kanji in Vertical writing mode\n"
   		   "        7=Don't show Kanji 'kern','vkrn' with 'palt','vpal' values applied\n"
		   "    -@  set proofing glyph point-size (does not apply to certain synopses)\n"
#if AUTOSCRIPT
		   "    -X  execute a series of complete command-lines from <scriptfile> [default: OTFproof.scr ]\n"
#endif
		   "Note: Proof options write a PostScript file to standard output, and must be redirected to a file. \n"
		   "Example: 'spot -P kern test.otf > kern.ps'   This file can then be converted to PDF with Distiller, or\n"
		   "downloaded to a printer.\n"
		   "\n"
		   "Version:\n"
		   "    %s\n",
#if AUTOSCRIPT
		   global.progname, 
#endif
		   global.progname, 
		   version);
	}

/* Show usage information */
static void showUsage(void)
	{
	printUsage();
	quit(0);
	}

/* Show usage and help information */
static void showHelp(void)
	{
	printUsage();
	sfntUsage();
	fprintf(OUTPUTBUFF, "Notes:\n"
		   "    This program dumps sfnt data from plain files or Macintosh\n"
		   "resource files. In the latter case, when there are 2 or more sfnt\n"
		   "resources, or in the absence of the -i option, the resource map\n"
		   "is dumped as a list of resource types and ids. The program may\n"
		   "then be rerun with an argument to the -i option specifying a\n"
		   "comma-separated sfnt id list of the sfnt resources to be dumped\n"
		   "or the argument 'all' which dumps all sfnt resources.\n"
		   "    All the multiple sfnts within a TrueType Collection (TTC)\n"
		   "will be dumped by default but may be selectively dumped via TTC\n"
		   "directory offsets specified with the -o option, e.g.\n"
		   "-o0x14,0x170. Use the -tttcf option to view all the TTC\n"
		   "directory offsets available.\n"
		   "    The argument to the -t option specifies a list of tables to\n"
		   "be dumped. Tables are selected by a comma-separated list of\n"
		   "table tags, e.g. cmap,MMVR,fdsc. An optional dump level may be\n"
		   "specified by appending '=' followed by the level to a tag,\n"
		   "e.g. cmap=2. Higher levels print successively more information\n"
		   "up to a maximum of 4 (the default) although all 4 levels are not\n"
		   "supported by all tables. Level 1 always prints the table tag and\n"
		   "file offset. When a level is specified its value becomes the\n"
		   "default for the remaining tables in the list unless a new level\n"
		   "is specified, e.g. cmap=2,MMVR,HFMX,fdsc=1 dumps the cmap, MMVR,\n"
		   "and HFMX tables at level 2 and the fdsc table at level 1. If a\n"
		   "level is given as 'x', e.g. cmap=x the table is dumped in\n"
		   "hexadecimal format. If an unsupported table tag is specified the\n"
		   "table is dumped in hexadecimal format regardless of whether a\n"
		   "level of 'x' is specified.\n");
	fprintf(OUTPUTBUFF, "    The -n option dumps glyph id to glyph name mapping for all\n"
		   "glyphs in the font. Naming information is extracted from post\n"
		   "format 1.0, 2.0, or 2.5 tables, or if they are not available,\n"
		   "the MS UGL cmap. In the event that none of these is available\n"
		   "the glyph name is simply printed as @ followed by the decimal\n"
		   "glyph id in decimal.\n"
		   "    The special tag 'sfnt' dumps the sfnt directory which lists\n"
		   "all the table present in the sfnt. The special tag 'uset' sets\n"
		   "the dump level on all unset tables in the font allowing tables\n"
		   "to be dumped without explicitly naming them, e.g. -tuset will\n"
		   "dump all tables at the default level 4. If this produces too\n"
		   "much information you can disable dumping of selected tables by\n"
		   "setting their dump level to 0, e.g. -tuset,glyf=0,hdmx.\n"
		   "    The -ht option provides additional table-specific help.\n");

	quit(0);
	}

#if AUTOSCRIPT
static void makeArgs(char *filename)
      {
      int state;
      long i;
      long length;
      IntX file;
      char *start = NULL;     /* Suppress optimizer warning */
      cmdlinetype *cmdl;

      /* Read whole file into buffer */
      file = sysOpenSearchpath(filename);
	  if (file < 0)
      	fatal(SPOT_MSG_BADSCRIPTFILE, filename);
      length = sysFileLen(file);
      if (length < 1) 
      	fatal(SPOT_MSG_BADSCRIPTFILE, filename);
      script.buf = memNew(length + 2);

	  message(SPOT_MSG_ECHOSCRIPTFILE, filename);
      sysSeek(file, 0, 0, filename);
      sysRead(file, (Card8 *)script.buf, length, filename);
      sysClose(file, filename);

      script.buf[length] = '\n';      /* Ensure termination */
      script.buf[length+1] = '\0';    /* Ensure termination */
      /* Parse buffer into args */
      state = 0;
      da_INIT(script.cmdline, 10, 10);
      cmdl = da_NEXT(script.cmdline);
      da_INIT(cmdl->args, 10, 10);
      *da_NEXT(cmdl->args) = global.progname;
      for (i = 0; i < length + 1; i++)
              {
              char c = script.buf[i];
              switch (state)
                      {
              case 0:
                      switch ((int)c)
                              {
                      case '\n':
                      case '\r':
                              cmdl = da_NEXT(script.cmdline);
                              da_INIT(cmdl->args, 10, 10);
                              *da_NEXT(cmdl->args) = global.progname;
                              break;
                      case '\f': 
                      case '\t': 
                              break;
                      case ' ': 
                              break;
                      case '#': 
                              state = 1;
                              break;
                      case '"': 
                              start = &script.buf[i + 1];
                              state = 2; 
                              break;
                      default:  
                              start = &script.buf[i];
                              state = 3; 
                              break;
                              }
                      break;
              case 1: /* Comment */
                      if (c == '\n' || c == '\r')
                              state = 0;
                      break;
              case 2: /* Quoted string */
                      if (c == '"')
                              {
                              script.buf[i] = '\0';   /* Terminate string */
                              *da_NEXT(cmdl->args) = start;
                              state = 0;
                              }
                      break;
              case 3: /* Space-delimited string */
                      if (isspace((int)c))
                              {
                              script.buf[i] = '\0';   /* Terminate string */
                              *da_NEXT(cmdl->args) = start;
                              state = 0;
                              if ((c == '\n') || (c == '\r')) 
                                {
                                      cmdl = da_NEXT(script.cmdline);
                                      da_INIT(cmdl->args, 10, 10);
                                      *da_NEXT(cmdl->args) = global.progname;
                                }
                              }
                      break;
                      }
              }
      }
#endif

#ifndef EXECUTABLE
#undef _DEBUG
#include "Python.h"

#ifdef WIN32
#ifdef OTFPROOFLIB_EXPORTS
#define OTFPROOFLIB_API __declspec(dllexport)
#else
#define OTFPROOFLIB_API  
#endif
#else
#define OTFPROOFLIB_API  
#endif

#if FDK_DEBUG
void OTFPROOFLIB_API initotfprooflibDB();
#else
void OTFPROOFLIB_API initotfprooflib();
#endif

PyObject OTFPROOFLIB_API *  main_python(PyObject *self, PyObject *args);
static PyObject *ErrorObject;
FILE *PyOutFile= NULL;
#define onError(message) {PyErr_SetString(ErrorObject, message); return NULL;}


/*Used to parse the parameter string passed in by python
 *start is the start index and is updated to the beginning of the next substring
 */

static char * NextToken(char * str, int *start)
{
	char *token;
	int end, current;
	
	if(str[*start]=='\0') return NULL;
	
	if(str[*start]=='\"'){
		for(end=*start+1; str[end]!='\"' && str[end]!='\0'; end++)
			;
		token=(char *)memNew((end-*start+1) * sizeof(char));
		for(current=*start+1; current<end; current++)
			token[current-*start-1]=str[current];
		token[current-*start-1]='\0';
		
		if(str[end]=='\"') end++;
		while(str[end]==' ') end++;
		*start=end;

	}else{
		for(end=*start; str[end]!=' ' && str[end]!='\0'; end++)
			;
	
		token=(char *)memNew((end-*start+1) * sizeof(char));
		for(current=*start; current<end; current++)
			token[current-*start]=str[current];
		token[current-*start]='\0';
		
		while(str[end]==' ') end++;
		*start=end;
	}
	return token;
}



PyObject * proof_complement(PyObject *self, PyObject *args)
{
	static double glyphptsize = STDPAGE_GLYPH_PTSIZE;
	static opt_Option opt[] =
	{
	  {"-u", opt_Call, showUsage},
	  {"-h", opt_Call, showHelp},
	  {"-ht", opt_Call, sfntTableSpecificUsage},
	  {"-l", opt_Flag},
	  {"-O", opt_Flag},
	  {"-r", opt_Flag},
	  {"-n", opt_Flag},
	  {"-nc", opt_Flag},
	  {"-T", opt_Flag},
	  {"-F", opt_Flag},
	  {"-G", opt_Flag},
	  {"-V", opt_Flag},
	  {"-m", opt_Flag},
	  {"-d", opt_Flag},
	  {"-br", opt_Flag},
	  {"-i", resIdScan},
	  {"-o", sfntTTCScan},
	  {"-t", sfntTagScan},
	  {"-P", sfntFeatScan}, 
	  {"-A", sfntFeatScan}, 
	  {"-p", proofPolicyScan}, 
	  {"-a", opt_Flag},
	  {"-R", opt_Flag},
	  {"-c", opt_Flag},
	  {"-g", glyfGlyphScan},
	  {"-b", glyfBBoxScan},
	  {"-s", glyfScaleScan},
	  {"-@", opt_Double, &glyphptsize},
	  {"-C", opt_Int, &cmapSelected},
	};

	IntN argi;
	Card32 value;
	char *outfilename;
	char *infilenameorig;
	IntN status;
	char *argv[30];
	int argc;
	int startIndex=0;
	int infilesSize;
	int useStdOut = 0;
	int i;
	

	{
	/* To allow gdb to attach to process */
	char* do_debug_sleep;
	do_debug_sleep = getenv("STOP_GLYPH_PROOFER");
	while (do_debug_sleep != NULL)
		sleep(1);
	}	


	value = setjmp(mark);
	
	if(value==-1){
		PyErr_SetString(ErrorObject, "Fatal Error");
		if (PyOutFile != NULL)
			{
			fclose(PyOutFile);
			PyOutFile = NULL;
			}
		freemap();
		return NULL;
	}
	
	if(!PyArg_ParseTuple(args, "s#iisiii", 
				&infilename, &infilesSize, 
				&(gcr.synOnly), &(gcr.numFonts), 
				&outfilename, 
				&(gcr.maxNumGlyphs), &(gcr.byname), &useStdOut)){
		freemap();
		return NULL;
	}

	if (PyOutFile != NULL)
		freopen(outfilename, "w", PyOutFile);
	else
		PyOutFile = fopen(outfilename, "w");
	
	infilenameorig=infilename;
	gcr.reportNumber=0;
	gcr.startGlyph=0;
	gcr.numGlyphs=2;
	gcr.endGlyph=1;
	
	VORGfound=0;
	status = setjmp(global.env);
	if (status)
	{
		if(status==2){
			PyErr_SetString(ErrorObject, "Fatal Error");
			if (PyOutFile != NULL)
				{
				fclose(PyOutFile);
				PyOutFile = NULL;
				}
			freemap();
			return NULL;
		}else{
			if (PyOutFile != NULL)
				{
				fclose(PyOutFile);
				PyOutFile = NULL;
				}
			freemap();
			return Py_None;
		}
	}

	da_SetMemFuncs(memNew, memResize, memFree);
	global.progname = "Glyph_Proofer";
	outputfilebase =  "glyphproofer";
	
	fprintf(OUTPUTBUFF, "%s-library v.%s\n\n", global.progname, libversion);
	
	
	while(gcr.endGlyph!=0 && gcr.endGlyph<gcr.numGlyphs)
	{
		int newstart=gcr.endGlyph;
		infilename=infilenameorig;
		
		if (newstart==1) newstart=0;
		
		/*fprintf(stderr, "Starting again with %d of %d\n", gcr.endGlyph, gcr.numGlyphs);*/
		gcr.endGlyph=0;
		
		for (i=0; i<gcr.numFonts; i++)
		{
			gcr.startGlyph=newstart;	
			/*fprintf(OUTPUTBUFF, "Processing %s\n", infilename);*/
			argc = 0;
			argv[argc++]=global.progname;
			if (useStdOut)
			argv[argc++]="-O";
			argv[argc++]="-tCFF_=9";
			argv[argc++]="-d";
			argv[argc++]="-l";
			argv[argc++]=infilename;
			
			argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);
			
			if (opt_hasError())
				{
				if (PyOutFile != NULL)
					{
					fclose(PyOutFile);
					PyOutFile = NULL;
					}
		 		  freemap();
				  return NULL;
				}
			
			fileOpen(infilename);
			
			if (!fileIsOpened())
			{
				warning(SPOT_MSG_BADFILE, infilename);
				fileClose();
				if (PyOutFile != NULL)
					{
					fclose(PyOutFile);
					PyOutFile = NULL;
					}
				freemap();
				return NULL;
			}else{
				/* See if we can recognize the file type */
				value = fileSniff();
				switch (value)
				{
					case bits_:
					case typ1_:		
					case true_:
					case mor0_:
					case OTTO_:
					case VERSION(1,0):
					  sfntRead(0, -1);	/* Read plain sfnt file */
					  sfntDump();
					  sfntFree(1);		  
					  break;
					case ttcf_:
					  sfntTTCRead(0);		/* Read TTC file */
					  break;
					case 256:
					  SEEK_ABS(0); /* Raw resource file as a data fork */ 
					  resRead(0);			/* Read and dump  Macintosh resource file */
					  break;
					default:
						warning(SPOT_MSG_BADFILE, infilename);
						PyErr_SetString(ErrorObject, "Fatal Error: Not a valid font file");
					  	fileClose();
						if (PyOutFile != NULL)
							{
							fclose(PyOutFile);
							PyOutFile = NULL;
							}
						freemap();
						return NULL;
				}
				fileClose();
				
			}
		
			while(*infilename)
				infilename++;
			infilename++;  /*Gets us past the next NULL character*/
		}
	}
	CFF_SynopsisFinish();
	fclose(PyOutFile);
	PyOutFile = NULL;
	freemap();
	return Py_None;
}

PyObject *getVersion(PyObject *self, PyObject *args)
{
	PyObject *pstr = Py_BuildValue("s", libversion);
	return pstr;
}

static void argFree(char **argfree, char** argv)
{
	int i;
	for(i=0; i<MAX_ARGS; i++)
		if (argfree[i])
			memFree(argfree[i]);
		else
			break;
	memFree(argfree);
	memFree(argv);
}

PyObject * main_python(PyObject *self, PyObject *args)
	{
	static double glyphptsize = STDPAGE_GLYPH_PTSIZE;
	static opt_Option opt[] =
		{
		  {"-u", opt_Call, showUsage},
		  {"-h", opt_Call, showHelp},
		  {"-ht", opt_Call, sfntTableSpecificUsage},
		  {"-l", opt_Flag},
		  {"-O", opt_Flag},
		  {"-r", opt_Flag},
		  {"-n", opt_Flag},
		  {"-nc", opt_Flag},
		  {"-T", opt_Flag},
		  {"-F", opt_Flag},
		  {"-f", opt_Flag},
		  {"-G", opt_Flag},
		  {"-V", opt_Flag},
		  {"-m", opt_Flag},
		  {"-d", opt_Flag},
		  {"-br", opt_Flag},
		  {"-i", resIdScan},
		  {"-o", sfntTTCScan},
		  {"-t", sfntTagScan},
		  {"-P", sfntFeatScan}, 
		  {"-A", sfntFeatScan}, 
		  {"-p", proofPolicyScan}, 
		  {"-a", opt_Flag},
		  {"-R", opt_Flag},
		  {"-c", opt_Flag},
		  {"-g", glyfGlyphScan},
		  {"-b", glyfBBoxScan},
		  {"-s", glyfScaleScan},
		  {"-@", opt_Double, &glyphptsize},
		  {"-C", opt_Int, &cmapSelected},
#if AUTOSCRIPT
		  {"-X", opt_String, scriptfilename},
#endif
		  {"-ag", opt_String, &glyphaliasfilename},
		  {"-of", opt_String, &outputfilebase},
		};
	char **argv, **argfree;
	int strIndex, argc, argtotal, tries;
	char * argString, *outfilename;
	IntX files;
	IntN argi;
    Byte8 *filename = NULL;
	Card32 value;
	int i = 0;
#if AUTOSCRIPT
	cmdlinetype *cmdl;
	Byte8 foundXswitch = 0;
#endif
    IntN status;

#ifdef SUNOS
	{
	/* To allow gdb to attach to process */
	char* do_debug_sleep;
	do_debug_sleep = getenv("STOP_OTFPROOF_FOR_GDB");
	while (do_debug_sleep != NULL)
		sleep(1);
	}	
#endif


	/* Resetting globals*/
	gcr.reportNumber=0;
	aliasfromfileinit = 0; 
	
/*	freopen("OTFProof.std.log", "w", stdout);*/
	
	value = setjmp(mark);
	
	if(value==-1){
		PyErr_SetString(ErrorObject, "Fatal Error");
		if (PyOutFile != NULL)
			{
			fclose(PyOutFile);
			PyOutFile = NULL;
			}
		freemap();
		return NULL;
	}
	
	if(!PyArg_ParseTuple(args, "ss", &argString, &outfilename)){
		freemap();
		return NULL;
	}
	
	if (PyOutFile == NULL)
		PyOutFile = fopen(outfilename, "w");
	else
		PyOutFile = freopen(outfilename, "w", PyOutFile);
	
	VORGfound=0;
	strIndex=0;
	argfree= (char **) memNew(MAX_ARGS*sizeof(char *));
	argv=(char **) memNew(MAX_ARGS*sizeof(char *));
	
	for(i=0; i<MAX_ARGS; i++){
		argfree[i]=argv[i]=NextToken(argString, &strIndex);
		if(argv[i]==NULL) {
			argtotal=argc=i;
			break;
		}else{
			/*fprintf(OUTPUTBUFF,  ">>%s<<\n", argv[i]);*/
		}
	}
	  status = setjmp(global.env);
	  if (status)
	  {
#if AUTOSCRIPT
	  	if (global.doingScripting)
	  		{
	  		  goto scriptAbEnd;
	  		}
	  	else
#endif
			if(status==2){
				PyErr_SetString(ErrorObject, "Fatal Error");
				if (PyOutFile != NULL)
					{
					fclose(PyOutFile);
					PyOutFile = NULL;
					}
				freemap();
				argFree(argfree, argv);
				return NULL;
			}else{
				if (PyOutFile != NULL)
					{
					fclose(PyOutFile);
					PyOutFile = NULL;
					}
				freemap();
				argFree(argfree, argv);
				return Py_None;
			}
	  }
	  
	  da_SetMemFuncs(memNew, memResize, memFree);
	  global.progname = "OTFproof";
#if AUTOSCRIPT
	scriptfilename[0]='\0';
	for (i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-X") == 0)
		{
			if ((argv[i+1] != NULL) && (argv[i+1][0] != '\0'))
			{
				strcpy(scriptfilename, argv[i+1]);
				foundXswitch = 1;
			}
			break;
		}
	}
#endif 
	proofResetPolicies();
	argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);
	
	if (opt_hasError())
		{
		if (PyOutFile != NULL)
			{
			fclose(PyOutFile);
			PyOutFile = NULL;
			}
 		  freemap();
		  argFree(argfree, argv);
		  return Py_None;
		}
		
	 if (opt_Present("-@"))
		proofSetGlyphSize(glyphptsize);

	  if (opt_Present("-V"))  /* equivalent to "-p6" */
		proofSetPolicy(6, 1); 

	  files = argc - argi;

	  for (; argi < argc; argi++)
		{
		  filename = argv[argi];
		  
			fileOpen(filename);
			if (!fileIsOpened())
		  {
		  	warning(SPOT_MSG_BADFILE, filename);
			fileClose();
			continue;
		  }
/*			fprintf(OUTPUTBUFF, "\nProofing %s.\n", filename);			*/
		  tries = 0;
		  /* See if we can recognize the file type */
		  value = fileSniff();	
		  switch (value)
			{
			case bits_:
			case typ1_:		
			case true_:
			case mor0_:
			case OTTO_:
			case VERSION(1,0):
			  sfntRead(0, -1);	/* Read plain sfnt file */
			  sfntDump();
			  sfntFree(1);		  
			  break;
			case ttcf_:
			  sfntTTCRead(0);		/* Read TTC file */
			  continue;
			  break;
			case 256:
			  SEEK_ABS(0); /* Raw resource file as a data fork */ 
			  resRead(0);			/* Read and dump  Macintosh resource file */
			  continue;
			  break;
			case sig_AppleSingle:
			case sig_AppleDouble:
				doASDFormats((ctlTag)value);
				break;
			default:
				warning(SPOT_MSG_BADFILE, filename);
			  	fileClose();
			  continue;
			}

		  fileClose();
		  freemap();
		  argFree(argfree, argv);
#if MEMCHECK
		  memReport();
#endif
		fclose(PyOutFile);
		PyOutFile = NULL;
		  return Py_None;
		}
#if AUTOSCRIPT	
execscript:
			{
				char * end;
				
				end=strrchr(scriptfilename, '\\');
				if(end==NULL)
					sourcepath="";
				else{
					char *scurr = scriptfilename;
					char *dcurr;
					
					sourcepath=(char *)memNew(strlen(scriptfilename));
					dcurr = sourcepath;
					while(scurr!=end)
					{
						*dcurr++=*scurr++;
					}		
					*dcurr=0;
				}
			
			}

	  for (i = 0; i < script.cmdline.cnt ; i++) 
	  {
		char * tempfilename;
		
		cmdl = da_INDEX(script.cmdline, i);
		if (cmdl->args.cnt < 2) continue;

		proofResetPolicies();
		
		{
			IntX a;
			
			inform(SPOT_MSG_EOLN);
			message(SPOT_MSG_ECHOSCRIPTCMD);
			for (a = 1; a < cmdl->args.cnt; a++)
			{
				inform(SPOT_MSG_RAWSTRING, cmdl->args.array[a]);
			}
			inform(SPOT_MSG_EOLN);
		}
		
		argi = opt_Scan(cmdl->args.cnt, cmdl->args.array, opt_NOPTS(opt), opt, NULL, NULL);		

		if (opt_hasError())
			{
			if (PyOutFile != NULL)
				{
				fclose(PyOutFile);
				PyOutFile = NULL;
				}
			freemap();
			argFree(argfree, argv);
			return Py_None;
			}

		if (opt_Present("-@"))
			proofSetGlyphSize(glyphptsize);
	  	if (opt_Present("-V"))  /* equivalent to "-p6" */
			proofSetPolicy(6, 1); 

		tempfilename = MakeFullPath(cmdl->args.array[cmdl->args.cnt-1]);
		
		
		if (fileExists(tempfilename) )
		  { 						/* (new) font filename on cmdline */
			memFree(tempfilename);
			if (filename != NULL) /* not first time */
			{
			  fileClose(); /* previous font file */
	  		  sfntFree(1);
		  	}
			if(sourcepath[0]!='\0')
				filename=MakeFullPath(cmdl->args.array[cmdl->args.cnt-1]);
			else
				filename = cmdl->args.array[cmdl->args.cnt-1];
			fileOpen(filename);
			  tries=0;
retry:
			/* See if we can recognize the file type */
			value = fileSniff();	
			switch (value)
			  {
			  case bits_:
			  case typ1_:		
			  case true_:
			  case mor0_:
			  case OTTO_:
			  case VERSION(1,0):
				sfntRead(0, -1);	/* Read plain sfnt file */
				break;
			  case ttcf_:
				sfntTTCRead(0);		/* Read TTC file */
				continue;
				break;
			  case 256:
				resRead(0);			/* Read Macintosh resource file */
				continue;
				break;
			  default:	 
 				warning(SPOT_MSG_BADFILE, filename);
				fileClose();
				continue;
			  }
		  }
		else
		{
		  /* none specified */
		  fatal(SPOT_MSG_MISSINGFILENAME);
		  memFree(tempfilename);
		  continue;
		}
		
		sfntDump();
		
scriptAbEnd:
		sfntFree(1);
	    fileClose();
	  }
	  global.doingScripting = 0;
#endif	

	/*fprintf(stderr, "\nDone.\n");*/
	
	fileClose();	
	freemap();
	argFree(argfree, argv);
#if MEMCHECK
	memReport();
#endif
	fclose(PyOutFile);
	PyOutFile = NULL;
	return Py_None;
}

static struct PyMethodDef otfprooflib_methods[] = {
	{"otfproof_run",  main_python, 1},
	{"otfproof_complement",  proof_complement, 1},
	{"otfproof_version", getVersion, 1},
	{NULL, NULL}
};


/* Initialization function for Python*/
#if FDK_DEBUG
void initotfprooflibDB(void)
#define PROGRAM_NAME "otfprooflibDB"
#else
void initotfprooflib(void)
#define PROGRAM_NAME "otfprooflib"
#endif
{
	PyObject *m ,*d;

	m = Py_InitModule(PROGRAM_NAME, otfprooflib_methods);
	d = PyModule_GetDict(m);
	ErrorObject = Py_BuildValue("s", PROGRAM_NAME".error");
	PyDict_SetItemString(d, "error", ErrorObject);
	
	if(PyErr_Occurred())
		Py_FatalError("can't initialize "PROGRAM_NAME".");
}


#endif     /* EXECUTABLE*/

/* Main program */
IntN main(IntN argc, Byte8 *argv[])
	{
	   IntX value = 0;
	  static double glyphptsize = STDPAGE_GLYPH_PTSIZE;
	  static opt_Option opt[] =
		{
		  {"-u", opt_Call, (void*)showUsage},
		  {"-h", opt_Call, (void*)showHelp},
		  {"-ht", opt_Call, (void*)sfntTableSpecificUsage},
#if AUTOSPOOL
		  {"-l", opt_Flag},
		  {"-O", opt_Flag},
#endif
		  {"-r", opt_Flag},
		  {"-n", opt_Flag},
		  {"-nc", opt_Flag},
		  {"-ngid", opt_Flag},
		  {"-T", opt_Flag},
		  {"-F", opt_Flag},
		  {"-f", opt_Flag},
		  {"-G", opt_Flag},
		  {"-V", opt_Flag},
		  {"-m", opt_Flag},
		  {"-d", opt_Flag},
		  {"-br", opt_Flag},
		  {"-i", resIdScan},
		  {"-o", sfntTTCScan},
		  {"-t", sfntTagScan},
		  {"-P", sfntFeatScan}, 
		  {"-A", sfntFeatScan}, 
		  {"-p", proofPolicyScan}, 
		  {"-a", opt_Flag},
		  {"-R", opt_Flag},
		  {"-c", opt_Flag},
		  {"-g", glyfGlyphScan},
		  {"-b", glyfBBoxScan},
		  {"-s", glyfScaleScan},
		  {"-@", opt_Double, &glyphptsize},
		  {"-C", opt_Int, &cmapSelected},
#if AUTOSCRIPT
		  {"-X", opt_String, scriptfilename},
#endif
		  {"-ag", opt_String, &glyphaliasfilename},
		  {"-of", opt_String, &outputfilebase},
		};

	  IntX files, goodFileCount=0;
	  IntN argi;
      Byte8 *filename = NULL;
	  volatile IntX i = 0;
#if AUTOSCRIPT
	  cmdlinetype *cmdl;
	  Byte8 foundXswitch = 0;
#endif
	  int status = 0; /* = setjmp(global.env); only ued when compiled as lib */

	  if (status)
	  {
#if AUTOSCRIPT
	  	if (global.doingScripting)
	  		{
	  		  goto scriptAbEnd;
	  		}
	  	else
#endif
			exit(status - 1);	/* Finish processing */
	  }
	  gcr.reportNumber=0;
	 /*  value = setjmp(mark); only used when comiled as lib */

         if (value==-1)
                 exit(1);

	  da_SetMemFuncs(memNew, memResize, memFree);
	  global.progname = "spot";

#if AUTOSCRIPT
	scriptfilename[0] = '\0'; /* init */

	if (!foundXswitch && (argc < 2)) /* if no -X on cmdline, and no OTHER switches */
	  strcpy(scriptfilename, "spot.scr");

/* see if scriptfile exists to Auto-execute */
	if ((scriptfilename[0] != '\0') && sysFileExists(scriptfilename))
		{
			global.doingScripting = 1;
			makeArgs(scriptfilename);
		}
#endif /* AUTOSCRIPT */

	  if (
#if AUTOSCRIPT
		  !global.doingScripting
#else
		  1
#endif
		  )
		{
	  	argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);
		if (opt_hasError())
			{
			exit(1);
			}
	  
	  if (argi == 0 )
		showUsage();

#if AUTOSCRIPT
	if (!global.doingScripting && opt_Present("-X"))
	{
		if (scriptfilename && scriptfilename[0] != '\0')
		{
		global.doingScripting = 1;
		makeArgs(scriptfilename);
		goto execscript;
		}
	}
#endif


	  if (opt_Present("-@"))
		proofSetGlyphSize(glyphptsize);

	  if (opt_Present("-V"))  /* equivalent to "-p6" */
		proofSetPolicy(6, 1); 

	  if (opt_Present("-ngid")) 
		global.flags |= SUPPRESS_GID_IN_NAME;

	  files = argc - argi;
      if ((files == 0) && (argc > 1)) /* no files on commandline, but other switches */
		{
		}

	  for (; argi < argc; argi++)
		{
		  filename = argv[argi];
		  
		  if (files > 1)
		  	{
			fprintf(stderr, "Proofing %s.\n", filename);
			fflush(stderr);
			}

		  if (outputfilebase== NULL)
		 	outputfilebase = filename;
		  fileOpen(filename);		  
		  
		  if (!fileIsOpened())
		  {
		  	warning(SPOT_MSG_BADFILE, filename);
			fileClose();
			continue;
		  }
		  if (readFile(filename))
			{
			  fileClose();
			  continue;
			}

		  goodFileCount++;
		  fileClose();
		}
		
	}
#if AUTOSCRIPT
	else /* executing cmdlines from a script file */
	{
execscript:
			{
				char * end;
				
			end=strrchr(scriptfilename, '\\');
				if(end==NULL)
					sourcepath="";
				else{
					char *scurr = scriptfilename;
					char *dcurr;
					
					sourcepath=(char *)memNew(strlen(scriptfilename));
					dcurr = sourcepath;
					while(scurr!=end)
					{
						*dcurr++=*scurr++;
					}		
					*dcurr=0;
				}
			
			}

	  for (i = 0; i < script.cmdline.cnt ; i++) 
	  {
		char * tempfilename;
		
		cmdl = da_INDEX(script.cmdline, i);
		if (cmdl->args.cnt < 2) continue;

		proofResetPolicies();
		
		{
			IntX a;
			
			inform(SPOT_MSG_EOLN);
			message(SPOT_MSG_ECHOSCRIPTCMD);
			for (a = 1; a < cmdl->args.cnt; a++)
			{
				inform(SPOT_MSG_RAWSTRING, cmdl->args.array[a]);
			}
			inform(SPOT_MSG_EOLN);
		}
		
		argi = opt_Scan(cmdl->args.cnt, cmdl->args.array, opt_NOPTS(opt), opt, NULL, NULL);		
		if (opt_hasError())
			{
			exit(1);
			}

		if (opt_Present("-@"))
			proofSetGlyphSize(glyphptsize);
	  	if (opt_Present("-V"))  /* equivalent to "-p6" */
			proofSetPolicy(6, 1); 

		tempfilename = MakeFullPath(cmdl->args.array[cmdl->args.cnt-1]);
		
		
		if (fileExists(tempfilename) )
		  { 						/* (new) font filename on cmdline */
			memFree(tempfilename);
			if (filename != NULL) /* not first time */
			{
			  fileClose(); /* previous font file */
	  		  sfntFree(1);
		  	}
			if(sourcepath[0]!='\0')
				filename=MakeFullPath(cmdl->args.array[cmdl->args.cnt-1]);
			else
				filename = cmdl->args.array[cmdl->args.cnt-1];
			fileOpen(filename);
			if (outputfilebase == NULL)
		 		outputfilebase = filename;
			fprintf(stderr, "Proofing %s.\n", filename);
			fflush(stderr);
		    goodFileCount++;
		
		  if (readFile(filename))
			{
			  goodFileCount--;
			  fileClose();
			  continue;
			}
			
		  }
		else
		{
		  /* none specified */
		  fatal(SPOT_MSG_MISSINGFILENAME);
		  memFree(tempfilename);
		  continue;
		}
		
		sfntDump();
		
scriptAbEnd:
		sfntFree(1);
	    fileClose();
	  }
	  global.doingScripting = 0;
	}
#endif /* AUTOSCRIPT */

/*	fprintf(stderr, "\nDone.\n");*/
	if(goodFileCount<=0)
		exit(1);

	quit(0);
	return 0;		
}

