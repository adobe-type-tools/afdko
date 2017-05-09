/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)Dmain.c	1.4
 * Changed:    2/12/99 10:38:07
 ***********************************************************************/
#include "Dglobal.h"
#include "Dsfnt.h"
#include "sfnt_sfnt.h"

#include "Dda.h"
#include "Dsys.h"
#include <ctype.h>
#include <string.h>

Byte8 *version = "2.21215";	/* Program version */

volatile IntX doingScripting = 0;
static Byte8 scriptfilename[256];
static char * sourcepath="";
#include "setjmp.h"

jmp_buf mark;
#define MAX_ARGS 200



typedef struct _cmdlinetype
{
  da_DCL(char *, args); /* arg list */  
} cmdlinetype;

static struct
{
  Byte8 *buf; /* input buffer */
  da_DCL(cmdlinetype, cmdline);
} script;

char * MakeFullPath(char *source)
{
		char * dest;
		
		dest = (char *) malloc(256);
		sprintf( dest, "%s\\%s", sourcepath, source);
		return dest;
}

/* Print usage information */
static void printUsage(void)
	{

	printf( 
		   "Usage: %s [-u|-h] [-T] [-d <level>] [-x<tags>|-i<tags>] " 
		   "<FONTS|DIRS>\n"
			"OR: %s  -X <scriptfile>\n\n"
		   "where: <FONTS|DIRS> is:\n"
		   "\t    <fontfile1> <fontfile2>\n"
		   "\tOR: <fontfile> <otherfontdir>\n"
		   "\tOR: <fontdir1> <fontdir2>\n "
		   "\nOptions:\n"
		   "    -u  print usage information\n"
		   "    -h  print usage and help information\n"
		   "    -T  show time-stamp of font files\n"
		   "    -d  set diff level of detail\n"
		   "    -x  exclude table(s)   _OR_\n"
		   "    -i  include table(s) e.g., -iname,head\n"
		   "    -X  execute a series of complete command-lines from <scriptfile> [default: sfntdiff.scr]\n"
		   "Version:\n"
		   "    %s\n",
		   global.progname, 
		   global.progname,
		   version);
	}

/* Show usage information */
static void showUsage(void)
	{
	printUsage();
	quit(1);
	}

/* Show usage and help information */
static void showHelp(void)
	{
	printUsage();
	printf( "Notes:\n"
           "    -d <level>  : level of detail in indicating differences\n"
           "        0: (default) indicate if there is a difference\n"
           "            in a particular table\n"
           "        1: indicate the byte-position within the table where\n"
           "            a difference begins\n"
           "        2: dump the tables in a Hex format and produce\n"
           "            a line-by-line diff\n"
           "        3: if the contents of the table is amenable, compare\n"
           "            the data-structures specific to that table, and\n"
           "            produce the differences in the fields of the\n"
           "            data-structures of the two files\n"
           "        4: a possibly more-verbose version of level 3\n"
		   );
    printf( "    -x <tag>[,<tag>]*\n"
           "        exclude one or more tables from comparisons.\n"
           "        e.g., -x cmap,name\n"
           "        will skip inspecting/comparing the 'cmap' and 'name' tables\n");
    printf( "    -i <tag>[,<tag>]*\n"
           "        include one or more tables for comparisons.\n"
           "        e.g., -i cmap,name\n"
           "        will inspect/compare ONLY the 'cmap' and 'name' tables\n");
	printf( "    -i and -x switches are exclusive of each other.\n");
	quit(1);
	}

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
      	fatal("Empty script file? [%s]\n", filename);
      length = sysFileLen(file);
      if (length < 1) 
      	fatal("Empty script file? [%s]\n", filename);
      script.buf = memNew(length + 2);

	  message("Executing script-file [%s]\n", filename);
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


/* Main program */
IntN main(IntN argc, Byte8 *argv[])
	{
	  static opt_Option opt[] =
		{
		  {"-u", opt_Call, showUsage},
		  {"-h", opt_Call, showHelp},
		  {"-T", opt_Flag},
		  {"-x", sfntTagScan},
		  {"-i", sfntTagScan},
		  {"-d", opt_Int, &level, "0", 0, 4},
		  {"-X", opt_String, scriptfilename},
		};

	  volatile IntX i = 0;
	  IntN argi;
	  Card32 value, value2;
	  Byte8 *filename1;
	  Byte8 *filename2;
	  IntN name1isDir, name2isDir;
  	  cmdlinetype *cmdl;
	  Byte8 foundXswitch = 0;
	  Byte8 **SimpleNameList;
	  IntN NumSimpleNames= 0;
	  IntN status = 0; /* setjmp(global.env); not neede in exe */
	  	  
	  if (status)
	  	{
	  		if (doingScripting)
	  			goto scriptAbEnd;
	  		else
				return (status - 1);	/* Finish processing */
		}
	  
	da_SetMemFuncs(memNew, memResize, memFree);
	global.progname = "sfntdiff";

	scriptfilename[0] = '\0'; /* init */
	
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
	
	if (!foundXswitch && (argc < 2)) /* if no -X on cmdline, and no OTHER switches */
	  strcpy(scriptfilename, "sfntdiff.scr");

/* see if scriptfile exists to Auto-execute */
	if (
		(scriptfilename[0] != '\0') && sysFileExists(scriptfilename)
		)
		{
			doingScripting = 1;
			makeArgs(scriptfilename);
		}


	  if (!doingScripting)
	  {
	  argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);
	  
	  if (argi == 0 
	  )
		showUsage();
		
	  if (!doingScripting && opt_Present("-X"))
	  {
		if (scriptfilename && scriptfilename[0] != '\0')
		{
			doingScripting = 1;
			makeArgs(scriptfilename);
			goto execscript;
		}
	}		

	  if (opt_Present("-x") && opt_Present("-i"))
		{
		  printf( "ERROR: '-x' switch and '-i' switch are exclusive of each other.\n");
		  showUsage();
		}

	  if (level > 4) level = 4;

	  if ((argc - argi) == 0)
		{
		if (argc > 1) 
		{
		  printf( "ERROR: not enough files/directories specified.\n");
		  showUsage();
		}
		}

	  filename1 = argv[argi];
	  filename2 = argv[argi+1];
	  
	  name1isDir = sysIsDir(filename1);
	  name2isDir = sysIsDir(filename2);

	  printf( "%s\n", ourtime());
	  printf( "%s (%s) (-d %d)  files:\n", global.progname, version, level);

	  if (!name1isDir && !name2isDir)
		{
		  if (!fileIsOpened(1)) fileOpen(1, filename1);
		  if (!fileIsOpened(2)) fileOpen(2, filename2);

		  if (opt_Present("-T"))
			{
			  printf( "< %s\t%s\n", filename1, fileModTimeString(1, filename1));
			  printf( "> %s\t%s\n", filename2, fileModTimeString(2, filename2));
			}
		  else
			{
			  printf( "< %s\n", filename1);
			  printf( "> %s\n", filename2);
			}

		  /* See if we can recognize the file type */
		  value = fileSniff(1);	
		  value2 = fileSniff(2);
		  if (value != value2)
		  	{
		  	  fileClose(1);  fileClose(2);
		  	  fatal("file1 [%s] is not the same type as file2 [%s]\n", filename1, filename2);
		  	}
		  	
		  switch (value)
			{
			case bits_:
			case typ1_:		
			case true_:
			case OTTO_:
			case VERSION(1,0):
			  sfntRead(0, -1, 0, -1);	/* Read plain sfnt file */
			  break;
			case ttcf_:
			  warning("unsupported file [%s] (ignored)\n", filename1);
			  fileClose(1); fileClose(2);
			  quit(1);			
			  break;
			default:		
			  warning("unsupported/bad file [%s] (ignored)\n", filename1);
			  fileClose(1); fileClose(2);
			  quit(1);
			}
		  sfntDump();
		  sfntFree();
		  fileClose(1);		  
		  fileClose(2);
		}

	  else if (name1isDir && name2isDir)
		{
		  Byte8 fil1[MAX_PATH];
		  Byte8 fil2[MAX_PATH];
		  IntN nn;
		  
		  NumSimpleNames = sysReadInputDir(filename1, &SimpleNameList);
		  for (nn = 0; nn < NumSimpleNames; nn++)
			{
			  strcpy(fil1, filename1);			  
			  strcat(fil1, sysPathSep);
			  strcat(fil1, SimpleNameList[nn]);

			  strcpy(fil2, filename2);
			  strcat(fil2, sysPathSep);
			  strcat(fil2, SimpleNameList[nn]);
			
			  fileOpen(1, fil1);
			  fileOpen(2, fil2);

			  printf( "\n---------------------------------------------\n");
			  if (opt_Present("-T"))
				{
				  printf( "< %s\t%s\n", fil1, fileModTimeString(1, fil1));
				  printf( "> %s\t%s\n", fil2, fileModTimeString(2, fil2));
				}
			  else
				{
				  printf( "< %s\n", fil1);
				  printf( "> %s\n", fil2);
				}
			  
			  /* See if we can recognize the file type */
			  value = fileSniff(1);	
			  value2 = fileSniff(2);
			  if (value != value2)
			  	{
	 	  	      fileClose(1);  fileClose(2);
			  	  fatal("file1 [%s] is not the same type as file2 [%s]\n", fil1, fil2);
			  	}

			  switch (value)
				{
				case bits_:
				case typ1_:		
				case true_:
				case OTTO_:
				case VERSION(1,0):
				  sfntRead(0, -1, 0, -1);	/* Read plain sfnt file */
				  break;
				case 256:
				case ttcf_:
			  		warning("unsupported file [%s] (ignored)\n", fil1);
			  		fileClose(1); fileClose(2);
			  		quit(1);
			  		break;
				default:
  			    warning("unsupported/bad file [%s] (ignored)\n", fil1);
		  	    fileClose(1);  fileClose(2);
			    continue;
			  }
	 		  sfntDump();
		  	  sfntFree();			  
			  fileClose(1);		  
			  fileClose(2);
			}
		}
	  else if (!name1isDir && name2isDir)
		{
		  Byte8 fil2[MAX_PATH];
		  Byte8 *c;

		  strcpy(fil2, filename2);
		  strcat(fil2, sysPathSep);
		  c = strrchr(filename1, sysPathSep[0]);
		  if (c == NULL)
			strcat(fil2, filename1);
		  else
			strcat(fil2, ++c);

		  fileOpen(1, filename1);
		  fileOpen(2, fil2);

		  if (opt_Present("-T"))
			{
			  printf( "< %s\t%s\n", filename1, fileModTimeString(1, filename1));
			  printf( "> %s\t%s\n", fil2, fileModTimeString(2, fil2));
			}
		  else
			{
			  printf( "< %s\n", filename1);
			  printf( "> %s\n", fil2);
			}
		  
		  /* See if we can recognize the file type */
		  value = fileSniff(1);	
		  value2 = fileSniff(2);
		  if (value != value2)
		  	{
		  	  fileClose(1);  fileClose(2);
		  	  fatal("file1 [%s] is not the same type as file2 [%s]\n", filename1, fil2);
		  	}
		
		  switch (value)
			{
			case bits_:
			case typ1_:		
			case true_:
			case OTTO_:
			case VERSION(1,0):
			  sfntRead(0, -1, 0, -1);	/* Read plain sfnt file */
			  break;
			case 256:
			case ttcf_:
			  		warning("unsupported file [%s] (ignored)\n", filename1);
			  		fileClose(1); fileClose(2);
			  		quit(1);
					break;
			default:
			  warning("unsupported/bad file [%s] (ignored)\n", filename1);
		  	  fileClose(1);  fileClose(2);
		  	  quit(1);
			}
		  sfntDump();
		  sfntFree();		  
		  fileClose(1);		  
		  fileClose(2);
		}
	  else
		{
		  printf( "ERROR: Incorrect/insufficient files/directories specified.\n");
		  showUsage();
		}
	}
	else
	{ /* Doing scripting */
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
		cmdl = da_INDEX(script.cmdline, i);
		if (cmdl->args.cnt < 2) continue;
		
		{
			IntX a;
			
			note("\n");
			message("Executing command-line: ");
			for (a = 1; a < cmdl->args.cnt; a++)
			{
			  note("%s ", cmdl->args.array[a]);
			}
			note("\n");
		}
		
		argi = opt_Scan(cmdl->args.cnt, cmdl->args.array, opt_NOPTS(opt), opt, NULL, NULL);		
	    if (opt_Present("-x") && opt_Present("-i"))
		{
		  printf( "ERROR: '-x' switch and '-i' switch are exclusive of each other.\n");
		  continue;
		}

	    if (level > 4) level = 4;
		if (((cmdl->args.cnt - argi) < 2)
			&& (cmdl->args.cnt > 1))
		{
		  printf( "ERROR: not enough files/directories specified.\n");
		  continue;
		}
			
			if(sourcepath[0]!='\0'){
				filename1 = MakeFullPath(cmdl->args.array[argi]);
		  		filename2 = MakeFullPath(cmdl->args.array[argi+1]);
		  	}else{
		  		filename1 = cmdl->args.array[argi];
		  		filename2 = cmdl->args.array[argi+1];
		  }
		  
		  name1isDir = sysIsDir(filename1);
		  name2isDir = sysIsDir(filename2);

		  printf( "%s\n", ourtime());
		  printf( "%s (%s) (-d %d)  files:\n", global.progname, version, level);

		  if (!name1isDir && !name2isDir)
			{
			  fileOpen(1, filename1);
			  fileOpen(2, filename2);

			  if (opt_Present("-T"))
				{
				  printf( "< %s\t%s\n", filename1, fileModTimeString(1, filename1));
				  printf( "> %s\t%s\n", filename2, fileModTimeString(2, filename2));
				}
			  else
				{
				  printf( "< %s\n", filename1);
				  printf( "> %s\n", filename2);
				}

			  /* See if we can recognize the file type */
			  value = fileSniff(1);	
			  value2 = fileSniff(2);
			  if (value != value2)
			  	{
			  	  printf( "ERROR: file1 [%s] is not the same type as file2 [%s] (skipped)\n", filename1, filename2);
				  goto scriptAbEnd;
			  	}
			  	
			  switch (value)
				{
				case bits_:
				case typ1_:		
				case true_:
				case OTTO_:
				case VERSION(1,0):
				  sfntRead(0, -1, 0, -1);	/* Read plain sfnt file */
				  break;
				case 256:
				case ttcf_:
				  warning("unsupported file [%s] (skipped)\n", filename1);
				  goto scriptAbEnd;
				  break;
				default:
		
				  warning("unsupported/bad file [%s] (skipped)\n", filename1);
				  goto scriptAbEnd;
				}
			  sfntDump();
			  sfntFree();
			  fileClose(1);		  
			  fileClose(2);
			}

		  else if (name1isDir && name2isDir)
			{
			  Byte8 fil1[MAX_PATH];
			  Byte8 fil2[MAX_PATH];
			  IntN nn;
			  
			  NumSimpleNames = sysReadInputDir(filename1, &SimpleNameList);
			  for (nn = 0; nn < NumSimpleNames; nn++)
				{
				  strcpy(fil1, filename1);			  
				  strcat(fil1, sysPathSep);
				  strcat(fil1, SimpleNameList[nn]);

				  strcpy(fil2, filename2);
				  strcat(fil2, sysPathSep);
				  strcat(fil2, SimpleNameList[nn]);
				
				  fileOpen(1, fil1);
				  fileOpen(2, fil2);

				  printf( "\n---------------------------------------------\n");
				  if (opt_Present("-T"))
					{
					  printf( "< %s\t%s\n", fil1, fileModTimeString(1, fil1));
					  printf( "> %s\t%s\n", fil2, fileModTimeString(2, fil2));
					}
				  else
					{
					  printf( "< %s\n", fil1);
					  printf( "> %s\n", fil2);
					}
				  
				  /* See if we can recognize the file type */
				  value = fileSniff(1);	
				  value2 = fileSniff(2);
				  if (value != value2)
				  	{
				  	  printf( "ERROR: file1 [%s] is not the same type as file2 [%s] (skipped)\n", fil1, fil2);
					  goto scriptAbEnd;
				   	}

				  switch (value)
					{
					case bits_:
					case typ1_:		
					case true_:
					case OTTO_:
					case VERSION(1,0):
					  sfntRead(0, -1, 0, -1);	/* Read plain sfnt file */
					  break;
					case 256:
					case ttcf_:
				  		warning("unsupported file [%s] (ignored)\n", fil1);
				  		fileClose(1); fileClose(2);
				  		quit(1);
				  		break;
					default:
	  			    warning("unsupported/bad file [%s] (skipped)\n", fil1);
			  	    goto scriptAbEnd;
				  }
		 		  sfntDump();
			  	  sfntFree();			  
				  fileClose(1);		  
				  fileClose(2);
				}
			}
		  else if (!name1isDir && name2isDir)
			{
			  Byte8 fil2[MAX_PATH];
			  Byte8 *c;

			  strcpy(fil2, filename2);
			  strcat(fil2, sysPathSep);
			  c = strrchr(filename1, sysPathSep[0]);
			  if (c == NULL)
				strcat(fil2, filename1);
			  else
				strcat(fil2, ++c);

			  fileOpen(1, filename1);
			  fileOpen(2, fil2);

			  if (opt_Present("-T"))
				{
				  printf( "< %s\t%s\n", filename1, fileModTimeString(1, filename1));
				  printf( "> %s\t%s\n", fil2, fileModTimeString(2, fil2));
				}
			  else
				{
				  printf( "< %s\n", filename1);
				  printf( "> %s\n", fil2);
				}
			  
			  /* See if we can recognize the file type */
			  value = fileSniff(1);	
			  value2 = fileSniff(2);
			  if (value != value2)
			  	{
			  	  printf( "ERROR: file1 [%s] is not the same type as file2 [%s] (skipped)\n", filename1, fil2);
				  goto scriptAbEnd;
			  	}
			
			  switch (value)
				{
				case bits_:
				case typ1_:		
				case true_:
				case OTTO_:
				case VERSION(1,0):
				  sfntRead(0, -1, 0, -1);	/* Read plain sfnt file */
				  break;
				case 256:
				case ttcf_:
				  		warning("unsupported file [%s] (skipped)\n", filename1);
				  		goto scriptAbEnd;
						break;
				default:
				  warning("unsupported/bad file [%s] (skipped)\n", filename1);
			  	  goto scriptAbEnd;
				}
			  sfntDump();
			  sfntFree();		  
			  fileClose(1);		  
			  fileClose(2);
			}
		  else
			{
			  printf( "ERROR: Incorrect/insufficient files/directories specified.\n");			  
			}

scriptAbEnd:
	    sfntFree();
	  	fileClose(1);		  
		fileClose(2);
	  }	
	  doingScripting = 0;
	}
	/*printf( "\nDone.\n");*/
	quit(status);
	return status;	
	}

