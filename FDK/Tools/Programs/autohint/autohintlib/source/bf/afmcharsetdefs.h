
#ifndef AFMCHARSETDEFS_H
#define AFMCHARSETDEFS_H

#include <stdio.h>

#define AFMCHARSETTABLE "AFMcharset.tbl"

struct FItoAFMCharSetMap {
    struct FItoAFMCharSetMap *next;
    char *fiName;
    char *afmName;
};

struct AFMCharSetTbl {
  char *defaultCSVal;
  struct FItoAFMCharSetMap *charSetMap;
};

extern struct AFMCharSetTbl *AFMCharSet_Parse(FILE *file);
extern void FreeAFMCharSetTbl ();
extern char *GetFItoAFMCharSetName(void);

#endif
