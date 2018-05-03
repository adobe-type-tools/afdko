/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)opt.c	1.5
 * Changed:    10/26/95 11:20:57
************************************************************************/

/*
 * Command line argument processing support.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Dopt.h"

/* Missing protypes */
extern int fprintf( FILE *stream, const char *format, ...);
extern int sscanf(const char *s, const char *format, ...);

#define opt_PRESENT		(1<<7)	/* Flags if option was present */

char *opt_progname;	/* Program name */
static struct
	{
	int nOpts;				/* Number of options */
	opt_Option *opts;		/* Options definitions */
	opt_Handler *handler;	/* Error handler */
	void *client;			/* Client data pointer */
	int error;				/* Flags if error occured */
	} global;

/* Compare two options */
static int cmpOptions(const void *a, const void *b)
	{
	return strcmp(((opt_Option *)a)->name, ((opt_Option *)b)->name);
	}

/* Scan option's value */
static int doScan(int argc, char *argv[], int argi, opt_Option *opt)
	{
	opt->flags |= opt_PRESENT;
	return (opt->scan == NULL) ? argi : opt->scan(argc, argv, argi, opt);
	}

/* Match whole key to option */
static int matchWhole(const void *key, const void *value)
	{
	return strcmp((char *)key, ((opt_Option *)value)->name);
	}

/* Match part key to option */
static int matchPart(const void *key, const void *value)
	{
	char *name = ((opt_Option *)value)->name;
	return strncmp((char *)key, name, strlen(name));
	}

/* Lookup option name using supplied match function */
static opt_Option *lookup(char *name,
						  int (*match)(const void *key, const void *value))
	{
	return (opt_Option *)bsearch(name, global.opts, global.nOpts,
								 sizeof(opt_Option), match);
	}

static void message(char *fmt, char *arg)
{
	printf( "%s [ERROR]: ", opt_progname);
	printf( fmt, arg);
}	 

static void message2(char *fmt, char *optarg, char *arg)
{
	printf( "%s [ERROR]: ", opt_progname);
	printf(fmt, optarg, arg);
}	 


/* Default error handler */
static int defaultHandler(int error, opt_Option *opt, char *arg, void *client)
	{
	switch (error)
		{
	case opt_NoScanner:
		message("no scanner (%s)\n",  opt->name);
		break;
	case opt_Missing:
		message("no value(s) (%s)\n", opt->name);
		break;
	case opt_Format:
		message2("bad value <%s> (%s)\n", arg, opt->name);
		break;
	case opt_Range:
		message2("value out of range <%s> (%s)\n", arg, opt->name);
		break;
	case opt_Required:
		message("required option missing (%s)\n",  opt->name);
		break;
	case opt_Exclusive:
		message("mutually exclusive option conflict (%s)\n", opt->name);
		break;
	case opt_Unknown:
		message("unknown option (%s)\n", arg);
		break;
		}
	return 1;
	}

/* Error handler wrapper */
void opt_Error(int error, opt_Option *opt, char *arg)
	{
	global.error += global.handler(error, opt, arg, global.client);
	}	

/* Process argument list */
int opt_Scan(int argc, char *argv[],
			 int nOpts, opt_Option *opts, opt_Handler handler, void *client)
	{
	int i;
	int argi;
	char *p = argv[0] + strlen(argv[0]);

	/* Extract program name, scan for Unix, DOS, and Macintosh separators */
	while (--p >= argv[0] && *p != '/' && *p != '\\' && *p != ':')
			;
	opt_progname = p + 1;

	/* Initialize argument package */
	global.nOpts = nOpts;
	global.opts = opts;
	if (handler != NULL)
		{
		global.handler = handler;
		global.client = client;
		}
	else
		{
		global.handler = defaultHandler;
		global.client = NULL;
		}

	/* Sort options into alphabetical order */
	qsort(opts, nOpts, sizeof(opt_Option), cmpOptions);

	/* Set defaults */
	for (i = 0; i < global.nOpts; i++)
		{
		opt_Option *opt = &global.opts[i];
		opt->flags &= ~opt_PRESENT;
		if (opt->scan == NULL)
			opt_Error(opt_NoScanner, opt, NULL);
		else if (opt->scan != opt_Call)
			opt->scan(1, &opt->dflt, 0, opt);
		}

	argi = 1;
	while (argi < argc)
		{
		char *arg = argv[argi];
		opt_Option *opt = lookup(arg, matchWhole);
		if (opt != NULL)
			/* Whole argument matched option */
			argi = doScan(argc, argv, argi + 1, opt);
		else
			{
			opt = lookup(arg, matchPart);
			if (opt != NULL)
				{
				/* Initial part of argument matched option */
				if (opt->flags & opt_COMBINED)
					{
					/* Argument is a combination of options */
					int plen = opt->length;

					argi = doScan(argc, argv, argi + 1, opt);
					for (;;)
						{
						/* Trim last option name from argument and rematch */
						strcpy(&arg[plen], &arg[strlen(opt->name)]);
						if (arg[plen] == '\0')
							break;	/* No more options left */

						opt = lookup(arg, matchPart);
						if (opt == NULL)
							{
							opt_Error(opt_Unknown, NULL, arg);
							break;
							}
						argi = doScan(argc, argv, argi, opt);
						}
					}
				else
					{
					/* Option and value combined together */
					argv[argi] = arg + strlen(opt->name);
					argi = doScan(argc, argv, argi, opt);
					}
				}
			else
				break;
			}
		}

	/* Test that required options were actually present */
	for (i = 0; i < global.nOpts; i++)
		{
		opt_Option *opt = &global.opts[i];
		if (opt->flags & opt_REQUIRED && !(opt->flags & opt_PRESENT))
			opt_Error(opt_Required, opt, NULL);
		}

	return (global.error != 0) ? 0 : argi;
	}

/* If option was present return non-0 else return 0 */
int opt_Present(char *name)
	{
	opt_Option *opt = lookup(name, matchWhole);
	if (opt == NULL)
		{
		message("unknown option (%s)\n", name);
		return 0;
		}
	return opt->flags & opt_PRESENT;
	}

/* If option exists and was present return its value else return NULL */
void *opt_Value(char *name)
	{
	opt_Option *opt = lookup(name, matchWhole);
	if (opt == NULL)
		{
		message("unknown option (%s)\n",  name);
		return NULL;
		}
	return (opt->flags & opt_PRESENT) ? opt->value : (void*)NULL;
	}

/* --- Standard value scanners --- */

/* Macro for declaring standard numeric scanners. 

   I found a bug when testing this code on a Sun3 using a gcc compiler.
   Apparently, gcc normally makes string constants read-only. A consequence of
   this is that functions that modify string constants passed to them will
   cause a segmentation fault. On some systems, e.g. Sun3, the sscanf()
   function modifies the source string and causes a segmentation fault when
   called thus: sscanf("3.142", "%le", &dbl). Initialization of numeric options
   will call sscanf() in a similar way so I defend against this by copying the
   string to temporary read/write storage and passing this copy to sscanf().
 */
#define DECLARE(n,t,f) \
int opt_##n(int argc, char *argv[], int argi, opt_Option *opt) \
	{ \
	if (argv[0] == NULL) \
		  return argi; \
	if (argi == argc) \
		opt_Error(opt_Missing, opt, NULL); \
	else \
		{ \
		t value; \
		char s[64]; \
		strncpy(s, argv[argi], 63); \
		s[63] = '\0'; \
		if (sscanf(s, f, &value) != 1) \
			opt_Error(opt_Format, opt, argv[argi]); \
		else if ((opt->min != 0 || opt->max != 0) && \
				 (value < opt->min || value > opt->max)) \
			opt_Error(opt_Range, opt, argv[argi]); \
		else \
			*(t *)opt->value = value; \
		argi++; \
		} \
	return argi; \
	}

DECLARE(Short, short, "%hi")
DECLARE(Int, int, "%i")
DECLARE(Long, long, "%li")
DECLARE(UShort, unsigned short, "%hu")
DECLARE(UInt, unsigned int, "%u")
DECLARE(ULong, unsigned long, "%lu")
DECLARE(Double, double, "%lf")

/* (char) scanner */
int opt_Char(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argv[0] == NULL)
		  return argi;
	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		{
		unsigned int value;
		char *arg = argv[argi];
		int len = strlen(arg);
		int format = 0;

		if (len == 1)
			value = arg[0];
		else
			{
			if (arg[0] == '\\')
				switch (arg[1])
					{
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					/* Octal */
					if (sscanf(&arg[1], "%o", &value) != 1)
						format = 1;
					break;
				case 'a':
					value = '\a';
					break;
				case 'b':
					value = '\b';
					break;
				case 'f':
					value = '\f';
					break;
				case 'n':
					value = '\n';
					break;
				case 'r':
					value = '\r';
					break;
				case 't':
					value = '\t';
					break;
				case 'x':
					/* Hexadecimal */
					if (sscanf(&arg[2], "%x", &value) != 1)
						format = 1;
					break;
				case 'v':
					value = '\v';
					break;
				default:
					if (len > 2)
						format = 1;
					else
						value = arg[1];	/* Ignore '\' */
					}
			else
				format = 1;
			}

		if (format)
			opt_Error(opt_Format, opt, arg);
		else if ((opt->min != 0 || opt->max != 0) &&
			(value < opt->min || value > opt->max))
			opt_Error(opt_Range, opt, arg);
		else
			*(char *)opt->value = value;
		argi++;
		}

	return argi;
	}

/* (char *) scanner */
int opt_String(int argc, char *argv[], int argi, opt_Option *opt)
	{
	if (argv[0] == NULL)
		return argi;
	if (argi == argc)
		opt_Error(opt_Missing, opt, NULL);
	else
		{
		char *arg = argv[argi];
		int len = strlen(arg);
		
		if ((opt->min != 0 || opt->max != 0) &&
			(len < opt->min || len > opt->max))
			opt_Error(opt_Range, opt, arg);
		else
			*(char **)opt->value = arg;
		argi++;
		}

	return argi;
	}

/* Simply call function */
int opt_Call(int argc, char *argv[], int argi, opt_Option *opt)
	{
	((void (*)(void))opt->value)();
	return argi;
	}

/* Flag option scanner. Does nothing! */
int opt_Flag(int argc, char *argv[], int argi, opt_Option *opt)
	{
	return argi;
	}

