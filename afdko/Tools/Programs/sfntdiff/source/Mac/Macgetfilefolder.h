// generic.getfolder.h

#ifndef GENERIC_GETFOLDER_H
#define GENERIC_GETFOLDER_H

#if MACINTOSH
#include <Types.h>

Boolean MacGetFileFolder (Boolean onlyFolders, FSSpec *fileSpec);
#endif

#endif // GENERIC_GETFOLDER_H
