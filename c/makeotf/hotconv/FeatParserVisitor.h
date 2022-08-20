
// Generated from FeatParser.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "FeatParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by FeatParser.
 */
class  FeatParserVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by FeatParser.
   */
    virtual antlrcpp::Any visitFile(FeatParser::FileContext *context) = 0;

    virtual antlrcpp::Any visitTopLevelStatement(FeatParser::TopLevelStatementContext *context) = 0;

    virtual antlrcpp::Any visitInclude(FeatParser::IncludeContext *context) = 0;

    virtual antlrcpp::Any visitGlyphClassAssign(FeatParser::GlyphClassAssignContext *context) = 0;

    virtual antlrcpp::Any visitLangsysAssign(FeatParser::LangsysAssignContext *context) = 0;

    virtual antlrcpp::Any visitMark_statement(FeatParser::Mark_statementContext *context) = 0;

    virtual antlrcpp::Any visitAnchorDef(FeatParser::AnchorDefContext *context) = 0;

    virtual antlrcpp::Any visitValueRecordDef(FeatParser::ValueRecordDefContext *context) = 0;

    virtual antlrcpp::Any visitFeatureBlock(FeatParser::FeatureBlockContext *context) = 0;

    virtual antlrcpp::Any visitTableBlock(FeatParser::TableBlockContext *context) = 0;

    virtual antlrcpp::Any visitAnonBlock(FeatParser::AnonBlockContext *context) = 0;

    virtual antlrcpp::Any visitLookupBlockTopLevel(FeatParser::LookupBlockTopLevelContext *context) = 0;

    virtual antlrcpp::Any visitFeatureStatement(FeatParser::FeatureStatementContext *context) = 0;

    virtual antlrcpp::Any visitLookupBlockOrUse(FeatParser::LookupBlockOrUseContext *context) = 0;

    virtual antlrcpp::Any visitCvParameterBlock(FeatParser::CvParameterBlockContext *context) = 0;

    virtual antlrcpp::Any visitCvParameterStatement(FeatParser::CvParameterStatementContext *context) = 0;

    virtual antlrcpp::Any visitCvParameter(FeatParser::CvParameterContext *context) = 0;

    virtual antlrcpp::Any visitStatement(FeatParser::StatementContext *context) = 0;

    virtual antlrcpp::Any visitFeatureUse(FeatParser::FeatureUseContext *context) = 0;

    virtual antlrcpp::Any visitScriptAssign(FeatParser::ScriptAssignContext *context) = 0;

    virtual antlrcpp::Any visitLangAssign(FeatParser::LangAssignContext *context) = 0;

    virtual antlrcpp::Any visitLookupflagAssign(FeatParser::LookupflagAssignContext *context) = 0;

    virtual antlrcpp::Any visitLookupflagElement(FeatParser::LookupflagElementContext *context) = 0;

    virtual antlrcpp::Any visitIgnoreSubOrPos(FeatParser::IgnoreSubOrPosContext *context) = 0;

    virtual antlrcpp::Any visitSubstitute(FeatParser::SubstituteContext *context) = 0;

    virtual antlrcpp::Any visitPosition(FeatParser::PositionContext *context) = 0;

    virtual antlrcpp::Any visitValuePattern(FeatParser::ValuePatternContext *context) = 0;

    virtual antlrcpp::Any visitValueRecord(FeatParser::ValueRecordContext *context) = 0;

    virtual antlrcpp::Any visitValueLiteral(FeatParser::ValueLiteralContext *context) = 0;

    virtual antlrcpp::Any visitCursiveElement(FeatParser::CursiveElementContext *context) = 0;

    virtual antlrcpp::Any visitBaseToMarkElement(FeatParser::BaseToMarkElementContext *context) = 0;

    virtual antlrcpp::Any visitLigatureMarkElement(FeatParser::LigatureMarkElementContext *context) = 0;

    virtual antlrcpp::Any visitParameters(FeatParser::ParametersContext *context) = 0;

    virtual antlrcpp::Any visitSizemenuname(FeatParser::SizemenunameContext *context) = 0;

    virtual antlrcpp::Any visitFeatureNames(FeatParser::FeatureNamesContext *context) = 0;

    virtual antlrcpp::Any visitSubtable(FeatParser::SubtableContext *context) = 0;

    virtual antlrcpp::Any visitTable_BASE(FeatParser::Table_BASEContext *context) = 0;

    virtual antlrcpp::Any visitBaseStatement(FeatParser::BaseStatementContext *context) = 0;

    virtual antlrcpp::Any visitAxisTags(FeatParser::AxisTagsContext *context) = 0;

    virtual antlrcpp::Any visitAxisScripts(FeatParser::AxisScriptsContext *context) = 0;

    virtual antlrcpp::Any visitBaseScript(FeatParser::BaseScriptContext *context) = 0;

    virtual antlrcpp::Any visitTable_GDEF(FeatParser::Table_GDEFContext *context) = 0;

    virtual antlrcpp::Any visitGdefStatement(FeatParser::GdefStatementContext *context) = 0;

    virtual antlrcpp::Any visitGdefGlyphClass(FeatParser::GdefGlyphClassContext *context) = 0;

    virtual antlrcpp::Any visitGdefAttach(FeatParser::GdefAttachContext *context) = 0;

    virtual antlrcpp::Any visitGdefLigCaretPos(FeatParser::GdefLigCaretPosContext *context) = 0;

    virtual antlrcpp::Any visitGdefLigCaretIndex(FeatParser::GdefLigCaretIndexContext *context) = 0;

    virtual antlrcpp::Any visitTable_head(FeatParser::Table_headContext *context) = 0;

    virtual antlrcpp::Any visitHeadStatement(FeatParser::HeadStatementContext *context) = 0;

    virtual antlrcpp::Any visitHead(FeatParser::HeadContext *context) = 0;

    virtual antlrcpp::Any visitTable_hhea(FeatParser::Table_hheaContext *context) = 0;

    virtual antlrcpp::Any visitHheaStatement(FeatParser::HheaStatementContext *context) = 0;

    virtual antlrcpp::Any visitHhea(FeatParser::HheaContext *context) = 0;

    virtual antlrcpp::Any visitTable_vhea(FeatParser::Table_vheaContext *context) = 0;

    virtual antlrcpp::Any visitVheaStatement(FeatParser::VheaStatementContext *context) = 0;

    virtual antlrcpp::Any visitVhea(FeatParser::VheaContext *context) = 0;

    virtual antlrcpp::Any visitTable_name(FeatParser::Table_nameContext *context) = 0;

    virtual antlrcpp::Any visitNameStatement(FeatParser::NameStatementContext *context) = 0;

    virtual antlrcpp::Any visitNameID(FeatParser::NameIDContext *context) = 0;

    virtual antlrcpp::Any visitTable_OS_2(FeatParser::Table_OS_2Context *context) = 0;

    virtual antlrcpp::Any visitOs_2Statement(FeatParser::Os_2StatementContext *context) = 0;

    virtual antlrcpp::Any visitOs_2(FeatParser::Os_2Context *context) = 0;

    virtual antlrcpp::Any visitTable_STAT(FeatParser::Table_STATContext *context) = 0;

    virtual antlrcpp::Any visitStatStatement(FeatParser::StatStatementContext *context) = 0;

    virtual antlrcpp::Any visitDesignAxis(FeatParser::DesignAxisContext *context) = 0;

    virtual antlrcpp::Any visitAxisValue(FeatParser::AxisValueContext *context) = 0;

    virtual antlrcpp::Any visitAxisValueStatement(FeatParser::AxisValueStatementContext *context) = 0;

    virtual antlrcpp::Any visitAxisValueLocation(FeatParser::AxisValueLocationContext *context) = 0;

    virtual antlrcpp::Any visitAxisValueFlags(FeatParser::AxisValueFlagsContext *context) = 0;

    virtual antlrcpp::Any visitElidedFallbackName(FeatParser::ElidedFallbackNameContext *context) = 0;

    virtual antlrcpp::Any visitNameEntryStatement(FeatParser::NameEntryStatementContext *context) = 0;

    virtual antlrcpp::Any visitElidedFallbackNameID(FeatParser::ElidedFallbackNameIDContext *context) = 0;

    virtual antlrcpp::Any visitNameEntry(FeatParser::NameEntryContext *context) = 0;

    virtual antlrcpp::Any visitTable_vmtx(FeatParser::Table_vmtxContext *context) = 0;

    virtual antlrcpp::Any visitVmtxStatement(FeatParser::VmtxStatementContext *context) = 0;

    virtual antlrcpp::Any visitVmtx(FeatParser::VmtxContext *context) = 0;

    virtual antlrcpp::Any visitAnchor(FeatParser::AnchorContext *context) = 0;

    virtual antlrcpp::Any visitLookupPattern(FeatParser::LookupPatternContext *context) = 0;

    virtual antlrcpp::Any visitLookupPatternElement(FeatParser::LookupPatternElementContext *context) = 0;

    virtual antlrcpp::Any visitPattern(FeatParser::PatternContext *context) = 0;

    virtual antlrcpp::Any visitPatternElement(FeatParser::PatternElementContext *context) = 0;

    virtual antlrcpp::Any visitGlyphClassOptional(FeatParser::GlyphClassOptionalContext *context) = 0;

    virtual antlrcpp::Any visitGlyphClass(FeatParser::GlyphClassContext *context) = 0;

    virtual antlrcpp::Any visitGcLiteral(FeatParser::GcLiteralContext *context) = 0;

    virtual antlrcpp::Any visitGcLiteralElement(FeatParser::GcLiteralElementContext *context) = 0;

    virtual antlrcpp::Any visitGlyph(FeatParser::GlyphContext *context) = 0;

    virtual antlrcpp::Any visitGlyphName(FeatParser::GlyphNameContext *context) = 0;

    virtual antlrcpp::Any visitLabel(FeatParser::LabelContext *context) = 0;

    virtual antlrcpp::Any visitTag(FeatParser::TagContext *context) = 0;

    virtual antlrcpp::Any visitFixedNum(FeatParser::FixedNumContext *context) = 0;

    virtual antlrcpp::Any visitGenNum(FeatParser::GenNumContext *context) = 0;

    virtual antlrcpp::Any visitFeatureFile(FeatParser::FeatureFileContext *context) = 0;

    virtual antlrcpp::Any visitStatementFile(FeatParser::StatementFileContext *context) = 0;

    virtual antlrcpp::Any visitCvStatementFile(FeatParser::CvStatementFileContext *context) = 0;

    virtual antlrcpp::Any visitBaseFile(FeatParser::BaseFileContext *context) = 0;

    virtual antlrcpp::Any visitHeadFile(FeatParser::HeadFileContext *context) = 0;

    virtual antlrcpp::Any visitHheaFile(FeatParser::HheaFileContext *context) = 0;

    virtual antlrcpp::Any visitVheaFile(FeatParser::VheaFileContext *context) = 0;

    virtual antlrcpp::Any visitGdefFile(FeatParser::GdefFileContext *context) = 0;

    virtual antlrcpp::Any visitNameFile(FeatParser::NameFileContext *context) = 0;

    virtual antlrcpp::Any visitVmtxFile(FeatParser::VmtxFileContext *context) = 0;

    virtual antlrcpp::Any visitOs_2File(FeatParser::Os_2FileContext *context) = 0;

    virtual antlrcpp::Any visitStatFile(FeatParser::StatFileContext *context) = 0;

    virtual antlrcpp::Any visitAxisValueFile(FeatParser::AxisValueFileContext *context) = 0;

    virtual antlrcpp::Any visitNameEntryFile(FeatParser::NameEntryFileContext *context) = 0;

    virtual antlrcpp::Any visitSubtok(FeatParser::SubtokContext *context) = 0;

    virtual antlrcpp::Any visitRevtok(FeatParser::RevtokContext *context) = 0;

    virtual antlrcpp::Any visitAnontok(FeatParser::AnontokContext *context) = 0;

    virtual antlrcpp::Any visitEnumtok(FeatParser::EnumtokContext *context) = 0;

    virtual antlrcpp::Any visitPostok(FeatParser::PostokContext *context) = 0;

    virtual antlrcpp::Any visitMarkligtok(FeatParser::MarkligtokContext *context) = 0;


};

