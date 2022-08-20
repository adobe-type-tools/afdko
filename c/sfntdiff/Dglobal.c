/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Miscellaneous functions and system calls.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "Dglobal.h"
#include "Dsfnt.h"
#include "Dhead.h"
#include "Dname.h"

int level = 0;
int DiffExists = 0;

static int nameLookupType1 = 0;
static int nameLookupType2 = 0;
static char *dateFormat = "%a %b %d %H:%M:%S %Y";

#if WIN32
#define SAFE_LOCALTIME(x, y) localtime_s(y, x)
#define LOCALTIME_FAILURE(x) (x != 0)
#else
#define SAFE_LOCALTIME(x, y) localtime_r(x, y)
#define LOCALTIME_FAILURE(x) (x == NULL)
#endif


/* Warn of missing table */
int sdTableMissing(uint32_t table, uint32_t client) {
    sdWarning("%c%c%c%c can't read %c%c%c%c because table missing\n",
            TAG_ARG(client), TAG_ARG(table));
    return 1;
}

/* ### Miscellaneous */

static char tday[32];

char *fileModTimeString(uint8_t which, const char *fname) {
    char *str;
    struct stat file_stat;

    str = sdHeadGetModifiedDate(which, TAG('f', 'i', 'l', 'e'));
    if ((str != NULL) && (str[0] != '\0'))
        return str;
    else {
        struct tm local_time;
        tday[0] = '\0';

        stat(fname, &file_stat);
        SAFE_LOCALTIME(&(file_stat.st_mtime), &local_time);
        if (strftime(tday, sizeof(tday), dateFormat, &local_time) == 0) {
            fprintf(stderr, "strftime returned 0");
            exit(EXIT_FAILURE);
        }
        return tday;
    }
}

char *ourtime(void) {
    time_t seconds_since_epoch;
    struct tm local_time;
    tday[0] = '\0';
    time(&seconds_since_epoch);
    if (LOCALTIME_FAILURE(SAFE_LOCALTIME(&seconds_since_epoch, &local_time))) {
        perror("localtime failed");
        exit(EXIT_FAILURE);
    }

    if (strftime(tday, sizeof(tday), dateFormat, &local_time) == 0) {
        fprintf(stderr, "strftime returned 0");
        exit(EXIT_FAILURE);
    }
    return tday;
}
