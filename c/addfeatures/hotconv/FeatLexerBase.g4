/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

// -------------------------- Feature file tokens ---------------------------

lexer grammar FeatLexerBase;

COMMENT                 : '#' ~[\r\n]* -> skip ;
WHITESPACE              : [ \t\r\n]+ -> skip ;

INCLUDE                 : 'include' -> pushMode(Include) ;
FEATURE                 : 'feature' ;
TABLE                   : 'table' ;
SCRIPT                  : 'script' ;
LANGUAGE                : 'language' ;
LANGSYS                 : 'languagesystem' ;
SUBTABLE                : 'subtable';
LOOKUP                  : 'lookup' ;
LOOKUPFLAG              : 'lookupflag' ;
NOTDEF                  : '.notdef' ;

RIGHT_TO_LEFT           : 'RightToLeft' ;
IGNORE_BASE_GLYPHS      : 'IgnoreBaseGlyphs' ;
IGNORE_LIGATURES        : 'IgnoreLigatures' ;
IGNORE_MARKS            : 'IgnoreMarks' ;
USE_MARK_FILTERING_SET  : 'UseMarkFilteringSet' ;
MARK_ATTACHMENT_TYPE    : 'MarkAttachmentType' ;

ANON                    : 'anon' ;
ANON_v                  : 'anonymous' ;

EXCLUDE_DFLT            : 'excludeDFLT' ;
INCLUDE_DFLT            : 'includeDFLT' ;
EXCLUDE_dflt            : 'exclude_dflt' ;
INCLUDE_dflt            : 'include_dflt' ;

USE_EXTENSION           : 'useExtension' ;
BEGINVALUE              : '<' ;
ENDVALUE                : '>' ;
ENUMERATE               : 'enumerate' ;
ENUMERATE_v             : 'enum' ;
EXCEPT                  : 'except' ;
IGNORE                  : 'ignore' ;
SUBSTITUTE              : 'substitute' ;
SUBSTITUTE_v            : 'sub' ;
REVERSE                 : 'reversesub' ;
REVERSE_v               : 'rsub' ;
BY                      : 'by' ;
FROM                    : 'from' ;
POSITION                : 'position' ;
POSITION_v              : 'pos';
PARAMETERS              : 'parameters' ;
FEATURE_NAMES           : 'featureNames' ;
CV_PARAMETERS           : 'cvParameters' ;
CV_UI_LABEL             : 'FeatUILabelNameID' ;
CV_TOOLTIP              : 'FeatUITooltipTextNameID' ;
CV_SAMPLE_TEXT          : 'SampleTextNameID' ;
CV_PARAM_LABEL          : 'ParamUILabelNameID' ;
CV_CHARACTER            : 'Character' ;
SIZEMENUNAME            : 'sizemenuname' ;
CONTOURPOINT            : 'contourpoint' ;
ANCHOR                  : 'anchor' ;
ANCHOR_DEF              : 'anchorDef' ;
VALUE_RECORD_DEF        : 'valueRecordDef' ;
LOCATION_DEF            : 'locationDef' ;
MARK                    : 'mark';
MARK_CLASS              : 'markClass' ;
CURSIVE                 : 'cursive' ;
MARKBASE                : 'base' ;
MARKLIG                 : 'ligature' ;
MARKLIG_v               : 'lig' ;
LIG_COMPONENT           : 'ligComponent' ;

KNULL                   : 'NULL' ;

BASE                    : 'BASE' ;
HA_BTL                  : 'HorizAxis.BaseTagList' ;
VA_BTL                  : 'VertAxis.BaseTagList' ;
HA_BSL                  : 'HorizAxis.BaseScriptList' ;
VA_BSL                  : 'VertAxis.BaseScriptList' ;

GDEF                    : 'GDEF' ;
GLYPH_CLASS_DEF         : 'GlyphClassDef' ;
ATTACH                  : 'Attach' ;
LIG_CARET_BY_POS        : 'LigatureCaretByPos' ;
LIG_CARET_BY_IDX        : 'LigatureCaretByIndex' ;

HEAD                    : 'head' ;
FONT_REVISION           : 'FontRevision' ;

HHEA                    : 'hhea' ;
ASCENDER                : 'Ascender' ;
DESCENDER               : 'Descender' ;
LINE_GAP                : 'LineGap' ;

CARET_OFFSET            : 'CaretOffset' ;

NAME                    : 'name' ;
NAMEID                  : 'nameid' ;

OS_2                    : 'OS/2' ;
FS_TYPE                 : 'FSType' ;
FS_TYPE_v               : 'fsType' ;
OS2_LOWER_OP_SIZE       : 'LowerOpSize' ;
OS2_UPPER_OP_SIZE       : 'UpperOpSize' ;
PANOSE                  : 'Panose' ;
TYPO_ASCENDER           : 'TypoAscender' ;
TYPO_DESCENDER          : 'TypoDescender' ;
TYPO_LINE_GAP           : 'TypoLineGap' ;
WIN_ASCENT              : 'winAscent' ;
WIN_DESCENT             : 'winDescent' ;
X_HEIGHT                : 'XHeight' ;
CAP_HEIGHT              : 'CapHeight' ;
WEIGHT_CLASS            : 'WeightClass' ;
WIDTH_CLASS             : 'WidthClass' ;
VENDOR                  : 'Vendor' ;
UNICODE_RANGE           : 'UnicodeRange' ;
CODE_PAGE_RANGE         : 'CodePageRange' ;
FAMILY_CLASS            : 'FamilyClass' ;

STAT                    : 'STAT' ;
ELIDED_FALLBACK_NAME    : 'ElidedFallbackName' ;
ELIDED_FALLBACK_NAME_ID : 'ElidedFallbackNameID' ;
DESIGN_AXIS             : 'DesignAxis' ;
AXIS_VALUE              : 'AxisValue';
FLAG                    : 'flag' ;
LOCATION                : 'location';
AXIS_EAVN               : 'ElidableAxisValueName';
AXIS_OSFA               : 'OlderSiblingFontAttribute';

VHEA                    : 'vhea' ;
VERT_TYPO_ASCENDER      : 'VertTypoAscender' ;
VERT_TYPO_DESCENDER     : 'VertTypoDescender' ;
VERT_TYPO_LINE_GAP      : 'VertTypoLineGap' ;

VMTX                    : 'vmtx' ;
VERT_ORIGIN_Y           : 'VertOriginY' ;
VERT_ADVANCE_Y          : 'VertAdvanceY' ;

LCBRACE                 : '{' ;
RCBRACE                 : '}' ;
LBRACKET                : '[' ;
RBRACKET                : ']' ;
LPAREN                  : '(' ;
RPAREN                  : ')' ;
HYPHEN                  : '-' ;
SEMI                    : ';' ;
EQUALS                  : '=' ;
MARKER                  : '\'' ;
COMMA                   : ',' ;
COLON                   : ':' ;
STRVAL                  : '"' ( '\\"' | ~["] )* '"' ;

fragment GNST           : 'A' .. 'Z' | 'a' .. 'z' | '_' ;
fragment LCHR           : GNST | '0' .. '9' | '.' ;
fragment GCCHR          : LCHR | '-' ;
GCLASS                  : '@' GNST GCCHR* ;

AXISUNIT                : 'u' | 'd' | 'n' ;
CID                     : '\\' ( '0' .. '9' )+ ;
fragment GNCHR          : GCCHR | '+' | '*' | ':' | '~' | '^' | '|' ;
ESCGNAME                : '\\' GNST GNCHR* ;
NAMELABEL               : GNST LCHR* ;
EXTNAME                 : GNST GNCHR* ;
POINTNUM                : '-'? ( '0' .. '9' )+ '.' ( '0' .. '9' )+ ;
NUMEXT                  : '0x' ( '0' .. '9' | 'a' .. 'f' | 'A' .. 'F' )+ ;
NUMOCT                  : '0' ( '0' .. '7' )+ ;
NUM                     : '-'? ( '1' .. '9' ( '0' .. '9' )* | '0' ) ;

mode Include;

I_WHITESPACE            : [ \t\r\n]+ -> skip ;
I_RPAREN                : '(' -> mode(Ifile) ;

mode Ifile;

IFILE                   : ~')'+ ;
I_LPAREN                : ')' -> popMode ;
