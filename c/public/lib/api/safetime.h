/*
Copyright 2019 Adobe. All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0.
This license is available at: http://opensource.org/licenses/Apache-2.0.
*/

#ifndef SAFETIME_H
#define SAFETIME_H

#ifdef _WIN32
#define SAFE_LOCALTIME(x, y)    localtime_s(y, x)
#define LOCALTIME_FAILURE(x)    (x != 0)
#define SAFE_CTIME(x, y)        ctime_s(y, sizeof(y), x)
#define SAFE_GMTIME(x, y)       gmtime_s(y, x)
#else
#define SAFE_LOCALTIME(x, y)    localtime_r(x, y)
#define LOCALTIME_FAILURE(x)    (x == NULL)
#define SAFE_CTIME(x, y)        ctime_r(x, y)
#define SAFE_GMTIME(x, y)       gmtime_r(x, y)
#endif /* WIN32 */

#endif /* SAFETIME_H */
