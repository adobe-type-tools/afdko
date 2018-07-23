/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)Dname.c	1.2
 * Changed:    7/1/98 10:35:34
 ***********************************************************************/

#include <ctype.h>

#include "Dname.h"
#include "Ddesc.h"
#include "sfnt_name.h"

#include "Dsfnt.h"

static nameTbl name1;
static nameTbl name2;
static IntX loaded1 = 0;
static IntX loaded2 = 0;

void nameReset()
{
	loaded1=loaded2=0;
}

void nameRead(Card8 which, LongN start, Card32 length)
	{
	IntX i;
	IntX nameSize;
	nameTbl *name;

	if (which == 1)
	  {
		if (loaded1)	return;
		else name = &name1;
	  }
	else if (which == 2)
	  {
		if (loaded2)	return;
		else name = &name2;
	  }

	SEEK_ABS(which, start);


	/* Read header */
	IN(which, name->format);
	IN(which, name->count);
	IN(which, name->stringOffset);

	/* Read name records */
	name->record = memNew(sizeof(NameRecord) * name->count);
	for (i = 0; i < name->count; i++)
		{
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
	name->strings = memNew(nameSize);
	SEEK_ABS(which, start + name->stringOffset);
	IN_BYTES(which, nameSize, name->strings);

	if (which == 1) loaded1 = 1;
	else if (which == 2) loaded2 = 1;
	}

static NameRecord *findNameRecord(Card8 inwhich,  
								  Card16 platformId,
								  Card16 scriptId,
								  Card16 languageId,
								  Card16 nameId, IntX *where)
{
  nameTbl *name;
  IntX i;

  if (inwhich == 1)
	name = &name1;
  else if (inwhich == 2)
	name = &name2;

  for (i = 0; i < name->count; i++)
	{
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

static IntX nameRecordsDiffer(NameRecord *rec1, NameRecord *rec2)
{
  Card8 *p1, *p2;
  Card8 *end1, *end2;
  IntX twoByte;

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

  while ((p1 < end1) && (p2 < end2))
	{
	  Card32 code1 = *p1++;
	  Card32 code2 = *p2++;
	  if (twoByte)
		{
		  code1 = code1<<8 | *p1++;
		  code2 = code2<<8 | *p2++;
		}
	  if (code1 != code2) 
		return 1;
	}

  return 0;
}


/* Dump string */
static void dumpString(Card8 which, NameRecord *record)
	{
	Card8 *p;
	Card8 *end;
	IntX twoByte = record->platformId == 0 || record->platformId == 3;
	IntX precision = twoByte ? 4 : 2;

	p = (which == 1) ? 
	  &name1.strings[record->offset] : 
		&name2.strings[record->offset];
	end = p + record->length;

	note(" \"");
	while (p < end)
		{
		Card32 code = *p++;

		if (twoByte)
			code = code<<8 | *p++;
				
		if ((code & 0xff00) == 0 && isprint(code))
			note("%c", (IntN)code);
		else
			note("\\%0*lx", precision, code);
		}
	note("\"");
	}

static void makeString(Card8 which, NameRecord *record, Byte8 *str)
	{
	Card8 *p;
	Card8 *end;
	IntX twoByte = (record->platformId == 0 || record->platformId == 3);
	Byte8 *strptr = str;
	IntX len = 0;

	p = (which == 1) ? 
	  &name1.strings[record->offset] : 
		&name2.strings[record->offset];
	end = p + record->length;

	while ((p < end) && (len < record->length))
		{
		Card32 code = *p++;

		if (twoByte)
			code = code<<8 | *p++;
				
		if ((code & 0xff00) == 0 && isprint(code))
		  {
			*strptr++ = (Byte8)code;
			len++;
		  }
		}
	str[len] = '\0';
	}

void nameDiff(LongN offset1, LongN offset2)
	{
	IntX i;

	if (name1.format != name2.format)
	  {
		DiffExists++;
		note("< name format=%hd\n", name1.format);
		note("> name format=%hd\n", name2.format);
	  }
	if (name1.count != name2.count)
	  {
		DiffExists++;
		note("< name count=%hd\n", name1.count);
		note("> name count=%hd\n", name2.count);
	  }

	if (name1.count >= name2.count)
	  {
		for (i = 0; i < name1.count; i++)
		  {
			NameRecord *record1 = &name1.record[i];
			NameRecord *record2;
			IntX where;
			record2 = findNameRecord(2,
									 record1->platformId, record1->scriptId,
									 record1->languageId, record1->nameId, &where);
			if ((record2 == NULL) || (where < 0))
			  {
				DiffExists++;
				if (level > 3)
				  {
					note("< name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
						 i,
						 record1->platformId, record1->scriptId, 
						 record1->languageId, record1->nameId,
						 record1->length, record1->offset);
					note("[%s,%s,%s,%s]", 
						 descPlat(record1->platformId),
						 descScript(record1->platformId, record1->scriptId),
						 descLang(0, record1->platformId, record1->languageId),
						 descName(record1->nameId));
				  }
				else
				  {
					note("< name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
						 i,
						 record1->platformId, record1->scriptId, 
						 record1->languageId, record1->nameId);
				  }
				dumpString(1, record1);
				note("\n");
                note("> missing name record corresponding to [%2d]\n",i);
			  }
			else if (nameRecordsDiffer(record1, record2))
			  {
				DiffExists++;
				if (level > 3)
				  {
					note("< name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
						 i,
						 record1->platformId, record1->scriptId, 
						 record1->languageId, record1->nameId,
						 record1->length, record1->offset);
					note("[%s,%s,%s,%s]", 
						 descPlat(record1->platformId),
						 descScript(record1->platformId, record1->scriptId),
						 descLang(0, record1->platformId, record1->languageId),
						 descName(record1->nameId));
				  }
				else
				  {
					note("< name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
						 i,
						 record1->platformId, record1->scriptId, 
						 record1->languageId, record1->nameId);
				  }
				dumpString(1, record1);
				note("\n");
				if (level > 3)
				  {
					note("> name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
						 where,
						 record2->platformId, record2->scriptId, 
						 record2->languageId, record2->nameId,
						 record2->length, record2->offset);
					note("[%s,%s,%s,%s]", 
						 descPlat(record2->platformId),
						 descScript(record2->platformId, record2->scriptId),
						 descLang(0, record2->platformId, record2->languageId),
						 descName(record2->nameId));
				  }
				else
				  {
					note("> name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
						 where,
						 record2->platformId, record2->scriptId, 
						 record2->languageId, record2->nameId);
				  }
				dumpString(2, record2);
				note("\n");
			  }
		  }
	  }
	else
	  {
		for (i = 0; i < name2.count; i++)
		  {
			NameRecord *record2 = &name2.record[i];
			NameRecord *record1;
			IntX where;
			record1 = findNameRecord(1,
									 record2->platformId, record2->scriptId,
									 record2->languageId, record2->nameId, &where);
			if ((record1 == NULL) || (where < 0))
			  {
				DiffExists++;
                note("< missing name record corresponding to [%2d]\n",i);
				if (level > 3)
				  {
					note("> name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
						 i,
						 record2->platformId, record2->scriptId, 
						 record2->languageId, record2->nameId,
						 record2->length, record2->offset);
					note("[%s,%s,%s,%s]", 
						 descPlat(record2->platformId),
						 descScript(record2->platformId, record2->scriptId),
						 descLang(0, record2->platformId, record2->languageId),
						 descName(record2->nameId));
				  }
				else
				  {
					note("> name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
						 i,
						 record2->platformId, record2->scriptId, 
						 record2->languageId, record2->nameId);
				  }
				dumpString(2, record2);
				note("\n");
			  }
			else if (nameRecordsDiffer(record2, record1))
			  {
				DiffExists++;
				if (level > 3)
				  {
					note("< name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
						 i,
						 record1->platformId, record1->scriptId, 
						 record1->languageId, record1->nameId,
						 record1->length, record1->offset);
					note("[%s,%s,%s,%s]", 
						 descPlat(record1->platformId),
						 descScript(record1->platformId, record1->scriptId),
						 descLang(0, record1->platformId, record1->languageId),
						 descName(record1->nameId));
				  }
				else
				  {
					note("< name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
						 i,
						 record1->platformId, record1->scriptId, 
						 record1->languageId, record1->nameId);
				  }
				dumpString(1, record1);
				note("\n");
				if (level > 3)
				  {
					note("> name record[%2d]={platformId=%2hx, scriptId=%2hx, languageId=%4hx, nameId=%4hu, length=%4hu, offset=%04hx} ",
						 where,
						 record2->platformId, record2->scriptId, 
						 record2->languageId, record2->nameId,
						 record2->length, record2->offset);
					note("[%s,%s,%s,%s]", 
						 descPlat(record2->platformId),
						 descScript(record2->platformId, record2->scriptId),
						 descLang(0, record2->platformId, record2->languageId),
						 descName(record2->nameId));
				  }
				else
				  {
					note("> name record[%2d]={platId=%2hx, scriptId=%2hx, langId=%4hx, nameId=%4hu}",
						 where,
						 record2->platformId, record2->scriptId, 
						 record2->languageId, record2->nameId);
				  }
				dumpString(2, record2);
				note("\n");
			  }
		  }
	  }
	}

void nameFree(Card8 which)
{
    nameTbl *name;
    
    if (which == 1)
        if(!loaded1)
        {
            return;
        }
		else
        {
            name = &name1;
        }
        else if (which == 2)
        {
            
            if(!loaded2)
            {
                return;
            }
            else
            {
                name = &name2;
            }
        }
    memFree(name->strings);
    memFree(name->record);
    
    if (which == 1)
		loaded1 = 0;
    else if (which == 2) 
		loaded2 = 0;
}


#define CHECKREADASSIGN 	\
 if (which == 1){\
 if (!loaded1){\
  if (sfntReadATable(which, name_)){tableMissing(name_, name_); goto out;}}\
	name = &name1;}\
 else if (which == 2){\
 if (!loaded2){\
  if (sfntReadATable(which, name_)){tableMissing(name_, name_); goto out;}}\
    name = &name2;}


Byte8 *nameFontName(Card8 which)
	{
	  IntX i;
	  Byte8 *fullname = NULL;
	  nameTbl *name;

	  CHECKREADASSIGN

	  for (i = 0; i < name->count; i++)
		{
		  NameRecord *record = &name->record[i];
		  if (record->nameId == 4) /* Full Name */
			{
			  if ((record->languageId == 0) ||
				  (record->languageId == 0x409)) /* English */
				{

				  fullname = memNew(sizeof(Byte8) * (record->length + 1));
				  fullname[0] = '\0';
				  makeString(which, record, fullname);
				}
			}
		}
	  
	  return fullname;
	out:
	  return NULL;
	}

Byte8 *namePostScriptName(Card8 which)
	{
	  IntX i;
	  Byte8 *psname = NULL;
	  nameTbl *name;

	  CHECKREADASSIGN

	  for (i = 0; i < name->count; i++)
		{
		  NameRecord *record = &name->record[i];
		  if (record->nameId == 6) /* PostScript Name */
			{
			  if ((record->languageId == 0) ||
				  (record->languageId == 0x409)) /* English */
				{				  
				  psname = memNew(sizeof(Byte8) * (record->length + 1));
				  psname[0] = '\0';
				  makeString(which, record, psname);
				}
			}
		}
	  
	  return psname;
	out:
	  return NULL;
	}

