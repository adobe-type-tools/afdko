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

Byte8 *version = "2.21217";	/* Program version */

static char * sourcepath="";
#include "setjmp.h"

jmp_buf mark;
#define MAX_ARGS 200




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
	quit(0);
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
		};

	  volatile IntX i = 0;
	  IntN argi;
	  Card32 value, value2;
	  Byte8 *filename1;
	  Byte8 *filename2;
	  IntN name1isDir, name2isDir;
	  Byte8 foundXswitch = 0;
	  Byte8 **SimpleNameList;
	  IntN NumSimpleNames= 0;
	  
	da_SetMemFuncs(memNew, memResize, memFree);
	global.progname = "sfntdiff";

	  argi = opt_Scan(argc, argv, opt_NOPTS(opt), opt, NULL, NULL);
	  

	  if (opt_Present("-x") && opt_Present("-i"))
		{
		  printf( "ERROR: '-x' switch and '-i' switch are exclusive of each other.\n");
		  showUsage();
		}

	  if (level > 4) level = 4;

	  if ((argc - argi) < 2)
		{
		  printf( "ERROR: not enough files/directories specified.\n");
		  showUsage();
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

	/*printf( "\nDone.\n");*/
	return 0;	
	}

