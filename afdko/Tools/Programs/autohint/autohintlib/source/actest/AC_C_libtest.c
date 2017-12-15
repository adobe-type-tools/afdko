/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//* AC_C_libtest.c */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ac_C_lib.h"

int main(int argc, char *argv[])
{
	int result, i, actualsize, allocsize=9096;
	char returnvalue[9096], *current, source[9048], fontinfo[2048];
	FILE *file;
	int c;
	
	file=fopen(argv[argc-2], "r");
	if (!file)
	{
		fprintf(stderr, "Couldn't open %s\n", argv[argc-2]);
		return 0;
	}
	current=source;
	while((c=fgetc(file))!=EOF)
	{
		*current++=(char)c;
	}
	*current='\0';
	
	fclose(file);
	
	file=fopen(argv[argc-1], "r");
	if (!file)
	{
		fprintf(stderr, "Couldn't open %s\n", argv[argc-2]);
		return 0;
	}
	current=fontinfo;
	while((c=fgetc(file))!=EOF)
	{
		*current++=(char)c;
	}
	*current='\0';
	
	fclose(file);
	
	actualsize=allocsize;
	result=AutoColorString(NULL, fontinfo, returnvalue, &actualsize, 0);
	printf("AutoColorString with NULL source returned %d and size %d\n", result, actualsize);

	actualsize=allocsize;
	result=AutoColorString(source, NULL, returnvalue, &actualsize, 0);
	printf("AutoColorString with NULL fontinfo returned %d and size %d\n", result, actualsize);

	actualsize=allocsize;
	result=AutoColorString(NULL, NULL, returnvalue, &actualsize, 0);
	printf("AutoColorString with NULL source and fontinfo returned %d and size %d\n", result, actualsize);

	actualsize=allocsize;
	result=AutoColorString("", fontinfo, returnvalue, &actualsize, 0);
	printf("AutoColorString with empty source returned %d and size %d\n", result, actualsize);

	actualsize=allocsize;
	result=AutoColorString(source, "", returnvalue, &actualsize, 0);
	printf("AutoColorString with empty fontinfo returned %d and size %d\n", result, actualsize);

	actualsize=allocsize;
	result=AutoColorString("", "", returnvalue, &actualsize, 0);
	printf("AutoColorString with empty source and fontinfo returned %d and size %d\n", result, actualsize);




	for (i=0; i<0; i++) 
	{
		actualsize=allocsize;
		printf("Source size %d\n", strlen(source));
		result=AutoColorString(source, fontinfo, returnvalue, &actualsize, 0);
		printf("AutoColorString returned %d and size %d\n", result, actualsize);
	}
	actualsize=allocsize;
	printf("Source size %d\n", strlen(source));
	result=AutoColorString(source, fontinfo, returnvalue, &actualsize, 0);
	printf("AutoColorString returned %d and size %d\n", result, actualsize);
	if (!result)
	{
		FILE *outfile=fopen("AC_test.bez", "w");
		fprintf(outfile, "%s", returnvalue);
		fclose(outfile);
	}
	return 0;
}