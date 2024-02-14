
// Generated from FeatParser.g4 by ANTLR 4.13.1

#pragma once


#include "antlr4-runtime.h"
#include "FeatParserVisitor.h"


/**
 * This class provides an empty implementation of FeatParserVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  FeatParserBaseVisitor : public FeatParserVisitor {
public:

  virtual std::any visitFile(FeatParser::FileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTopLevelStatement(FeatParser::TopLevelStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInclude(FeatParser::IncludeContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGlyphClassAssign(FeatParser::GlyphClassAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLangsysAssign(FeatParser::LangsysAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMark_statement(FeatParser::Mark_statementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnchorDef(FeatParser::AnchorDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValueRecordDef(FeatParser::ValueRecordDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLocationDef(FeatParser::LocationDefContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureBlock(FeatParser::FeatureBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTableBlock(FeatParser::TableBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnonBlock(FeatParser::AnonBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLookupBlockTopLevel(FeatParser::LookupBlockTopLevelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureStatement(FeatParser::FeatureStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLookupBlockOrUse(FeatParser::LookupBlockOrUseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCvParameterBlock(FeatParser::CvParameterBlockContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCvParameterStatement(FeatParser::CvParameterStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCvParameter(FeatParser::CvParameterContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatement(FeatParser::StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureUse(FeatParser::FeatureUseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitScriptAssign(FeatParser::ScriptAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLangAssign(FeatParser::LangAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLookupflagAssign(FeatParser::LookupflagAssignContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLookupflagElement(FeatParser::LookupflagElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIgnoreSubOrPos(FeatParser::IgnoreSubOrPosContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubstitute(FeatParser::SubstituteContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPosition(FeatParser::PositionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValuePattern(FeatParser::ValuePatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValueRecord(FeatParser::ValueRecordContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitValueLiteral(FeatParser::ValueLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSingleValueLiteral(FeatParser::SingleValueLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParenLocationValue(FeatParser::ParenLocationValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLocationValueLiteral(FeatParser::LocationValueLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLocationMultiValueLiteral(FeatParser::LocationMultiValueLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLocationSpecifier(FeatParser::LocationSpecifierContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLocationLiteral(FeatParser::LocationLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisLocationLiteral(FeatParser::AxisLocationLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCursiveElement(FeatParser::CursiveElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBaseToMarkElement(FeatParser::BaseToMarkElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLigatureMarkElement(FeatParser::LigatureMarkElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitParameters(FeatParser::ParametersContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSizemenuname(FeatParser::SizemenunameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureNames(FeatParser::FeatureNamesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubtable(FeatParser::SubtableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_BASE(FeatParser::Table_BASEContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBaseStatement(FeatParser::BaseStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisTags(FeatParser::AxisTagsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisScripts(FeatParser::AxisScriptsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBaseScript(FeatParser::BaseScriptContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_GDEF(FeatParser::Table_GDEFContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGdefStatement(FeatParser::GdefStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGdefGlyphClass(FeatParser::GdefGlyphClassContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGdefAttach(FeatParser::GdefAttachContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGdefLigCaretPos(FeatParser::GdefLigCaretPosContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGdefLigCaretIndex(FeatParser::GdefLigCaretIndexContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_head(FeatParser::Table_headContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHeadStatement(FeatParser::HeadStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHead(FeatParser::HeadContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_hhea(FeatParser::Table_hheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHheaStatement(FeatParser::HheaStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHhea(FeatParser::HheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_vhea(FeatParser::Table_vheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVheaStatement(FeatParser::VheaStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVhea(FeatParser::VheaContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_name(FeatParser::Table_nameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNameStatement(FeatParser::NameStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNameID(FeatParser::NameIDContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_OS_2(FeatParser::Table_OS_2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOs_2Statement(FeatParser::Os_2StatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOs_2(FeatParser::Os_2Context *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_STAT(FeatParser::Table_STATContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatStatement(FeatParser::StatStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitDesignAxis(FeatParser::DesignAxisContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisValue(FeatParser::AxisValueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisValueStatement(FeatParser::AxisValueStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisValueLocation(FeatParser::AxisValueLocationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisValueFlags(FeatParser::AxisValueFlagsContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElidedFallbackName(FeatParser::ElidedFallbackNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNameEntryStatement(FeatParser::NameEntryStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitElidedFallbackNameID(FeatParser::ElidedFallbackNameIDContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNameEntry(FeatParser::NameEntryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTable_vmtx(FeatParser::Table_vmtxContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVmtxStatement(FeatParser::VmtxStatementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVmtx(FeatParser::VmtxContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnchor(FeatParser::AnchorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLookupPattern(FeatParser::LookupPatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLookupPatternElement(FeatParser::LookupPatternElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPattern(FeatParser::PatternContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPatternElement(FeatParser::PatternElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGlyphClassOptional(FeatParser::GlyphClassOptionalContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGlyphClass(FeatParser::GlyphClassContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGcLiteral(FeatParser::GcLiteralContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGcLiteralElement(FeatParser::GcLiteralElementContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGlyph(FeatParser::GlyphContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGlyphName(FeatParser::GlyphNameContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitLabel(FeatParser::LabelContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitTag(FeatParser::TagContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFixedNum(FeatParser::FixedNumContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGenNum(FeatParser::GenNumContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFeatureFile(FeatParser::FeatureFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatementFile(FeatParser::StatementFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitCvStatementFile(FeatParser::CvStatementFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBaseFile(FeatParser::BaseFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHeadFile(FeatParser::HeadFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHheaFile(FeatParser::HheaFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVheaFile(FeatParser::VheaFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitGdefFile(FeatParser::GdefFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNameFile(FeatParser::NameFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVmtxFile(FeatParser::VmtxFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitOs_2File(FeatParser::Os_2FileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitStatFile(FeatParser::StatFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAxisValueFile(FeatParser::AxisValueFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitNameEntryFile(FeatParser::NameEntryFileContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitSubtok(FeatParser::SubtokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRevtok(FeatParser::RevtokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAnontok(FeatParser::AnontokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEnumtok(FeatParser::EnumtokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPostok(FeatParser::PostokContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMarkligtok(FeatParser::MarkligtokContext *ctx) override {
    return visitChildren(ctx);
  }


};

