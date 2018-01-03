/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)dump.c	1.2
 * Changed:    8/13/98 16:17:46
 ***********************************************************************/

#include "dump.h"
#include "txops.h"
#define ARRAY_LEN(t) (sizeof(t)/sizeof((t)[0]))



void dump_csDump (Int32 length, Card8 *strng, Int32 nMasters)
	{
	static const char *opname[32] =
		{
		/*  0 */ "reserved0",
		/*  1 */ "hstem",
		/*  2 */ "compose",
		/*  3 */ "vstem",
		/*  4 */ "vmoveto",
		/*  5 */ "rlineto",
		/*  6 */ "hlineto",
		/*  7 */ "vlineto",
		/*  8 */ "rrcurveto",
		/*  9 */ "closepath",
		/* 10 */ "callsubr",
		/* 11 */ "return",
		/* 12 */ "escape",
		/* 13 */ "hsbw",
		/* 14 */ "endchar",
		/* 15 */ "moveto",
		/* 16 */ "blend",
		/* 17 */ "curveto",
		/* 18 */ "hstemhm",
		/* 19 */ "hintmask",
		/* 20 */ "cntrmask",
		/* 21 */ "rmoveto",
		/* 22 */ "hmoveto",
		/* 23 */ "vstemhm",
		/* 24 */ "rcurveline",
		/* 25 */ "rlinecurve",
		/* 26 */ "vvcurveto",
		/* 27 */ "hhcurveto",
		/* 28 */ "shortint",
		/* 29 */ "callgsubr",
		/* 30 */ "vhcurveto",
		/* 31 */ "hvcurveto",
		};
	static const char *escopname[] =
		{
		/*  0 */ "dotsection",
		/*  1 */ "vstem3",
		/*  2 */ "hstem3",
		/*  3 */ "and",
		/*  4 */ "or",
		/*  5 */ "not",
		/*  6 */ "seac",
		/*  7 */ "sbw",
		/*  8 */ "store",
		/*  9 */ "abs",
		/* 10 */ "add",
		/* 11 */ "sub",
		/* 12 */ "div",
		/* 13 */ "load",
		/* 14 */ "neg",
		/* 15 */ "eq",
		/* 16 */ "callother",
		/* 17 */ "pop",
		/* 18 */ "drop",
		/* 19 */ "setvw",
		/* 20 */ "put",
		/* 21 */ "get",
		/* 22 */ "ifelse",
		/* 23 */ "random",
		/* 24 */ "mul",
		/* 25 */ "div2",
		/* 26 */ "sqrt",
		/* 27 */ "dup",
		/* 28 */ "exch",
		/* 29 */ "index",
		/* 30 */ "roll",
		/* 31 */ "reservedESC31",
		/* 32 */ "reservedESC32",
		/* 33 */ "setcurrentpt",
		/* 34 */ "hflex",
		/* 35 */ "flex",
		/* 36 */ "hflex1",
		/* 37 */ "flex1",
		/* 38 */ "cntron",
		};
	Int32 single = 0;
	Int32 stems = 0;
	Int32 args = 0;
	Int32 i = 0;

	while (i < length)
		{
		IntX op = strng[i];
		switch (op)
			{
		case tx_return:
		case tx_endchar:
			fprintf(OUTPUTBUFF,  "%s ", opname[op]); 
			return;
			break;

		case tx_reserved0:
		case tx_compose:
		case tx_vmoveto:
		case tx_rlineto:
		case tx_hlineto:
		case tx_vlineto:
		case tx_rrcurveto:
		case t1_closepath:
		case tx_callsubr:
		case t1_hsbw:
		case t1_moveto:
		case t1_curveto:
		case tx_rmoveto:
		case tx_hmoveto:
		case t2_rcurveline:
		case t2_rlinecurve:
		case t2_vvcurveto:
		case t2_hhcurveto:
		case t2_callgsubr:
		case tx_vhcurveto:
		case tx_hvcurveto:
			fprintf(OUTPUTBUFF,  "%s ", opname[op]); 
			args = 0;
			i++;
			break;
		case tx_hstem:
		case tx_vstem:
		case t2_hstemhm:
		case t2_vstemhm:
			fprintf(OUTPUTBUFF,  "%s ", opname[op]);
			stems += args / 2;
			args = 0;
			i++;
			break;
		case t2_hintmask:
		case t2_cntrmask:
			{
			Int32 bytes;
			if (args > 0)
				stems += args / 2;
			bytes = (stems + 7) / 8;
			fprintf(OUTPUTBUFF,  "%s[", opname[op]); 
			i++;
			while (bytes--)
			{
				fprintf(OUTPUTBUFF,  "%02x", strng[i++]); 
			}
			fprintf(OUTPUTBUFF,  "] "); 
			args = 0;
			break;
			}
		case tx_escape:
			{
			/* Process escaped operator */
			CardX escop = strng[i + 1];
			if (escop > ARRAY_LEN(escopname) - 1)
			  fprintf(OUTPUTBUFF,  "? "); 
			else
			  fprintf(OUTPUTBUFF,  "%s ", escopname[escop]); 
			args = 0;
			i += 2;
			break;
			}
		case t2_blend:
			fprintf(OUTPUTBUFF,  "%s ", opname[op]); 
			args -= single * (nMasters - 1);
			i++;
			break;
		case t2_shortint:
			fprintf(OUTPUTBUFF,  "%d ", strng[i + 1]<<8|strng[i + 2]); 
			args++;
			i += 3;
			break;
		case 247: 
		case 248: 
		case 249: 
		case 250:
			/* +ve 2 byte number */
			fprintf(OUTPUTBUFF,  "%d ", 108 + 256 * (strng[i] - 247) + strng[i + 1]); 
			args++;
			i += 2;
			break;
		case 251:
		case 252:
		case 253:
		case 254:
			/* -ve 2 byte number */
			fprintf(OUTPUTBUFF,  "%d ", -108 - 256 * (strng[i] - 251) - strng[i + 1]); 
			i += 2;
			args++;
			break;
		case 255:
			{
			/* 5 byte number */
			Int32 value = (Int32)strng[i + 1]<<24 | (Int32)strng[i + 2]<<16 |
				strng[i + 3]<<8 | strng[i + 4];
#if 0
			if (-32000 <= value && value <= 32000)
			{
				fprintf(OUTPUTBUFF,  "%ld ", value); 
			}
			else
#endif /* JUDY */
			{
				fprintf(OUTPUTBUFF,  "%g ", value / 65536.0); 
			}
			args++;
			i += 5;
			break;
			}
		default:
			/* 1 byte number */
			single = strng[i] - 139;
			fprintf(OUTPUTBUFF,  "%ld ", single); 
			args++;
			i++;
			break;
			}
		}
	}


void dump_csDumpDerel (Card8 *strng, Int32 nMasters)
	{
	Int32 single = 0;
	Int32 args = 0;
	Int32 i = 0;
	Int32 arg0 = 0;
	Int32 val;

	while (1)
		{
		IntX op = strng[i];
		if (args >= nMasters) return;
		switch (op)
			{
		case tx_return:
		case tx_endchar:
			return;
			break;

		case t2_blend:
			args -= single * (nMasters - 1);
			i++;
			return;
			break;
		case t2_shortint:
			val = (strng[i + 1]<<8|strng[i + 2]); 
			if (args == 0) 
			  arg0 = val;
			else
			  val += arg0;
			fprintf(OUTPUTBUFF,  "%ld ", val);
			args++;
			i += 3;
			break;
		case 247: 
		case 248: 
		case 249: 
		case 250:
			/* +ve 2 byte number */
			val = (108 + 256 * (strng[i] - 247) + strng[i + 1]); 
			if (args == 0) 
			  arg0 = val;
			else
			  val += arg0;
			fprintf(OUTPUTBUFF,  "%ld ", val);
			args++;
			i += 2;
			break;
		case 251:
		case 252:
		case 253:
		case 254:
			/* -ve 2 byte number */
			val = (-108 - 256 * (strng[i] - 251) - strng[i + 1]); 
			if (args == 0) 
			  arg0 = val;
			else
			  val += arg0;
			fprintf(OUTPUTBUFF,  "%ld ", val);
			i += 2;
			args++;
			break;
		case 255:
			{
			/* 5 byte number */
			Int32 value = (Int32)strng[i + 1]<<24 | (Int32)strng[i + 2]<<16 |
				strng[i + 3]<<8 | strng[i + 4];
			if (args == 0) 
			  arg0 = value / 65536;
			else
			  value += arg0;
			fprintf(OUTPUTBUFF,  "%g ", value / 65536.0); 
			args++;
			i += 5;
			break;
			}
		default:
			/* 1 byte number */
			single = strng[i] - 139;
			if (args == 0) 
			  arg0 = single;
			else
			  single += arg0;
			fprintf(OUTPUTBUFF,  "%ld ", single); 
			args++;
			i++;
			break;
			}
		}
	}
