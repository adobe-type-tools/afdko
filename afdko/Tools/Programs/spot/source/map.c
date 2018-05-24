/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)map.c	1.1
 * Changed:    11/20/98 10:31:33
 ***********************************************************************/

#include "map.h"
#include <string.h>

typedef struct _map_MappingType
{
  const Byte8 *oldname;
  const Byte8 *nicename;
} map_MappingType;


static map_MappingType MAP[] = {

#include "nicename.h"

};
static map_MappingType * MAP2=NULL;


static IntX MAP_length = TABLE_LEN(MAP);
char aliasfromfileinit = 0; 
extern char * glyphaliasfilename;

/* Tries to load the glyph renaming data from GlyphAliasAndNameDB instead
   of using the static values.
   
 */

int mapcmp(const void *s, const void *d)
{
	map_MappingType *map1, *map2;
	
	map1=(map_MappingType *)s;
	map2=(map_MappingType *)d;
	return strcmp(map1->oldname, map2->oldname);
}

void InitAliasFromFile(void)
{
	FILE *file=NULL;
	int counter=0, value, i;
	
	if(glyphaliasfilename!=NULL)
		file = fopen(glyphaliasfilename, "r");
	if (file==NULL)
		{
			fprintf(OUTPUTBUFF, "Could not open GlyphAliasDB\n");
			return;
		}
	while(1){
		char temp1[MAX_NAME_LEN], temp2[MAX_NAME_LEN], c;
		value = fscanf(file, "%s %s", temp1, temp2);
		if (value==EOF || value == 0)
			break;
		do {
			c = getc(file);
		}while (c!='\r' && c!='\n');
		c = getc(file);
		if (c!='\r' && c!='\n')
			ungetc(c, file);
		counter++;
	}
	MAP2 = (map_MappingType *)memNew(counter * sizeof(map_MappingType));
	
	rewind(file);
	
	for(i=0; i<counter; i++){
		char *oldname, *nicename, c;
		oldname = memNew(32);
		nicename = memNew(32);
		fscanf(file, "%s %s\n", oldname, nicename);
		do {
			c = getc(file);
		}while (c!='\r' && c!='\n' && c!=EOF);
		c = getc(file);
		if (c!='\r' && c!='\n')
			ungetc(c, file);
		MAP2[i].oldname=oldname;
		MAP2[i].nicename=nicename;
	}
	MAP_length=counter;
	qsort(MAP2, counter, sizeof(map_MappingType), mapcmp);
	fclose(file);
}

const Byte8 *map_nicename (Byte8 *oldnam, IntX *newlength)
{
  IntX lo = 0;
  IntX hi;
  map_MappingType *mMAP;
  
  if (!aliasfromfileinit && glyphaliasfilename && strcmp(glyphaliasfilename, "unknown")!=0)
  {
  	InitAliasFromFile();
  	aliasfromfileinit=1;
  }
  
  if (aliasfromfileinit)
  	mMAP=MAP2;
  else
  	mMAP=MAP;
  
  hi = MAP_length - 1;

  while (lo <= hi)
	{
	  IntX mid = (lo + hi) / 2;
	  int match = strcmp(oldnam, mMAP[mid].oldname);
	  if (match > 0)
		lo = mid + 1;
	  else if (match < 0)
		hi = mid - 1;
	  else 
		{
		  *newlength = strlen(mMAP[mid].nicename);
		  return (mMAP[mid].nicename);
		}
	}
  /* not found */
  *newlength = (-1);
  return NULL;
}

void freemap(void)
{
	if (MAP2!=NULL)
	{
		memFree(MAP2);
		MAP2=NULL;
	}
}
