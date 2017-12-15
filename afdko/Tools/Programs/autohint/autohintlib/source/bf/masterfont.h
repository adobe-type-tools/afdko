/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
/* masterfont.h */

#ifndef MASTERFONT_H
#define MASTERFONT_H


#define MAXDESIGNS 16 /* maximum number of base designs for a multiple master font. */

extern void BlendIntArray(
  int *, int, int, char *
);

extern void BlendFltArray(
  float *, int, int, char *
);

/* Returns the default weight value for this directory. */
extern float DefaultWeightForDir(
  int
);

extern void DefaultWeightVector(
  char *
);

extern void FreeDirEntries(
  void
);

extern void GetMasterDirName(
  char *, indx
);

extern int GetMasterDirIx(
  char *
);

extern short GetMasterFontList(
  char *
);

extern int InteriorDesignsExist(
  void
);

extern void PrintInputDirNames(
  void
);

extern void SetDefaultWeightVector(
  void
);

extern void SetMasterDir(
indx
);

extern int TotalWtVecs(
  void
);

extern void WritePhantomIntercepts(
FILE *
);

#endif /*MASTERFONT_H*/
