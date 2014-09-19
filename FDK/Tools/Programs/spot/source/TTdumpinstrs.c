/* ----------------------- TrueType Instruction Dump  ---------------------- */


#include <stdio.h>

#include "spot.h"

/* Dump instruction sequence to debug stream. */
void dumpInstrs(long length, char *buf)
	{
	static char *inames[] =
		{
		"SVTCA[0]",		/* 0x00 */
		"SVTCA[1]",		/* 0x01 */
		"SPVTCA[0]",	/* 0x02 */
		"SPVTCA[1]",	/* 0x03 */
		"SFVTCA[0]",	/* 0x04 */
		"SFVTCA[1]",	/* 0x05 */
		"SPVTL[0]",		/* 0x06 */
		"SPVTL[1]",		/* 0x07 */
		"SFVTL[0]",		/* 0x08 */
		"SFVTL[1]",		/* 0x09 */
		"SPVFS",		/* 0x0a */
		"SFVFS",		/* 0x0b */
		"GPV",			/* 0x0c */
		"GFV",			/* 0x0d */
		"SFVTPV",		/* 0x0e */
		"ISECT",		/* 0x0f */

		"SRP0",			/* 0x10 */
		"SRP1",			/* 0x11 */
		"SRP2",			/* 0x12 */
		"SZP0",			/* 0x13 */
		"SZP1",			/* 0x14 */
		"SZP2",			/* 0x15 */
		"SZPS",			/* 0x16 */
		"SLOOP",		/* 0x17 */
		"RTG",			/* 0x18 */
		"RTHG",			/* 0x19 */
		"SMD",			/* 0x1a */
		"ELSE",			/* 0x1b */
		"JMPR",			/* 0x1c */
		"SCVTCI",		/* 0x1d */
		"SSWCI",		/* 0x1e */
		"SSW",			/* 0x1f */

		"DUP",			/* 0x20 */
		"POP",			/* 0x21 */
		"CLEAR",		/* 0x22 */
		"SWAP",			/* 0x23 */
		"DEPTH",		/* 0x24 */
		"CINDEX",		/* 0x25 */
		"MINDEX",		/* 0x26 */
		"ALIGNPTS",		/* 0x27 */
		"RESERVED[28]",	/* 0x28 */
		"UTP",			/* 0x29 */
		"LOOPCALL",		/* 0x2a */
		"CALL",			/* 0x2b */
		"FDEF",			/* 0x2c */
		"ENDF",			/* 0x2d */
		"MDAP[0]",		/* 0x2e */
		"MDAP[1]",		/* 0x2f */

		"IUP[0]",		/* 0x30 */
		"IUP[1]",		/* 0x31 */
		"SHP[0]",		/* 0x32 */
		"SHP[1]",		/* 0x33 */
		"SHC[0]",		/* 0x34 */
		"SHC[1]",		/* 0x35 */
		"SHZ[0]",		/* 0x36 */
		"SHZ[1]",		/* 0x37 */
		"SHPIX",		/* 0x38 */
		"IP",			/* 0x39 */
		"RESERVED[3a]",	/* 0x3a */
		"RESERVED[3b]",	/* 0x3b */
		"ALIGNP",		/* 0x3c */
		"RTDG",			/* 0x3d */
		"MIAP[0]",		/* 0x3e */
		"MIAP[1]",		/* 0x3f */

		"NPUSHB",		/* 0x40 */
		"NPUSHW",		/* 0x41 */
		"WS",			/* 0x42 */
		"RS",			/* 0x43 */
		"WCVTF",		/* 0x44 */
		"RCVT",			/* 0x45 */
		"GC[0]",		/* 0x46 */
		"GC[1]",		/* 0x47 */
		"SCFS",			/* 0x48 */
		"MD[0]",		/* 0x49 */
		"MD[1]",		/* 0x4a */
		"MPPEM",		/* 0x4b */
		"MPS",			/* 0x4c */
		"FLIPON",		/* 0x4d */
		"FLIPOFF",		/* 0x4e */
		"DEBUG",		/* 0x4f */

		"LT",			/* 0x50 */
		"LTEQ",			/* 0x51 */
		"GT",			/* 0x52 */
		"GTEQ",			/* 0x53 */
		"EQ",			/* 0x54 */
		"NEQ",			/* 0x55 */
		"ODD",			/* 0x56 */
		"EVEN",			/* 0x57 */
		"IF",			/* 0x58 */
		"EIF",			/* 0x59 */
		"AND",			/* 0x5a */
		"OR",			/* 0x5b */
		"NOT",			/* 0x5c */
		"DELTAP1",		/* 0x5d */
		"SDB",			/* 0x5e */
		"SDS",			/* 0x5f */

		"ADD",			/* 0x60 */
		"SUB",			/* 0x61 */
		"DIV",			/* 0x62 */
		"MUL",			/* 0x63 */
		"ABS",			/* 0x64 */
		"NEG",			/* 0x65 */
		"FLOOR",		/* 0x66 */
		"CEILING",		/* 0x67 */
		"ROUND[00]",	/* 0x68 */
		"ROUND[01]",	/* 0x69 */
		"ROUND[10]",	/* 0x6a */
		"ROUND[11]",	/* 0x6b */
		"NROUND[00]",	/* 0x6c */
		"NROUND[01]",	/* 0x6d */
		"NROUND[10]",	/* 0x6e */
		"NROUND[11]",	/* 0x6f */

		"WCVTF",		/* 0x70 */
		"DELTAP2",		/* 0x71 */
		"DELTAP3",		/* 0x72 */
		"DELTAC1",		/* 0x73 */
		"DELTAC2",		/* 0x74 */
		"DELTAC3",		/* 0x75 */
		"SROUND",		/* 0x76 */
		"S45ROUND",		/* 0x77 */
		"JROT",			/* 0x78 */
		"JROF",			/* 0x79 */
		"ROFF",			/* 0x7a */
		"RESERVED[7b]",	/* 0x7b */
		"RUTG",			/* 0x7c */
		"RDTG",			/* 0x7d */
		"SANGW",		/* 0x7e */
		"AA",			/* 0x7f */

		"FLIPPT",		/* 0x80 */
		"FLIPRGON",		/* 0x81 */
		"FLIPRGOFF",	/* 0x82 */
		"RESERVED[83]",	/* 0x83 */
		"RESERVED[84]",	/* 0x84 */
		"SCANCTRL",		/* 0x85 */
		"SDPVTL[0]",	/* 0x86 */
		"SDPVTL[1]",	/* 0x87 */
		"GETINFO",		/* 0x88 */
		"IDEF",			/* 0x89 */
		"ROLL",			/* 0x8a */
		"MAX",			/* 0x8b */
		"MIN",			/* 0x8c */
		"SCANTYPE",		/* 0x8d */
		"RESERVED[8e]",	/* 0x8e */
		"RESERVED[8f]",	/* 0x8f */

		"RESERVED[90]",	/* 0x90 */
		"RESERVED[91]",	/* 0x91 */
		"RESERVED[92]",	/* 0x92 */
		"RESERVED[93]",	/* 0x93 */
		"RESERVED[94]",	/* 0x94 */
		"RESERVED[95]",	/* 0x95 */
		"RESERVED[96]",	/* 0x96 */
		"RESERVED[97]",	/* 0x97 */
		"RESERVED[98]",	/* 0x98 */
		"RESERVED[99]",	/* 0x99 */
		"RESERVED[9a]",	/* 0x9a */
		"RESERVED[9b]",	/* 0x9b */
		"RESERVED[9c]",	/* 0x9c */
		"RESERVED[9d]",	/* 0x9d */
		"RESERVED[9e]",	/* 0x9e */
		"RESERVED[9f]",	/* 0x9f */

		"RESERVED[a0]",	/* 0xa0 */
		"RESERVED[a1]",	/* 0xa1 */
		"RESERVED[a2]",	/* 0xa2 */
		"RESERVED[a3]",	/* 0xa3 */
		"RESERVED[a4]",	/* 0xa4 */
		"RESERVED[a5]",	/* 0xa5 */
		"RESERVED[a6]",	/* 0xa6 */
		"RESERVED[a7]",	/* 0xa7 */
		"RESERVED[a8]",	/* 0xa8 */
		"RESERVED[a9]",	/* 0xa9 */
		"RESERVED[aa]",	/* 0xaa */
		"RESERVED[ab]",	/* 0xab */
		"RESERVED[ac]",	/* 0xac */
		"RESERVED[ad]",	/* 0xad */
		"RESERVED[ae]",	/* 0xae */
		"RESERVED[af]",	/* 0xaf */

		"PUSHB[000]",	/* 0xb0 */
		"PUSHB[001]",	/* 0xb1 */
		"PUSHB[010]",	/* 0xb2 */
		"PUSHB[011]",	/* 0xb3 */
		"PUSHB[100]",	/* 0xb4 */
		"PUSHB[101]",	/* 0xb5 */
		"PUSHB[110]",	/* 0xb6 */
		"PUSHB[111]",	/* 0xb7 */
		"PUSHW[000]",	/* 0xb8 */
		"PUSHW[001]",	/* 0xb9 */
		"PUSHW[010]",	/* 0xba */
		"PUSHW[011]",	/* 0xbb */
		"PUSHW[100]",	/* 0xbc */
		"PUSHW[101]",	/* 0xbd */
		"PUSHW[110]",	/* 0xbe */
		"PUSHW[111]",	/* 0xbf */

		"MDRP[00000]",	/* 0xc0 */
		"MDRP[00001]",	/* 0xc1 */
		"MDRP[00010]",	/* 0xc2 */
		"MDRP[00011]",	/* 0xc3 */
		"MDRP[00100]",	/* 0xc4 */
		"MDRP[00101]",	/* 0xc5 */
		"MDRP[00110]",	/* 0xc6 */
		"MDRP[00111]",	/* 0xc7 */
		"MDRP[01000]",	/* 0xc8 */
		"MDRP[01001]",	/* 0xc9 */
		"MDRP[01010]",	/* 0xca */
		"MDRP[01011]",	/* 0xcb */
		"MDRP[01100]",	/* 0xcc */
		"MDRP[01101]",	/* 0xcd */
		"MDRP[01110]",	/* 0xce */
		"MDRP[01111]",	/* 0xcf */

		"MDRP[10000]",	/* 0xd0 */
		"MDRP[10001]",	/* 0xd1 */
		"MDRP[10010]",	/* 0xd2 */
		"MDRP[10011]",	/* 0xd3 */
		"MDRP[10100]",	/* 0xd4 */
		"MDRP[10101]",	/* 0xd5 */
		"MDRP[10110]",	/* 0xd6 */
		"MDRP[10111]",	/* 0xd7 */
		"MDRP[10000]",	/* 0xd8 */
		"MDRP[10001]",	/* 0xd9 */
		"MDRP[10010]",	/* 0xda */
		"MDRP[10011]",	/* 0xdb */
		"MDRP[10100]",	/* 0xdc */
		"MDRP[10101]",	/* 0xdd */
		"MDRP[10110]",	/* 0xde */
		"MDRP[10111]",	/* 0xdf */

		"MIRP[00000]",	/* 0xe0 */
		"MIRP[00001]",	/* 0xe1 */
		"MIRP[00010]",	/* 0xe2 */
		"MIRP[00011]",	/* 0xe3 */
		"MIRP[00100]",	/* 0xe4 */
		"MIRP[00101]",	/* 0xe5 */
		"MIRP[00110]",	/* 0xe6 */
		"MIRP[00111]",	/* 0xe7 */
		"MIRP[01000]",	/* 0xe8 */
		"MIRP[01001]",	/* 0xe9 */
		"MIRP[01010]",	/* 0xea */
		"MIRP[01011]",	/* 0xeb */
		"MIRP[01100]",	/* 0xec */
		"MIRP[01101]",	/* 0xed */
		"MIRP[01110]",	/* 0xee */
		"MIRP[01111]",	/* 0xef */

		"MIRP[10000]",	/* 0xf0 */
		"MIRP[10001]",	/* 0xf1 */
		"MIRP[10010]",	/* 0xf2 */
		"MIRP[10011]",	/* 0xf3 */
		"MIRP[10100]",	/* 0xf4 */
		"MIRP[10101]",	/* 0xf5 */
		"MIRP[10110]",	/* 0xf6 */
		"MIRP[10111]",	/* 0xf7 */
		"MIRP[10000]",	/* 0xf8 */
		"MIRP[10001]",	/* 0xf9 */
		"MIRP[10010]",	/* 0xfa */
		"MIRP[10011]",	/* 0xfb */
		"MIRP[10100]",	/* 0xfc */
		"MIRP[10101]",	/* 0xfd */
		"MIRP[10110]",	/* 0xfe */
		"MIRP[10111]",	/* 0xff */
		};
	unsigned char *next = (unsigned char *)buf;
	unsigned char *end = next + length;
	while (next < end)
		{
		int count;
		unsigned op = *next++;
		fprintf(OUTPUTBUFF, "%s", inames[op]);

		switch (op)
			{
		case 0x40:
			/* NPUSHB */
			count = *next++;
			fprintf(OUTPUTBUFF, " %02x", count);
			while (count--)
				fprintf(OUTPUTBUFF, " %02x", *next++);
			break;
		case 0x41:
			/* NPUSHW */
			count = *next++;
			fprintf(OUTPUTBUFF, " %02x", count);
			while (count--)
				{
				fprintf(OUTPUTBUFF, " %04hx", (unsigned short)(next[0]<<8 | next[1]));
				next += 2;
				}
			break;
		case 0xb0: case 0xb1: case 0xb2: case 0xb3: 
		case 0xb4: case 0xb5: case 0xb6: case 0xb7: 
			/* PUSHB ops */
			count = op - 0xaf;
			while (count--)
				fprintf(OUTPUTBUFF, " %02x", *next++);
			break;
		case 0xb8: case 0xb9: case 0xba: case 0xbb: 
		case 0xbc: case 0xbd: case 0xbe: case 0xbf: 
			/* PUSHW ops */
			count = op - 0xb7;
			while (count--)
				{
				fprintf(OUTPUTBUFF, " %04hx", (unsigned short)(next[0]<<8 | next[1]));
				next += 2;
				}
			break;
			}
		fprintf(OUTPUTBUFF, "\n");
		}
	}



