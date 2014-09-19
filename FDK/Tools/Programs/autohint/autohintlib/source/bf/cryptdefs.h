/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* cryptdefs.h
Original Version: Linda Gass January 4, 1987
Edit History:
Linda Gass: Fri Jul 17 15:16:59 1987
Linda Weinert: 3/10/87 - introduce ifndef
End Edit History.
*/

#include "basic.h"

#define MAXINT32 0x7fffffffL

#define BIN 0L
#define HEX 1L

#define OTHER 0
#define EEXEC 1
#define FONTPASSWORD 2

#define INLEN -1L

extern long DoContEncrypt(
    char *, FILE *, boolean, long
);

extern long ContEncrypt(char *indata, FILE  *outstream, boolean fileinput, long incount, boolean dblenc);

extern short DoInitEncrypt(
    FILE *, short, long, long, boolean
);

extern long DoEncrypt(
    char *, FILE *, boolean, long, short, long, long, boolean, boolean
);

extern long ContDecrypt(
    char *, char *, boolean, boolean, long, unsigned long
);

extern unsigned long ReadDecFile(
    FILE *, char *, char *, boolean, long, unsigned long, short
);

extern void SetLenIV (
    short
);
