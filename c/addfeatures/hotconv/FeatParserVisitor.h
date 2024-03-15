
// Generated from FeatParser.g4 by ANTLR 4.13.1

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
    virtual std::any visitFile(FeatParser::FileContext *context) = 0;

    virtual std::any visitTopLevelStatement(FeatParser::TopLevelStatementContext *context) = 0;

    virtual std::any visitInclude(FeatParser::IncludeContext *context) = 0;

    virtual std::any visitGlyphClassAssign(FeatParser::GlyphClassAssignContext *context) = 0;

    virtual std::any visitLangsysAssign(FeatParser::LangsysAssignContext *context) = 0;

    virtual std::any visitMark_statement(FeatParser::Mark_statementContext *context) = 0;

    virtual std::any visitAnchorDef(FeatParser::AnchorDefContext *context) = 0;

    virtual std::any visitValueRecordDef(FeatParser::ValueRecordDefContext *context) = 0;

    virtual std::any visitLocationDef(FeatParser::LocationDefContext *context) = 0;

    virtual std::any visitDefaultAxisUnit(FeatParser::DefaultAxisUnitContext *context) = 0;

    virtual std::any visitFeatureBlock(FeatParser::FeatureBlockContext *context) = 0;

    virtual std::any visitTableBlock(FeatParser::TableBlockContext *context) = 0;

    virtual std::any visitAnonBlock(FeatParser::AnonBlockContext *context) = 0;

    virtual std::any visitLookupBlockTopLevel(FeatParser::LookupBlockTopLevelContext *context) = 0;

    virtual std::any visitFeatureStatement(FeatParser::FeatureStatementContext *context) = 0;

    virtual std::any visitLookupBlockOrUse(FeatParser::LookupBlockOrUseContext *context) = 0;

    virtual std::any visitCvParameterBlock(FeatParser::CvParameterBlockContext *context) = 0;

    virtual std::any visitCvParameterStatement(FeatParser::CvParameterStatementContext *context) = 0;

    virtual std::any visitCvParameter(FeatParser::CvParameterContext *context) = 0;

    virtual std::any visitStatement(FeatParser::StatementContext *context) = 0;

    virtual std::any visitFeatureUse(FeatParser::FeatureUseContext *context) = 0;

    virtual std::any visitScriptAssign(FeatParser::ScriptAssignContext *context) = 0;

    virtual std::any visitLangAssign(FeatParser::LangAssignContext *context) = 0;

    virtual std::any visitLookupflagAssign(FeatParser::LookupflagAssignContext *context) = 0;

    virtual std::any visitLookupflagElement(FeatParser::LookupflagElementContext *context) = 0;

    virtual std::any visitIgnoreSubOrPos(FeatParser::IgnoreSubOrPosContext *context) = 0;

    virtual std::any visitSubstitute(FeatParser::SubstituteContext *context) = 0;

    virtual std::any visitPosition(FeatParser::PositionContext *context) = 0;

    virtual std::any visitValuePattern(FeatParser::ValuePatternContext *context) = 0;

    virtual std::any visitValueRecord(FeatParser::ValueRecordContext *context) = 0;

    virtual std::any visitValueLiteral(FeatParser::ValueLiteralContext *context) = 0;

    virtual std::any visitSingleValueLiteral(FeatParser::SingleValueLiteralContext *context) = 0;

    virtual std::any visitParenLocationValue(FeatParser::ParenLocationValueContext *context) = 0;

    virtual std::any visitLocationValueLiteral(FeatParser::LocationValueLiteralContext *context) = 0;

    virtual std::any visitLocationMultiValueLiteral(FeatParser::LocationMultiValueLiteralContext *context) = 0;

    virtual std::any visitLocationSpecifier(FeatParser::LocationSpecifierContext *context) = 0;

    virtual std::any visitLocationLiteral(FeatParser::LocationLiteralContext *context) = 0;

    virtual std::any visitAxisLocationLiteral(FeatParser::AxisLocationLiteralContext *context) = 0;

    virtual std::any visitCursiveElement(FeatParser::CursiveElementContext *context) = 0;

    virtual std::any visitBaseToMarkElement(FeatParser::BaseToMarkElementContext *context) = 0;

    virtual std::any visitLigatureMarkElement(FeatParser::LigatureMarkElementContext *context) = 0;

    virtual std::any visitParameters(FeatParser::ParametersContext *context) = 0;

    virtual std::any visitSizemenuname(FeatParser::SizemenunameContext *context) = 0;

    virtual std::any visitFeatureNames(FeatParser::FeatureNamesContext *context) = 0;

    virtual std::any visitSubtable(FeatParser::SubtableContext *context) = 0;

    virtual std::any visitTable_BASE(FeatParser::Table_BASEContext *context) = 0;

    virtual std::any visitBaseStatement(FeatParser::BaseStatementContext *context) = 0;

    virtual std::any visitAxisTags(FeatParser::AxisTagsContext *context) = 0;

    virtual std::any visitAxisScripts(FeatParser::AxisScriptsContext *context) = 0;

    virtual std::any visitBaseScript(FeatParser::BaseScriptContext *context) = 0;

    virtual std::any visitTable_GDEF(FeatParser::Table_GDEFContext *context) = 0;

    virtual std::any visitGdefStatement(FeatParser::GdefStatementContext *context) = 0;

    virtual std::any visitGdefGlyphClass(FeatParser::GdefGlyphClassContext *context) = 0;

    virtual std::any visitGdefAttach(FeatParser::GdefAttachContext *context) = 0;

    virtual std::any visitGdefLigCaretPos(FeatParser::GdefLigCaretPosContext *context) = 0;

    virtual std::any visitGdefLigCaretIndex(FeatParser::GdefLigCaretIndexContext *context) = 0;

    virtual std::any visitTable_head(FeatParser::Table_headContext *context) = 0;

    virtual std::any visitHeadStatement(FeatParser::HeadStatementContext *context) = 0;

    virtual std::any visitHead(FeatParser::HeadContext *context) = 0;

    virtual std::any visitTable_hhea(FeatParser::Table_hheaContext *context) = 0;

    virtual std::any visitHheaStatement(FeatParser::HheaStatementContext *context) = 0;

    virtual std::any visitHhea(FeatParser::HheaContext *context) = 0;

    virtual std::any visitTable_vhea(FeatParser::Table_vheaContext *context) = 0;

    virtual std::any visitVheaStatement(FeatParser::VheaStatementContext *context) = 0;

    virtual std::any visitVhea(FeatParser::VheaContext *context) = 0;

    virtual std::any visitTable_name(FeatParser::Table_nameContext *context) = 0;

    virtual std::any visitNameStatement(FeatParser::NameStatementContext *context) = 0;

    virtual std::any visitNameID(FeatParser::NameIDContext *context) = 0;

    virtual std::any visitTable_OS_2(FeatParser::Table_OS_2Context *context) = 0;

    virtual std::any visitOs_2Statement(FeatParser::Os_2StatementContext *context) = 0;

    virtual std::any visitOs_2(FeatParser::Os_2Context *context) = 0;

    virtual std::any visitTable_STAT(FeatParser::Table_STATContext *context) = 0;

    virtual std::any visitStatStatement(FeatParser::StatStatementContext *context) = 0;

    virtual std::any visitDesignAxis(FeatParser::DesignAxisContext *context) = 0;

    virtual std::any visitAxisValue(FeatParser::AxisValueContext *context) = 0;

    virtual std::any visitAxisValueStatement(FeatParser::AxisValueStatementContext *context) = 0;

    virtual std::any visitAxisValueLocation(FeatParser::AxisValueLocationContext *context) = 0;

    virtual std::any visitAxisValueFlags(FeatParser::AxisValueFlagsContext *context) = 0;

    virtual std::any visitElidedFallbackName(FeatParser::ElidedFallbackNameContext *context) = 0;

    virtual std::any visitNameEntryStatement(FeatParser::NameEntryStatementContext *context) = 0;

    virtual std::any visitElidedFallbackNameID(FeatParser::ElidedFallbackNameIDContext *context) = 0;

    virtual std::any visitNameEntry(FeatParser::NameEntryContext *context) = 0;

    virtual std::any visitTable_vmtx(FeatParser::Table_vmtxContext *context) = 0;

    virtual std::any visitVmtxStatement(FeatParser::VmtxStatementContext *context) = 0;

    virtual std::any visitVmtx(FeatParser::VmtxContext *context) = 0;

    virtual std::any visitAnchor(FeatParser::AnchorContext *context) = 0;

    virtual std::any visitAnchorLiteral(FeatParser::AnchorLiteralContext *context) = 0;

    virtual std::any visitLookupPattern(FeatParser::LookupPatternContext *context) = 0;

    virtual std::any visitLookupPatternElement(FeatParser::LookupPatternElementContext *context) = 0;

    virtual std::any visitPattern(FeatParser::PatternContext *context) = 0;

    virtual std::any visitPatternElement(FeatParser::PatternElementContext *context) = 0;

    virtual std::any visitGlyphClassOptional(FeatParser::GlyphClassOptionalContext *context) = 0;

    virtual std::any visitGlyphClass(FeatParser::GlyphClassContext *context) = 0;

    virtual std::any visitGcLiteral(FeatParser::GcLiteralContext *context) = 0;

    virtual std::any visitGcLiteralElement(FeatParser::GcLiteralElementContext *context) = 0;

    virtual std::any visitGclass(FeatParser::GclassContext *context) = 0;

    virtual std::any visitGlyph(FeatParser::GlyphContext *context) = 0;

    virtual std::any visitGlyphName(FeatParser::GlyphNameContext *context) = 0;

    virtual std::any visitLabel(FeatParser::LabelContext *context) = 0;

    virtual std::any visitTag(FeatParser::TagContext *context) = 0;

    virtual std::any visitFixedNum(FeatParser::FixedNumContext *context) = 0;

    virtual std::any visitGenNum(FeatParser::GenNumContext *context) = 0;

    virtual std::any visitFeatureFile(FeatParser::FeatureFileContext *context) = 0;

    virtual std::any visitStatementFile(FeatParser::StatementFileContext *context) = 0;

    virtual std::any visitCvStatementFile(FeatParser::CvStatementFileContext *context) = 0;

    virtual std::any visitBaseFile(FeatParser::BaseFileContext *context) = 0;

    virtual std::any visitHeadFile(FeatParser::HeadFileContext *context) = 0;

    virtual std::any visitHheaFile(FeatParser::HheaFileContext *context) = 0;

    virtual std::any visitVheaFile(FeatParser::VheaFileContext *context) = 0;

    virtual std::any visitGdefFile(FeatParser::GdefFileContext *context) = 0;

    virtual std::any visitNameFile(FeatParser::NameFileContext *context) = 0;

    virtual std::any visitVmtxFile(FeatParser::VmtxFileContext *context) = 0;

    virtual std::any visitOs_2File(FeatParser::Os_2FileContext *context) = 0;

    virtual std::any visitStatFile(FeatParser::StatFileContext *context) = 0;

    virtual std::any visitAxisValueFile(FeatParser::AxisValueFileContext *context) = 0;

    virtual std::any visitNameEntryFile(FeatParser::NameEntryFileContext *context) = 0;

    virtual std::any visitSubtok(FeatParser::SubtokContext *context) = 0;

    virtual std::any visitRevtok(FeatParser::RevtokContext *context) = 0;

    virtual std::any visitAnontok(FeatParser::AnontokContext *context) = 0;

    virtual std::any visitEnumtok(FeatParser::EnumtokContext *context) = 0;

    virtual std::any visitPostok(FeatParser::PostokContext *context) = 0;

    virtual std::any visitMarkligtok(FeatParser::MarkligtokContext *context) = 0;


};

