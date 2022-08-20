/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <ctype.h>

#include "name.h"
#include "desc.h"
#include "sfnt_name.h"
#include "sfnt.h"

static nameTbl *name = NULL;
static int loaded = 0;

void nameRead(int32_t start, uint32_t length) {
    int i;
    int nameSize;

    if (loaded)
        return;

    name = (nameTbl *)sMemNew(sizeof(nameTbl));
    SEEK_ABS(start);

    /* Read header */
    IN1(name->format);
    IN1(name->count);
    IN1(name->stringOffset);

    /* Read name records */
    name->record = sMemNew(sizeof(NameRecord) * name->count);
    for (i = 0; i < name->count; i++) {
        NameRecord *record = &name->record[i];

        IN1(record->platformId);
        IN1(record->scriptId);
        IN1(record->languageId);
        IN1(record->nameId);
        IN1(record->length);
        IN1(record->offset);
    }

    if (name->format == 1) {
        IN1(name->langTagCount);
        IN1(name->langTagCount);
        /* Read lang tag records */
        name->langTag = sMemNew(sizeof(LangTagRecord) * name->langTagCount);
        for (i = 0; i < name->langTagCount; i++) {
            LangTagRecord *langTagRecord = &name->langTag[i];
            IN1(langTagRecord->length);
            IN1(langTagRecord->offset);
        }
    }

    /* Read the string table */
    nameSize = length - name->stringOffset;
    name->strings = sMemNew(nameSize);
    SEEK_ABS(start + name->stringOffset);
    IN_BYTES(nameSize, name->strings);

    loaded = 1;
}

/* Dump string */
static void dumpString(NameRecord *record, int level) {
    uint8_t *p = &name->strings[record->offset];
    uint8_t *end = p + record->length;
    int twoByte = record->platformId == 0 || record->platformId == 3;
    int precision = twoByte ? 4 : 2;

    DL(3, (OUTPUTBUFF, "[%04hx]=<", record->offset));
    while (p < end) {
        uint32_t code = *p++;

        if (twoByte)
            code = code << 8 | *p++;

        if ((code & 0xff00) == 0 && isprint(code))
            DL(3, (OUTPUTBUFF, "%c", (int)code));
        else
            DL(3, (OUTPUTBUFF, "\\%0*x", precision, code));
    }
    DL(3, (OUTPUTBUFF, ">\n"));
}

/* Dump LangTag string */
static void dumpLanguageTagString(LangTagRecord *langTagRec, int level) {
    uint8_t *p = &name->strings[langTagRec->offset];
    uint8_t *end = p + langTagRec->length;
    int precision = 4;

    DL(3, (OUTPUTBUFF, "[%04hx]=<", langTagRec->offset));
    while (p < end) {
        uint32_t code = *p++;

        code = code << 8 | *p++;

        if ((code & 0xff00) == 0 && isprint(code))
            DL(3, (OUTPUTBUFF, "%c", (int)code));
        else
            DL(3, (OUTPUTBUFF, "\\%0*x", precision, code));
    }
    DL(3, (OUTPUTBUFF, ">\n"));
}

static void makeString(NameRecord *record, int8_t *str) {
    uint8_t *p = &name->strings[record->offset];
    uint8_t *end = p + record->length;
    int twoByte = record->platformId == 0 || record->platformId == 3;
    int8_t *strptr = str;
    int len = 0;

    while ((p < end) && (len < record->length)) {
        uint32_t code = *p++;

        if (twoByte)
            code = code << 8 | *p++;

        if ((code & 0xff00) == 0 && isprint(code)) {
            *strptr++ = (int8_t)code;
            len++;
        }
    }
    str[len] = '\0';
}

void nameDump(int level, int32_t start) {
    int i;
    int lastOffset;
    int lastLength;
    if (level == 3) {
        DL(2, (OUTPUTBUFF, "### [name] \n"));

        /* Dump string table (unique strings only) */
        lastOffset = -1;
        lastLength = -1;
        DL(2, (OUTPUTBUFF,
               "--- record[index]="
               "{platformId,scriptId,languageId,nameId,length,offset} = <name value>\n"));
        for (i = 0; i < name->count; i++) {
            NameRecord *record = &name->record[i];
            DL(2, (OUTPUTBUFF, "[%2d]={%2hx,%2hx,%4hx,%4hu,%4hu,%04hx} ", i,
                   record->platformId, record->scriptId, record->languageId,
                   record->nameId, record->length, record->offset));

            dumpString(record, level);
            lastOffset = record->offset;
            lastLength = record->length;
            if (LANG_TAG_BASE_INDEX & record->languageId) {
                unsigned langIndex = record->languageId - LANG_TAG_BASE_INDEX;
                LangTagRecord *langTagRec;
                if (langIndex > name->langTagCount) {
                    DL(1, (OUTPUTBUFF, "Error: name index '%d' references lang tag record index 'd', but there are only '%d' Language Tag records.\n", langIndex, name->langTagCount));
                    continue;
                }
                langTagRec = &name->langTag[langIndex];
                DL(2, (OUTPUTBUFF, "\tLanguage Tag Index[%d]={%04hx} ", langIndex, langTagRec->offset));
                dumpLanguageTagString(langTagRec, level);
            }
        }
    } else {
        DL(1, (OUTPUTBUFF, "### [name] (%08lx)\n", start));

        /* Dump header */
        DLu(2, "format      =", name->format);
        DLu(2, "count       =", name->count);
        if (name->format == 1)
            DLu(2, "langTagCount       =", name->langTagCount);

        DLx(2, "stringOffset=", name->stringOffset);

        /* Dump name records */
        DL(2, (OUTPUTBUFF,
               "--- record[index]="
               "{platformId,scriptId,languageId,nameId,length,offset}\n"));
        for (i = 0; i < name->count; i++) {
            NameRecord *record = &name->record[i];

            DL(2, (OUTPUTBUFF, "[%2d]={%2hx,%2hx,%4hx,%4hu,%4hu,%04hx} ", i,
                   record->platformId, record->scriptId, record->languageId,
                   record->nameId, record->length, record->offset));
            DL(4, (OUTPUTBUFF, "[%s,%s,%s,%s]",
                   descPlat(record->platformId),
                   descScript(record->platformId, record->scriptId),
                   descLang(0, record->platformId, record->languageId),
                   descName(record->nameId)));
            DL(2, (OUTPUTBUFF, "\n"));
        }

        /* Dump lang tag records */
        DL(2, (OUTPUTBUFF, "--- LangTag[index]={length,offset}\n"));
        for (i = 0; i < name->langTagCount; i++) {
            LangTagRecord *langTagRec = &name->langTag[i];

            DL(2, (OUTPUTBUFF, "[%2d]={%4hu, %04hx} ", i, langTagRec->length, langTagRec->offset));
            DL(2, (OUTPUTBUFF, "\n"));
        }

        /* Dump string table (unique strings only) */
        DL(2, (OUTPUTBUFF, "--- string[name string offset]=<string>\n"));
        lastOffset = -1;
        lastLength = -1;
        for (i = 0; i < name->count; i++) {
            NameRecord *record = &name->record[i];

            if (lastOffset != record->offset || lastLength != record->length) {
                dumpString(record, level);
                lastOffset = record->offset;
                lastLength = record->length;
            }
        }

        /* Dump lang tag strings in string table */
        DL(2, (OUTPUTBUFF, "--- string[lang tag string offset]=<string>\n"));
        for (i = 0; i < name->langTagCount; i++) {
            LangTagRecord *record;
            if (i > name->langTagCount) {
                DL(1, (OUTPUTBUFF, "Error: name index '%d' references lang tag record index 'd', but there are only '%d' Language Tag records.\n", i, name->langTagCount));
                continue;
            }
            record = &name->langTag[i];
            dumpLanguageTagString(record, level);
        }
    }
}

void nameFree_spot(void) {
    if (!loaded)
        return;

    sMemFree(name->strings);
    sMemFree(name->record);
    sMemFree(name);
    name = NULL;
    loaded = 0;
}

void nameUsage(void) {
    fprintf(OUTPUTBUFF, "--- name\n");
    fprintf(OUTPUTBUFF, "=2  raw field and table dump.\n");
    fprintf(OUTPUTBUFF, "=3  print single line for all name record values:\n");
    fprintf(OUTPUTBUFF, "          record[index]={platformId,scriptId,languageId,nameId,length,offset} = <name value>\n");
    fprintf(OUTPUTBUFF, "=4  raw field and table dump, but with descriptive names for some field values.\n");
}

int8_t *nameFontName(void) {
    int i;
    int8_t *fullname = NULL;

    if (!loaded) {
        if (sfntReadTable(name_))
            return NULL;
    }

    for (i = 0; i < name->count; i++) {
        NameRecord *record = &name->record[i];
        if (record->nameId == 4) /* Full Name */
        {
            if ((record->languageId == 0) ||
                (record->languageId == 0x409)) /* English */
            {
                if (fullname) sMemFree(fullname);
                fullname = sMemNew(sizeof(int8_t) * (record->length + 1));
                fullname[0] = '\0';
                makeString(record, fullname);
                if ((record->platformId == 1 /*Mac*/ && record->scriptId == 0 /*Roman*/) || (record->platformId == 3 /*Win*/ && record->scriptId == 1 /*Unicode*/))
                    break;
            }
        }
    }

    return fullname;
}

int8_t *namePostScriptName(void) {
    int i;
    int8_t *psname = NULL;

    if (!loaded) {
        if (sfntReadTable(name_))
            return NULL;
    }

    for (i = 0; i < name->count; i++) {
        NameRecord *record = &name->record[i];
        if (record->nameId == 6) /* PostScript Name */
        {
            if ((record->languageId == 0) ||
                (record->languageId == 0x409)) /* English */
            {
                if (!psname) sMemFree(psname);
                psname = sMemNew(sizeof(int8_t) * (record->length + 1));
                psname[0] = '\0';
                makeString(record, psname);
                if ((record->platformId == 1 /*Mac*/ && record->scriptId == 0 /*Roman*/) || (record->platformId == 3 /*Win*/ && record->scriptId == 1 /*Unicode*/))
                    break;
            }
        }
    }

    return psname;
}
