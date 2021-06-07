/* Copyright 2021 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
 * This software is licensed as OpenSource, under the Apache License, Version 2.0.
 * This license is available at: http://opensource.org/licenses/Apache-2.0.
 */

// ------------------------- Feature file grammar ---------------------------

parser grammar FeatParser;

options { tokenVocab = FeatLexer; }

file:
    ( topLevelStatement
    | featureBlock
    | tableBlock
    | anonBlock
    | lookupBlockTopLevel
    )* EOF
;

topLevelStatement:
    ( include
    | glyphClassAssign
    | langsysAssign
    | mark_statement
    | anchorDef
    | valueRecordDef
    )
    SEMI
;

include:
    INCLUDE I_RPAREN IFILE I_LPAREN
;

glyphClassAssign:
    GCLASS EQUALS glyphClass
;

langsysAssign:
    LANGSYS script=tag lang=tag
;

mark_statement:
    MARK_CLASS ( glyph | glyphClass ) anchor GCLASS
;

anchorDef:
    ANCHOR_DEF xval=NUM yval=NUM ( CONTOURPOINT cp=NUM )? name=label
;

valueRecordDef:
    VALUE_RECORD_DEF valueLiteral label
;

featureBlock:
    FEATURE starttag=tag USE_EXTENSION? LCBRACE
    featureStatement+
    RCBRACE endtag=tag SEMI
;

tableBlock:
    TABLE
    ( table_BASE
    | table_GDEF
    | table_head
    | table_hhea
    | table_vhea
    | table_name
    | table_OS_2
    | table_STAT
    | table_vmtx
    )
;

anonBlock:
    anontok A_LABEL A_LBRACE A_LINE* A_CLOSE
;

lookupBlockTopLevel:
    LOOKUP startlabel=label USE_EXTENSION? LCBRACE
    statement+
    RCBRACE endlabel=label SEMI
;

featureStatement:
      statement
    | lookupBlockOrUse
    | cvParameterBlock
;

lookupBlockOrUse:
    LOOKUP startlabel=label ( USE_EXTENSION? LCBRACE
    statement+
    RCBRACE endlabel=label )? SEMI
;

cvParameterBlock:
    CV_PARAMETERS LCBRACE
    cvParameterStatement*
    RCBRACE SEMI
;

cvParameterStatement:
    ( cvParameter
    | include
    ) SEMI
;

cvParameter:
      ( CV_UI_LABEL | CV_TOOLTIP | CV_SAMPLE_TEXT | CV_PARAM_LABEL ) LCBRACE
      nameEntryStatement+
      RCBRACE
    | CV_CHARACTER genNum
;

statement:
    ( featureUse
    | scriptAssign
    | langAssign
    | lookupflagAssign
    | glyphClassAssign
    | ignoreSubOrPos
    | substitute
    | mark_statement
    | position
    | parameters
    | sizemenuname
    | featureNames
    | subtable
    | include
    ) SEMI
;

featureUse:
    FEATURE tag
;

scriptAssign:
    SCRIPT tag
;

langAssign:
    LANGUAGE tag ( EXCLUDE_DFLT | INCLUDE_DFLT | EXCLUDE_dflt | INCLUDE_dflt )?
;

lookupflagAssign:
    LOOKUPFLAG ( NUM | lookupflagElement+ )
;

lookupflagElement:
      RIGHT_TO_LEFT
    | IGNORE_BASE_GLYPHS
    | IGNORE_LIGATURES
    | IGNORE_MARKS
    | ( MARK_ATTACHMENT_TYPE glyphClass )
    | ( USE_MARK_FILTERING_SET glyphClass )
;

ignoreSubOrPos:
    IGNORE ( subtok | revtok | postok ) lookupPattern ( COMMA lookupPattern )*
;

substitute:
    ( EXCEPT lookupPattern ( COMMA lookupPattern )* )?
    (   revtok startpat=lookupPattern ( BY ( KNULL | endpat=lookupPattern ) )?
      | subtok startpat=lookupPattern ( ( BY | FROM ) ( KNULL | endpat=lookupPattern ) )? )
;

position:
    enumtok? postok startpat=pattern?
    (
        ( valueRecord valuePattern* )
      | ( ( LOOKUP label )+ lookupPatternElement* )
      | ( CURSIVE cursiveElement endpat=pattern? )
      | ( MARKBASE midpat=pattern baseToMarkElement+ endpat=pattern? )
      | ( markligtok midpat=pattern ligatureMarkElement+ endpat=pattern? )
      | ( MARK midpat=pattern baseToMarkElement+ endpat=pattern? )
    )
;

valuePattern:
    patternElement valueRecord?
;

valueRecord:
    BEGINVALUE valuename=label ENDVALUE | valueLiteral
;

valueLiteral:
    ( BEGINVALUE NUM NUM NUM NUM ENDVALUE ) | NUM
;

cursiveElement:
    patternElement anchor anchor
;

baseToMarkElement:
    anchor MARK GCLASS MARKER?
;

ligatureMarkElement:
    anchor ( MARK GCLASS )? LIG_COMPONENT? MARKER?
;

parameters:
    PARAMETERS fixedNum+
;

sizemenuname:
    SIZEMENUNAME ( genNum ( genNum genNum )? )? QUOTE STRVAL EQUOTE
;

featureNames:
    FEATURE_NAMES LCBRACE
    nameEntryStatement+
    RCBRACE
;

subtable:
    SUBTABLE
;

table_BASE:
    BASE LCBRACE
    baseStatement+
    RCBRACE BASE SEMI
;

baseStatement:
    ( axisTags
    | axisScripts
    | include
    ) SEMI
;

axisTags:
    ( HA_BTL | VA_BTL ) tag+
;

axisScripts:
    ( HA_BSL | VA_BSL ) baseScript ( COMMA baseScript )*
;

baseScript:
    script=tag db=tag NUM+
;

table_GDEF:
    GDEF LCBRACE
    gdefStatement+
    RCBRACE GDEF SEMI
;

gdefStatement:
    ( gdefGlyphClass
    | gdefAttach
    | gdefLigCaretPos
    | gdefLigCaretIndex
    | include
    ) SEMI
;

gdefGlyphClass:
    GLYPH_CLASS_DEF glyphClassOptional COMMA
                    glyphClassOptional COMMA
                    glyphClassOptional COMMA
                    glyphClassOptional
;

gdefAttach:
    ATTACH lookupPattern NUM+
;

gdefLigCaretPos:
    LIG_CARET_BY_POS lookupPattern NUM+
;

gdefLigCaretIndex:
    LIG_CARET_BY_IDX lookupPattern NUM+
;

table_head:
    HEAD LCBRACE
    headStatement+
    RCBRACE HEAD SEMI
;

headStatement:
    ( head
    | include
    ) SEMI
;

head:
    FONT_REVISION POINTNUM
;

table_hhea:
    HHEA LCBRACE
    hheaStatement*
    RCBRACE HHEA SEMI
;

hheaStatement:
    ( hhea
    | include
    ) SEMI
;

hhea:
    ( CARET_OFFSET | ASCENDER | DESCENDER | LINE_GAP ) NUM
;

table_vhea:
    VHEA LCBRACE
    vheaStatement*
    RCBRACE VHEA SEMI
;

vheaStatement:
    ( vhea
    | include
    ) SEMI
;

vhea:
    ( VERT_TYPO_ASCENDER | VERT_TYPO_DESCENDER | VERT_TYPO_LINE_GAP ) NUM
;

table_name:
    NAME LCBRACE
    nameStatement+
    RCBRACE NAME SEMI
;

nameStatement:
    ( nameID
    | include
    ) SEMI
;

nameID:
    NAMEID id=genNum ( plat=genNum ( spec=genNum lang=genNum )? )? QUOTE STRVAL EQUOTE
;

table_OS_2:
    OS_2 LCBRACE
    os_2Statement+
    RCBRACE OS_2 SEMI
;

os_2Statement:
    ( os_2
    | include
    ) SEMI
;

os_2:
      ( TYPO_ASCENDER | TYPO_DESCENDER | TYPO_LINE_GAP
      | WIN_ASCENT | WIN_DESCENT | X_HEIGHT | CAP_HEIGHT ) num=NUM
    |
      ( FS_TYPE | FS_TYPE_v | WEIGHT_CLASS | WIDTH_CLASS
      | OS2_LOWER_OP_SIZE | OS2_UPPER_OP_SIZE ) unum=NUM
    | FAMILY_CLASS gnum=genNum
    | VENDOR QUOTE STRVAL EQUOTE
    | PANOSE NUM NUM NUM NUM NUM NUM NUM NUM NUM NUM
    | ( UNICODE_RANGE | CODE_PAGE_RANGE ) NUM+
;


table_STAT:
    STAT LCBRACE
    statStatement+
    RCBRACE STAT SEMI
;

statStatement:
    ( designAxis
    | axisValue
    | elidedFallbackName
    | elidedFallbackNameID
    | include
    ) SEMI
;

designAxis:
    DESIGN_AXIS tag NUM LCBRACE
    nameEntryStatement+
    RCBRACE
;

axisValue:
    AXIS_VALUE LCBRACE
    axisValueStatement+
    RCBRACE
;

axisValueStatement:
    ( nameEntry
    | axisValueLocation
    | axisValueFlags
    | include
    ) SEMI
;

axisValueLocation:
    LOCATION tag fixedNum ( fixedNum fixedNum? )?
;

axisValueFlags:
    FLAG ( AXIS_OSFA | AXIS_EAVN )+
;

elidedFallbackName:
    ELIDED_FALLBACK_NAME LCBRACE
    nameEntryStatement+
    RCBRACE
;

nameEntryStatement:
    ( nameEntry
    | include
    ) SEMI
;

elidedFallbackNameID:
    ELIDED_FALLBACK_NAME_ID genNum
;

nameEntry:
    NAME ( genNum ( genNum genNum )? )? QUOTE STRVAL EQUOTE
;

table_vmtx:
    VMTX LCBRACE
    vmtxStatement+
    RCBRACE VMTX SEMI
;

vmtxStatement:
    ( vmtx
    | include
    ) SEMI
;

vmtx:
    ( VERT_ORIGIN_Y | VERT_ADVANCE_Y ) glyph NUM
;

anchor:
    BEGINVALUE ANCHOR
    (   ( xval=NUM yval=NUM ( CONTOURPOINT cp=NUM )? )
      | KNULL
      | name=label
    ) ENDVALUE
;

lookupPattern:
    lookupPatternElement+
;

lookupPatternElement:
    patternElement ( LOOKUP label )*
;

pattern:
    patternElement+
;

patternElement:
    ( glyphClass | glyph ) MARKER?
;

glyphClassOptional:
    glyphClass?
;

glyphClass:
    GCLASS | gcLiteral
;

gcLiteral:
    LBRACKET gcLiteralElement+ RBRACKET
;

gcLiteralElement:
      startg=glyph ( HYPHEN endg=glyph )?
    | GCLASS
;

glyph:
      glyphName
    | CID
;

glyphName:
    ESCGNAME | NAMELABEL | EXTNAME | NOTDEF
;

label:
    NAMELABEL | MARK
;

tag:
    NAMELABEL | EXTNAME | CATCHTAG | MARK     // MARK included for "feature mark"
;

fixedNum:
    POINTNUM | NUM
;

genNum:
    NUM | NUMOCT | NUMEXT
;

// These are for an include directive in a block with statements

featureFile:
    featureStatement* EOF
;

statementFile:
    statement* EOF
;

cvStatementFile:
    cvParameterStatement* EOF
;

baseFile:
    baseStatement* EOF
;

headFile:
    headStatement* EOF
;

hheaFile:
    hheaStatement* EOF
;

vheaFile:
    vheaStatement* EOF
;

gdefFile:
    gdefStatement* EOF
;

nameFile:
    nameStatement* EOF
;

vmtxFile:
    vmtxStatement* EOF
;

os_2File:
    os_2Statement* EOF
;

statFile:
    statStatement* EOF
;

axisValueFile:
    axisValueStatement* EOF
;

nameEntryFile:
    nameEntryStatement* EOF
;

/* These tokens are defined this way because they slightly improves
 * Antlr 4's default error reporting.  If we wind up overloading the
 * class with the token literals at the C++ level I will devolve these
 * back into the Lexer grammar.
 */
subtok:
    SUBSTITUTE | SUBSTITUTE_v
;

revtok:
    REVERSE | REVERSE_v
;

anontok:
    ANON | ANON_v
;

enumtok:
    ENUMERATE | ENUMERATE_v
;

postok:
    POSITION | POSITION_v
;

markligtok:
    MARKLIG | MARKLIG_v
;
