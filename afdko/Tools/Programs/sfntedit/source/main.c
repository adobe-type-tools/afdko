/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/
/*
 * sfnt table edit utility. 
 */
#ifdef __MWERKS__
#include <SIOUX.h>
#include <console.h>
#include <unix.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include "Eglobal.h"
#include "Efile.h"
#include "Esys.h"
#include "otter.h"
#include "otftableeditor.h"
#include "setjmp.h"

#define MAX_ARGS 200

jmp_buf mark;


#if SUNOS
/* Fixup SUNOS libc */
#include <sys/unistd.h>		/* For SEEK_* macros */

#ifndef FILENAME_MAX
/* SunOS doesn't define this ANSI macro anywhere therefore derive it */
#include <sys/param.h>
#define	FILENAME_MAX	MAXPATHLEN
#endif /* FILENAME_MAX */

#endif /* SUNOS */

#define VERSION "1.4"

/* Data type sizes (bytes) */
#define uint16_	2
#define int16_	2
#define uint32_	4
#define	int32_	4

/* Tag support */
typedef Card32 Tag;
#define TAG(a,b,c,d) ((Tag)(a)<<24|(Tag)(b)<<16|(c)<<8|(d))
#define TAG_ARG(t) (char)((t)>>24&0xff),(char)((t)>>16&0xff), \
	(char)((t)>>8&0xff),(char)((t)&0xff)

typedef struct			/* sfnt table directory entry */
	{
	Card32 tag;
	Card32 checksum;
	Card32 offset;
	Card32 length;
	Card16 flags;		/* Option flags */
#define TBL_SRC	(1<<10)	/* Flags table in source sfnt */
#define TBL_DST	(1<<11)	/* Flags table in destination sfnt */
	Int16 order;		/* Table ordering */
	char *xfilename;	/* Extract filename */
	char *afilename;	/* Add filename */
	} Table;
#define ENTRY_SIZE (uint32_*4)	/* Size of written fields */

#define MAX_TABLES 60
typedef struct			/* sfnt header */
	{
	Fixed version;
	Card16 numTables;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	Table directory[MAX_TABLES];	/* [numTables] */
	} sfntHdr;
#define DIR_HDR_SIZE (int32_+uint16_*4)	/* Size of written fields */

/* head.checkSumAdjustment offset within head table */
#define HEAD_ADJUST_OFFSET	(2*int32_)

/* Recommended table data order for OTF fonts */
static Tag otfOrder[] =
	{
	TAG('h','e','a','d'),
	TAG('h','h','e','a'),
	TAG('m','a','x','p'),
	TAG('O','S','/','2'),
	TAG('n','a','m','e'),
	TAG('c','m','a','p'),
	TAG('p','o','s','t'),
	TAG('f','v','a','r'),
	TAG('M','M','S','D'),
	TAG('C','F','F',' '),
	0,					/* Sentinel */
	};

/* Recommended table data order for TTF fonts */
static Tag ttfOrder[] =
	{
	TAG('h','e','a','d'),
	TAG('h','h','e','a'),
	TAG('m','a','x','p'),
	TAG('O','S','/','2'),
	TAG('h','m','t','x'),
	TAG('L','T','S','H'),
	TAG('V','D','M','X'),
	TAG('h','d','m','x'),
	TAG('c','m','a','p'),
	TAG('f','p','g','m'),
	TAG('p','r','e','p'),
	TAG('c','v','t',' '),
	TAG('l','o','c','a'),
	TAG('g','l','y','f'),
	TAG('k','e','r','n'),
	TAG('n','a','m','e'),
	TAG('p','o','s','t'),
	TAG('g','a','s','p'),
	TAG('P','C','L','T'),
	TAG('D','S','I','G'),
	0,					/* Sentinel */
	};


static sfntHdr sfnt;
static File srcfile;
static File dstfile;
char *tmpname = "sfntedit.tmp";	/* Temporary filename */
#define BACKUPNAME  "sfntedit.BAK"

static long options;	/* Options seen */
#define OPT_EXTRACT	(1<<0)
#define OPT_DELETE	(1<<1)
#define OPT_ADD		(1<<2)
#define OPT_LIST	(1<<3)
#define OPT_CHECK	(1<<4)
#define OPT_FIX		(1<<5)

volatile int doingScripting = 0;
int foundXswitch = 0;

static char scriptfilename[256];
char *sourcepath="";

typedef struct _cmdlinetype
{
  da_DCL(char *, args); /* arg list */  
} cmdlinetype;

static struct
{
  char *buf; /* input buffer */
  da_DCL(cmdlinetype, cmdline);
} script;


char * MakeFullPath(char *source)
{
		char * dest;
		
		dest = (char *) malloc(256);
	if(sourcepath[0]=='\0' || strchr(source, '\\')!=NULL)
		sprintf(dest, "%s", source);
	else
		sprintf(dest, "%s\\%s", sourcepath, source);
		return dest;
}

/* ----------------------------- Error Handling ---------------------------- */

/* Cleanup on abnormal program termination */
static void cleanup(int code)
	{
	
	fileClose(&srcfile);
	if (dstfile.fp != NULL)
		{
		fileClose(&dstfile);
		(void)remove(dstfile.name);
		}
	fclose(stderr);

	longjmp(mark, 1);

	}


/* ----------------------------- Usage and Help ---------------------------- */

/* Print usage information */
static void printUsage(void)
	{
fprintf(stderr, 
"Usage:\n"
"    %s [options] <srcfile> [<dstfile>]\n"
"OR: %s  -X <scriptfile>\n\n"
"Options:\n"
"    -x <tag>[=<file>][,<tag>[=<file>]]+ extract table to file\n"
"    -d <tag>[,<tag>]+ delete table\n"
"    -a <tag>=<file>[,<tag>=<file>]+ add (or replace) table\n"
"    -l list sfnt directory (default)\n"
"    -c check checksums\n"
"    -f fix checksums (implies -c)\n"
"    -u print usage\n"
"    -h print help\n"
"    -X execute command-lines from <scriptfile> [default: sfntedit.scr]\n"
"Build:\n"
"    Version: %s\n", 
	 global.progname,	 
	 global.progname,
	  VERSION);
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
	fprintf(stderr, 
"Notes:\n"
"    This program supports table-editing, listing, and checksumming options\n"
"on sfnt-formatted files such as OpenType Format (OTF) or TrueType. The\n"
"mandatory source file is specified as an argument to the program. An\n"
"optional destination file may also be specified which receives the edited\n"
"data otherwise the source data is edited in-place thus modifying the source\n"
);
fprintf(stderr,
"file. In-place editing is achieved by the use of a temporary file called\n"
"%s that is created in the directory of execution (requiring you\n"
"to have write permission to that directory).\n"
"    The target table of an editing option (-x, -d, and -a) is specified\n"
"with a table tag argument that is nominally 4 characters long. If fewer\n"
"than 4 characters are specified the tag is padded with spaces (more than 4\n"
, tmpname
);
fprintf(stderr,
"characters is a fatal error). Multiple tables may be specified as a single\n"
"argument composed from a comma-separated list of tags.\n"
"    The extract option (-x) copies the table data into a file whose default\n"
"name is the concatenation of the source filename (less its .otf or .ttf\n"
"extension), a period character (.), and the table tag. If the tag contains\n"
"non-alphnumeric characters they are replaced by underscore characters (_)\n"
);
fprintf(stderr, 
"and finally trailing underscores are removed. The default filename may be\n"
"overridden by appending an equals character (=) followed by an alternate\n"
"filename to the table tag argument. The delete option (-d) deletes a table.\n"
"Unlike the -x option no files may be specified in the table tag list. The\n"
"add option (-a) adds a table or replaces one if the table already exists.\n"
"The source file containing the table data is specified by appending an\n"
);
fprintf(stderr,
"equals character (=) followed by a filename to the table tag.\n"
"    The 3 editing options may be specified together as acting on the same\n"
"table. In such cases the -x option is applied before the -d option which is\n"
"applied before the -a option. (The -d option applied to the same table as a\n"
"subsequent -a option is permitted but redundant.) The -d and -a options\n"
"change the contents of the sfnt and cause the table checksums and the head\n"
);
fprintf(stderr,
"table's checksum adjustment field to be recomputed.\n"
"    The list option (-l) simply lists the contents of the sfnt table\n"
"directory. This is the default action if no other options are specified.\n"
"The check checksum option (-c) performs a check of all the table checksums\n"
"and the head table's checksum adjustment field and reports any errors. The\n"
"fix checksum option (-f) fixes any checksum errors.\n"
);
fprintf(stderr,
"    The -d, -a, and -f options create a new sfnt file by copying tables\n"
"from the source file to the destination file. The tables are copied in the\n"
"order recommended in the OpenType specification. A side effect of copying\n"
"is that all table information including checksums and sfnt search fields\n"
"is recalculated.\n"
"Examples:\n"
);
fprintf(stderr,
"o Extract GPOS and GSUB tables to files minion.GPOS and minion.GSUB.\n"
"    sfntedit -x GPOS,GSUB minion.otf\n"
"o Add tables extracted previously to different font.\n"
"    sfntedit -a GPOS=minion.GPOS,GSUB=minion.GSUB minion.ttf\n"
"o Delete temporary tables from font.\n"
"    sfntedit -d TR01,TR02,TR03 pala.ttf\n"
"o Copy font to new file fixing checksums and reordering tables.\n"
"    sfntedit -f helv.ttf newhelv.ttf\n"
);
	quit(0); 
	}
	
static void makeArgs(char *filename)
      {
      int state;
      long i;
      long length;
      File file;
      char *start = NULL;     /* Suppress optimizer warning */
      cmdlinetype *cmdl;

      /* Read whole file into buffer */
      fileOpenRead(filename, &file);
      length = fileLength(&file);
      if (length < 1) 
      	fatal(SFED_MSG_BADSCRIPTFILE, filename);
      script.buf = memNew(length + 2);

      fileSeek(&file, 0, 0);
      fileReadN(&file, length, script.buf);
      fileClose(&file);

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

/* ---------------------------- Argument Parsing --------------------------- */

/* Find table with given tag and return its index if found otherwise return
   index of insert position */
static int getTableIndex(Tag tag)
	{
	int i;
	for (i = 0; i < sfnt.numTables; i++)
		if (tag <= sfnt.directory[i].tag)
			break;
	return i;
	}

/* Insert table into sfnt directory in tag order */
static Table *insertTable(Tag tag)
	{
	int index = getTableIndex(tag);
	Table *tbl = &sfnt.directory[index];

	if (tbl->tag != tag)
		{
		/* Insert table */
		int i;

		if (++sfnt.numTables > MAX_TABLES)
			fatal(SFED_MSG_TABLELIMIT, MAX_TABLES);
		
		for (i = sfnt.numTables - 2; i >= index; i--)
			sfnt.directory[i + 1] = sfnt.directory[i];

		tbl->flags = 0;
		tbl->xfilename = NULL;
		tbl->afilename = NULL;
		}

	return tbl;
	}

/* Parse table tag list */
static void parseTagList(char *arg, int option, int flag)
	{
	char *p = arg;
	for (p = strtok(arg, ","); p != NULL; p = strtok(NULL, ","))
		{
		int i;
		int taglen;
		Tag tag;
		char *filename;
		Table *tbl;

		/* Find filename separator and set to nul if present */
		filename = strchr(p, '=');
		if (filename != NULL)
			{
			*filename++ = '\0';
			if (strlen(filename) == 0)
				fatal(SFED_MSG_BADFNAMOPT, option);
			}

		/* Validate tag length */
		taglen = strlen(p);
		if (taglen == 0 || taglen > 4)
			fatal(SFED_MSG_BADTAGOPT, p, option);

		/* Make tag */
		tag = 0;
		for (i = 0; i < taglen; i++)
			tag = tag<<8 | *p++;

		/* Pad tag with space */
		for (; i < 4; i++)
			tag = tag<<8 | ' ';

		/* Add option */
		if (option == 'd' && filename != NULL)
			{
			warning(SFED_MSG_FILENIGNORED);
			filename = NULL;
			}
		if (option == 'a' && filename == NULL)
			fatal(SFED_MSG_FILENMISSING);

		tbl = insertTable(tag);
			
		/* Check tag not already in list */
		if (tbl->flags & flag)
			warning(SFED_MSG_DUPTAGOPT, TAG_ARG(tag), option);

		/* Save values */
		tbl->tag 	= tag;
		tbl->flags 	|= flag;
		if (option == 'x')
            {
            if(sourcepath[0]!='\0'  && filename!=NULL)
				tbl->xfilename = MakeFullPath(filename);
			else
				tbl->xfilename = filename;
            }
        else if (option == 'a')
            {
            if(sourcepath[0]!='\0'  && filename!=NULL)
				tbl->afilename = MakeFullPath(filename);
			else
				tbl->afilename = filename;
            }
        }
	}

/* Count bits in word */
static int countbits(long value)
	{
	int count;
	for (count = 0; value; count++)
		value &= value - 1;
	return count;
	}

/* Process options */
static int parseArgs(int argc, char *argv[])
	{
	int i;

	options = 0; /* reset options */
	
	for (i = 0; i < argc; i++)
		{
		char *arg = argv[i];
		int argsleft = argc - i - 1;
		switch (arg[0])
			{
		case '-':
			switch (arg[1])
				{
			case 'X': /* script file to execute */
				foundXswitch = 1;
				if ((argsleft > 0) && argv[i+1][0] != '-')
					{
					   strcpy(scriptfilename, argv[++i]);
					   if (doingScripting) /* disallow nesting */
					   {
					   	 foundXswitch = 0;
					   	 scriptfilename[0] = '\0'; 
					   }
					 }

				break;
			case 'x':		/* Extract table */
				if (arg[2] != '\0')
					fatal(SFED_MSG_BADOPTION, arg);
				else if (argsleft == 0)
					fatal(SFED_MSG_NOARG, arg[1]);
				parseTagList(argv[++i], 'x', OPT_EXTRACT);
				options |= OPT_EXTRACT;
				break;
			case 'd':		/* Delete table */
				if (arg[2] != '\0')
					fatal(SFED_MSG_BADOPTION, arg);
				else if (argsleft == 0)
					fatal(SFED_MSG_NOARG, arg[1]);
				parseTagList(argv[++i], 'd', OPT_DELETE);
				options |= OPT_DELETE;
				break;
			case 'a':		/* Add table */
				if (arg[2] != '\0')
					fatal(SFED_MSG_BADOPTION, arg);
				else if (argsleft == 0)
					fatal(SFED_MSG_NOARG, arg[1]);
				parseTagList(argv[++i], 'a', OPT_ADD);
				options |= OPT_ADD;
				break;
			case 'l':		/* List sfnt table directory */
				options |= OPT_LIST;
				break;
			case 'c':		/* Check checksum */
				options |= OPT_CHECK;
				break;
			case 'f':		/* Fix checksum */
				options |= OPT_FIX;
				break;
			case 'u':
				showUsage();
				break;
			case 'h':
				showHelp();
				break;
			default:
				fatal(SFED_MSG_UNRECOGOPT, arg);
				}
			break;
		default: /* Non-option arg is taken to be filename */
			{
			int writefile = options & (OPT_DELETE|OPT_ADD|OPT_FIX);
			
			/* Validate options */
			if (options & (OPT_LIST|OPT_CHECK|OPT_FIX) &&
				countbits(options) > 1)
				fatal(SFED_MSG_OPTCONFLICT);

			if (options == 0)
				options |= OPT_LIST;	/* No options set; apply default */

			/* Set up filenames */
			if(sourcepath[0]!='\0'){
				srcfile.name = MakeFullPath(arg);
			}else{
				srcfile.name=arg;
			}
			
			
			if (argsleft == 0)
				{
				if (writefile)
					{ /* Use temporary file; but check doesn't exist first */
						char * fulltemp;
						
						fulltemp=MakeFullPath(tmpname);
		 				/* rkr 5/11/2006 - I see no need for this.
						if (fileExists(fulltemp))
							fatal(SFED_MSG_TEMPFEXISTS, tmpname);
		 				*/

						dstfile.name = fulltemp;
					}
				}
			else if (argsleft == 1)
				{
				if (writefile){
					if(sourcepath[0]!='\0'){
						dstfile.name = MakeFullPath(argv[i + 1]);
					}else{
						dstfile.name=argv[i + 1];
					}
				
				}else
					warning(SFED_MSG_OUTFIGNORED);
				}
			else
				fatal(SFED_MSG_TOOMANYFILES);
			}
			return (argsleft+1);
			}
		}
		
	/* No files provided */
	if ((options & (OPT_DELETE|OPT_ADD|OPT_FIX)) &&
		(dstfile.name == NULL))
		{
		 /* rkr 5/11/2006 - I see no need for this.
		 if (fileExists(tmpname))
			fatal(SFED_MSG_TEMPFEXISTS, tmpname);
		 */
		 dstfile.name = tmpname;
		}
	return 0;
	}

/* ------------------------- sfnt Table Processing ------------------------- */

/* Read sfnt header */
static void sfntReadHdr(void)
	{
	int i;
	Int16 numTables=0;

	/* Read and validate version */
	fileReadObject(&srcfile, 4, &sfnt.version);
	switch (sfnt.version)
		{
	case 0x00010000:	/* 1.0 */
	case TAG('t','r','u','e'):
	case TAG('t','y','p','1'):
	case TAG('O','T','T','O'):
	case TAG('b','i','t','s'):
		break;
	case TAG('t','t','c','f'):
	default:
		fileClose(&srcfile);
		fatal(SFED_MSG_UNRECFILE, srcfile.name);
		}

	/* Read rest of header */
	fileReadObject(&srcfile, 2, &numTables);
	fileReadObject(&srcfile, 2, &sfnt.searchRange);
	fileReadObject(&srcfile, 2, &sfnt.entrySelector);
	fileReadObject(&srcfile, 2, &sfnt.rangeShift);
	for (i = 0; i < numTables; i++)
		{
		Tag tag;  
		Table *tbl;
		fileReadObject(&srcfile, sizeof(Tag), &tag);
		tbl = insertTable(tag);
		tbl->tag		= tag;
		fileReadObject(&srcfile, 4, &tbl->checksum);
		fileReadObject(&srcfile, 4, &tbl->offset);
		fileReadObject(&srcfile, 4, &tbl->length);
		tbl->flags		|= TBL_SRC;
		}

	/* Check options have corresponding tables */
	for (i = 0; i < sfnt.numTables; i++)
		{
		Table *tbl = &sfnt.directory[i];
		
		if (tbl->flags & (OPT_EXTRACT|OPT_DELETE) && !(tbl->flags & TBL_SRC))
			fatal(SFED_MSG_TABLEMISSING, TAG_ARG(tbl->tag));
		}
	}

/* Dump sfnt header */
static void sfntDumpHdr(void)
	{
	int i;

	fprintf(stderr,"--- sfnt header [%s]\n", srcfile.name);
	if (sfnt.version == 0x00010000)
		fprintf(stderr,"version      =1.0 (00010000)\n");
	else
		fprintf(stderr,"version      =%c%c%c%c (%08x)\n", 
			   TAG_ARG(sfnt.version), sfnt.version);
	fprintf(stderr,"numTables    =%hu\n", sfnt.numTables);
	fprintf(stderr,"searchRange  =%hu\n", sfnt.searchRange);
	fprintf(stderr,"entrySelector=%hu\n", sfnt.entrySelector);
	fprintf(stderr,"rangeShift   =%hu\n", sfnt.rangeShift);

	fprintf(stderr,"--- table directory [index]={tag,checksum,offset,length}\n");
	for (i = 0; i < sfnt.numTables; i++)
		{
		Table *tbl = &sfnt.directory[i];
		fprintf(stderr,"[%2d]={%c%c%c%c,%08x,%08x,%08x}\n", i, 
			   TAG_ARG(tbl->tag), tbl->checksum, tbl->offset, tbl->length);
		}
	}

/* Calculate values of binary search table parameters */
static void calcSearchParams(unsigned nUnits, 
					  Card16 *searchRange,
					  Card16 *entrySelector,
					  Card16 *rangeShift)
	{
	unsigned log2;
	unsigned pwr2;

	pwr2 = 2;
	for (log2 = 0; pwr2 <= nUnits; log2++)
		pwr2 *= 2;
	pwr2 /= 2;

	*searchRange = ENTRY_SIZE * pwr2;
	*entrySelector = log2;
	*rangeShift = ENTRY_SIZE * (nUnits - pwr2);
	}

/* Check that the table checksums and the head adjustment checksums are
   calculated correctly. Also validate the sfnt search fields */
static void checkChecksums(void)
	{
	int i;
	long nLongs;
	int fail = 0;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	Card32 checkSumAdjustment;
	Card32 totalsum = 0;

	/* Validate sfnt search fields */
	calcSearchParams(sfnt.numTables, &searchRange, &entrySelector,&rangeShift);
	if (sfnt.searchRange != searchRange)
		{
		warning(SFED_MSG_BADSEARCH,  sfnt.searchRange, searchRange);
		fail = 1;
		}
	if (sfnt.entrySelector != entrySelector)
		{
		warning(SFED_MSG_BADSELECT, sfnt.entrySelector, entrySelector);
		fail = 1;
		}
	if (sfnt.rangeShift != rangeShift)
		{
		warning(SFED_MSG_BADRSHIFT, sfnt.rangeShift, rangeShift);
		fail = 1;
		}

	/* Read directory header */
	fileSeek(&srcfile, 0, SEEK_SET);
	nLongs = (DIR_HDR_SIZE + ENTRY_SIZE * sfnt.numTables) / 4;
	while (nLongs--)
		{
		Card32 amt;
		fileReadObject(&srcfile, 4, &amt);
		totalsum += amt; 
		}

	for (i = 0; i < sfnt.numTables; i++)
		{
		Card32 checksum = 0;
		Card32 amt;

		Table *entry = &sfnt.directory[i];

		fileSeek(&srcfile, entry->offset, SEEK_SET);

		nLongs = (entry->length + 3) / 4;
		while (nLongs--)
			{
			fileReadObject(&srcfile, 4, &amt);
			checksum += amt;
			}

		if (entry->tag == TAG('h','e','a','d'))
			{
			/* Adjust sum to ignore head.checkSumAdjustment field */
			fileSeek(&srcfile, entry->offset + HEAD_ADJUST_OFFSET, SEEK_SET);
			fileReadObject(&srcfile, 4, &checkSumAdjustment);
			checksum -= checkSumAdjustment;
			}

		if (entry->checksum != checksum)
			{
			warning(SFED_MSG_BADCHECKSUM,TAG_ARG(entry->tag), entry->checksum, checksum);
			fail = 1;
			}

		totalsum += checksum;
		}

	totalsum = 0xb1b0afba - totalsum; 
	if (!fail && totalsum != checkSumAdjustment)
		{
		warning(SFED_MSG_BADCKSUMADJ, checkSumAdjustment, totalsum);
		fail = 1;
		}
	message(fail ? SFED_MSG_CHECKFAILED : SFED_MSG_CHECKPASSED, srcfile.name);
	}

/* Return tail component of path */
static char *tail(char *path)
	{
	char *p;
	p = strrchr(path, '/');
	if (p== NULL)
		p = strrchr(path, '\\');
	p = strrchr(path, '\\');
	return (p == NULL)? path: p + 1;
	}

/* Make extract filename from option filename or src filename plus table tag */
static char *makexFilename(Table *tbl)
	{
	if (tbl->xfilename != NULL)
		return tbl->xfilename;
	else
		{
		static char filename[FILENAME_MAX];
		int i;
		char *p;
		char *q;
		char tag[5];

		tag[0] = (char)(tbl->tag>>24);
		tag[1] = (char)(tbl->tag>>16);
		tag[2] = (char)(tbl->tag>>8);
		tag[3] = (char)(tbl->tag);
		tag[4] = '\0';

		/* Replace potentially troublesome filename chars by underscores */
		for (i = 0; i < 4; i++)
			if (!isalnum(tag[i]))
				tag[i] = '_';

		/* Trim trailing underscores */
		for (i = 3; i > 0; i--)
			if (tag[i] == '_')
				tag[i] = '\0';
			else
				break;

		p = tail(srcfile.name);
		q = strstr(p, ".otf");
		if (q == NULL)
			q = strstr(p, ".ttf");
		
		if (q == NULL)
			sprintf(filename, "%s.%s", p, tag);
		else
			sprintf(filename, "%.*s.%s", (int)(q - p), p, tag);

		return filename;
		}
	}

/* Extract (-x) tables from source file */
static void extractTables(void)
	{
	int i;

	for (i = 0; i < sfnt.numTables; i++)
		{
		File file;
		char *filename;
		Table *tbl = &sfnt.directory[i];

		if (!(tbl->flags & OPT_EXTRACT))
			continue;

		filename = makexFilename(tbl);

		fileOpenWrite(filename, &file);
		fileSeek(&srcfile, tbl->offset, SEEK_SET);
		fileCopy(&srcfile, &file, tbl->length);
		fileClose(&file);
		}
	}

/* Compare order fields */
static int cmpOrder(const void *first, const void *second)
	{
	const Table *a = first;
	const Table *b = second;
	if (a->order < b->order)
		return -1;
	else if (a->order > b->order)
		return 1;
	else 
		return 0;
	}

/* Copy table and compute its checksum */
static Card32 tableCopy(File *src, File *dst, long offset, long length)
	{
	int i;
	Card32 value;
	Card32 checksum = 0;

	fileSeek(src, offset, SEEK_SET);
	for (; length > 3; length -= 4)
		{
		fileReadObject(src, 4, &value);
		fileWriteObject(dst, 4, value);
		checksum += value;
		}

	if (length == 0)
		return checksum;

	/* Read remaining bytes and pad to 4-byte boundary */
	value = 0;
	for (i = 0; i < length; i++)
		{
		Card8 b;
		fileReadN(src, 1, &b);
		value = value<<8 | b;
		}
	value <<= (4 - length) * 8;
	fileWriteObject(dst, 4, value);

	return checksum + value;
	}

/* Add table from file */
static Card32 addTable(Table *tbl, Card32 *length)
	{
	File file;
	Card32 checksum;

	fileOpenRead(tbl->afilename, &file);
	fileSeek(&file, 0, SEEK_END);
	*length = fileTell(&file);
	checksum = tableCopy(&file, &dstfile, 0, *length);
	fileClose(&file);

	return checksum;
	}

/* Compare tag fields */
static int cmpTags(const void *first, const void *second)
	{
	const Table *a = first;
	const Table *b = second;
	if (a->tag < b->tag)
		return -1;
	else if (a->tag > b->tag)
		return 1;
	else 
		return 0;
	}

/* Copy tables from source file to destination file applying (-d, -a, and -f)
   options */
static void sfntCopy(void)
	{
	int i;
	int nLongs;
	Tag *tags;
	Card16 numDstTables;
	Card32 checksum;
	Card32 offset;
	Card32 length;
	Card32 adjustOff;
	Card32 totalsum;
	int headSeen = 0;
	char outputfilename[FILENAME_MAX];
	char *dstfilename = dstfile.name;
	FILE* f;
	
	strcpy(outputfilename, dstfile.name);
	f = freopen(outputfilename, "r+b", dstfile.fp);
	if (f==NULL)
	{
		fatal(SFED_MSG_sysFERRORSTR, strerror(errno), dstfile.name);
	}
	
	/* Count destination tables */
	numDstTables = 0;
	for (i = 0; i < sfnt.numTables; i++)
		if (!(sfnt.directory[i].flags & OPT_DELETE))
			numDstTables++;

	/* Assign table order */
	tags = (sfnt.version == TAG('O','T','T','O'))? otfOrder: ttfOrder;
	for (i = 0; i < sfnt.numTables; i++)
		{
		Tag *tagp = tags;
		Table *tbl = &sfnt.directory[i];
		
		for (tagp = tags; *tagp != 0; tagp++)
			if (*tagp == tbl->tag)
				{
				tbl->order = tagp - tags;
				goto found;
				}
		
		/* Tag not found; sort to end */
		tbl->order = 100;
	found:
		;
		}

	/* Sort tables into recommended order */
	qsort(sfnt.directory, sfnt.numTables, sizeof(Table), cmpOrder);

	/* Skip directory */
	fileSeek(&dstfile, DIR_HDR_SIZE + ENTRY_SIZE * numDstTables, SEEK_SET);

	

	totalsum = 0;
	for (i = 0; i < sfnt.numTables; i++)
		{
		Table *tbl = &sfnt.directory[i];

		offset = fileTell(&dstfile);

		if (tbl->flags & OPT_ADD)
			checksum = addTable(tbl, &length);
		else
			{
			if (tbl->flags & OPT_DELETE)
				continue;	/* Skip deleted table */
			else
				{
				length = tbl->length;
				checksum = 
					tableCopy(&srcfile, &dstfile, tbl->offset, length);
				}
			}

		if (tbl->tag == TAG('h','e','a','d'))
			{
			  Card32 b;
			/* Adjust sum to ignore head.checkSumAdjustment field */
			
			
			
			adjustOff = offset + HEAD_ADJUST_OFFSET;
			fileSeek(&dstfile, adjustOff, SEEK_SET);
			fileReadObject(&dstfile, 4, &b);
			checksum -= b;
			
			
			fileSeek(&dstfile, 0, SEEK_END);
			headSeen = 1;
			}
		

		/* Update table entry */
		tbl->checksum	= checksum;
		tbl->offset		= offset;
		tbl->length		= length;
		tbl->flags		|= TBL_DST;

		totalsum += checksum;
		}

	/* Initialize sfnt header */
	calcSearchParams(numDstTables, &sfnt.searchRange, 
					 &sfnt.entrySelector, &sfnt.rangeShift);
	
	/* Write sfnt header */
	fileSeek(&dstfile, 0, SEEK_SET);

	fileWriteObject(&dstfile, 4, sfnt.version);
	fileWriteObject(&dstfile, 2, numDstTables);
	fileWriteObject(&dstfile, 2, sfnt.searchRange);
	fileWriteObject(&dstfile, 2, sfnt.entrySelector);
	fileWriteObject(&dstfile, 2, sfnt.rangeShift);

	/* Sort directory into tag order */
	qsort(sfnt.directory, sfnt.numTables, sizeof(Table), cmpTags);


	/* Write sfnt directory */
	for (i = 0; i < sfnt.numTables; i++)
		{
		Table *tbl = &sfnt.directory[i];

		if (tbl->flags & TBL_DST)
			{
			fileWriteObject(&dstfile, 4, tbl->tag);
			fileWriteObject(&dstfile, 4, tbl->checksum);
			fileWriteObject(&dstfile, 4, tbl->offset);
			fileWriteObject(&dstfile, 4, tbl->length);
			}
		}

	/* Checksum sfnt header */

	fileSeek(&dstfile, 0, SEEK_SET);

	nLongs = (DIR_HDR_SIZE + ENTRY_SIZE * numDstTables) / 4;
	for (i = 0; i < nLongs; i++)
		{
		Card32 b;
		fileReadObject(&dstfile, 4, &b);
		totalsum += b;
		}
	
	
	if (headSeen)
		{
		/* Write head.checkSumAdjustment */
		fileSeek(&dstfile, adjustOff, SEEK_SET);
		fileWriteObject(&dstfile, 4, 0xb1b0afba - totalsum);
		}
	dstfile.name=dstfilename;
	}
	

int main(int argc, char *argv[])
	{
	int argi;
	volatile int i;
	cmdlinetype *cmdl;
	int status = 0; /*setjmp(global.env); Not needed as lib */
	
	

	if (status)
	  {
	  	if (doingScripting)
	  		{
	  		  goto scriptAbEnd;
	  		}
	  	else
			exit(status - 1);	/* Finish processing */
	  }

	/* Set signal handler */
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, cleanup);

	da_SetMemFuncs(memNew, memResize, memFree);
	global.progname = "sfntedit";
	
	scriptfilename[0] = '\0'; /* init */
	foundXswitch = 0;
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
	
	if (!foundXswitch && (argc < 2))
	  strcpy(scriptfilename, "sfntedit.scr");

/* see if scriptfile exists to Auto-execute */
	if (

		sysFileExists(scriptfilename)

		)
		{
			doingScripting = 1;
			makeArgs(scriptfilename);
		}	  
		   

	if (!doingScripting)
		{
		/* Initialize */
		sfnt.numTables = 0;
		for (i = 0; i < MAX_TABLES; i++)
			{
			sfnt.directory[i].tag = 0;
			sfnt.directory[i].flags = 0;
			sfnt.directory[i].xfilename = NULL;
			sfnt.directory[i].afilename = NULL;
			}
		srcfile.name = NULL;	srcfile.fp = NULL;
		dstfile.name = NULL;	dstfile.fp = NULL;		
		
		argi = parseArgs(--argc, ++argv);
		if (argi == 0)
			showUsage();
		
		if (!doingScripting && foundXswitch)
			{
				if (scriptfilename && scriptfilename[0] != '\0')
				{
				doingScripting = 1;
				makeArgs(scriptfilename);
				goto execscript;
				}

			}
		if ((srcfile.name == NULL) && 
 				(argc > 1)

		) /* no files on commandline, but other switches */
			{
			fatal(SFED_MSG_MISSINGFILENAME);
			showUsage();			
			}

		fileOpenRead(srcfile.name, &srcfile);
		
		{
			char * end;

			end=strrchr(srcfile.name, '\\');
			if(end==NULL)
				sourcepath="";
			else{
				char *scurr = srcfile.name;
				char *dcurr;
				
				sourcepath=(char *)malloc(strlen(srcfile.name));
				dcurr = sourcepath;
				while(scurr!=end)
				{
					*dcurr++=*scurr++;
				}		
				*dcurr=0;
			}
		
		}

		if (dstfile.name != NULL) /* Open destination file */
			fileOpenWrite(dstfile.name, &dstfile);

		/* Read sfnt header */
		sfntReadHdr();

		/* Process font */
		if (options & OPT_LIST)
			sfntDumpHdr();
		else if (options & OPT_CHECK)
			checkChecksums();
		else
			{
			if (options & OPT_EXTRACT)
				extractTables();
			if (options & (OPT_DELETE|OPT_ADD|OPT_FIX))
				sfntCopy();
			}

		/* Close files */
		fileClose(&srcfile);
		if (dstfile.fp != NULL)
			{
			fileClose(&dstfile);

			if (strcmp(dstfile.name + strlen(dstfile.name) - strlen(tmpname), tmpname)==0)
				{	/* Rename tmp file to source file */
				if (rename(srcfile.name, BACKUPNAME) == -1)
					fatal(SFED_MSG_BADRENAME, strerror(errno), srcfile.name);	
				if (rename(dstfile.name, srcfile.name) == -1)
					fatal(SFED_MSG_BADRENAME, strerror(errno), dstfile.name);
				if (remove(BACKUPNAME) == -1)
					fatal(SFED_MSG_REMOVEERR, strerror(errno), BACKUPNAME);	
				}
			}			

		}
		
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
					
					sourcepath=(char *)malloc(strlen(scriptfilename));
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
			int j;
			/* Initialize */
			sfnt.numTables = 0;
			for (j = 0; j < MAX_TABLES; j++)
				{
				sfnt.directory[j].tag = 0;
				sfnt.directory[j].flags = 0;
				sfnt.directory[j].xfilename = NULL;
				sfnt.directory[j].afilename = NULL;
				}
			srcfile.name = NULL;	srcfile.fp = NULL;
			dstfile.name = NULL;	dstfile.fp = NULL;	
					
			cmdl = da_INDEX(script.cmdline, i);
			if (cmdl->args.cnt < 2) continue;
			
			{
			int a;				
			inform(SFED_MSG_EOLN);
			message(SFED_MSG_ECHOSCRIPTCMD);
			for (a = 1; a < cmdl->args.cnt; a++)
				inform(SFED_MSG_RAWSTRING, cmdl->args.array[a]);					
			inform(SFED_MSG_EOLN);
			}
			
			argi = parseArgs(cmdl->args.cnt - 1, cmdl->args.array +1);	
			if (argi == 0)
				{
				fatal(SFED_MSG_MISSINGFILENAME);
				showUsage();
				}

			fileOpenRead(srcfile.name, &srcfile);

			if (dstfile.name != NULL) /* Open destination file */
				fileOpenWrite(dstfile.name, &dstfile);

			/* Read sfnt header */
			sfntReadHdr();

			/* Process font */
			if (options & OPT_LIST)
				sfntDumpHdr();
			else if (options & OPT_CHECK)
				checkChecksums();
			else
				{
				if (options & OPT_EXTRACT)
					extractTables();
				if (options & (OPT_DELETE|OPT_ADD|OPT_FIX))
					sfntCopy();
				}

scriptAbEnd:
			/* Close files */
			fileClose(&srcfile);
			if (dstfile.fp != NULL)
				{
				char * fullbackupname;
				fullbackupname = MakeFullPath(BACKUPNAME);
				fileClose(&dstfile);

				if (strcmp(dstfile.name + strlen(dstfile.name) - strlen(tmpname), tmpname)==0)
					{	/* Rename tmp file to source file */
					if (rename(srcfile.name, fullbackupname) == -1)
						fatal(SFED_MSG_BADRENAME, strerror(errno), srcfile.name);	
					if (rename(dstfile.name, srcfile.name) == -1)
						fatal(SFED_MSG_BADRENAME, strerror(errno), dstfile.name);
					if (remove(fullbackupname) == -1)
						fatal(SFED_MSG_REMOVEERR, strerror(errno), fullbackupname);	
					}
					free(fullbackupname);
				}
			}

		doingScripting = 0;	
		}
		
	fprintf(stdout, "\nDone.\n");
	return status;
	}


