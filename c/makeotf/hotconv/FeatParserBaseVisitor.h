
// Generated from FeatParser.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"
#include "FeatParserVisitor.h"


/**
 * This class provides an empty implementation of FeatParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  FeatParserBaseVisitor : public FeatParserVisitor {
public:

  virtual antlrcpp::Any visitFile(FeatParser::FileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTopLevelStatement(FeatParser::TopLevelStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitInclude(FeatParser::IncludeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGlyphClassAssign(FeatParser::GlyphClassAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLangsysAssign(FeatParser::LangsysAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMark_statement(FeatParser::Mark_statementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnchorDef(FeatParser::AnchorDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitValueRecordDef(FeatParser::ValueRecordDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFeatureBlock(FeatParser::FeatureBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTableBlock(FeatParser::TableBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnonBlock(FeatParser::AnonBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLookupBlockTopLevel(FeatParser::LookupBlockTopLevelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFeatureStatement(FeatParser::FeatureStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLookupBlockOrUse(FeatParser::LookupBlockOrUseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCvParameterBlock(FeatParser::CvParameterBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCvParameterStatement(FeatParser::CvParameterStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCvParameter(FeatParser::CvParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStatement(FeatParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFeatureUse(FeatParser::FeatureUseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitScriptAssign(FeatParser::ScriptAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLangAssign(FeatParser::LangAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLookupflagAssign(FeatParser::LookupflagAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLookupflagElement(FeatParser::LookupflagElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitIgnoreSubOrPos(FeatParser::IgnoreSubOrPosContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSubstitute(FeatParser::SubstituteContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitPosition(FeatParser::PositionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitValuePattern(FeatParser::ValuePatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitValueRecord(FeatParser::ValueRecordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitValueLiteral(FeatParser::ValueLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCursiveElement(FeatParser::CursiveElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitBaseToMarkElement(FeatParser::BaseToMarkElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLigatureMarkElement(FeatParser::LigatureMarkElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitParameters(FeatParser::ParametersContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSizemenuname(FeatParser::SizemenunameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFeatureNames(FeatParser::FeatureNamesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSubtable(FeatParser::SubtableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_BASE(FeatParser::Table_BASEContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitBaseStatement(FeatParser::BaseStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAxisTags(FeatParser::AxisTagsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAxisScripts(FeatParser::AxisScriptsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitBaseScript(FeatParser::BaseScriptContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_GDEF(FeatParser::Table_GDEFContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGdefStatement(FeatParser::GdefStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGdefGlyphClass(FeatParser::GdefGlyphClassContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGdefAttach(FeatParser::GdefAttachContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGdefLigCaretPos(FeatParser::GdefLigCaretPosContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGdefLigCaretIndex(FeatParser::GdefLigCaretIndexContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_head(FeatParser::Table_headContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitHeadStatement(FeatParser::HeadStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitHead(FeatParser::HeadContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_hhea(FeatParser::Table_hheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitHheaStatement(FeatParser::HheaStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitHhea(FeatParser::HheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_vhea(FeatParser::Table_vheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVheaStatement(FeatParser::VheaStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVhea(FeatParser::VheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_name(FeatParser::Table_nameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitNameStatement(FeatParser::NameStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitNameID(FeatParser::NameIDContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_OS_2(FeatParser::Table_OS_2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitOs_2Statement(FeatParser::Os_2StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitOs_2(FeatParser::Os_2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_STAT(FeatParser::Table_STATContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStatStatement(FeatParser::StatStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitDesignAxis(FeatParser::DesignAxisContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAxisValue(FeatParser::AxisValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAxisValueStatement(FeatParser::AxisValueStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAxisValueLocation(FeatParser::AxisValueLocationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAxisValueFlags(FeatParser::AxisValueFlagsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitElidedFallbackName(FeatParser::ElidedFallbackNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitNameEntryStatement(FeatParser::NameEntryStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitElidedFallbackNameID(FeatParser::ElidedFallbackNameIDContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitNameEntry(FeatParser::NameEntryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTable_vmtx(FeatParser::Table_vmtxContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVmtxStatement(FeatParser::VmtxStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVmtx(FeatParser::VmtxContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnchor(FeatParser::AnchorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLookupPattern(FeatParser::LookupPatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLookupPatternElement(FeatParser::LookupPatternElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitPattern(FeatParser::PatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitPatternElement(FeatParser::PatternElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGlyphClassOptional(FeatParser::GlyphClassOptionalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGlyphClass(FeatParser::GlyphClassContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGcLiteral(FeatParser::GcLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGcLiteralElement(FeatParser::GcLiteralElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGlyph(FeatParser::GlyphContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGlyphName(FeatParser::GlyphNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitLabel(FeatParser::LabelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitTag(FeatParser::TagContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFixedNum(FeatParser::FixedNumContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGenNum(FeatParser::GenNumContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFeatureFile(FeatParser::FeatureFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStatementFile(FeatParser::StatementFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitCvStatementFile(FeatParser::CvStatementFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitBaseFile(FeatParser::BaseFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitHeadFile(FeatParser::HeadFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitHheaFile(FeatParser::HheaFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVheaFile(FeatParser::VheaFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitGdefFile(FeatParser::GdefFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitNameFile(FeatParser::NameFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVmtxFile(FeatParser::VmtxFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitOs_2File(FeatParser::Os_2FileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitStatFile(FeatParser::StatFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAxisValueFile(FeatParser::AxisValueFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitNameEntryFile(FeatParser::NameEntryFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitSubtok(FeatParser::SubtokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitRevtok(FeatParser::RevtokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAnontok(FeatParser::AnontokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitEnumtok(FeatParser::EnumtokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitPostok(FeatParser::PostokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMarkligtok(FeatParser::MarkligtokContext *ctx) override {
    return visitChildren(ctx);
  }


};

