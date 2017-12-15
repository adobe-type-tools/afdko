/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/
#ifndef MASTERFONTPRIV_H
#define MASTERFONTPRIV_H

typedef struct _MasterDesignPosit {
  char *MasterDirName;
  float posit[4];
} MasterDesignPosit;


extern void GetMasterDesignPositions(MasterDesignPosit **MDP, int *nummast, int *numaxes);

#endif  /* MASTERFONTPRIV_H */
