/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <string.h>

#include "FNAM.h"
#include "sfnt_FNAM.h"
#include "sfnt.h"

static FNAMTbl *FNAM = NULL;
static int loaded = 0;

void FNAMRead(int32_t start, uint32_t length) {
    int i;

    if (loaded)
        return;

    FNAM = (FNAMTbl *)sMemNew(sizeof(FNAMTbl));
    SEEK_ABS(start);

    IN1(FNAM->version);
    IN1(FNAM->nEncodings);

    FNAM->offset = sMemNew(sizeof(FNAM->offset[0]) * (FNAM->nEncodings + 1));
    for (i = 0; i < FNAM->nEncodings + 1; i++)
        IN1(FNAM->offset[i]);

    FNAM->encoding = sMemNew(sizeof(Encoding) * FNAM->nEncodings);
    for (i = 0; i < FNAM->nEncodings; i++) {
        int j;
        Encoding *encoding = &FNAM->encoding[i];
        uint32_t offset = FNAM->offset[i] + SIZEOF(Client, style);

        /* Count clients */
        for (encoding->nClients = 0; offset < FNAM->offset[i + 1];
             encoding->nClients++) {
            uint8_t record_length;

            SEEK_ABS(start + offset);

            IN1(record_length);
            offset += record_length + 1 + SIZEOF(Client, style);
        }

        SEEK_ABS(FNAM->offset[i] + start);

        encoding->client = sMemNew(sizeof(Client) * encoding->nClients);
        for (j = 0; j < encoding->nClients; j++) {
            uint8_t record_length;
            Client *client = &encoding->client[j];

            IN1(client->style);
            IN1(record_length);

            client->name = sMemNew(record_length + 1);

            IN_BYTES(record_length, client->name);
            client->name[record_length] = '\0';
        }
    }
    loaded = 1;
}

void FNAMDump(int level, int32_t start) {
    int i;

    DL(1, (OUTPUTBUFF, "### [FNAM] (%08lx)\n", start));

    DLV(2, "version   =", FNAM->version);
    DLu(2, "nEncodings=", FNAM->nEncodings);

    DL(2, (OUTPUTBUFF, "--- offset[index]=offset\n"));
    for (i = 0; i <= FNAM->nEncodings; i++)
        DL(2, (OUTPUTBUFF, "[%d]=%04hx ", i, FNAM->offset[i]));
    DL(2, (OUTPUTBUFF, "\n"));

    for (i = 0; i < FNAM->nEncodings; i++) {
        int j;
        Encoding *encoding = &FNAM->encoding[i];

        DL(2, (OUTPUTBUFF, "--- encoding[%d]\n", i));

        DL(2, (OUTPUTBUFF, "--- client[index]={style,length,name}\n"));
        for (j = 0; j < encoding->nClients; j++) {
            Client *client = &encoding->client[j];

            DL(2, (OUTPUTBUFF, "[%d]={%u,%lu,<%s>}\n", j, client->style,
                   STR_ARG(client->name)));
        }
    }
}

void FNAMFree(void) {
    int i;

    if (!loaded)
        return;

    for (i = 0; i < FNAM->nEncodings; i++) {
        int j;
        Encoding *encoding = &FNAM->encoding[i];

        for (j = 0; j < encoding->nClients; j++)
            sMemFree(encoding->client[j].name);
        sMemFree(encoding->client);
    }
    sMemFree(FNAM->encoding);
    sMemFree(FNAM->offset);
    sMemFree(FNAM);
    FNAM = NULL;
    loaded = 0;
}

/* Return FNAM->nEncodings */
int FNAMGetNEncodings(uint16_t *nEncodings, uint32_t client) {
    if (!loaded) {
        if (sfntReadTable(FNAM_))
            return tableMissing(FNAM_, client);
    }
    *nEncodings = FNAM->nEncodings;
    return 0;
}
