#ifndef feattoks_h
#define feattoks_h
/* feattoks.h -- List of labelled tokens and stuff
 *
 * Generated from: featgram.g
 *
 * Terence Parr, Will Cohen, and Hank Dietz: 1989-2001
 * Purdue University Electrical Engineering
 * ANTLR Version 1.33MR33
 */
#define zzEOF_TOKEN 1
#define Eof 1
#define K_include 5
#define T_TAG 7
#define T_LABEL 11
#define INCLFILE 15
#define K_feature 35
#define K_table 36
#define K_script 37
#define K_language 38
#define K_languagesystem 39
#define K_subtable 40
#define K_lookup 41
#define K_lookupflag 42
#define K_RightToLeft 43
#define K_IgnoreBaseGlyphs 44
#define K_IgnoreLigatures 45
#define K_IgnoreMarks 46
#define K_UseMarkFilteringSet 47
#define K_MarkAttachmentType 48
#define K_anon 49
#define K_includeDFLT 50
#define K_excludeDFLT 51
#define K_include_dflt 52
#define K_exclude_dflt 53
#define K_useExtension 54
#define K_BeginValue 55
#define K_EndValue 56
#define K_enumerate 57
#define K_except 58
#define K_ignore 59
#define K_substitute 60
#define K_reverse 61
#define K_by 62
#define K_from 63
#define K_position 64
#define K_parameters 65
#define K_feat_names 66
#define K_cv_params 67
#define K_cvUILabel 68
#define K_cvToolTip 69
#define K_cvSampletext 70
#define K_cvParameterLabel 71
#define K_cvCharacter 72
#define K_sizemenuname 73
#define K_contourpoint 74
#define K_device 75
#define K_anchor 76
#define K_anchordef 77
#define K_valueRecordDef 78
#define K_mark 79
#define K_markClass 80
#define K_cursive 81
#define K_markBase 82
#define K_markLigature 83
#define K_LigatureComponent 84
#define K_caret 85
#define K_NULL 86
#define K_BASE 87
#define K_HorizAxis_BaseTagList 88
#define K_HorizAxis_BaseScriptList 89
#define K_VertAxis_BaseTagList 90
#define K_VertAxis_BaseScriptList 91
#define K_GDEF 92
#define K_GlyphClassDef 93
#define K_Attach 94
#define K_GDEFMarkAttachClass 95
#define K_LigatureCaret1 96
#define K_LigatureCaret2 97
#define K_LigatureCaret3 98
#define K_head 99
#define K_FontRevision 100
#define K_hhea 101
#define K_Ascender 102
#define K_Descender 103
#define K_LineGap 104
#define K_CaretOffset 105
#define K_name 106
#define K_nameid 107
#define K_OS_2 108
#define K_FSType 109
#define K_FSType2 110
#define K_LowerOpticalPointSize 111
#define K_UpperOpticalPointSize 112
#define K_Panose 113
#define K_TypoAscender 114
#define K_TypoDescender 115
#define K_TypoLineGap 116
#define K_winAscent 117
#define K_winDescent 118
#define K_XHeight 119
#define K_CapHeight 120
#define K_UnicodeRange 121
#define K_CodePageRange 122
#define K_WeightClass 123
#define K_WidthClass 124
#define K_Vendor 125
#define K_vhea 126
#define K_VertTypoAscender 127
#define K_VertTypoDescender 128
#define K_VertTypoLineGap 129
#define K_vmtx 130
#define K_VertOriginY 131
#define K_VertAdvanceY 132
#define T_FONTREV 133
#define T_NUMEXT 134
#define T_NUM 135
#define T_GCLASS 136
#define T_CID 137
#define T_GNAME 138
#define T_STRING 139

#ifdef __USE_PROTOS
extern GID glyph(char * tok,int allowNotdef);
#else
extern GID glyph();
#endif

#ifdef __USE_PROTOS
extern GNode * glyphClass(int named,char * gcname);
#else
extern GNode * glyphClass();
#endif

#ifdef __USE_PROTOS
extern unsigned char numUInt8(void);
#else
extern unsigned char numUInt8();
#endif

#ifdef __USE_PROTOS
extern short numInt16(void);
#else
extern short numInt16();
#endif

#ifdef __USE_PROTOS
extern unsigned numUInt16(void);
#else
extern unsigned numUInt16();
#endif

#ifdef __USE_PROTOS
extern short parameterValue(void);
#else
extern short parameterValue();
#endif

#ifdef __USE_PROTOS
extern unsigned numUInt16Ext(void);
#else
extern unsigned numUInt16Ext();
#endif

#ifdef __USE_PROTOS
extern unsigned numUInt32Ext(void);
#else
extern unsigned numUInt32Ext();
#endif

#ifdef __USE_PROTOS
extern short metric(void);
#else
extern short metric();
#endif

#ifdef __USE_PROTOS
void deviceEntry(void);
#else
extern void deviceEntry();
#endif

#ifdef __USE_PROTOS
extern unsigned short contourpoint(void);
#else
extern unsigned short contourpoint();
#endif

#ifdef __USE_PROTOS
extern unsigned short device(void);
#else
extern unsigned short device();
#endif

#ifdef __USE_PROTOS
void caret(void);
#else
extern void caret();
#endif

#ifdef __USE_PROTOS
void valueRecordDef(void);
#else
extern void valueRecordDef();
#endif

#ifdef __USE_PROTOS
void valueRecord(GNode* gnode);
#else
extern void valueRecord();
#endif

#ifdef __USE_PROTOS
void valueRecord3(GNode* gnode);
#else
extern void valueRecord3();
#endif

#ifdef __USE_PROTOS
void anchorDef(void);
#else
extern void anchorDef();
#endif

#ifdef __USE_PROTOS
extern int anchor(int componentIndex);
#else
extern int anchor();
#endif

#ifdef __USE_PROTOS
extern GNode * pattern(int markedOK);
#else
extern GNode * pattern();
#endif

#ifdef __USE_PROTOS
extern GNode * pattern2(int markedOK,GNode** headP);
#else
extern GNode * pattern2();
#endif

#ifdef __USE_PROTOS
extern GNode * pattern3(int markedOK,GNode** headP);
#else
extern GNode * pattern3();
#endif

#ifdef __USE_PROTOS
void ignoresub_or_pos(void);
#else
extern void ignoresub_or_pos();
#endif

#ifdef __USE_PROTOS
void substitute(void);
#else
extern void substitute();
#endif

#ifdef __USE_PROTOS
void mark_statement(void);
#else
extern void mark_statement();
#endif

#ifdef __USE_PROTOS
void position(void);
#else
extern void position();
#endif

#ifdef __USE_PROTOS
void parameters(void);
#else
extern void parameters();
#endif

#ifdef __USE_PROTOS
void featureNameEntry(void);
#else
extern void featureNameEntry();
#endif

#ifdef __USE_PROTOS
void featureNames(void);
#else
extern void featureNames();
#endif

#ifdef __USE_PROTOS
void cvParameterBlock(void);
#else
extern void cvParameterBlock();
#endif

#ifdef __USE_PROTOS
extern GNode * cursive(int markedOK,GNode** headP);
#else
extern GNode * cursive();
#endif

#ifdef __USE_PROTOS
extern GNode * baseToMark(int markedOK,GNode** headP);
#else
extern GNode * baseToMark();
#endif

#ifdef __USE_PROTOS
extern GNode * ligatureMark(int markedOK,GNode** headP);
#else
extern GNode * ligatureMark();
#endif

#ifdef __USE_PROTOS
void glyphClassAssign(void);
#else
extern void glyphClassAssign();
#endif

#ifdef __USE_PROTOS
void scriptAssign(void);
#else
extern void scriptAssign();
#endif

#ifdef __USE_PROTOS
void languageAssign(void);
#else
extern void languageAssign();
#endif

#ifdef __USE_PROTOS
void namedLookupFlagValue(unsigned short * val);
#else
extern void namedLookupFlagValue();
#endif

#ifdef __USE_PROTOS
void lookupflagAssign(void);
#else
extern void lookupflagAssign();
#endif

#ifdef __USE_PROTOS
void featureUse(void);
#else
extern void featureUse();
#endif

#ifdef __USE_PROTOS
void subtable(void);
#else
extern void subtable();
#endif

#ifdef __USE_PROTOS
void sizemenuname(void);
#else
extern void sizemenuname();
#endif

#ifdef __USE_PROTOS
void statement(void);
#else
extern void statement();
#endif

#ifdef __USE_PROTOS
void lookupBlockOrUse(void);
#else
extern void lookupBlockOrUse();
#endif

#ifdef __USE_PROTOS
void lookupBlockStandAlone(void);
#else
extern void lookupBlockStandAlone();
#endif

#ifdef __USE_PROTOS
void featureBlock(void);
#else
extern void featureBlock();
#endif

#ifdef __USE_PROTOS
void baseScript(int vert,long nTag);
#else
extern void baseScript();
#endif

#ifdef __USE_PROTOS
void axisSpecs(void);
#else
extern void axisSpecs();
#endif

#ifdef __USE_PROTOS
void table_BASE(void);
#else
extern void table_BASE();
#endif

#ifdef __USE_PROTOS
void table_OS_2(void);
#else
extern void table_OS_2();
#endif

#ifdef __USE_PROTOS
extern GNode * glyphClassOptional(void);
#else
extern GNode * glyphClassOptional();
#endif

#ifdef __USE_PROTOS
void table_GDEF(void);
#else
extern void table_GDEF();
#endif

#ifdef __USE_PROTOS
void table_head(void);
#else
extern void table_head();
#endif

#ifdef __USE_PROTOS
void table_hhea(void);
#else
extern void table_hhea();
#endif

#ifdef __USE_PROTOS
void table_name(void);
#else
extern void table_name();
#endif

#ifdef __USE_PROTOS
void table_vhea(void);
#else
extern void table_vhea();
#endif

#ifdef __USE_PROTOS
void table_vmtx(void);
#else
extern void table_vmtx();
#endif

#ifdef __USE_PROTOS
void tableBlock(void);
#else
extern void tableBlock();
#endif

#ifdef __USE_PROTOS
void languagesystemAssign(void);
#else
extern void languagesystemAssign();
#endif

#ifdef __USE_PROTOS
void topLevelStatement(void);
#else
extern void topLevelStatement();
#endif

#ifdef __USE_PROTOS
void anonBlock(void);
#else
extern void anonBlock();
#endif

#ifdef __USE_PROTOS
void featureFile(void);
#else
extern void featureFile();
#endif

#endif
extern SetWordType zzerr1[];
extern SetWordType zzerr2[];
extern SetWordType zzerr3[];
extern SetWordType zzerr4[];
extern SetWordType zzerr5[];
extern SetWordType zzerr6[];
extern SetWordType setwd1[];
extern SetWordType zzerr7[];
extern SetWordType zzerr8[];
extern SetWordType zzerr9[];
extern SetWordType zzerr10[];
extern SetWordType zzerr11[];
extern SetWordType zzerr12[];
extern SetWordType setwd2[];
extern SetWordType zzerr13[];
extern SetWordType zzerr14[];
extern SetWordType zzerr15[];
extern SetWordType zzerr16[];
extern SetWordType zzerr17[];
extern SetWordType setwd3[];
extern SetWordType zzerr18[];
extern SetWordType zzerr19[];
extern SetWordType setwd4[];
extern SetWordType zzerr20[];
extern SetWordType zzerr21[];
extern SetWordType zzerr22[];
extern SetWordType zzerr23[];
extern SetWordType zzerr24[];
extern SetWordType zzerr25[];
extern SetWordType zzerr26[];
extern SetWordType zzerr27[];
extern SetWordType zzerr28[];
extern SetWordType zzerr29[];
extern SetWordType setwd5[];
extern SetWordType zzerr30[];
extern SetWordType zzerr31[];
extern SetWordType zzerr32[];
extern SetWordType zzerr33[];
extern SetWordType setwd6[];
extern SetWordType zzerr34[];
extern SetWordType zzerr35[];
extern SetWordType zzerr36[];
extern SetWordType zzerr37[];
extern SetWordType zzerr38[];
extern SetWordType zzerr39[];
extern SetWordType zzerr40[];
extern SetWordType setwd7[];
extern SetWordType zzerr41[];
extern SetWordType zzerr42[];
extern SetWordType zzerr43[];
extern SetWordType zzerr44[];
extern SetWordType setwd8[];
extern SetWordType zzerr45[];
extern SetWordType zzerr46[];
extern SetWordType zzerr47[];
extern SetWordType zzerr48[];
extern SetWordType zzerr49[];
extern SetWordType zzerr50[];
extern SetWordType setwd9[];
extern SetWordType zzerr51[];
extern SetWordType zzerr52[];
extern SetWordType zzerr53[];
extern SetWordType zzerr54[];
extern SetWordType zzerr55[];
extern SetWordType setwd10[];
extern SetWordType zzerr56[];
extern SetWordType zzerr57[];
extern SetWordType zzerr58[];
extern SetWordType zzerr59[];
extern SetWordType zzerr60[];
extern SetWordType setwd11[];
extern SetWordType zzerr61[];
extern SetWordType zzerr62[];
extern SetWordType zzerr63[];
extern SetWordType zzerr64[];
extern SetWordType zzerr65[];
extern SetWordType zzerr66[];
extern SetWordType setwd12[];
extern SetWordType zzerr67[];
extern SetWordType zzerr68[];
extern SetWordType zzerr69[];
extern SetWordType setwd13[];
extern SetWordType zzerr70[];
extern SetWordType zzerr71[];
extern SetWordType zzerr72[];
extern SetWordType zzerr73[];
extern SetWordType zzerr74[];
extern SetWordType zzerr75[];
extern SetWordType setwd14[];
extern SetWordType zzerr76[];
extern SetWordType zzerr77[];
extern SetWordType zzerr78[];
extern SetWordType zzerr79[];
extern SetWordType setwd15[];
extern SetWordType zzerr80[];
extern SetWordType zzerr81[];
extern SetWordType zzerr82[];
extern SetWordType zzerr83[];
extern SetWordType setwd16[];
extern SetWordType zzerr84[];
extern SetWordType zzerr85[];
extern SetWordType zzerr86[];
extern SetWordType zzerr87[];
extern SetWordType zzerr88[];
extern SetWordType zzerr89[];
extern SetWordType setwd17[];
extern SetWordType zzerr90[];
extern SetWordType setwd18[];
