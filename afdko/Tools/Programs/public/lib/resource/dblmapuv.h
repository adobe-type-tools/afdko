/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* 
 * Adobe Glyph List double mapped glyphs aggregate initializer.
 *
 * Refer to the document "Unicode and Glyph Names" at:
 * http://partners.adobe.com/asn/developer/typeforum/unicodegn.html
 * for more information.
 *
 * Element values of type:
 *
 * struct
 *		{
 *		unsigned short secondary_uv;
 *		unsigned short primary_uv;
 *		};
 * 
 * Search by secondary_uv get primary_uv.
 */

	{ 0x00A0, 0x0020 },	/* space */                
	{ 0x00AD, 0x002D },	/* hyphen */               
	{ 0x021A, 0x0162 },	/* Tcommaaccent */         
	{ 0x021B, 0x0163 },	/* tcommaaccent */
	{ 0x02C9, 0x00AF },	/* macron */               
	{ 0x0394, 0x2206 },	/* Delta */                
	{ 0x03A9, 0x2126 },	/* Omega */                
	{ 0x03BC, 0x00B5 },	/* mu */                   
	{ 0x2215, 0x2044 },	/* fraction */             
	{ 0x2219, 0x00B7 },	/* periodcentered */       
	{ 0xF6C1, 0x015E },	/* Scedilla */             
	{ 0xF6C2, 0x015F },	/* scedilla */             
