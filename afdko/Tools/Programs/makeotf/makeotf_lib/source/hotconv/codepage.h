/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Windows and Macintosh code page/encoding information. See struct
   CodePage in map.c for description and usage. (Symbol, CJK and Expert are
   handled separately.) */
/* Windows code page:      Mac enc:     */
/* ----------------------- ------------ */
{
	0, 0x00EA, "ecircumflex",     UV_UNDEF, NULL,       -1, latn_,                0,                0
},                                                                                                      /* 1252 Latin 1 (ANSI)     Roman        */
{
	1, 0x0148, "ncaron",          0x010C,  "Ccaron",    -1, latn_,               29,                0
},                                                                                                      /* 1250 Latin 2 (CE)       CE           */
{
	2, 0x0451, "afii10071",       0x042F,  "afii10049", -1, cyrl_,                7,                0
},                                                                                                      /* 1251 Cyrillic (Slavic)  9.0 Cyrillic */
{
	3, 0x03CB, "upsilondieresis",  UV_UNDEF, NULL,     -1, grek_,                0,               15
},                                                                                                     /* 1253 Greek              Greek        */
{
	4, 0x0130, "Idotaccent",      UV_UNDEF, NULL,       -1, latn_,                0,               18
},                                                                                                      /* 1254 Latin 5 (Turkish)  Turkish      */
{
	5, 0x05D0, "afii57664",       UV_UNDEF, NULL,       -1, hebr_,                5,                0
},                                                                                                      /* 1255 Hebrew             Hebrew       */
{
	6, 0x0622, "afii57410",       UV_UNDEF, NULL,       -1, arab_,                4,                0
},                                                                                                      /* 1256 Arabic             Arabic       */
{
	7, 0x0173, "uogonek",         UV_UNDEF, NULL,       -1, latn_, HOT_CMAP_UNKNOWN, HOT_CMAP_UNKNOWN
},                                                                                                      /* 1257 Baltic Rim         -            */
{
	8, 0x20AB, "dong",            0x01B0, NULL,       -1, latn_, HOT_CMAP_UNKNOWN, HOT_CMAP_UNKNOWN
},                                                                                                    /* 1258 Vietnamese         -            */
{
	16, 0x0E01,  NULL,             UV_UNDEF, NULL,       -1, thai_,               21,                0
},                                                                                                      /* 874 Thai                Thai         */
{
	29, 0xFB02, "fl",              0x0131,   "dotlessi", -1, latn_,                0,                0
},                                                                                                      /* 1252 Latin 1 (ANSI)     Roman        */
{
	30, 0x2592, "shade",           UV_UNDEF, NULL,       -1, latn_, HOT_CMAP_UNKNOWN, HOT_CMAP_UNKNOWN
},                                                                                                      /* OEM character set       -            */
{
	-1, 0x0111, "dcroat",          UV_UNDEF, NULL,       -1, latn_,                0,               19
},                                                                                                      /* -                       Croatian     */
{
	-1, 0x00F0, "eth",             UV_UNDEF, NULL,       -1, latn_,                0,               16
},                                                                                                      /* -                       Icelandic    */
{
	-1, 0x0103, "abreve",          UV_UNDEF, NULL,       -1, latn_,                0,               38
},                                                                                                      /* -                       Romanian     */
{
	-1, 0x06F4,  NULL,             UV_UNDEF, NULL,       -1, arab_,                4,               32
},                                                                                                      /* -                       Farsi        */
{
	-1, 0x0915,  NULL,             UV_UNDEF, NULL,       -1, deva_,                9,                0
},                                                                                                      /* -                       Devanagari   */
{
	-1, 0x0A95,  NULL,             UV_UNDEF, NULL,       -1, gujr_,               11,                0
},                                                                                                      /* -                       Gujarati     */
{
	-1, 0x0A15,  NULL,             UV_UNDEF, NULL,       -1, punj_,               10,                0
},                                                                                                      /* -                       Gurmukhi     */
