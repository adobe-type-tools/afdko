/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * Windows ANSI encoding aggregate initializer. Element values are Unicodes.
 */

UV_UNDEF,           /* 0x00                */
UV_UNDEF,           /* 0x01                */
UV_UNDEF,           /* 0x02                */
UV_UNDEF,           /* 0x03                */
UV_UNDEF,           /* 0x04                */
UV_UNDEF,           /* 0x05                */
UV_UNDEF,           /* 0x06                */
UV_UNDEF,           /* 0x07                */
UV_UNDEF,           /* 0x08                */
UV_UNDEF,           /* 0x09                */
UV_UNDEF,           /* 0x0A                */
UV_UNDEF,           /* 0x0B                */
UV_UNDEF,           /* 0x0C                */
UV_UNDEF,           /* 0x0D                */
UV_UNDEF,           /* 0x0E                */
UV_UNDEF,           /* 0x0F                */
UV_UNDEF,           /* 0x10                */
UV_UNDEF,           /* 0x11                */
UV_UNDEF,           /* 0x12                */
UV_UNDEF,           /* 0x13                */
UV_UNDEF,           /* 0x14                */
UV_UNDEF,           /* 0x15                */
UV_UNDEF,           /* 0x16                */
UV_UNDEF,           /* 0x17                */
UV_UNDEF,           /* 0x18                */
UV_UNDEF,           /* 0x19                */
UV_UNDEF,           /* 0x1A                */
UV_UNDEF,           /* 0x1B                */
UV_UNDEF,           /* 0x1C                */
UV_UNDEF,           /* 0x1D                */
UV_UNDEF,           /* 0x1E                */
UV_UNDEF,           /* 0x1F                */
0x0020,             /* 0x20 space          */
0x0021,             /* 0x21 exclam         */
0x0022,             /* 0x22 quotedbl       */
0x0023,             /* 0x23 numbersign     */
0x0024,             /* 0x24 dollar         */
0x0025,             /* 0x25 percent        */
0x0026,             /* 0x26 ampersand      */
0x0027,             /* 0x27 quotesingle    */
0x0028,             /* 0x28 parenleft      */
0x0029,             /* 0x29 parenright     */
0x002A,             /* 0x2A asterisk       */
0x002B,             /* 0x2B plus           */
0x002C,             /* 0x2C comma          */
0x002D,             /* 0x2D hyphen         */
0x002E,             /* 0x2E period         */
0x002F,             /* 0x2F slash          */
0x0030,             /* 0x30 zero           */
0x0031,             /* 0x31 one            */
0x0032,             /* 0x32 two            */
0x0033,             /* 0x33 three          */
0x0034,             /* 0x34 four           */
0x0035,             /* 0x35 five           */
0x0036,             /* 0x36 six            */
0x0037,             /* 0x37 seven          */
0x0038,             /* 0x38 eight          */
0x0039,             /* 0x39 nine           */
0x003A,             /* 0x3A colon          */
0x003B,             /* 0x3B semicolon      */
0x003C,             /* 0x3C less           */
0x003D,             /* 0x3D equal          */
0x003E,             /* 0x3E greater        */
0x003F,             /* 0x3F question       */
0x0040,             /* 0x40 at             */
0x0041,             /* 0x41 A              */
0x0042,             /* 0x42 B              */
0x0043,             /* 0x43 C              */
0x0044,             /* 0x44 D              */
0x0045,             /* 0x45 E              */
0x0046,             /* 0x46 F              */
0x0047,             /* 0x47 G              */
0x0048,             /* 0x48 H              */
0x0049,             /* 0x49 I              */
0x004A,             /* 0x4A J              */
0x004B,             /* 0x4B K              */
0x004C,             /* 0x4C L              */
0x004D,             /* 0x4D M              */
0x004E,             /* 0x4E N              */
0x004F,             /* 0x4F O              */
0x0050,             /* 0x50 P              */
0x0051,             /* 0x51 Q              */
0x0052,             /* 0x52 R              */
0x0053,             /* 0x53 S              */
0x0054,             /* 0x54 T              */
0x0055,             /* 0x55 U              */
0x0056,             /* 0x56 V              */
0x0057,             /* 0x57 W              */
0x0058,             /* 0x58 X              */
0x0059,             /* 0x59 Y              */
0x005A,             /* 0x5A Z              */
0x005B,             /* 0x5B bracketleft    */
0x005C,             /* 0x5C backslash      */
0x005D,             /* 0x5D bracketright   */
0x005E,             /* 0x5E asciicircum    */
0x005F,             /* 0x5F underscore     */
0x0060,             /* 0x60 grave          */
0x0061,             /* 0x61 a              */
0x0062,             /* 0x62 b              */
0x0063,             /* 0x63 c              */
0x0064,             /* 0x64 d              */
0x0065,             /* 0x65 e              */
0x0066,             /* 0x66 f              */
0x0067,             /* 0x67 g              */
0x0068,             /* 0x68 h              */
0x0069,             /* 0x69 i              */
0x006A,             /* 0x6A j              */
0x006B,             /* 0x6B k              */
0x006C,             /* 0x6C l              */
0x006D,             /* 0x6D m              */
0x006E,             /* 0x6E n              */
0x006F,             /* 0x6F o              */
0x0070,             /* 0x70 p              */
0x0071,             /* 0x71 q              */
0x0072,             /* 0x72 r              */
0x0073,             /* 0x73 s              */
0x0074,             /* 0x74 t              */
0x0075,             /* 0x75 u              */
0x0076,             /* 0x76 v              */
0x0077,             /* 0x77 w              */
0x0078,             /* 0x78 x              */
0x0079,             /* 0x79 y              */
0x007A,             /* 0x7A z              */
0x007B,             /* 0x7B braceleft      */
0x007C,             /* 0x7C bar            */
0x007D,             /* 0x7D braceright     */
0x007E,             /* 0x7E asciitilde     */
UV_UNDEF,           /* 0x7F                */
0x20AC,             /* 0x80 Euro           */
UV_UNDEF,           /* 0x81                */
0x201A,             /* 0x82 quotesinglbase */
0x0192,             /* 0x83 florin         */
0x201E,             /* 0x84 quotedblbase   */
0x2026,             /* 0x85 ellipsis       */
0x2020,             /* 0x86 dagger         */
0x2021,             /* 0x87 daggerdbl      */
0x02C6,             /* 0x88 circumflex     */
0x2030,             /* 0x89 perthousand    */
0x0160,             /* 0x8A Scaron         */
0x2039,             /* 0x8B guilsinglleft  */
0x0152,             /* 0x8C OE             */
UV_UNDEF,           /* 0x8D                */
0x017D,             /* 0x8E Zcaron         */
UV_UNDEF,           /* 0x8F                */
UV_UNDEF,           /* 0x90                */
0x2018,             /* 0x91 quoteleft      */
0x2019,             /* 0x92 quoteright     */
0x201C,             /* 0x93 quotedblleft   */
0x201D,             /* 0x94 quotedblright  */
0x2022,             /* 0x95 bullet         */
0x2013,             /* 0x96 endash         */
0x2014,             /* 0x97 emdash         */
0x02DC,             /* 0x98 tilde          */
0x2122,             /* 0x99 trademark      */
0x0161,             /* 0x9A scaron         */
0x203A,             /* 0x9B guilsinglright */
0x0153,             /* 0x9C oe             */
UV_UNDEF,           /* 0x9D                */
0x017E,             /* 0x9E zcaron         */
0x0178,             /* 0x9F Ydieresis      */
0x00A0,             /* 0xA0 <nbspace>      */
0x00A1,             /* 0xA1 exclamdown     */
0x00A2,             /* 0xA2 cent           */
0x00A3,             /* 0xA3 sterling       */
0x00A4,             /* 0xA4 currency       */
0x00A5,             /* 0xA5 yen            */
0x00A6,             /* 0xA6 brokenbar      */
0x00A7,             /* 0xA7 section        */
0x00A8,             /* 0xA8 dieresis       */
0x00A9,             /* 0xA9 copyright      */
0x00AA,             /* 0xAA ordfeminine    */
0x00AB,             /* 0xAB guillemotleft  */
0x00AC,             /* 0xAC logicalnot     */
0x00AD,             /* 0xAD <sfthyphen>    */
0x00AE,             /* 0xAE registered     */
0x00AF,             /* 0xAF macron         */
0x00B0,             /* 0xB0 degree         */
0x00B1,             /* 0xB1 plusminus      */
0x00B2,             /* 0xB2 twosuperior    */
0x00B3,             /* 0xB3 threesuperior  */
0x00B4,             /* 0xB4 acute          */
0x00B5,             /* 0xB5 mu             */
0x00B6,             /* 0xB6 paragraph      */
0x00B7,             /* 0xB7 periodcentered */
0x00B8,             /* 0xB8 cedilla        */
0x00B9,             /* 0xB9 onesuperior    */
0x00BA,             /* 0xBA ordmasculine   */
0x00BB,             /* 0xBB guillemotright */
0x00BC,             /* 0xBC onequarter     */
0x00BD,             /* 0xBD onehalf        */
0x00BE,             /* 0xBE threequarters  */
0x00BF,             /* 0xBF questiondown   */
0x00C0,             /* 0xC0 Agrave         */
0x00C1,             /* 0xC1 Aacute         */
0x00C2,             /* 0xC2 Acircumflex    */
0x00C3,             /* 0xC3 Atilde         */
0x00C4,             /* 0xC4 Adieresis      */
0x00C5,             /* 0xC5 Aring          */
0x00C6,             /* 0xC6 AE             */
0x00C7,             /* 0xC7 Ccedilla       */
0x00C8,             /* 0xC8 Egrave         */
0x00C9,             /* 0xC9 Eacute         */
0x00CA,             /* 0xCA Ecircumflex    */
0x00CB,             /* 0xCB Edieresis      */
0x00CC,             /* 0xCC Igrave         */
0x00CD,             /* 0xCD Iacute         */
0x00CE,             /* 0xCE Icircumflex    */
0x00CF,             /* 0xCF Idieresis      */
0x00D0,             /* 0xD0 Eth            */
0x00D1,             /* 0xD1 Ntilde         */
0x00D2,             /* 0xD2 Ograve         */
0x00D3,             /* 0xD3 Oacute         */
0x00D4,             /* 0xD4 Ocircumflex    */
0x00D5,             /* 0xD5 Otilde         */
0x00D6,             /* 0xD6 Odieresis      */
0x00D7,             /* 0xD7 multiply       */
0x00D8,             /* 0xD8 Oslash         */
0x00D9,             /* 0xD9 Ugrave         */
0x00DA,             /* 0xDA Uacute         */
0x00DB,             /* 0xDB Ucircumflex    */
0x00DC,             /* 0xDC Udieresis      */
0x00DD,             /* 0xDD Yacute         */
0x00DE,             /* 0xDE Thorn          */
0x00DF,             /* 0xDF germandbls     */
0x00E0,             /* 0xE0 agrave         */
0x00E1,             /* 0xE1 aacute         */
0x00E2,             /* 0xE2 acircumflex    */
0x00E3,             /* 0xE3 atilde         */
0x00E4,             /* 0xE4 adieresis      */
0x00E5,             /* 0xE5 aring          */
0x00E6,             /* 0xE6 ae             */
0x00E7,             /* 0xE7 ccedilla       */
0x00E8,             /* 0xE8 egrave         */
0x00E9,             /* 0xE9 eacute         */
0x00EA,             /* 0xEA ecircumflex    */
0x00EB,             /* 0xEB edieresis      */
0x00EC,             /* 0xEC igrave         */
0x00ED,             /* 0xED iacute         */
0x00EE,             /* 0xEE icircumflex    */
0x00EF,             /* 0xEF idieresis      */
0x00F0,             /* 0xF0 eth            */
0x00F1,             /* 0xF1 ntilde         */
0x00F2,             /* 0xF2 ograve         */
0x00F3,             /* 0xF3 oacute         */
0x00F4,             /* 0xF4 ocircumflex    */
0x00F5,             /* 0xF5 otilde         */
0x00F6,             /* 0xF6 odieresis      */
0x00F7,             /* 0xF7 divide         */
0x00F8,             /* 0xF8 oslash         */
0x00F9,             /* 0xF9 ugrave         */
0x00FA,             /* 0xFA uacute         */
0x00FB,             /* 0xFB ucircumflex    */
0x00FC,             /* 0xFC udieresis      */
0x00FD,             /* 0xFD yacute         */
0x00FE,             /* 0xFE thorn          */
0x00FF,             /* 0xFF ydieresis      */
