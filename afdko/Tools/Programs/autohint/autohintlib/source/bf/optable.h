/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/


#define ESCVAL 100
#define MAXOPLEN 5

#ifdef SUN
extern char beginsubr[10], endsubr[8], preflx[7], newcolors[10];
#endif

extern void GetOperator(
  short, char *
);

extern short op_known(
    char *
);

extern void init_ops(
    void
);

extern void freeoptable (
    void
);
