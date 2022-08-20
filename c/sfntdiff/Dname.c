/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <ctype.h>

#include "Dname.h"
#include "desc.h"
#include "sfnt_name.h"

#include "Dsfnt.h"

static nameTbl name1;
static nameTbl name2;
static int loaded1 = 0;
static int loaded2 = 0;

void sdNameReset() {
    loaded1 = loaded2 = 0;
}

void sdNameRead(uint8_t which, int32_t start, uint32_t length) {
    int i;
    int nameSize;
    nameTbl *name = NULL;

    if (which == 1) {
        if (loaded1)
            return;
        else
            name = &name1;
    } else if (which == 2) {
        if (loaded2)
            return;
        else
            name = &name2;
    }

    SEEK_ABS(which, start);

    /* Read header */
    IN(which, name->format);
    IN(which, name->count);
    IN(which, name->stringOffset);

    /* Read name records */
    name->record = sMemNew(sizeof(NameRecord) * name->count);
    for (i = 0; i < name->count; i++) {
        NameRecord *record = &name->record[i];

        IN(which, record->platformId);
        IN(which, record->scriptId);
        IN(which, record->languageId);
        IN(which, record->nameId);
        IN(which, record->length);
        IN(which, record->offset);
    }

    /* Read the string table */
    nameSize = length - name->stringOffset;
    name->strings = sMemNew(nameSize);
    SEEK_ABS(which, start + name->stringOffset);
    IN_BYTES(which, nameSize, name->strings);

    if (which == 1)
        loaded1 = 1;
    else if (which == 2)
        loaded2 = 1;
}

static NameRecord *findNameRecord(uint8_t inwhich,
                                  uint16_t platformId,
                                  uint16_t scriptId,
                                  uint16_t languageId,
                                  uint16_t nameId, int *where) {
    nameTbl *name = NULL;
    int i;

    if (inwhich == 1)
        name = &name1;
    else if (inwhich == 2)
        name = &name2;

    for (i = 0; i < name->count; i++) {
        NameRecord *rec = &name->record[i];
        if (rec->platformId != platformId)
            continue;
        if (rec->scriptId != scriptId)
            continue;
        if (rec->languageId != languageId)
            continue;
        if (rec->nameId != nameId)
            continue;
        /* seems to have succeeded */
        *where = i;
        return rec;
    }
    *where = (-1);
    return NULL;
}

static int nameRecordsDiffer(NameRecord *rec1, NameRecord *rec2) {
    uint8_t *p1, *p2;
    uint8_t *end1, *end2;
    int twoByte;

    if (rec1->platformId != rec2->platformId)
        return 1;
    if (rec1->scriptId != rec2->scriptId)
        return 1;
    if (rec1->languageId != rec2->languageId)
        return 1;
    if (rec1->nameId != rec2->nameId)
        return 1;
    if (rec1->length != rec2->length)
        return 1;

    twoByte = (rec1->platformId == 0 || rec1->platformId == 3);
    p1 = &name1.strings[rec1->offset];
    end1 = p1 + rec1->length;
    p2 = &name2.strings[rec2->offset];
    end2 = p2 + rec2->length;

    while ((p1 < end1) && (p2 < end2)) {
        uint32_t code1 = *p1++;
        uint32_t code2 = *p2++;
        if (twoByte) {
            code1 = code1 << 8 | *p1++;
            code2 = code2 << 8 | *p2++;
        }
        if (code1 != code2)
            return 1;
    }

    return 0;
}

/* Dump string */
static void dumpString(uint8_t which, NameRecord *record) {
    uint8_t *p;
    uint8_t *end;
    int twoByte = record->platformId == 0 || record->platformId == 3;
    int precision = twoByte ? 4 : 2;

    p = (which == 1) ? &name1.strings[record->offset] : &name2.strings[record->offset];
    end = p + record->length;

    sdNote(" \"");
    while (p < end) {
        uint32_t code = *p++;

        if (twoByte)
            code = code << 8 | *p++;

        if ((code & 0xff00) == 0 && ISPRINT(code))
            sdNote("%c", (int)code);
        else
            sdNote("\\%0*x", precision, code);
    }
    sdNote("\"");
}

static void makeString(uint8_t which, NameRecord *record, int8_t *str) {
    uint8_t *p;
    uint8_t *end;
    int twoByte = (record->platformId == 0 || record->platformId == 3);
    int8_t *strptr = str;
    int len = 0;

    p = (which == 1) ? &name1.strings[record->offset] : &name2.strings[record->offset];
    end = p + record->length;

    while ((p < end) && (len < record->length)) {
        uint32_t code = *p++;

        if (twoByte)
            code = code << 8 | *p++;

        if ((code & 0xff00) == 0 && ISPRINT(code)) {
            *strptr++ = (int8_t)code;
            len++;
        }
    }
    str[len] = '\0';
}

void sdNameDiff(int32_t offset1, int32_t offset2) {
    int i;

    if (name1.format != name2.format) {
        DiffExists++;
        sdNote("< name format=%hd\n", name1.format);
        sdNote("> name format=%hd\n", name2.format);
    }
    if (name1.count != name2.count) {
        DiffExists++;
        sdNote("< name count=%hd\n", name1.count);
        sdNote("> name count=%hd\n", name2.count);
    }

    if (name1.count >= name2.count) {
        for (i = 0; i < name1.count; i++) {
            NameRecord *record1 = &name1.record[i];
            NameRecord *record2;
            int where;
            record2 = findNameRecord(2,
                                     record1->platformId, record1->scriptId,
                                     record1->languageId, record1->nameId, &where);
            if ((record2 == NULL) || (where < 0)) {
                DiffExists++;
                if (level > 3) {
                    sdNote("< name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
                         i,
                         record1->platformId, record1->scriptId,
                         record1->languageId, record1->nameId,
                         record1->length, record1->offset);
                    sdNote("[%s,%s,%s,%s]",
                         descPlat(record1->platformId),
                         descScript(record1->platformId, record1->scriptId),
                         descLang(0, record1->platformId, record1->languageId),
                         descName(record1->nameId));
                } else {
                    sdNote("< name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
                         i,
                         record1->platformId, record1->scriptId,
                         record1->languageId, record1->nameId);
                }
                dumpString(1, record1);
                sdNote("\n");
                sdNote("> missing name record corresponding to [%2d]\n", i);
            } else if (nameRecordsDiffer(record1, record2)) {
                DiffExists++;
                if (level > 3) {
                    sdNote("< name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
                         i,
                         record1->platformId, record1->scriptId,
                         record1->languageId, record1->nameId,
                         record1->length, record1->offset);
                    sdNote("[%s,%s,%s,%s]",
                         descPlat(record1->platformId),
                         descScript(record1->platformId, record1->scriptId),
                         descLang(0, record1->platformId, record1->languageId),
                         descName(record1->nameId));
                } else {
                    sdNote("< name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
                         i,
                         record1->platformId, record1->scriptId,
                         record1->languageId, record1->nameId);
                }
                dumpString(1, record1);
                sdNote("\n");
                if (level > 3) {
                    sdNote("> name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
                         where,
                         record2->platformId, record2->scriptId,
                         record2->languageId, record2->nameId,
                         record2->length, record2->offset);
                    sdNote("[%s,%s,%s,%s]",
                         descPlat(record2->platformId),
                         descScript(record2->platformId, record2->scriptId),
                         descLang(0, record2->platformId, record2->languageId),
                         descName(record2->nameId));
                } else {
                    sdNote("> name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
                         where,
                         record2->platformId, record2->scriptId,
                         record2->languageId, record2->nameId);
                }
                dumpString(2, record2);
                sdNote("\n");
            }
        }
    } else {
        for (i = 0; i < name2.count; i++) {
            NameRecord *record2 = &name2.record[i];
            NameRecord *record1;
            int where;
            record1 = findNameRecord(1,
                                     record2->platformId, record2->scriptId,
                                     record2->languageId, record2->nameId, &where);
            if ((record1 == NULL) || (where < 0)) {
                DiffExists++;
                sdNote("< missing name record corresponding to [%2d]\n", i);
                if (level > 3) {
                    sdNote("> name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
                         i,
                         record2->platformId, record2->scriptId,
                         record2->languageId, record2->nameId,
                         record2->length, record2->offset);
                    sdNote("[%s,%s,%s,%s]",
                         descPlat(record2->platformId),
                         descScript(record2->platformId, record2->scriptId),
                         descLang(0, record2->platformId, record2->languageId),
                         descName(record2->nameId));
                } else {
                    sdNote("> name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
                         i,
                         record2->platformId, record2->scriptId,
                         record2->languageId, record2->nameId);
                }
                dumpString(2, record2);
                sdNote("\n");
            } else if (nameRecordsDiffer(record2, record1)) {
                DiffExists++;
                if (level > 3) {
                    sdNote("< name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
                         i,
                         record1->platformId, record1->scriptId,
                         record1->languageId, record1->nameId,
                         record1->length, record1->offset);
                    sdNote("[%s,%s,%s,%s]",
                         descPlat(record1->platformId),
                         descScript(record1->platformId, record1->scriptId),
                         descLang(0, record1->platformId, record1->languageId),
                         descName(record1->nameId));
                } else {
                    sdNote("< name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
                         i,
                         record1->platformId, record1->scriptId,
                         record1->languageId, record1->nameId);
                }
                dumpString(1, record1);
                sdNote("\n");
                if (level > 3) {
                    sdNote("> name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
                         where,
                         record2->platformId, record2->scriptId,
                         record2->languageId, record2->nameId,
                         record2->length, record2->offset);
                    sdNote("[%s,%s,%s,%s]",
                         descPlat(record2->platformId),
                         descScript(record2->platformId, record2->scriptId),
                         descLang(0, record2->platformId, record2->languageId),
                         descName(record2->nameId));
                } else {
                    sdNote("> name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
                         where,
                         record2->platformId, record2->scriptId,
                         record2->languageId, record2->nameId);
                }
                dumpString(2, record2);
                sdNote("\n");
            }
        }
    }
}

void sdNameFree(uint8_t which) {
    nameTbl *name = NULL;

    if (which == 1) {
        if (!loaded1) {
            return;
        } else {
            name = &name1;
        }
    } else if (which == 2) {
        if (!loaded2) {
            return;
        } else {
            name = &name2;
        }
    }
    sMemFree(name->strings);
    sMemFree(name->record);

    if (which == 1)
        loaded1 = 0;
    else if (which == 2)
        loaded2 = 0;
}

#define CHECKREADASSIGN                         \
    if (which == 1) {                           \
        if (!loaded1) {                         \
            if (sdSfntReadATable(which, name_)) { \
                sdTableMissing(name_, name_);   \
                goto out;                       \
            }                                   \
        }                                       \
        name = &name1;                          \
    } else if (which == 2) {                    \
        if (!loaded2) {                         \
            if (sdSfntReadATable(which, name_)) { \
                sdTableMissing(name_, name_);   \
                goto out;                       \
            }                                   \
        }                                       \
        name = &name2;                          \
    }
