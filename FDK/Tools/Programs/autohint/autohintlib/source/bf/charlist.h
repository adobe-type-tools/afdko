/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

extern void AddCharListEntry(
char *, char *, long, long, boolean, boolean, boolean, boolean
);

extern boolean CheckCharListEntry(
char *, char *, boolean, boolean, boolean, boolean, indx
);

extern boolean NameInCharList (
char *
);

extern indx FindCharListEntry(
char *
);

/*
extern void makecharlist(
boolean, boolean, char *, indx
);*/

extern void sortcharlist(
boolean
);

extern boolean CompareCharListToBez (
char *, boolean, indx, boolean
);
/*
extern void CopyCharListToTable (
void
);*/

extern void CheckAllListInTable (
boolean
);

extern void FreeCharList (
void
);

extern void ResetCharListBools(
boolean, indx
);

