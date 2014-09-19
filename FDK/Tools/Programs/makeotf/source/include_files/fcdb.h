/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef FCDB_H
#define FCDB_H

/* Font Conversion Database Parser Library
   =======================================
   This library provides facilities for parsing multiple database files that
   contain OpenType font conversion data.

   This library is initialized by a single call to to fcdbNew() which allocates
   an opaque context (fcdbCtx) which is passed to subsequent functions and is
   destroyed by a single call to fcdbFree().

   Once the context has been allocated, database files may be pre-parsed by
   calling fcdbAddFile(). During this phase a list of database access keys
   (FontNames) are stored in the context that associates each key with
   information about the location of the record within the database files. This
   information is subsequently used by the fcdbGetRec() function to locate and
   parse the data associated with the requested key.

   The library can handle multiple database files and keeps track of them using
   a number called a "fileid". This number is provided by the client via
   fcdbAddFile() as each database file is added. It is the responsibility of
   the client to devise a scheme based around the fileid concept. 

   One suggestion is for the client to create an array whose elements
   correspond to data related to each database file and have the fileid be an
   index into that array, starting at 0. However, the client is completely free
   to use the fileids in any way they choose since the library merely stores
   and retrieves this number for the client and doesn't try to interpret its
   value. */

typedef struct fcdbCallbacks_ fcdbCallbacks;
typedef struct fcdbCtx_ *fcdbCtx;
enum
    {
    fcdbStyleBold = 1,
    fcdbStyleItalic,
    fcdbStyleBoldItalic
    };
  
fcdbCtx fcdbNew(fcdbCallbacks *cb, void *dna_ctx);

/* fcdbNew() initializes the library and returns an opaque context that is
   subsequently passed to all the other library functions. It must be the first
   function in the library to be called by the client. The client passes a set
   of callback functions to the library via the cb argument which is an
   fcdbCallbacks data structure whose fields are described below. */

struct fcdbCallbacks_
    {
    void *ctx;
    
/* [Optional] ctx is a client context that is passed back to the client as the
   first argument to each callback function. It is intended to be used in
   multi-threaded environments. */

    char *(*refill)(void *ctx, unsigned fileid, size_t *count);

/* [Required] refill() is called in order to refill the library's database
   input buffer. It is called in response to a client call to fcdbAddFile(). It
   returns a pointer to the new data and the count of the number of bytes of
   new data available via the count argument. The client must arrange that
   successive calls to refill() return consecutive blocks of data starting with
   the first byte in the database file. The fileid argument is a copy of the
   argument of the same name passed to fcdbAddFile(). The end-of-file
   condition is signalled to the library by appending a null byte to the
   returned buffer. (A client might implement refill() with fread() and fill
   buffers of BUFSIZ bytes.) */

    void (*getbuf)(void *ctx, 
                   unsigned fileid, long offset, long length, char *buf);

/* [Required] getbuf() is called in order to retrieve the data associated with
   a particular database record. It is called in response to a client call to
   fcdbGetRec(). The client must copy length bytes of data into buf (which is
   guaranteed to be length-bytes long) from the database file identified by
   fileid at offset bytes from the beginning of the file. An offset of 0 refers
   to the first byte in the file. (A client might implement getbuf() by calling
   fseek(), with a SEEK_SET argument, and then fread().) */

    int (*addname)(void *ctx,
                    unsigned short platformId, unsigned short platspecId,
                    unsigned short languageId, unsigned short nameId,
                    char *str);

/* [Optional] addname() is called in order to pass back name data to the
   client. It is called in response to a client call to fcdbGetRec() during the
   record parse and may be called several times, once per name, for each call
   to fcdbGetRec(). The platformId, platspecId, languageId, and nameId
   arguments correspond to name attributes in the sfnt name table. The str
   argument points to a null-terminated name string extracted from the database
   file. Return 0 on success and 1 on failure (for example if the string fails
   validation). */ 

    void (*addlink)(void *ctx, int style, char *fontname);

/* [Optional] addlink() is called in order to pass back the style and linked
   FontName to the client. It is called in response to a client call to
   fcdbGetRec() during the record parse and may be called up to 3 times, once
   per style. The style argument encodes the style as a number that may be
   decoded by the enumerations: fcdbStyleBold, fcdbStyleItalic,
   fcdbStyleBoldIatlic. The fontname argument points to a null-terminated
   PostScript FontName of the linked font. */

    void (*addenc)(void *ctx, int code, char *gname);

/* [Optional] addenc() is called in order to pass back encoding data to the
   client. It is called in response to a client call to fcdbGetRec() during the
   record parse and may be called several times, once per encoding, for each
   call to fcdbGetRec(). The code argument is the encoding in the range 0-255
   and the gname argument is a null-terminated glyph name encoded at that
   position. */

    void (*error)(void *ctx, unsigned fileid, long line, int errid);

/* Required. Indicates which version of syntax is being used. */
	void (*setMenuVersion)(void *ctx, unsigned fileid, unsigned short syntaxVersion);	

	};

/* [Required] error() is called to report error conditions that may arise as a
   result of calling fcdbAddFile() or fcdbGetRec(). The fileid argument
   identifies the database file being parsed and the line argument identifies
   the line number in that file where the error occurred. The errid argument
   may be one of the following values: */

enum 
    {
    fcdbSyntaxErr,      /* Syntax error */
    fcdbDuplicateErr,   /* Duplicate record */
    fcdbKeyLengthErr,   /* Maximum record key length exceeded (63 chars) */
    fcdbIdRangeErr,     /* Name id range exceeded (0-65535) */
    fcdbCodeRangeErr,   /* Code range exceeded (0-255) */
    fcdbEmptyNameErr,   /* Empty name string */
	fcdbWinCompatibleFullError, /* The version 2 syntax is allowed only for Mac platform */
	fcdbMixedSyntax, /* Both version 1 and 2 font menu name syntax is present in the file */
    fcdbErrCnt
    };

/* The fcdbDuplicateErr error is reported when record with a key (FontName)
   that matched an earlier key was found. The second and subsequent duplicate
   records are ignored. */

void fcdbAddFile(fcdbCtx h, unsigned fileid, void *callBackCtx);

/* fcdbAddFile() adds a new database file to the context that is identified by
   the fileid argument. Calling this function causes cb.refill() to be called
   repeatedly until the end of the database file is reached. */

int fcdbGetRec(fcdbCtx h, char *FontName);

/* fcdbGetRec() retrieves a record form one of the database files added with
   fcdbAddFile(). The record key is the specified by the FontName argument.
   Calling this function cause cb.getbuf() to be called to retrieve the data
   for a database file. This data is subsequently parsed causing cb.addname()
   and cb.addenc() to be called one or more times depending on the data parsed
   from the record. Functions cb.addname() and cb.addenc() pass names as null
   terminated string pointers back to the client. These pointers are guaranteed
   to remain stable until a subsequent call to fcdbGetRec(). */


void fcdbFree(fcdbCtx h);

/* fcdbFree() destroys the library context and all resources allocated to it.*/

/* Font Conversion Database File Format
   ====================================
   The font conversion database file uses a simple line-oriented ASCII format
   that is loosely modeled on Windows .INI-style file formats.

   Lines are terminated by either a carriage return (13), a line feed (10),
   or a carriage return followed by a line feed. Lines may not exceed 255
   characters in length (including the line terminating character(s)).

   Any of the tab (9), vertical tab (11), form feed (12), or space (32)
   characters are regarded as "blank" characters for the purposes of this
   description.

   Empty lines, or lines containing blank characters only, are ignored.

   Lines whose first non-blank character is a semicolon (;) are treated as
   comment lines which extend to the line end and are ignored.

   Lines whose first non-blank character is an open square bracket ([)
   introduce a new section in the file which corresponds to a new database
   record. The record key (which is the PostScript FontName) is specified in a
   section line by enclosing the key between square brackets, for example:

   [Lithos-ExtraLight]

   Blank space surrounding the square brackets is ignored. The FontName may be
   up to 63 characters in length but may not contain blank characters.
   
   Lines within a section are of the following form:

   <keyword>=<value>

   Valid <keywords> are shown in the following table.

   Short Form   Long Form
   ----------   ---------
   f            family
   s            subfamily
   c            compatible
   b            bold
   i            italic
   t            bolditalic
   -none-       macenc

   The short form keywords (where defined) are simply synonyms for the
   corresponding long form keywords and are provided for economy where there
   might be concerns regarding the size of database files with thousands of
   records.

   The <value> may be any non-blank sequence of characters that conform to the
   formats described below. Blank characters surrounding the equals (=)
   character are ignored.

   The family, subfamily, and compatible keywords introduce names that will be
   stored in the sfnt name table. (For details of the name table please consult
   the OpenType specification.) The name is specified by the value field and
   may be optionally preceded by a name attribute.

   A name attribute is one or three comma-terminated numbers that specify the
   platform, platform-specific, and language ids to be stored in the name
   record of the name table. If only one number is specified it represents the
   platform id. The platform id may be either 1 or 3, corresponding to the
   Macintosh or Microsoft (hereafter called Windows) platforms, respectively.
   The other id numbers must be in the range 0-65535 and are not otherwise
   validated. Blanks surrounding the id numbers are ignored.

   Decimal numbers must begin with a non-0 digit, octal numbers with a 0 digit,
   and hexadecimal numbers with a 0x prefix to numbers and hexadecimal letters
   a-f or A-F.

   If some or all of the id numbers aren't specified their values are defaulted
   as follows:
   
   platform id      3 (Windows)
   
   Windows platform selected:

   platspec id      1 (Unicode)
   language id      0x0409 (Windows default English)

   Macintosh platform selected:

   platspec id      0 (Roman)
   language id      0 (English)

   Putting this all together gives the following valid line formats and the ids
   that are assigned.

   value format     platform id platspec id language id
   ---------------- ----------- ----------- -----------
   <name>           3           1           0x0409
   3,<name>         3           1           0x0409
   3,S,L,<name>     3           S           L
   1,<name>         1           0           0
   1,S,L,<name>     1           S           L

   The name string is composed of 1-byte ASCII characters. These are converted
   on the Windows platform to Unicode by adding a high byte of 0. 2-byte
   Unicode values for the Windows platform may be specified using a special
   character sequence of a backslash character (\) followed by exactly four
   hexadecimal numbers (of either case) which may not all be zero, e.g. \4e2d .
   The ASCII backslash character must be represented as the sequence \005c .

   There is no corresponding conversion to Unicode for the Macintosh platform
   but character codes in the range 128-255 may be specified using a special
   character sequence of a backslash character (\) followed by exactly two
   hexadecimal numbers (of either case) which may not both be zero, e.g. \83 .

   The special notations described above aren't needed for Latin fonts because
   their menu names have traditionally been restricted to ASCII characters
   only. However, the special notations are required for CJK fonts where menu
   names are typically composed of characters outside of the usual ASCII range.

   If only one family and subfamily name is specified for the default platform,
   and language (Windows, Unicode, English) the conversion software will
   automatically make a copy of this for the Macintosh default script and
   language (Roman, English) provided the original name doesn't contain any
   2-byte Unicode characters. This behavior doesn't apply to the compatible
   name.

   The name ids stored in the name table for the various names is
   platform-dependent and is shown in the following table.

   Keyword      Windows                 Macintosh
   -------      -------                 ---------
   family       16 (Pref. Family)        1 (Family)
   subfamily    17 (Pref. Subfamily)     2 (Subfamily)
   compatible    1 (Family)             18 (Comp. Full)

   The bold, italic, and bolditalic keywords specify the PostScript FontName
   of the font that is linked to the current font by the style specified by the
   keyword. For example, the Mephis-Medium font is linked to the Memphis-Bold,
   Memphis-MediumItalic, and Memphis-BoldItalic by the bold, italic, and bold
   italic styles, respectively, as shown below.

    [Memphis-Medium]
       f=Memphis
       s=Medium
       c=Memphis Medium
       b=Memphis-Bold
       i=Memphis-MediumItalic
       t=Memphis-BoldItalic

   A few Macintosh fonts contain encodings that cannot be derived correctly
   from Windows font data. Such fonts may be correctly supported by specifying
   an encoding in the font record using the the macenc keyword. Each Macintosh
   encoding line specifies a single code to glyph name mapping. Normally, many
   such lines must be specifed in order to build up the complete encoding for
   one font.

   The value specified with the macenc keyword consists of a code number in the
   range 0-255 and a glyph name separated by a comma. Blanks surrounding the
   comma are ignored. Decimal, octal, and hexadecimal numbers are supported as
   described above. The glyph name may not contain blanks but is otherwise
   unvalidated.

   Here are some examples of valid font records.

   [AdobeCorpID-Bullet]
        f=Adobe Corporate ID
        s=Bullet
        c=AdobeCorpID Bullet
        c=1,AdobeCorpID Bullet
        macenc=32,space
        macenc=165,bullet
   
    [KozMin-Medium]
        f=Kozuka Mincho
        s=M
        c=Kozuka Mincho Medium
        f=3,1,0x0411,\5c0f\585a\660e\671d
        s=3,1,0x0411,M
        c=3,1,0x0411,\5c0f\585a\660e\671dM
        s=1,1,11,M
        f=1,1,11,\8f\ac\92\cb\96\be\92\a9
        c=1,1,11,\8f\ac\92\cb\96\be\92\a9M 

	*/

#endif /* FCDB_H */

