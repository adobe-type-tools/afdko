
// Generated from FeatParser.g4 by ANTLR 4.9.3


#include "FeatParserVisitor.h"

#include "FeatParser.h"


using namespace antlrcpp;
using namespace antlr4;

FeatParser::FeatParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

FeatParser::~FeatParser() {
  delete _interpreter;
}

std::string FeatParser::getGrammarFileName() const {
  return "FeatParser.g4";
}

const std::vector<std::string>& FeatParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& FeatParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- FileContext ------------------------------------------------------------------

FeatParser::FileContext::FileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::FileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::TopLevelStatementContext *> FeatParser::FileContext::topLevelStatement() {
  return getRuleContexts<FeatParser::TopLevelStatementContext>();
}

FeatParser::TopLevelStatementContext* FeatParser::FileContext::topLevelStatement(size_t i) {
  return getRuleContext<FeatParser::TopLevelStatementContext>(i);
}

std::vector<FeatParser::FeatureBlockContext *> FeatParser::FileContext::featureBlock() {
  return getRuleContexts<FeatParser::FeatureBlockContext>();
}

FeatParser::FeatureBlockContext* FeatParser::FileContext::featureBlock(size_t i) {
  return getRuleContext<FeatParser::FeatureBlockContext>(i);
}

std::vector<FeatParser::TableBlockContext *> FeatParser::FileContext::tableBlock() {
  return getRuleContexts<FeatParser::TableBlockContext>();
}

FeatParser::TableBlockContext* FeatParser::FileContext::tableBlock(size_t i) {
  return getRuleContext<FeatParser::TableBlockContext>(i);
}

std::vector<FeatParser::AnonBlockContext *> FeatParser::FileContext::anonBlock() {
  return getRuleContexts<FeatParser::AnonBlockContext>();
}

FeatParser::AnonBlockContext* FeatParser::FileContext::anonBlock(size_t i) {
  return getRuleContext<FeatParser::AnonBlockContext>(i);
}

std::vector<FeatParser::LookupBlockTopLevelContext *> FeatParser::FileContext::lookupBlockTopLevel() {
  return getRuleContexts<FeatParser::LookupBlockTopLevelContext>();
}

FeatParser::LookupBlockTopLevelContext* FeatParser::FileContext::lookupBlockTopLevel(size_t i) {
  return getRuleContext<FeatParser::LookupBlockTopLevelContext>(i);
}


size_t FeatParser::FileContext::getRuleIndex() const {
  return FeatParser::RuleFile;
}


antlrcpp::Any FeatParser::FileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::FileContext* FeatParser::file() {
  FileContext *_localctx = _tracker.createInstance<FileContext>(_ctx, getState());
  enterRule(_localctx, 0, FeatParser::RuleFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(229);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::ANON)
      | (1ULL << FeatParser::ANON_v)
      | (1ULL << FeatParser::INCLUDE)
      | (1ULL << FeatParser::FEATURE)
      | (1ULL << FeatParser::TABLE)
      | (1ULL << FeatParser::LANGSYS)
      | (1ULL << FeatParser::LOOKUP)
      | (1ULL << FeatParser::ANCHOR_DEF)
      | (1ULL << FeatParser::VALUE_RECORD_DEF)
      | (1ULL << FeatParser::MARK_CLASS))) != 0) || _la == FeatParser::GCLASS) {
      setState(227);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case FeatParser::INCLUDE:
        case FeatParser::LANGSYS:
        case FeatParser::ANCHOR_DEF:
        case FeatParser::VALUE_RECORD_DEF:
        case FeatParser::MARK_CLASS:
        case FeatParser::GCLASS: {
          setState(222);
          topLevelStatement();
          break;
        }

        case FeatParser::FEATURE: {
          setState(223);
          featureBlock();
          break;
        }

        case FeatParser::TABLE: {
          setState(224);
          tableBlock();
          break;
        }

        case FeatParser::ANON:
        case FeatParser::ANON_v: {
          setState(225);
          anonBlock();
          break;
        }

        case FeatParser::LOOKUP: {
          setState(226);
          lookupBlockTopLevel();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(231);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(232);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TopLevelStatementContext ------------------------------------------------------------------

FeatParser::TopLevelStatementContext::TopLevelStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::TopLevelStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::IncludeContext* FeatParser::TopLevelStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}

FeatParser::GlyphClassAssignContext* FeatParser::TopLevelStatementContext::glyphClassAssign() {
  return getRuleContext<FeatParser::GlyphClassAssignContext>(0);
}

FeatParser::LangsysAssignContext* FeatParser::TopLevelStatementContext::langsysAssign() {
  return getRuleContext<FeatParser::LangsysAssignContext>(0);
}

FeatParser::Mark_statementContext* FeatParser::TopLevelStatementContext::mark_statement() {
  return getRuleContext<FeatParser::Mark_statementContext>(0);
}

FeatParser::AnchorDefContext* FeatParser::TopLevelStatementContext::anchorDef() {
  return getRuleContext<FeatParser::AnchorDefContext>(0);
}

FeatParser::ValueRecordDefContext* FeatParser::TopLevelStatementContext::valueRecordDef() {
  return getRuleContext<FeatParser::ValueRecordDefContext>(0);
}


size_t FeatParser::TopLevelStatementContext::getRuleIndex() const {
  return FeatParser::RuleTopLevelStatement;
}


antlrcpp::Any FeatParser::TopLevelStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTopLevelStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::TopLevelStatementContext* FeatParser::topLevelStatement() {
  TopLevelStatementContext *_localctx = _tracker.createInstance<TopLevelStatementContext>(_ctx, getState());
  enterRule(_localctx, 2, FeatParser::RuleTopLevelStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(240);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::INCLUDE: {
        setState(234);
        include();
        break;
      }

      case FeatParser::GCLASS: {
        setState(235);
        glyphClassAssign();
        break;
      }

      case FeatParser::LANGSYS: {
        setState(236);
        langsysAssign();
        break;
      }

      case FeatParser::MARK_CLASS: {
        setState(237);
        mark_statement();
        break;
      }

      case FeatParser::ANCHOR_DEF: {
        setState(238);
        anchorDef();
        break;
      }

      case FeatParser::VALUE_RECORD_DEF: {
        setState(239);
        valueRecordDef();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(242);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IncludeContext ------------------------------------------------------------------

FeatParser::IncludeContext::IncludeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::IncludeContext::INCLUDE() {
  return getToken(FeatParser::INCLUDE, 0);
}

tree::TerminalNode* FeatParser::IncludeContext::I_RPAREN() {
  return getToken(FeatParser::I_RPAREN, 0);
}

tree::TerminalNode* FeatParser::IncludeContext::IFILE() {
  return getToken(FeatParser::IFILE, 0);
}

tree::TerminalNode* FeatParser::IncludeContext::I_LPAREN() {
  return getToken(FeatParser::I_LPAREN, 0);
}


size_t FeatParser::IncludeContext::getRuleIndex() const {
  return FeatParser::RuleInclude;
}


antlrcpp::Any FeatParser::IncludeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitInclude(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::IncludeContext* FeatParser::include() {
  IncludeContext *_localctx = _tracker.createInstance<IncludeContext>(_ctx, getState());
  enterRule(_localctx, 4, FeatParser::RuleInclude);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(244);
    match(FeatParser::INCLUDE);
    setState(245);
    match(FeatParser::I_RPAREN);
    setState(246);
    match(FeatParser::IFILE);
    setState(247);
    match(FeatParser::I_LPAREN);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GlyphClassAssignContext ------------------------------------------------------------------

FeatParser::GlyphClassAssignContext::GlyphClassAssignContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GlyphClassAssignContext::GCLASS() {
  return getToken(FeatParser::GCLASS, 0);
}

tree::TerminalNode* FeatParser::GlyphClassAssignContext::EQUALS() {
  return getToken(FeatParser::EQUALS, 0);
}

FeatParser::GlyphClassContext* FeatParser::GlyphClassAssignContext::glyphClass() {
  return getRuleContext<FeatParser::GlyphClassContext>(0);
}


size_t FeatParser::GlyphClassAssignContext::getRuleIndex() const {
  return FeatParser::RuleGlyphClassAssign;
}


antlrcpp::Any FeatParser::GlyphClassAssignContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGlyphClassAssign(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GlyphClassAssignContext* FeatParser::glyphClassAssign() {
  GlyphClassAssignContext *_localctx = _tracker.createInstance<GlyphClassAssignContext>(_ctx, getState());
  enterRule(_localctx, 6, FeatParser::RuleGlyphClassAssign);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(249);
    match(FeatParser::GCLASS);
    setState(250);
    match(FeatParser::EQUALS);
    setState(251);
    glyphClass();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LangsysAssignContext ------------------------------------------------------------------

FeatParser::LangsysAssignContext::LangsysAssignContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::LangsysAssignContext::LANGSYS() {
  return getToken(FeatParser::LANGSYS, 0);
}

std::vector<FeatParser::TagContext *> FeatParser::LangsysAssignContext::tag() {
  return getRuleContexts<FeatParser::TagContext>();
}

FeatParser::TagContext* FeatParser::LangsysAssignContext::tag(size_t i) {
  return getRuleContext<FeatParser::TagContext>(i);
}


size_t FeatParser::LangsysAssignContext::getRuleIndex() const {
  return FeatParser::RuleLangsysAssign;
}


antlrcpp::Any FeatParser::LangsysAssignContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLangsysAssign(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LangsysAssignContext* FeatParser::langsysAssign() {
  LangsysAssignContext *_localctx = _tracker.createInstance<LangsysAssignContext>(_ctx, getState());
  enterRule(_localctx, 8, FeatParser::RuleLangsysAssign);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(253);
    match(FeatParser::LANGSYS);
    setState(254);
    antlrcpp::downCast<LangsysAssignContext *>(_localctx)->script = tag();
    setState(255);
    antlrcpp::downCast<LangsysAssignContext *>(_localctx)->lang = tag();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Mark_statementContext ------------------------------------------------------------------

FeatParser::Mark_statementContext::Mark_statementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::Mark_statementContext::MARK_CLASS() {
  return getToken(FeatParser::MARK_CLASS, 0);
}

FeatParser::AnchorContext* FeatParser::Mark_statementContext::anchor() {
  return getRuleContext<FeatParser::AnchorContext>(0);
}

tree::TerminalNode* FeatParser::Mark_statementContext::GCLASS() {
  return getToken(FeatParser::GCLASS, 0);
}

FeatParser::GlyphContext* FeatParser::Mark_statementContext::glyph() {
  return getRuleContext<FeatParser::GlyphContext>(0);
}

FeatParser::GlyphClassContext* FeatParser::Mark_statementContext::glyphClass() {
  return getRuleContext<FeatParser::GlyphClassContext>(0);
}


size_t FeatParser::Mark_statementContext::getRuleIndex() const {
  return FeatParser::RuleMark_statement;
}


antlrcpp::Any FeatParser::Mark_statementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitMark_statement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Mark_statementContext* FeatParser::mark_statement() {
  Mark_statementContext *_localctx = _tracker.createInstance<Mark_statementContext>(_ctx, getState());
  enterRule(_localctx, 10, FeatParser::RuleMark_statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(257);
    match(FeatParser::MARK_CLASS);
    setState(260);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NOTDEF:
      case FeatParser::CID:
      case FeatParser::ESCGNAME:
      case FeatParser::NAMELABEL:
      case FeatParser::EXTNAME: {
        setState(258);
        glyph();
        break;
      }

      case FeatParser::LBRACKET:
      case FeatParser::GCLASS: {
        setState(259);
        glyphClass();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(262);
    anchor();
    setState(263);
    match(FeatParser::GCLASS);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AnchorDefContext ------------------------------------------------------------------

FeatParser::AnchorDefContext::AnchorDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AnchorDefContext::ANCHOR_DEF() {
  return getToken(FeatParser::ANCHOR_DEF, 0);
}

std::vector<tree::TerminalNode *> FeatParser::AnchorDefContext::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::AnchorDefContext::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}

FeatParser::LabelContext* FeatParser::AnchorDefContext::label() {
  return getRuleContext<FeatParser::LabelContext>(0);
}

tree::TerminalNode* FeatParser::AnchorDefContext::CONTOURPOINT() {
  return getToken(FeatParser::CONTOURPOINT, 0);
}


size_t FeatParser::AnchorDefContext::getRuleIndex() const {
  return FeatParser::RuleAnchorDef;
}


antlrcpp::Any FeatParser::AnchorDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAnchorDef(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AnchorDefContext* FeatParser::anchorDef() {
  AnchorDefContext *_localctx = _tracker.createInstance<AnchorDefContext>(_ctx, getState());
  enterRule(_localctx, 12, FeatParser::RuleAnchorDef);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(265);
    match(FeatParser::ANCHOR_DEF);
    setState(266);
    antlrcpp::downCast<AnchorDefContext *>(_localctx)->xval = match(FeatParser::NUM);
    setState(267);
    antlrcpp::downCast<AnchorDefContext *>(_localctx)->yval = match(FeatParser::NUM);
    setState(270);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::CONTOURPOINT) {
      setState(268);
      match(FeatParser::CONTOURPOINT);
      setState(269);
      antlrcpp::downCast<AnchorDefContext *>(_localctx)->cp = match(FeatParser::NUM);
    }
    setState(272);
    antlrcpp::downCast<AnchorDefContext *>(_localctx)->name = label();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValueRecordDefContext ------------------------------------------------------------------

FeatParser::ValueRecordDefContext::ValueRecordDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::ValueRecordDefContext::VALUE_RECORD_DEF() {
  return getToken(FeatParser::VALUE_RECORD_DEF, 0);
}

FeatParser::ValueLiteralContext* FeatParser::ValueRecordDefContext::valueLiteral() {
  return getRuleContext<FeatParser::ValueLiteralContext>(0);
}

FeatParser::LabelContext* FeatParser::ValueRecordDefContext::label() {
  return getRuleContext<FeatParser::LabelContext>(0);
}


size_t FeatParser::ValueRecordDefContext::getRuleIndex() const {
  return FeatParser::RuleValueRecordDef;
}


antlrcpp::Any FeatParser::ValueRecordDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitValueRecordDef(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ValueRecordDefContext* FeatParser::valueRecordDef() {
  ValueRecordDefContext *_localctx = _tracker.createInstance<ValueRecordDefContext>(_ctx, getState());
  enterRule(_localctx, 14, FeatParser::RuleValueRecordDef);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(274);
    match(FeatParser::VALUE_RECORD_DEF);
    setState(275);
    valueLiteral();
    setState(276);
    label();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FeatureBlockContext ------------------------------------------------------------------

FeatParser::FeatureBlockContext::FeatureBlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::FeatureBlockContext::FEATURE() {
  return getToken(FeatParser::FEATURE, 0);
}

tree::TerminalNode* FeatParser::FeatureBlockContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::FeatureBlockContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::FeatureBlockContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::TagContext *> FeatParser::FeatureBlockContext::tag() {
  return getRuleContexts<FeatParser::TagContext>();
}

FeatParser::TagContext* FeatParser::FeatureBlockContext::tag(size_t i) {
  return getRuleContext<FeatParser::TagContext>(i);
}

tree::TerminalNode* FeatParser::FeatureBlockContext::USE_EXTENSION() {
  return getToken(FeatParser::USE_EXTENSION, 0);
}

std::vector<FeatParser::FeatureStatementContext *> FeatParser::FeatureBlockContext::featureStatement() {
  return getRuleContexts<FeatParser::FeatureStatementContext>();
}

FeatParser::FeatureStatementContext* FeatParser::FeatureBlockContext::featureStatement(size_t i) {
  return getRuleContext<FeatParser::FeatureStatementContext>(i);
}


size_t FeatParser::FeatureBlockContext::getRuleIndex() const {
  return FeatParser::RuleFeatureBlock;
}


antlrcpp::Any FeatParser::FeatureBlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitFeatureBlock(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::FeatureBlockContext* FeatParser::featureBlock() {
  FeatureBlockContext *_localctx = _tracker.createInstance<FeatureBlockContext>(_ctx, getState());
  enterRule(_localctx, 16, FeatParser::RuleFeatureBlock);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(278);
    match(FeatParser::FEATURE);
    setState(279);
    antlrcpp::downCast<FeatureBlockContext *>(_localctx)->starttag = tag();
    setState(281);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::USE_EXTENSION) {
      setState(280);
      match(FeatParser::USE_EXTENSION);
    }
    setState(283);
    match(FeatParser::LCBRACE);
    setState(285); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(284);
      featureStatement();
      setState(287); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::INCLUDE)
      | (1ULL << FeatParser::FEATURE)
      | (1ULL << FeatParser::SCRIPT)
      | (1ULL << FeatParser::LANGUAGE)
      | (1ULL << FeatParser::SUBTABLE)
      | (1ULL << FeatParser::LOOKUP)
      | (1ULL << FeatParser::LOOKUPFLAG)
      | (1ULL << FeatParser::ENUMERATE)
      | (1ULL << FeatParser::ENUMERATE_v)
      | (1ULL << FeatParser::EXCEPT)
      | (1ULL << FeatParser::IGNORE)
      | (1ULL << FeatParser::SUBSTITUTE)
      | (1ULL << FeatParser::SUBSTITUTE_v)
      | (1ULL << FeatParser::REVERSE)
      | (1ULL << FeatParser::REVERSE_v)
      | (1ULL << FeatParser::POSITION)
      | (1ULL << FeatParser::POSITION_v)
      | (1ULL << FeatParser::PARAMETERS)
      | (1ULL << FeatParser::FEATURE_NAMES)
      | (1ULL << FeatParser::CV_PARAMETERS)
      | (1ULL << FeatParser::SIZEMENUNAME)
      | (1ULL << FeatParser::MARK_CLASS))) != 0) || _la == FeatParser::GCLASS);
    setState(289);
    match(FeatParser::RCBRACE);
    setState(290);
    antlrcpp::downCast<FeatureBlockContext *>(_localctx)->endtag = tag();
    setState(291);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TableBlockContext ------------------------------------------------------------------

FeatParser::TableBlockContext::TableBlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::TableBlockContext::TABLE() {
  return getToken(FeatParser::TABLE, 0);
}

FeatParser::Table_BASEContext* FeatParser::TableBlockContext::table_BASE() {
  return getRuleContext<FeatParser::Table_BASEContext>(0);
}

FeatParser::Table_GDEFContext* FeatParser::TableBlockContext::table_GDEF() {
  return getRuleContext<FeatParser::Table_GDEFContext>(0);
}

FeatParser::Table_headContext* FeatParser::TableBlockContext::table_head() {
  return getRuleContext<FeatParser::Table_headContext>(0);
}

FeatParser::Table_hheaContext* FeatParser::TableBlockContext::table_hhea() {
  return getRuleContext<FeatParser::Table_hheaContext>(0);
}

FeatParser::Table_vheaContext* FeatParser::TableBlockContext::table_vhea() {
  return getRuleContext<FeatParser::Table_vheaContext>(0);
}

FeatParser::Table_nameContext* FeatParser::TableBlockContext::table_name() {
  return getRuleContext<FeatParser::Table_nameContext>(0);
}

FeatParser::Table_OS_2Context* FeatParser::TableBlockContext::table_OS_2() {
  return getRuleContext<FeatParser::Table_OS_2Context>(0);
}

FeatParser::Table_STATContext* FeatParser::TableBlockContext::table_STAT() {
  return getRuleContext<FeatParser::Table_STATContext>(0);
}

FeatParser::Table_vmtxContext* FeatParser::TableBlockContext::table_vmtx() {
  return getRuleContext<FeatParser::Table_vmtxContext>(0);
}


size_t FeatParser::TableBlockContext::getRuleIndex() const {
  return FeatParser::RuleTableBlock;
}


antlrcpp::Any FeatParser::TableBlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTableBlock(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::TableBlockContext* FeatParser::tableBlock() {
  TableBlockContext *_localctx = _tracker.createInstance<TableBlockContext>(_ctx, getState());
  enterRule(_localctx, 18, FeatParser::RuleTableBlock);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(293);
    match(FeatParser::TABLE);
    setState(303);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::BASE: {
        setState(294);
        table_BASE();
        break;
      }

      case FeatParser::GDEF: {
        setState(295);
        table_GDEF();
        break;
      }

      case FeatParser::HEAD: {
        setState(296);
        table_head();
        break;
      }

      case FeatParser::HHEA: {
        setState(297);
        table_hhea();
        break;
      }

      case FeatParser::VHEA: {
        setState(298);
        table_vhea();
        break;
      }

      case FeatParser::NAME: {
        setState(299);
        table_name();
        break;
      }

      case FeatParser::OS_2: {
        setState(300);
        table_OS_2();
        break;
      }

      case FeatParser::STAT: {
        setState(301);
        table_STAT();
        break;
      }

      case FeatParser::VMTX: {
        setState(302);
        table_vmtx();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AnonBlockContext ------------------------------------------------------------------

FeatParser::AnonBlockContext::AnonBlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::AnontokContext* FeatParser::AnonBlockContext::anontok() {
  return getRuleContext<FeatParser::AnontokContext>(0);
}

tree::TerminalNode* FeatParser::AnonBlockContext::A_LABEL() {
  return getToken(FeatParser::A_LABEL, 0);
}

tree::TerminalNode* FeatParser::AnonBlockContext::A_LBRACE() {
  return getToken(FeatParser::A_LBRACE, 0);
}

tree::TerminalNode* FeatParser::AnonBlockContext::A_CLOSE() {
  return getToken(FeatParser::A_CLOSE, 0);
}

std::vector<tree::TerminalNode *> FeatParser::AnonBlockContext::A_LINE() {
  return getTokens(FeatParser::A_LINE);
}

tree::TerminalNode* FeatParser::AnonBlockContext::A_LINE(size_t i) {
  return getToken(FeatParser::A_LINE, i);
}


size_t FeatParser::AnonBlockContext::getRuleIndex() const {
  return FeatParser::RuleAnonBlock;
}


antlrcpp::Any FeatParser::AnonBlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAnonBlock(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AnonBlockContext* FeatParser::anonBlock() {
  AnonBlockContext *_localctx = _tracker.createInstance<AnonBlockContext>(_ctx, getState());
  enterRule(_localctx, 20, FeatParser::RuleAnonBlock);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(305);
    anontok();
    setState(306);
    match(FeatParser::A_LABEL);
    setState(307);
    match(FeatParser::A_LBRACE);
    setState(311);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::A_LINE) {
      setState(308);
      match(FeatParser::A_LINE);
      setState(313);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(314);
    match(FeatParser::A_CLOSE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LookupBlockTopLevelContext ------------------------------------------------------------------

FeatParser::LookupBlockTopLevelContext::LookupBlockTopLevelContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::LookupBlockTopLevelContext::LOOKUP() {
  return getToken(FeatParser::LOOKUP, 0);
}

tree::TerminalNode* FeatParser::LookupBlockTopLevelContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::LookupBlockTopLevelContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::LookupBlockTopLevelContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::LabelContext *> FeatParser::LookupBlockTopLevelContext::label() {
  return getRuleContexts<FeatParser::LabelContext>();
}

FeatParser::LabelContext* FeatParser::LookupBlockTopLevelContext::label(size_t i) {
  return getRuleContext<FeatParser::LabelContext>(i);
}

tree::TerminalNode* FeatParser::LookupBlockTopLevelContext::USE_EXTENSION() {
  return getToken(FeatParser::USE_EXTENSION, 0);
}

std::vector<FeatParser::StatementContext *> FeatParser::LookupBlockTopLevelContext::statement() {
  return getRuleContexts<FeatParser::StatementContext>();
}

FeatParser::StatementContext* FeatParser::LookupBlockTopLevelContext::statement(size_t i) {
  return getRuleContext<FeatParser::StatementContext>(i);
}


size_t FeatParser::LookupBlockTopLevelContext::getRuleIndex() const {
  return FeatParser::RuleLookupBlockTopLevel;
}


antlrcpp::Any FeatParser::LookupBlockTopLevelContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLookupBlockTopLevel(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LookupBlockTopLevelContext* FeatParser::lookupBlockTopLevel() {
  LookupBlockTopLevelContext *_localctx = _tracker.createInstance<LookupBlockTopLevelContext>(_ctx, getState());
  enterRule(_localctx, 22, FeatParser::RuleLookupBlockTopLevel);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(316);
    match(FeatParser::LOOKUP);
    setState(317);
    antlrcpp::downCast<LookupBlockTopLevelContext *>(_localctx)->startlabel = label();
    setState(319);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::USE_EXTENSION) {
      setState(318);
      match(FeatParser::USE_EXTENSION);
    }
    setState(321);
    match(FeatParser::LCBRACE);
    setState(323); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(322);
      statement();
      setState(325); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::INCLUDE)
      | (1ULL << FeatParser::FEATURE)
      | (1ULL << FeatParser::SCRIPT)
      | (1ULL << FeatParser::LANGUAGE)
      | (1ULL << FeatParser::SUBTABLE)
      | (1ULL << FeatParser::LOOKUPFLAG)
      | (1ULL << FeatParser::ENUMERATE)
      | (1ULL << FeatParser::ENUMERATE_v)
      | (1ULL << FeatParser::EXCEPT)
      | (1ULL << FeatParser::IGNORE)
      | (1ULL << FeatParser::SUBSTITUTE)
      | (1ULL << FeatParser::SUBSTITUTE_v)
      | (1ULL << FeatParser::REVERSE)
      | (1ULL << FeatParser::REVERSE_v)
      | (1ULL << FeatParser::POSITION)
      | (1ULL << FeatParser::POSITION_v)
      | (1ULL << FeatParser::PARAMETERS)
      | (1ULL << FeatParser::FEATURE_NAMES)
      | (1ULL << FeatParser::SIZEMENUNAME)
      | (1ULL << FeatParser::MARK_CLASS))) != 0) || _la == FeatParser::GCLASS);
    setState(327);
    match(FeatParser::RCBRACE);
    setState(328);
    antlrcpp::downCast<LookupBlockTopLevelContext *>(_localctx)->endlabel = label();
    setState(329);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FeatureStatementContext ------------------------------------------------------------------

FeatParser::FeatureStatementContext::FeatureStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::StatementContext* FeatParser::FeatureStatementContext::statement() {
  return getRuleContext<FeatParser::StatementContext>(0);
}

FeatParser::LookupBlockOrUseContext* FeatParser::FeatureStatementContext::lookupBlockOrUse() {
  return getRuleContext<FeatParser::LookupBlockOrUseContext>(0);
}

FeatParser::CvParameterBlockContext* FeatParser::FeatureStatementContext::cvParameterBlock() {
  return getRuleContext<FeatParser::CvParameterBlockContext>(0);
}


size_t FeatParser::FeatureStatementContext::getRuleIndex() const {
  return FeatParser::RuleFeatureStatement;
}


antlrcpp::Any FeatParser::FeatureStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitFeatureStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::FeatureStatementContext* FeatParser::featureStatement() {
  FeatureStatementContext *_localctx = _tracker.createInstance<FeatureStatementContext>(_ctx, getState());
  enterRule(_localctx, 24, FeatParser::RuleFeatureStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(334);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::INCLUDE:
      case FeatParser::FEATURE:
      case FeatParser::SCRIPT:
      case FeatParser::LANGUAGE:
      case FeatParser::SUBTABLE:
      case FeatParser::LOOKUPFLAG:
      case FeatParser::ENUMERATE:
      case FeatParser::ENUMERATE_v:
      case FeatParser::EXCEPT:
      case FeatParser::IGNORE:
      case FeatParser::SUBSTITUTE:
      case FeatParser::SUBSTITUTE_v:
      case FeatParser::REVERSE:
      case FeatParser::REVERSE_v:
      case FeatParser::POSITION:
      case FeatParser::POSITION_v:
      case FeatParser::PARAMETERS:
      case FeatParser::FEATURE_NAMES:
      case FeatParser::SIZEMENUNAME:
      case FeatParser::MARK_CLASS:
      case FeatParser::GCLASS: {
        enterOuterAlt(_localctx, 1);
        setState(331);
        statement();
        break;
      }

      case FeatParser::LOOKUP: {
        enterOuterAlt(_localctx, 2);
        setState(332);
        lookupBlockOrUse();
        break;
      }

      case FeatParser::CV_PARAMETERS: {
        enterOuterAlt(_localctx, 3);
        setState(333);
        cvParameterBlock();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LookupBlockOrUseContext ------------------------------------------------------------------

FeatParser::LookupBlockOrUseContext::LookupBlockOrUseContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::LookupBlockOrUseContext::LOOKUP() {
  return getToken(FeatParser::LOOKUP, 0);
}

tree::TerminalNode* FeatParser::LookupBlockOrUseContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::LabelContext *> FeatParser::LookupBlockOrUseContext::label() {
  return getRuleContexts<FeatParser::LabelContext>();
}

FeatParser::LabelContext* FeatParser::LookupBlockOrUseContext::label(size_t i) {
  return getRuleContext<FeatParser::LabelContext>(i);
}

tree::TerminalNode* FeatParser::LookupBlockOrUseContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::LookupBlockOrUseContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::LookupBlockOrUseContext::USE_EXTENSION() {
  return getToken(FeatParser::USE_EXTENSION, 0);
}

std::vector<FeatParser::StatementContext *> FeatParser::LookupBlockOrUseContext::statement() {
  return getRuleContexts<FeatParser::StatementContext>();
}

FeatParser::StatementContext* FeatParser::LookupBlockOrUseContext::statement(size_t i) {
  return getRuleContext<FeatParser::StatementContext>(i);
}


size_t FeatParser::LookupBlockOrUseContext::getRuleIndex() const {
  return FeatParser::RuleLookupBlockOrUse;
}


antlrcpp::Any FeatParser::LookupBlockOrUseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLookupBlockOrUse(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LookupBlockOrUseContext* FeatParser::lookupBlockOrUse() {
  LookupBlockOrUseContext *_localctx = _tracker.createInstance<LookupBlockOrUseContext>(_ctx, getState());
  enterRule(_localctx, 26, FeatParser::RuleLookupBlockOrUse);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(336);
    match(FeatParser::LOOKUP);
    setState(337);
    antlrcpp::downCast<LookupBlockOrUseContext *>(_localctx)->startlabel = label();
    setState(350);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::USE_EXTENSION || _la == FeatParser::LCBRACE) {
      setState(339);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == FeatParser::USE_EXTENSION) {
        setState(338);
        match(FeatParser::USE_EXTENSION);
      }
      setState(341);
      match(FeatParser::LCBRACE);
      setState(343); 
      _errHandler->sync(this);
      _la = _input->LA(1);
      do {
        setState(342);
        statement();
        setState(345); 
        _errHandler->sync(this);
        _la = _input->LA(1);
      } while ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & ((1ULL << FeatParser::INCLUDE)
        | (1ULL << FeatParser::FEATURE)
        | (1ULL << FeatParser::SCRIPT)
        | (1ULL << FeatParser::LANGUAGE)
        | (1ULL << FeatParser::SUBTABLE)
        | (1ULL << FeatParser::LOOKUPFLAG)
        | (1ULL << FeatParser::ENUMERATE)
        | (1ULL << FeatParser::ENUMERATE_v)
        | (1ULL << FeatParser::EXCEPT)
        | (1ULL << FeatParser::IGNORE)
        | (1ULL << FeatParser::SUBSTITUTE)
        | (1ULL << FeatParser::SUBSTITUTE_v)
        | (1ULL << FeatParser::REVERSE)
        | (1ULL << FeatParser::REVERSE_v)
        | (1ULL << FeatParser::POSITION)
        | (1ULL << FeatParser::POSITION_v)
        | (1ULL << FeatParser::PARAMETERS)
        | (1ULL << FeatParser::FEATURE_NAMES)
        | (1ULL << FeatParser::SIZEMENUNAME)
        | (1ULL << FeatParser::MARK_CLASS))) != 0) || _la == FeatParser::GCLASS);
      setState(347);
      match(FeatParser::RCBRACE);
      setState(348);
      antlrcpp::downCast<LookupBlockOrUseContext *>(_localctx)->endlabel = label();
    }
    setState(352);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CvParameterBlockContext ------------------------------------------------------------------

FeatParser::CvParameterBlockContext::CvParameterBlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::CvParameterBlockContext::CV_PARAMETERS() {
  return getToken(FeatParser::CV_PARAMETERS, 0);
}

tree::TerminalNode* FeatParser::CvParameterBlockContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::CvParameterBlockContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::CvParameterBlockContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::CvParameterStatementContext *> FeatParser::CvParameterBlockContext::cvParameterStatement() {
  return getRuleContexts<FeatParser::CvParameterStatementContext>();
}

FeatParser::CvParameterStatementContext* FeatParser::CvParameterBlockContext::cvParameterStatement(size_t i) {
  return getRuleContext<FeatParser::CvParameterStatementContext>(i);
}


size_t FeatParser::CvParameterBlockContext::getRuleIndex() const {
  return FeatParser::RuleCvParameterBlock;
}


antlrcpp::Any FeatParser::CvParameterBlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitCvParameterBlock(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::CvParameterBlockContext* FeatParser::cvParameterBlock() {
  CvParameterBlockContext *_localctx = _tracker.createInstance<CvParameterBlockContext>(_ctx, getState());
  enterRule(_localctx, 28, FeatParser::RuleCvParameterBlock);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(354);
    match(FeatParser::CV_PARAMETERS);
    setState(355);
    match(FeatParser::LCBRACE);
    setState(359);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::INCLUDE)
      | (1ULL << FeatParser::CV_UI_LABEL)
      | (1ULL << FeatParser::CV_TOOLTIP)
      | (1ULL << FeatParser::CV_SAMPLE_TEXT)
      | (1ULL << FeatParser::CV_PARAM_LABEL)
      | (1ULL << FeatParser::CV_CHARACTER))) != 0)) {
      setState(356);
      cvParameterStatement();
      setState(361);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(362);
    match(FeatParser::RCBRACE);
    setState(363);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CvParameterStatementContext ------------------------------------------------------------------

FeatParser::CvParameterStatementContext::CvParameterStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::CvParameterStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::CvParameterContext* FeatParser::CvParameterStatementContext::cvParameter() {
  return getRuleContext<FeatParser::CvParameterContext>(0);
}

FeatParser::IncludeContext* FeatParser::CvParameterStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::CvParameterStatementContext::getRuleIndex() const {
  return FeatParser::RuleCvParameterStatement;
}


antlrcpp::Any FeatParser::CvParameterStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitCvParameterStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::CvParameterStatementContext* FeatParser::cvParameterStatement() {
  CvParameterStatementContext *_localctx = _tracker.createInstance<CvParameterStatementContext>(_ctx, getState());
  enterRule(_localctx, 30, FeatParser::RuleCvParameterStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(367);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::CV_UI_LABEL:
      case FeatParser::CV_TOOLTIP:
      case FeatParser::CV_SAMPLE_TEXT:
      case FeatParser::CV_PARAM_LABEL:
      case FeatParser::CV_CHARACTER: {
        setState(365);
        cvParameter();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(366);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(369);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CvParameterContext ------------------------------------------------------------------

FeatParser::CvParameterContext::CvParameterContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::CvParameterContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::CvParameterContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::CvParameterContext::CV_UI_LABEL() {
  return getToken(FeatParser::CV_UI_LABEL, 0);
}

tree::TerminalNode* FeatParser::CvParameterContext::CV_TOOLTIP() {
  return getToken(FeatParser::CV_TOOLTIP, 0);
}

tree::TerminalNode* FeatParser::CvParameterContext::CV_SAMPLE_TEXT() {
  return getToken(FeatParser::CV_SAMPLE_TEXT, 0);
}

tree::TerminalNode* FeatParser::CvParameterContext::CV_PARAM_LABEL() {
  return getToken(FeatParser::CV_PARAM_LABEL, 0);
}

std::vector<FeatParser::NameEntryStatementContext *> FeatParser::CvParameterContext::nameEntryStatement() {
  return getRuleContexts<FeatParser::NameEntryStatementContext>();
}

FeatParser::NameEntryStatementContext* FeatParser::CvParameterContext::nameEntryStatement(size_t i) {
  return getRuleContext<FeatParser::NameEntryStatementContext>(i);
}

tree::TerminalNode* FeatParser::CvParameterContext::CV_CHARACTER() {
  return getToken(FeatParser::CV_CHARACTER, 0);
}

FeatParser::GenNumContext* FeatParser::CvParameterContext::genNum() {
  return getRuleContext<FeatParser::GenNumContext>(0);
}


size_t FeatParser::CvParameterContext::getRuleIndex() const {
  return FeatParser::RuleCvParameter;
}


antlrcpp::Any FeatParser::CvParameterContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitCvParameter(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::CvParameterContext* FeatParser::cvParameter() {
  CvParameterContext *_localctx = _tracker.createInstance<CvParameterContext>(_ctx, getState());
  enterRule(_localctx, 32, FeatParser::RuleCvParameter);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(382);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::CV_UI_LABEL:
      case FeatParser::CV_TOOLTIP:
      case FeatParser::CV_SAMPLE_TEXT:
      case FeatParser::CV_PARAM_LABEL: {
        enterOuterAlt(_localctx, 1);
        setState(371);
        _la = _input->LA(1);
        if (!((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << FeatParser::CV_UI_LABEL)
          | (1ULL << FeatParser::CV_TOOLTIP)
          | (1ULL << FeatParser::CV_SAMPLE_TEXT)
          | (1ULL << FeatParser::CV_PARAM_LABEL))) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(372);
        match(FeatParser::LCBRACE);
        setState(374); 
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
          setState(373);
          nameEntryStatement();
          setState(376); 
          _errHandler->sync(this);
          _la = _input->LA(1);
        } while (_la == FeatParser::INCLUDE || _la == FeatParser::NAME);
        setState(378);
        match(FeatParser::RCBRACE);
        break;
      }

      case FeatParser::CV_CHARACTER: {
        enterOuterAlt(_localctx, 2);
        setState(380);
        match(FeatParser::CV_CHARACTER);
        setState(381);
        genNum();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementContext ------------------------------------------------------------------

FeatParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::StatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::FeatureUseContext* FeatParser::StatementContext::featureUse() {
  return getRuleContext<FeatParser::FeatureUseContext>(0);
}

FeatParser::ScriptAssignContext* FeatParser::StatementContext::scriptAssign() {
  return getRuleContext<FeatParser::ScriptAssignContext>(0);
}

FeatParser::LangAssignContext* FeatParser::StatementContext::langAssign() {
  return getRuleContext<FeatParser::LangAssignContext>(0);
}

FeatParser::LookupflagAssignContext* FeatParser::StatementContext::lookupflagAssign() {
  return getRuleContext<FeatParser::LookupflagAssignContext>(0);
}

FeatParser::GlyphClassAssignContext* FeatParser::StatementContext::glyphClassAssign() {
  return getRuleContext<FeatParser::GlyphClassAssignContext>(0);
}

FeatParser::IgnoreSubOrPosContext* FeatParser::StatementContext::ignoreSubOrPos() {
  return getRuleContext<FeatParser::IgnoreSubOrPosContext>(0);
}

FeatParser::SubstituteContext* FeatParser::StatementContext::substitute() {
  return getRuleContext<FeatParser::SubstituteContext>(0);
}

FeatParser::Mark_statementContext* FeatParser::StatementContext::mark_statement() {
  return getRuleContext<FeatParser::Mark_statementContext>(0);
}

FeatParser::PositionContext* FeatParser::StatementContext::position() {
  return getRuleContext<FeatParser::PositionContext>(0);
}

FeatParser::ParametersContext* FeatParser::StatementContext::parameters() {
  return getRuleContext<FeatParser::ParametersContext>(0);
}

FeatParser::SizemenunameContext* FeatParser::StatementContext::sizemenuname() {
  return getRuleContext<FeatParser::SizemenunameContext>(0);
}

FeatParser::FeatureNamesContext* FeatParser::StatementContext::featureNames() {
  return getRuleContext<FeatParser::FeatureNamesContext>(0);
}

FeatParser::SubtableContext* FeatParser::StatementContext::subtable() {
  return getRuleContext<FeatParser::SubtableContext>(0);
}

FeatParser::IncludeContext* FeatParser::StatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::StatementContext::getRuleIndex() const {
  return FeatParser::RuleStatement;
}


antlrcpp::Any FeatParser::StatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::StatementContext* FeatParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 34, FeatParser::RuleStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(398);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::FEATURE: {
        setState(384);
        featureUse();
        break;
      }

      case FeatParser::SCRIPT: {
        setState(385);
        scriptAssign();
        break;
      }

      case FeatParser::LANGUAGE: {
        setState(386);
        langAssign();
        break;
      }

      case FeatParser::LOOKUPFLAG: {
        setState(387);
        lookupflagAssign();
        break;
      }

      case FeatParser::GCLASS: {
        setState(388);
        glyphClassAssign();
        break;
      }

      case FeatParser::IGNORE: {
        setState(389);
        ignoreSubOrPos();
        break;
      }

      case FeatParser::EXCEPT:
      case FeatParser::SUBSTITUTE:
      case FeatParser::SUBSTITUTE_v:
      case FeatParser::REVERSE:
      case FeatParser::REVERSE_v: {
        setState(390);
        substitute();
        break;
      }

      case FeatParser::MARK_CLASS: {
        setState(391);
        mark_statement();
        break;
      }

      case FeatParser::ENUMERATE:
      case FeatParser::ENUMERATE_v:
      case FeatParser::POSITION:
      case FeatParser::POSITION_v: {
        setState(392);
        position();
        break;
      }

      case FeatParser::PARAMETERS: {
        setState(393);
        parameters();
        break;
      }

      case FeatParser::SIZEMENUNAME: {
        setState(394);
        sizemenuname();
        break;
      }

      case FeatParser::FEATURE_NAMES: {
        setState(395);
        featureNames();
        break;
      }

      case FeatParser::SUBTABLE: {
        setState(396);
        subtable();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(397);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(400);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FeatureUseContext ------------------------------------------------------------------

FeatParser::FeatureUseContext::FeatureUseContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::FeatureUseContext::FEATURE() {
  return getToken(FeatParser::FEATURE, 0);
}

FeatParser::TagContext* FeatParser::FeatureUseContext::tag() {
  return getRuleContext<FeatParser::TagContext>(0);
}


size_t FeatParser::FeatureUseContext::getRuleIndex() const {
  return FeatParser::RuleFeatureUse;
}


antlrcpp::Any FeatParser::FeatureUseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitFeatureUse(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::FeatureUseContext* FeatParser::featureUse() {
  FeatureUseContext *_localctx = _tracker.createInstance<FeatureUseContext>(_ctx, getState());
  enterRule(_localctx, 36, FeatParser::RuleFeatureUse);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(402);
    match(FeatParser::FEATURE);
    setState(403);
    tag();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ScriptAssignContext ------------------------------------------------------------------

FeatParser::ScriptAssignContext::ScriptAssignContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::ScriptAssignContext::SCRIPT() {
  return getToken(FeatParser::SCRIPT, 0);
}

FeatParser::TagContext* FeatParser::ScriptAssignContext::tag() {
  return getRuleContext<FeatParser::TagContext>(0);
}


size_t FeatParser::ScriptAssignContext::getRuleIndex() const {
  return FeatParser::RuleScriptAssign;
}


antlrcpp::Any FeatParser::ScriptAssignContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitScriptAssign(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ScriptAssignContext* FeatParser::scriptAssign() {
  ScriptAssignContext *_localctx = _tracker.createInstance<ScriptAssignContext>(_ctx, getState());
  enterRule(_localctx, 38, FeatParser::RuleScriptAssign);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(405);
    match(FeatParser::SCRIPT);
    setState(406);
    tag();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LangAssignContext ------------------------------------------------------------------

FeatParser::LangAssignContext::LangAssignContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::LangAssignContext::LANGUAGE() {
  return getToken(FeatParser::LANGUAGE, 0);
}

FeatParser::TagContext* FeatParser::LangAssignContext::tag() {
  return getRuleContext<FeatParser::TagContext>(0);
}

tree::TerminalNode* FeatParser::LangAssignContext::EXCLUDE_DFLT() {
  return getToken(FeatParser::EXCLUDE_DFLT, 0);
}

tree::TerminalNode* FeatParser::LangAssignContext::INCLUDE_DFLT() {
  return getToken(FeatParser::INCLUDE_DFLT, 0);
}

tree::TerminalNode* FeatParser::LangAssignContext::EXCLUDE_dflt() {
  return getToken(FeatParser::EXCLUDE_dflt, 0);
}

tree::TerminalNode* FeatParser::LangAssignContext::INCLUDE_dflt() {
  return getToken(FeatParser::INCLUDE_dflt, 0);
}


size_t FeatParser::LangAssignContext::getRuleIndex() const {
  return FeatParser::RuleLangAssign;
}


antlrcpp::Any FeatParser::LangAssignContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLangAssign(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LangAssignContext* FeatParser::langAssign() {
  LangAssignContext *_localctx = _tracker.createInstance<LangAssignContext>(_ctx, getState());
  enterRule(_localctx, 40, FeatParser::RuleLangAssign);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(408);
    match(FeatParser::LANGUAGE);
    setState(409);
    tag();
    setState(411);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::EXCLUDE_DFLT)
      | (1ULL << FeatParser::INCLUDE_DFLT)
      | (1ULL << FeatParser::EXCLUDE_dflt)
      | (1ULL << FeatParser::INCLUDE_dflt))) != 0)) {
      setState(410);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & ((1ULL << FeatParser::EXCLUDE_DFLT)
        | (1ULL << FeatParser::INCLUDE_DFLT)
        | (1ULL << FeatParser::EXCLUDE_dflt)
        | (1ULL << FeatParser::INCLUDE_dflt))) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LookupflagAssignContext ------------------------------------------------------------------

FeatParser::LookupflagAssignContext::LookupflagAssignContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::LookupflagAssignContext::LOOKUPFLAG() {
  return getToken(FeatParser::LOOKUPFLAG, 0);
}

tree::TerminalNode* FeatParser::LookupflagAssignContext::NUM() {
  return getToken(FeatParser::NUM, 0);
}

std::vector<FeatParser::LookupflagElementContext *> FeatParser::LookupflagAssignContext::lookupflagElement() {
  return getRuleContexts<FeatParser::LookupflagElementContext>();
}

FeatParser::LookupflagElementContext* FeatParser::LookupflagAssignContext::lookupflagElement(size_t i) {
  return getRuleContext<FeatParser::LookupflagElementContext>(i);
}


size_t FeatParser::LookupflagAssignContext::getRuleIndex() const {
  return FeatParser::RuleLookupflagAssign;
}


antlrcpp::Any FeatParser::LookupflagAssignContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLookupflagAssign(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LookupflagAssignContext* FeatParser::lookupflagAssign() {
  LookupflagAssignContext *_localctx = _tracker.createInstance<LookupflagAssignContext>(_ctx, getState());
  enterRule(_localctx, 42, FeatParser::RuleLookupflagAssign);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(413);
    match(FeatParser::LOOKUPFLAG);
    setState(420);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NUM: {
        setState(414);
        match(FeatParser::NUM);
        break;
      }

      case FeatParser::RIGHT_TO_LEFT:
      case FeatParser::IGNORE_BASE_GLYPHS:
      case FeatParser::IGNORE_LIGATURES:
      case FeatParser::IGNORE_MARKS:
      case FeatParser::USE_MARK_FILTERING_SET:
      case FeatParser::MARK_ATTACHMENT_TYPE: {
        setState(416); 
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
          setState(415);
          lookupflagElement();
          setState(418); 
          _errHandler->sync(this);
          _la = _input->LA(1);
        } while ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & ((1ULL << FeatParser::RIGHT_TO_LEFT)
          | (1ULL << FeatParser::IGNORE_BASE_GLYPHS)
          | (1ULL << FeatParser::IGNORE_LIGATURES)
          | (1ULL << FeatParser::IGNORE_MARKS)
          | (1ULL << FeatParser::USE_MARK_FILTERING_SET)
          | (1ULL << FeatParser::MARK_ATTACHMENT_TYPE))) != 0));
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LookupflagElementContext ------------------------------------------------------------------

FeatParser::LookupflagElementContext::LookupflagElementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::LookupflagElementContext::RIGHT_TO_LEFT() {
  return getToken(FeatParser::RIGHT_TO_LEFT, 0);
}

tree::TerminalNode* FeatParser::LookupflagElementContext::IGNORE_BASE_GLYPHS() {
  return getToken(FeatParser::IGNORE_BASE_GLYPHS, 0);
}

tree::TerminalNode* FeatParser::LookupflagElementContext::IGNORE_LIGATURES() {
  return getToken(FeatParser::IGNORE_LIGATURES, 0);
}

tree::TerminalNode* FeatParser::LookupflagElementContext::IGNORE_MARKS() {
  return getToken(FeatParser::IGNORE_MARKS, 0);
}

tree::TerminalNode* FeatParser::LookupflagElementContext::MARK_ATTACHMENT_TYPE() {
  return getToken(FeatParser::MARK_ATTACHMENT_TYPE, 0);
}

FeatParser::GlyphClassContext* FeatParser::LookupflagElementContext::glyphClass() {
  return getRuleContext<FeatParser::GlyphClassContext>(0);
}

tree::TerminalNode* FeatParser::LookupflagElementContext::USE_MARK_FILTERING_SET() {
  return getToken(FeatParser::USE_MARK_FILTERING_SET, 0);
}


size_t FeatParser::LookupflagElementContext::getRuleIndex() const {
  return FeatParser::RuleLookupflagElement;
}


antlrcpp::Any FeatParser::LookupflagElementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLookupflagElement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LookupflagElementContext* FeatParser::lookupflagElement() {
  LookupflagElementContext *_localctx = _tracker.createInstance<LookupflagElementContext>(_ctx, getState());
  enterRule(_localctx, 44, FeatParser::RuleLookupflagElement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(430);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::RIGHT_TO_LEFT: {
        enterOuterAlt(_localctx, 1);
        setState(422);
        match(FeatParser::RIGHT_TO_LEFT);
        break;
      }

      case FeatParser::IGNORE_BASE_GLYPHS: {
        enterOuterAlt(_localctx, 2);
        setState(423);
        match(FeatParser::IGNORE_BASE_GLYPHS);
        break;
      }

      case FeatParser::IGNORE_LIGATURES: {
        enterOuterAlt(_localctx, 3);
        setState(424);
        match(FeatParser::IGNORE_LIGATURES);
        break;
      }

      case FeatParser::IGNORE_MARKS: {
        enterOuterAlt(_localctx, 4);
        setState(425);
        match(FeatParser::IGNORE_MARKS);
        break;
      }

      case FeatParser::MARK_ATTACHMENT_TYPE: {
        enterOuterAlt(_localctx, 5);
        setState(426);
        match(FeatParser::MARK_ATTACHMENT_TYPE);
        setState(427);
        glyphClass();
        break;
      }

      case FeatParser::USE_MARK_FILTERING_SET: {
        enterOuterAlt(_localctx, 6);
        setState(428);
        match(FeatParser::USE_MARK_FILTERING_SET);
        setState(429);
        glyphClass();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IgnoreSubOrPosContext ------------------------------------------------------------------

FeatParser::IgnoreSubOrPosContext::IgnoreSubOrPosContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::IgnoreSubOrPosContext::IGNORE() {
  return getToken(FeatParser::IGNORE, 0);
}

std::vector<FeatParser::LookupPatternContext *> FeatParser::IgnoreSubOrPosContext::lookupPattern() {
  return getRuleContexts<FeatParser::LookupPatternContext>();
}

FeatParser::LookupPatternContext* FeatParser::IgnoreSubOrPosContext::lookupPattern(size_t i) {
  return getRuleContext<FeatParser::LookupPatternContext>(i);
}

FeatParser::SubtokContext* FeatParser::IgnoreSubOrPosContext::subtok() {
  return getRuleContext<FeatParser::SubtokContext>(0);
}

FeatParser::RevtokContext* FeatParser::IgnoreSubOrPosContext::revtok() {
  return getRuleContext<FeatParser::RevtokContext>(0);
}

FeatParser::PostokContext* FeatParser::IgnoreSubOrPosContext::postok() {
  return getRuleContext<FeatParser::PostokContext>(0);
}

std::vector<tree::TerminalNode *> FeatParser::IgnoreSubOrPosContext::COMMA() {
  return getTokens(FeatParser::COMMA);
}

tree::TerminalNode* FeatParser::IgnoreSubOrPosContext::COMMA(size_t i) {
  return getToken(FeatParser::COMMA, i);
}


size_t FeatParser::IgnoreSubOrPosContext::getRuleIndex() const {
  return FeatParser::RuleIgnoreSubOrPos;
}


antlrcpp::Any FeatParser::IgnoreSubOrPosContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitIgnoreSubOrPos(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::IgnoreSubOrPosContext* FeatParser::ignoreSubOrPos() {
  IgnoreSubOrPosContext *_localctx = _tracker.createInstance<IgnoreSubOrPosContext>(_ctx, getState());
  enterRule(_localctx, 46, FeatParser::RuleIgnoreSubOrPos);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(432);
    match(FeatParser::IGNORE);
    setState(436);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::SUBSTITUTE:
      case FeatParser::SUBSTITUTE_v: {
        setState(433);
        subtok();
        break;
      }

      case FeatParser::REVERSE:
      case FeatParser::REVERSE_v: {
        setState(434);
        revtok();
        break;
      }

      case FeatParser::POSITION:
      case FeatParser::POSITION_v: {
        setState(435);
        postok();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(438);
    lookupPattern();
    setState(443);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::COMMA) {
      setState(439);
      match(FeatParser::COMMA);
      setState(440);
      lookupPattern();
      setState(445);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SubstituteContext ------------------------------------------------------------------

FeatParser::SubstituteContext::SubstituteContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::RevtokContext* FeatParser::SubstituteContext::revtok() {
  return getRuleContext<FeatParser::RevtokContext>(0);
}

FeatParser::SubtokContext* FeatParser::SubstituteContext::subtok() {
  return getRuleContext<FeatParser::SubtokContext>(0);
}

tree::TerminalNode* FeatParser::SubstituteContext::EXCEPT() {
  return getToken(FeatParser::EXCEPT, 0);
}

std::vector<FeatParser::LookupPatternContext *> FeatParser::SubstituteContext::lookupPattern() {
  return getRuleContexts<FeatParser::LookupPatternContext>();
}

FeatParser::LookupPatternContext* FeatParser::SubstituteContext::lookupPattern(size_t i) {
  return getRuleContext<FeatParser::LookupPatternContext>(i);
}

tree::TerminalNode* FeatParser::SubstituteContext::BY() {
  return getToken(FeatParser::BY, 0);
}

std::vector<tree::TerminalNode *> FeatParser::SubstituteContext::COMMA() {
  return getTokens(FeatParser::COMMA);
}

tree::TerminalNode* FeatParser::SubstituteContext::COMMA(size_t i) {
  return getToken(FeatParser::COMMA, i);
}

tree::TerminalNode* FeatParser::SubstituteContext::FROM() {
  return getToken(FeatParser::FROM, 0);
}

tree::TerminalNode* FeatParser::SubstituteContext::KNULL() {
  return getToken(FeatParser::KNULL, 0);
}


size_t FeatParser::SubstituteContext::getRuleIndex() const {
  return FeatParser::RuleSubstitute;
}


antlrcpp::Any FeatParser::SubstituteContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitSubstitute(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::SubstituteContext* FeatParser::substitute() {
  SubstituteContext *_localctx = _tracker.createInstance<SubstituteContext>(_ctx, getState());
  enterRule(_localctx, 48, FeatParser::RuleSubstitute);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(455);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::EXCEPT) {
      setState(446);
      match(FeatParser::EXCEPT);
      setState(447);
      lookupPattern();
      setState(452);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == FeatParser::COMMA) {
        setState(448);
        match(FeatParser::COMMA);
        setState(449);
        lookupPattern();
        setState(454);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
    }
    setState(475);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::REVERSE:
      case FeatParser::REVERSE_v: {
        setState(457);
        revtok();
        setState(458);
        antlrcpp::downCast<SubstituteContext *>(_localctx)->startpat = lookupPattern();
        setState(464);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::BY) {
          setState(459);
          match(FeatParser::BY);
          setState(462);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case FeatParser::KNULL: {
              setState(460);
              match(FeatParser::KNULL);
              break;
            }

            case FeatParser::NOTDEF:
            case FeatParser::LBRACKET:
            case FeatParser::GCLASS:
            case FeatParser::CID:
            case FeatParser::ESCGNAME:
            case FeatParser::NAMELABEL:
            case FeatParser::EXTNAME: {
              setState(461);
              antlrcpp::downCast<SubstituteContext *>(_localctx)->endpat = lookupPattern();
              break;
            }

          default:
            throw NoViableAltException(this);
          }
        }
        break;
      }

      case FeatParser::SUBSTITUTE:
      case FeatParser::SUBSTITUTE_v: {
        setState(466);
        subtok();
        setState(467);
        antlrcpp::downCast<SubstituteContext *>(_localctx)->startpat = lookupPattern();
        setState(473);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::BY

        || _la == FeatParser::FROM) {
          setState(468);
          _la = _input->LA(1);
          if (!(_la == FeatParser::BY

          || _la == FeatParser::FROM)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(471);
          _errHandler->sync(this);
          switch (_input->LA(1)) {
            case FeatParser::KNULL: {
              setState(469);
              match(FeatParser::KNULL);
              break;
            }

            case FeatParser::NOTDEF:
            case FeatParser::LBRACKET:
            case FeatParser::GCLASS:
            case FeatParser::CID:
            case FeatParser::ESCGNAME:
            case FeatParser::NAMELABEL:
            case FeatParser::EXTNAME: {
              setState(470);
              antlrcpp::downCast<SubstituteContext *>(_localctx)->endpat = lookupPattern();
              break;
            }

          default:
            throw NoViableAltException(this);
          }
        }
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PositionContext ------------------------------------------------------------------

FeatParser::PositionContext::PositionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::PostokContext* FeatParser::PositionContext::postok() {
  return getRuleContext<FeatParser::PostokContext>(0);
}

FeatParser::EnumtokContext* FeatParser::PositionContext::enumtok() {
  return getRuleContext<FeatParser::EnumtokContext>(0);
}

std::vector<FeatParser::PatternContext *> FeatParser::PositionContext::pattern() {
  return getRuleContexts<FeatParser::PatternContext>();
}

FeatParser::PatternContext* FeatParser::PositionContext::pattern(size_t i) {
  return getRuleContext<FeatParser::PatternContext>(i);
}

FeatParser::ValueRecordContext* FeatParser::PositionContext::valueRecord() {
  return getRuleContext<FeatParser::ValueRecordContext>(0);
}

tree::TerminalNode* FeatParser::PositionContext::CURSIVE() {
  return getToken(FeatParser::CURSIVE, 0);
}

FeatParser::CursiveElementContext* FeatParser::PositionContext::cursiveElement() {
  return getRuleContext<FeatParser::CursiveElementContext>(0);
}

tree::TerminalNode* FeatParser::PositionContext::MARKBASE() {
  return getToken(FeatParser::MARKBASE, 0);
}

FeatParser::MarkligtokContext* FeatParser::PositionContext::markligtok() {
  return getRuleContext<FeatParser::MarkligtokContext>(0);
}

tree::TerminalNode* FeatParser::PositionContext::MARK() {
  return getToken(FeatParser::MARK, 0);
}

std::vector<FeatParser::ValuePatternContext *> FeatParser::PositionContext::valuePattern() {
  return getRuleContexts<FeatParser::ValuePatternContext>();
}

FeatParser::ValuePatternContext* FeatParser::PositionContext::valuePattern(size_t i) {
  return getRuleContext<FeatParser::ValuePatternContext>(i);
}

std::vector<tree::TerminalNode *> FeatParser::PositionContext::LOOKUP() {
  return getTokens(FeatParser::LOOKUP);
}

tree::TerminalNode* FeatParser::PositionContext::LOOKUP(size_t i) {
  return getToken(FeatParser::LOOKUP, i);
}

std::vector<FeatParser::LabelContext *> FeatParser::PositionContext::label() {
  return getRuleContexts<FeatParser::LabelContext>();
}

FeatParser::LabelContext* FeatParser::PositionContext::label(size_t i) {
  return getRuleContext<FeatParser::LabelContext>(i);
}

std::vector<FeatParser::LookupPatternElementContext *> FeatParser::PositionContext::lookupPatternElement() {
  return getRuleContexts<FeatParser::LookupPatternElementContext>();
}

FeatParser::LookupPatternElementContext* FeatParser::PositionContext::lookupPatternElement(size_t i) {
  return getRuleContext<FeatParser::LookupPatternElementContext>(i);
}

std::vector<FeatParser::BaseToMarkElementContext *> FeatParser::PositionContext::baseToMarkElement() {
  return getRuleContexts<FeatParser::BaseToMarkElementContext>();
}

FeatParser::BaseToMarkElementContext* FeatParser::PositionContext::baseToMarkElement(size_t i) {
  return getRuleContext<FeatParser::BaseToMarkElementContext>(i);
}

std::vector<FeatParser::LigatureMarkElementContext *> FeatParser::PositionContext::ligatureMarkElement() {
  return getRuleContexts<FeatParser::LigatureMarkElementContext>();
}

FeatParser::LigatureMarkElementContext* FeatParser::PositionContext::ligatureMarkElement(size_t i) {
  return getRuleContext<FeatParser::LigatureMarkElementContext>(i);
}


size_t FeatParser::PositionContext::getRuleIndex() const {
  return FeatParser::RulePosition;
}


antlrcpp::Any FeatParser::PositionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitPosition(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::PositionContext* FeatParser::position() {
  PositionContext *_localctx = _tracker.createInstance<PositionContext>(_ctx, getState());
  enterRule(_localctx, 50, FeatParser::RulePosition);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(478);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::ENUMERATE

    || _la == FeatParser::ENUMERATE_v) {
      setState(477);
      enumtok();
    }
    setState(480);
    postok();
    setState(482);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
      | (1ULL << (FeatParser::GCLASS - 117))
      | (1ULL << (FeatParser::CID - 117))
      | (1ULL << (FeatParser::ESCGNAME - 117))
      | (1ULL << (FeatParser::NAMELABEL - 117))
      | (1ULL << (FeatParser::EXTNAME - 117)))) != 0)) {
      setState(481);
      antlrcpp::downCast<PositionContext *>(_localctx)->startpat = pattern();
    }
    setState(538);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::BEGINVALUE:
      case FeatParser::NUM: {
        setState(484);
        valueRecord();
        setState(488);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
          | (1ULL << (FeatParser::GCLASS - 117))
          | (1ULL << (FeatParser::CID - 117))
          | (1ULL << (FeatParser::ESCGNAME - 117))
          | (1ULL << (FeatParser::NAMELABEL - 117))
          | (1ULL << (FeatParser::EXTNAME - 117)))) != 0)) {
          setState(485);
          valuePattern();
          setState(490);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        break;
      }

      case FeatParser::LOOKUP: {
        setState(493); 
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
          setState(491);
          match(FeatParser::LOOKUP);
          setState(492);
          label();
          setState(495); 
          _errHandler->sync(this);
          _la = _input->LA(1);
        } while (_la == FeatParser::LOOKUP);
        setState(500);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
          | (1ULL << (FeatParser::GCLASS - 117))
          | (1ULL << (FeatParser::CID - 117))
          | (1ULL << (FeatParser::ESCGNAME - 117))
          | (1ULL << (FeatParser::NAMELABEL - 117))
          | (1ULL << (FeatParser::EXTNAME - 117)))) != 0)) {
          setState(497);
          lookupPatternElement();
          setState(502);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        break;
      }

      case FeatParser::CURSIVE: {
        setState(503);
        match(FeatParser::CURSIVE);
        setState(504);
        cursiveElement();
        setState(506);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
          | (1ULL << (FeatParser::GCLASS - 117))
          | (1ULL << (FeatParser::CID - 117))
          | (1ULL << (FeatParser::ESCGNAME - 117))
          | (1ULL << (FeatParser::NAMELABEL - 117))
          | (1ULL << (FeatParser::EXTNAME - 117)))) != 0)) {
          setState(505);
          antlrcpp::downCast<PositionContext *>(_localctx)->endpat = pattern();
        }
        break;
      }

      case FeatParser::MARKBASE: {
        setState(508);
        match(FeatParser::MARKBASE);
        setState(509);
        antlrcpp::downCast<PositionContext *>(_localctx)->midpat = pattern();
        setState(511); 
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
          setState(510);
          baseToMarkElement();
          setState(513); 
          _errHandler->sync(this);
          _la = _input->LA(1);
        } while (_la == FeatParser::BEGINVALUE);
        setState(516);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
          | (1ULL << (FeatParser::GCLASS - 117))
          | (1ULL << (FeatParser::CID - 117))
          | (1ULL << (FeatParser::ESCGNAME - 117))
          | (1ULL << (FeatParser::NAMELABEL - 117))
          | (1ULL << (FeatParser::EXTNAME - 117)))) != 0)) {
          setState(515);
          antlrcpp::downCast<PositionContext *>(_localctx)->endpat = pattern();
        }
        break;
      }

      case FeatParser::MARKLIG:
      case FeatParser::MARKLIG_v: {
        setState(518);
        markligtok();
        setState(519);
        antlrcpp::downCast<PositionContext *>(_localctx)->midpat = pattern();
        setState(521); 
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
          setState(520);
          ligatureMarkElement();
          setState(523); 
          _errHandler->sync(this);
          _la = _input->LA(1);
        } while (_la == FeatParser::BEGINVALUE);
        setState(526);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
          | (1ULL << (FeatParser::GCLASS - 117))
          | (1ULL << (FeatParser::CID - 117))
          | (1ULL << (FeatParser::ESCGNAME - 117))
          | (1ULL << (FeatParser::NAMELABEL - 117))
          | (1ULL << (FeatParser::EXTNAME - 117)))) != 0)) {
          setState(525);
          antlrcpp::downCast<PositionContext *>(_localctx)->endpat = pattern();
        }
        break;
      }

      case FeatParser::MARK: {
        setState(528);
        match(FeatParser::MARK);
        setState(529);
        antlrcpp::downCast<PositionContext *>(_localctx)->midpat = pattern();
        setState(531); 
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
          setState(530);
          baseToMarkElement();
          setState(533); 
          _errHandler->sync(this);
          _la = _input->LA(1);
        } while (_la == FeatParser::BEGINVALUE);
        setState(536);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
          | (1ULL << (FeatParser::GCLASS - 117))
          | (1ULL << (FeatParser::CID - 117))
          | (1ULL << (FeatParser::ESCGNAME - 117))
          | (1ULL << (FeatParser::NAMELABEL - 117))
          | (1ULL << (FeatParser::EXTNAME - 117)))) != 0)) {
          setState(535);
          antlrcpp::downCast<PositionContext *>(_localctx)->endpat = pattern();
        }
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValuePatternContext ------------------------------------------------------------------

FeatParser::ValuePatternContext::ValuePatternContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::PatternElementContext* FeatParser::ValuePatternContext::patternElement() {
  return getRuleContext<FeatParser::PatternElementContext>(0);
}

FeatParser::ValueRecordContext* FeatParser::ValuePatternContext::valueRecord() {
  return getRuleContext<FeatParser::ValueRecordContext>(0);
}


size_t FeatParser::ValuePatternContext::getRuleIndex() const {
  return FeatParser::RuleValuePattern;
}


antlrcpp::Any FeatParser::ValuePatternContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitValuePattern(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ValuePatternContext* FeatParser::valuePattern() {
  ValuePatternContext *_localctx = _tracker.createInstance<ValuePatternContext>(_ctx, getState());
  enterRule(_localctx, 52, FeatParser::RuleValuePattern);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(540);
    patternElement();
    setState(542);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::BEGINVALUE || _la == FeatParser::NUM) {
      setState(541);
      valueRecord();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValueRecordContext ------------------------------------------------------------------

FeatParser::ValueRecordContext::ValueRecordContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::ValueRecordContext::BEGINVALUE() {
  return getToken(FeatParser::BEGINVALUE, 0);
}

tree::TerminalNode* FeatParser::ValueRecordContext::ENDVALUE() {
  return getToken(FeatParser::ENDVALUE, 0);
}

FeatParser::LabelContext* FeatParser::ValueRecordContext::label() {
  return getRuleContext<FeatParser::LabelContext>(0);
}

FeatParser::ValueLiteralContext* FeatParser::ValueRecordContext::valueLiteral() {
  return getRuleContext<FeatParser::ValueLiteralContext>(0);
}


size_t FeatParser::ValueRecordContext::getRuleIndex() const {
  return FeatParser::RuleValueRecord;
}


antlrcpp::Any FeatParser::ValueRecordContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitValueRecord(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ValueRecordContext* FeatParser::valueRecord() {
  ValueRecordContext *_localctx = _tracker.createInstance<ValueRecordContext>(_ctx, getState());
  enterRule(_localctx, 54, FeatParser::RuleValueRecord);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(549);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 47, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(544);
      match(FeatParser::BEGINVALUE);
      setState(545);
      antlrcpp::downCast<ValueRecordContext *>(_localctx)->valuename = label();
      setState(546);
      match(FeatParser::ENDVALUE);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(548);
      valueLiteral();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ValueLiteralContext ------------------------------------------------------------------

FeatParser::ValueLiteralContext::ValueLiteralContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::ValueLiteralContext::BEGINVALUE() {
  return getToken(FeatParser::BEGINVALUE, 0);
}

std::vector<tree::TerminalNode *> FeatParser::ValueLiteralContext::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::ValueLiteralContext::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}

tree::TerminalNode* FeatParser::ValueLiteralContext::ENDVALUE() {
  return getToken(FeatParser::ENDVALUE, 0);
}


size_t FeatParser::ValueLiteralContext::getRuleIndex() const {
  return FeatParser::RuleValueLiteral;
}


antlrcpp::Any FeatParser::ValueLiteralContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitValueLiteral(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ValueLiteralContext* FeatParser::valueLiteral() {
  ValueLiteralContext *_localctx = _tracker.createInstance<ValueLiteralContext>(_ctx, getState());
  enterRule(_localctx, 56, FeatParser::RuleValueLiteral);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(558);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::BEGINVALUE: {
        enterOuterAlt(_localctx, 1);
        setState(551);
        match(FeatParser::BEGINVALUE);
        setState(552);
        match(FeatParser::NUM);
        setState(553);
        match(FeatParser::NUM);
        setState(554);
        match(FeatParser::NUM);
        setState(555);
        match(FeatParser::NUM);
        setState(556);
        match(FeatParser::ENDVALUE);
        break;
      }

      case FeatParser::NUM: {
        enterOuterAlt(_localctx, 2);
        setState(557);
        match(FeatParser::NUM);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CursiveElementContext ------------------------------------------------------------------

FeatParser::CursiveElementContext::CursiveElementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::PatternElementContext* FeatParser::CursiveElementContext::patternElement() {
  return getRuleContext<FeatParser::PatternElementContext>(0);
}

std::vector<FeatParser::AnchorContext *> FeatParser::CursiveElementContext::anchor() {
  return getRuleContexts<FeatParser::AnchorContext>();
}

FeatParser::AnchorContext* FeatParser::CursiveElementContext::anchor(size_t i) {
  return getRuleContext<FeatParser::AnchorContext>(i);
}


size_t FeatParser::CursiveElementContext::getRuleIndex() const {
  return FeatParser::RuleCursiveElement;
}


antlrcpp::Any FeatParser::CursiveElementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitCursiveElement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::CursiveElementContext* FeatParser::cursiveElement() {
  CursiveElementContext *_localctx = _tracker.createInstance<CursiveElementContext>(_ctx, getState());
  enterRule(_localctx, 58, FeatParser::RuleCursiveElement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(560);
    patternElement();
    setState(561);
    anchor();
    setState(562);
    anchor();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BaseToMarkElementContext ------------------------------------------------------------------

FeatParser::BaseToMarkElementContext::BaseToMarkElementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::AnchorContext* FeatParser::BaseToMarkElementContext::anchor() {
  return getRuleContext<FeatParser::AnchorContext>(0);
}

tree::TerminalNode* FeatParser::BaseToMarkElementContext::MARK() {
  return getToken(FeatParser::MARK, 0);
}

tree::TerminalNode* FeatParser::BaseToMarkElementContext::GCLASS() {
  return getToken(FeatParser::GCLASS, 0);
}

tree::TerminalNode* FeatParser::BaseToMarkElementContext::MARKER() {
  return getToken(FeatParser::MARKER, 0);
}


size_t FeatParser::BaseToMarkElementContext::getRuleIndex() const {
  return FeatParser::RuleBaseToMarkElement;
}


antlrcpp::Any FeatParser::BaseToMarkElementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitBaseToMarkElement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::BaseToMarkElementContext* FeatParser::baseToMarkElement() {
  BaseToMarkElementContext *_localctx = _tracker.createInstance<BaseToMarkElementContext>(_ctx, getState());
  enterRule(_localctx, 60, FeatParser::RuleBaseToMarkElement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(564);
    anchor();
    setState(565);
    match(FeatParser::MARK);
    setState(566);
    match(FeatParser::GCLASS);
    setState(568);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::MARKER) {
      setState(567);
      match(FeatParser::MARKER);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LigatureMarkElementContext ------------------------------------------------------------------

FeatParser::LigatureMarkElementContext::LigatureMarkElementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::AnchorContext* FeatParser::LigatureMarkElementContext::anchor() {
  return getRuleContext<FeatParser::AnchorContext>(0);
}

tree::TerminalNode* FeatParser::LigatureMarkElementContext::MARK() {
  return getToken(FeatParser::MARK, 0);
}

tree::TerminalNode* FeatParser::LigatureMarkElementContext::GCLASS() {
  return getToken(FeatParser::GCLASS, 0);
}

tree::TerminalNode* FeatParser::LigatureMarkElementContext::LIG_COMPONENT() {
  return getToken(FeatParser::LIG_COMPONENT, 0);
}

tree::TerminalNode* FeatParser::LigatureMarkElementContext::MARKER() {
  return getToken(FeatParser::MARKER, 0);
}


size_t FeatParser::LigatureMarkElementContext::getRuleIndex() const {
  return FeatParser::RuleLigatureMarkElement;
}


antlrcpp::Any FeatParser::LigatureMarkElementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLigatureMarkElement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LigatureMarkElementContext* FeatParser::ligatureMarkElement() {
  LigatureMarkElementContext *_localctx = _tracker.createInstance<LigatureMarkElementContext>(_ctx, getState());
  enterRule(_localctx, 62, FeatParser::RuleLigatureMarkElement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(570);
    anchor();
    setState(573);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::MARK) {
      setState(571);
      match(FeatParser::MARK);
      setState(572);
      match(FeatParser::GCLASS);
    }
    setState(576);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::LIG_COMPONENT) {
      setState(575);
      match(FeatParser::LIG_COMPONENT);
    }
    setState(579);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::MARKER) {
      setState(578);
      match(FeatParser::MARKER);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParametersContext ------------------------------------------------------------------

FeatParser::ParametersContext::ParametersContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::ParametersContext::PARAMETERS() {
  return getToken(FeatParser::PARAMETERS, 0);
}

std::vector<FeatParser::FixedNumContext *> FeatParser::ParametersContext::fixedNum() {
  return getRuleContexts<FeatParser::FixedNumContext>();
}

FeatParser::FixedNumContext* FeatParser::ParametersContext::fixedNum(size_t i) {
  return getRuleContext<FeatParser::FixedNumContext>(i);
}


size_t FeatParser::ParametersContext::getRuleIndex() const {
  return FeatParser::RuleParameters;
}


antlrcpp::Any FeatParser::ParametersContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitParameters(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ParametersContext* FeatParser::parameters() {
  ParametersContext *_localctx = _tracker.createInstance<ParametersContext>(_ctx, getState());
  enterRule(_localctx, 64, FeatParser::RuleParameters);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(581);
    match(FeatParser::PARAMETERS);
    setState(583); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(582);
      fixedNum();
      setState(585); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::POINTNUM

    || _la == FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SizemenunameContext ------------------------------------------------------------------

FeatParser::SizemenunameContext::SizemenunameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::SizemenunameContext::SIZEMENUNAME() {
  return getToken(FeatParser::SIZEMENUNAME, 0);
}

tree::TerminalNode* FeatParser::SizemenunameContext::QUOTE() {
  return getToken(FeatParser::QUOTE, 0);
}

tree::TerminalNode* FeatParser::SizemenunameContext::STRVAL() {
  return getToken(FeatParser::STRVAL, 0);
}

tree::TerminalNode* FeatParser::SizemenunameContext::EQUOTE() {
  return getToken(FeatParser::EQUOTE, 0);
}

std::vector<FeatParser::GenNumContext *> FeatParser::SizemenunameContext::genNum() {
  return getRuleContexts<FeatParser::GenNumContext>();
}

FeatParser::GenNumContext* FeatParser::SizemenunameContext::genNum(size_t i) {
  return getRuleContext<FeatParser::GenNumContext>(i);
}


size_t FeatParser::SizemenunameContext::getRuleIndex() const {
  return FeatParser::RuleSizemenuname;
}


antlrcpp::Any FeatParser::SizemenunameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitSizemenuname(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::SizemenunameContext* FeatParser::sizemenuname() {
  SizemenunameContext *_localctx = _tracker.createInstance<SizemenunameContext>(_ctx, getState());
  enterRule(_localctx, 66, FeatParser::RuleSizemenuname);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(587);
    match(FeatParser::SIZEMENUNAME);
    setState(594);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 131) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 131)) & ((1ULL << (FeatParser::NUMEXT - 131))
      | (1ULL << (FeatParser::NUMOCT - 131))
      | (1ULL << (FeatParser::NUM - 131)))) != 0)) {
      setState(588);
      genNum();
      setState(592);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (((((_la - 131) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 131)) & ((1ULL << (FeatParser::NUMEXT - 131))
        | (1ULL << (FeatParser::NUMOCT - 131))
        | (1ULL << (FeatParser::NUM - 131)))) != 0)) {
        setState(589);
        genNum();
        setState(590);
        genNum();
      }
    }
    setState(596);
    match(FeatParser::QUOTE);
    setState(597);
    match(FeatParser::STRVAL);
    setState(598);
    match(FeatParser::EQUOTE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FeatureNamesContext ------------------------------------------------------------------

FeatParser::FeatureNamesContext::FeatureNamesContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::FeatureNamesContext::FEATURE_NAMES() {
  return getToken(FeatParser::FEATURE_NAMES, 0);
}

tree::TerminalNode* FeatParser::FeatureNamesContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::FeatureNamesContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

std::vector<FeatParser::NameEntryStatementContext *> FeatParser::FeatureNamesContext::nameEntryStatement() {
  return getRuleContexts<FeatParser::NameEntryStatementContext>();
}

FeatParser::NameEntryStatementContext* FeatParser::FeatureNamesContext::nameEntryStatement(size_t i) {
  return getRuleContext<FeatParser::NameEntryStatementContext>(i);
}


size_t FeatParser::FeatureNamesContext::getRuleIndex() const {
  return FeatParser::RuleFeatureNames;
}


antlrcpp::Any FeatParser::FeatureNamesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitFeatureNames(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::FeatureNamesContext* FeatParser::featureNames() {
  FeatureNamesContext *_localctx = _tracker.createInstance<FeatureNamesContext>(_ctx, getState());
  enterRule(_localctx, 68, FeatParser::RuleFeatureNames);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(600);
    match(FeatParser::FEATURE_NAMES);
    setState(601);
    match(FeatParser::LCBRACE);
    setState(603); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(602);
      nameEntryStatement();
      setState(605); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || _la == FeatParser::NAME);
    setState(607);
    match(FeatParser::RCBRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SubtableContext ------------------------------------------------------------------

FeatParser::SubtableContext::SubtableContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::SubtableContext::SUBTABLE() {
  return getToken(FeatParser::SUBTABLE, 0);
}


size_t FeatParser::SubtableContext::getRuleIndex() const {
  return FeatParser::RuleSubtable;
}


antlrcpp::Any FeatParser::SubtableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitSubtable(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::SubtableContext* FeatParser::subtable() {
  SubtableContext *_localctx = _tracker.createInstance<SubtableContext>(_ctx, getState());
  enterRule(_localctx, 70, FeatParser::RuleSubtable);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(609);
    match(FeatParser::SUBTABLE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_BASEContext ------------------------------------------------------------------

FeatParser::Table_BASEContext::Table_BASEContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_BASEContext::BASE() {
  return getTokens(FeatParser::BASE);
}

tree::TerminalNode* FeatParser::Table_BASEContext::BASE(size_t i) {
  return getToken(FeatParser::BASE, i);
}

tree::TerminalNode* FeatParser::Table_BASEContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_BASEContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_BASEContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::BaseStatementContext *> FeatParser::Table_BASEContext::baseStatement() {
  return getRuleContexts<FeatParser::BaseStatementContext>();
}

FeatParser::BaseStatementContext* FeatParser::Table_BASEContext::baseStatement(size_t i) {
  return getRuleContext<FeatParser::BaseStatementContext>(i);
}


size_t FeatParser::Table_BASEContext::getRuleIndex() const {
  return FeatParser::RuleTable_BASE;
}


antlrcpp::Any FeatParser::Table_BASEContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_BASE(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_BASEContext* FeatParser::table_BASE() {
  Table_BASEContext *_localctx = _tracker.createInstance<Table_BASEContext>(_ctx, getState());
  enterRule(_localctx, 72, FeatParser::RuleTable_BASE);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(611);
    match(FeatParser::BASE);
    setState(612);
    match(FeatParser::LCBRACE);
    setState(614); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(613);
      baseStatement();
      setState(616); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (((((_la - 5) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 5)) & ((1ULL << (FeatParser::INCLUDE - 5))
      | (1ULL << (FeatParser::HA_BTL - 5))
      | (1ULL << (FeatParser::VA_BTL - 5))
      | (1ULL << (FeatParser::HA_BSL - 5))
      | (1ULL << (FeatParser::VA_BSL - 5)))) != 0));
    setState(618);
    match(FeatParser::RCBRACE);
    setState(619);
    match(FeatParser::BASE);
    setState(620);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BaseStatementContext ------------------------------------------------------------------

FeatParser::BaseStatementContext::BaseStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::BaseStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::AxisTagsContext* FeatParser::BaseStatementContext::axisTags() {
  return getRuleContext<FeatParser::AxisTagsContext>(0);
}

FeatParser::AxisScriptsContext* FeatParser::BaseStatementContext::axisScripts() {
  return getRuleContext<FeatParser::AxisScriptsContext>(0);
}

FeatParser::IncludeContext* FeatParser::BaseStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::BaseStatementContext::getRuleIndex() const {
  return FeatParser::RuleBaseStatement;
}


antlrcpp::Any FeatParser::BaseStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitBaseStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::BaseStatementContext* FeatParser::baseStatement() {
  BaseStatementContext *_localctx = _tracker.createInstance<BaseStatementContext>(_ctx, getState());
  enterRule(_localctx, 74, FeatParser::RuleBaseStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(625);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::HA_BTL:
      case FeatParser::VA_BTL: {
        setState(622);
        axisTags();
        break;
      }

      case FeatParser::HA_BSL:
      case FeatParser::VA_BSL: {
        setState(623);
        axisScripts();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(624);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(627);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AxisTagsContext ------------------------------------------------------------------

FeatParser::AxisTagsContext::AxisTagsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AxisTagsContext::HA_BTL() {
  return getToken(FeatParser::HA_BTL, 0);
}

tree::TerminalNode* FeatParser::AxisTagsContext::VA_BTL() {
  return getToken(FeatParser::VA_BTL, 0);
}

std::vector<FeatParser::TagContext *> FeatParser::AxisTagsContext::tag() {
  return getRuleContexts<FeatParser::TagContext>();
}

FeatParser::TagContext* FeatParser::AxisTagsContext::tag(size_t i) {
  return getRuleContext<FeatParser::TagContext>(i);
}


size_t FeatParser::AxisTagsContext::getRuleIndex() const {
  return FeatParser::RuleAxisTags;
}


antlrcpp::Any FeatParser::AxisTagsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAxisTags(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AxisTagsContext* FeatParser::axisTags() {
  AxisTagsContext *_localctx = _tracker.createInstance<AxisTagsContext>(_ctx, getState());
  enterRule(_localctx, 76, FeatParser::RuleAxisTags);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(629);
    _la = _input->LA(1);
    if (!(_la == FeatParser::HA_BTL

    || _la == FeatParser::VA_BTL)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(631); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(630);
      tag();
      setState(633); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::MARK || ((((_la - 128) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 128)) & ((1ULL << (FeatParser::NAMELABEL - 128))
      | (1ULL << (FeatParser::EXTNAME - 128))
      | (1ULL << (FeatParser::CATCHTAG - 128)))) != 0));
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AxisScriptsContext ------------------------------------------------------------------

FeatParser::AxisScriptsContext::AxisScriptsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<FeatParser::BaseScriptContext *> FeatParser::AxisScriptsContext::baseScript() {
  return getRuleContexts<FeatParser::BaseScriptContext>();
}

FeatParser::BaseScriptContext* FeatParser::AxisScriptsContext::baseScript(size_t i) {
  return getRuleContext<FeatParser::BaseScriptContext>(i);
}

tree::TerminalNode* FeatParser::AxisScriptsContext::HA_BSL() {
  return getToken(FeatParser::HA_BSL, 0);
}

tree::TerminalNode* FeatParser::AxisScriptsContext::VA_BSL() {
  return getToken(FeatParser::VA_BSL, 0);
}

std::vector<tree::TerminalNode *> FeatParser::AxisScriptsContext::COMMA() {
  return getTokens(FeatParser::COMMA);
}

tree::TerminalNode* FeatParser::AxisScriptsContext::COMMA(size_t i) {
  return getToken(FeatParser::COMMA, i);
}


size_t FeatParser::AxisScriptsContext::getRuleIndex() const {
  return FeatParser::RuleAxisScripts;
}


antlrcpp::Any FeatParser::AxisScriptsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAxisScripts(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AxisScriptsContext* FeatParser::axisScripts() {
  AxisScriptsContext *_localctx = _tracker.createInstance<AxisScriptsContext>(_ctx, getState());
  enterRule(_localctx, 78, FeatParser::RuleAxisScripts);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(635);
    _la = _input->LA(1);
    if (!(_la == FeatParser::HA_BSL

    || _la == FeatParser::VA_BSL)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(636);
    baseScript();
    setState(641);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::COMMA) {
      setState(637);
      match(FeatParser::COMMA);
      setState(638);
      baseScript();
      setState(643);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BaseScriptContext ------------------------------------------------------------------

FeatParser::BaseScriptContext::BaseScriptContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<FeatParser::TagContext *> FeatParser::BaseScriptContext::tag() {
  return getRuleContexts<FeatParser::TagContext>();
}

FeatParser::TagContext* FeatParser::BaseScriptContext::tag(size_t i) {
  return getRuleContext<FeatParser::TagContext>(i);
}

std::vector<tree::TerminalNode *> FeatParser::BaseScriptContext::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::BaseScriptContext::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}


size_t FeatParser::BaseScriptContext::getRuleIndex() const {
  return FeatParser::RuleBaseScript;
}


antlrcpp::Any FeatParser::BaseScriptContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitBaseScript(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::BaseScriptContext* FeatParser::baseScript() {
  BaseScriptContext *_localctx = _tracker.createInstance<BaseScriptContext>(_ctx, getState());
  enterRule(_localctx, 80, FeatParser::RuleBaseScript);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(644);
    antlrcpp::downCast<BaseScriptContext *>(_localctx)->script = tag();
    setState(645);
    antlrcpp::downCast<BaseScriptContext *>(_localctx)->db = tag();
    setState(647); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(646);
      match(FeatParser::NUM);
      setState(649); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_GDEFContext ------------------------------------------------------------------

FeatParser::Table_GDEFContext::Table_GDEFContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_GDEFContext::GDEF() {
  return getTokens(FeatParser::GDEF);
}

tree::TerminalNode* FeatParser::Table_GDEFContext::GDEF(size_t i) {
  return getToken(FeatParser::GDEF, i);
}

tree::TerminalNode* FeatParser::Table_GDEFContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_GDEFContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_GDEFContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::GdefStatementContext *> FeatParser::Table_GDEFContext::gdefStatement() {
  return getRuleContexts<FeatParser::GdefStatementContext>();
}

FeatParser::GdefStatementContext* FeatParser::Table_GDEFContext::gdefStatement(size_t i) {
  return getRuleContext<FeatParser::GdefStatementContext>(i);
}


size_t FeatParser::Table_GDEFContext::getRuleIndex() const {
  return FeatParser::RuleTable_GDEF;
}


antlrcpp::Any FeatParser::Table_GDEFContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_GDEF(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_GDEFContext* FeatParser::table_GDEF() {
  Table_GDEFContext *_localctx = _tracker.createInstance<Table_GDEFContext>(_ctx, getState());
  enterRule(_localctx, 82, FeatParser::RuleTable_GDEF);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(651);
    match(FeatParser::GDEF);
    setState(652);
    match(FeatParser::LCBRACE);
    setState(654); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(653);
      gdefStatement();
      setState(656); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || ((((_la - 67) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 67)) & ((1ULL << (FeatParser::GLYPH_CLASS_DEF - 67))
      | (1ULL << (FeatParser::ATTACH - 67))
      | (1ULL << (FeatParser::LIG_CARET_BY_POS - 67))
      | (1ULL << (FeatParser::LIG_CARET_BY_IDX - 67)))) != 0));
    setState(658);
    match(FeatParser::RCBRACE);
    setState(659);
    match(FeatParser::GDEF);
    setState(660);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GdefStatementContext ------------------------------------------------------------------

FeatParser::GdefStatementContext::GdefStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GdefStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::GdefGlyphClassContext* FeatParser::GdefStatementContext::gdefGlyphClass() {
  return getRuleContext<FeatParser::GdefGlyphClassContext>(0);
}

FeatParser::GdefAttachContext* FeatParser::GdefStatementContext::gdefAttach() {
  return getRuleContext<FeatParser::GdefAttachContext>(0);
}

FeatParser::GdefLigCaretPosContext* FeatParser::GdefStatementContext::gdefLigCaretPos() {
  return getRuleContext<FeatParser::GdefLigCaretPosContext>(0);
}

FeatParser::GdefLigCaretIndexContext* FeatParser::GdefStatementContext::gdefLigCaretIndex() {
  return getRuleContext<FeatParser::GdefLigCaretIndexContext>(0);
}

FeatParser::IncludeContext* FeatParser::GdefStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::GdefStatementContext::getRuleIndex() const {
  return FeatParser::RuleGdefStatement;
}


antlrcpp::Any FeatParser::GdefStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGdefStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GdefStatementContext* FeatParser::gdefStatement() {
  GdefStatementContext *_localctx = _tracker.createInstance<GdefStatementContext>(_ctx, getState());
  enterRule(_localctx, 84, FeatParser::RuleGdefStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(667);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::GLYPH_CLASS_DEF: {
        setState(662);
        gdefGlyphClass();
        break;
      }

      case FeatParser::ATTACH: {
        setState(663);
        gdefAttach();
        break;
      }

      case FeatParser::LIG_CARET_BY_POS: {
        setState(664);
        gdefLigCaretPos();
        break;
      }

      case FeatParser::LIG_CARET_BY_IDX: {
        setState(665);
        gdefLigCaretIndex();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(666);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(669);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GdefGlyphClassContext ------------------------------------------------------------------

FeatParser::GdefGlyphClassContext::GdefGlyphClassContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GdefGlyphClassContext::GLYPH_CLASS_DEF() {
  return getToken(FeatParser::GLYPH_CLASS_DEF, 0);
}

std::vector<FeatParser::GlyphClassOptionalContext *> FeatParser::GdefGlyphClassContext::glyphClassOptional() {
  return getRuleContexts<FeatParser::GlyphClassOptionalContext>();
}

FeatParser::GlyphClassOptionalContext* FeatParser::GdefGlyphClassContext::glyphClassOptional(size_t i) {
  return getRuleContext<FeatParser::GlyphClassOptionalContext>(i);
}

std::vector<tree::TerminalNode *> FeatParser::GdefGlyphClassContext::COMMA() {
  return getTokens(FeatParser::COMMA);
}

tree::TerminalNode* FeatParser::GdefGlyphClassContext::COMMA(size_t i) {
  return getToken(FeatParser::COMMA, i);
}


size_t FeatParser::GdefGlyphClassContext::getRuleIndex() const {
  return FeatParser::RuleGdefGlyphClass;
}


antlrcpp::Any FeatParser::GdefGlyphClassContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGdefGlyphClass(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GdefGlyphClassContext* FeatParser::gdefGlyphClass() {
  GdefGlyphClassContext *_localctx = _tracker.createInstance<GdefGlyphClassContext>(_ctx, getState());
  enterRule(_localctx, 86, FeatParser::RuleGdefGlyphClass);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(671);
    match(FeatParser::GLYPH_CLASS_DEF);
    setState(672);
    glyphClassOptional();
    setState(673);
    match(FeatParser::COMMA);
    setState(674);
    glyphClassOptional();
    setState(675);
    match(FeatParser::COMMA);
    setState(676);
    glyphClassOptional();
    setState(677);
    match(FeatParser::COMMA);
    setState(678);
    glyphClassOptional();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GdefAttachContext ------------------------------------------------------------------

FeatParser::GdefAttachContext::GdefAttachContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GdefAttachContext::ATTACH() {
  return getToken(FeatParser::ATTACH, 0);
}

FeatParser::LookupPatternContext* FeatParser::GdefAttachContext::lookupPattern() {
  return getRuleContext<FeatParser::LookupPatternContext>(0);
}

std::vector<tree::TerminalNode *> FeatParser::GdefAttachContext::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::GdefAttachContext::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}


size_t FeatParser::GdefAttachContext::getRuleIndex() const {
  return FeatParser::RuleGdefAttach;
}


antlrcpp::Any FeatParser::GdefAttachContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGdefAttach(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GdefAttachContext* FeatParser::gdefAttach() {
  GdefAttachContext *_localctx = _tracker.createInstance<GdefAttachContext>(_ctx, getState());
  enterRule(_localctx, 88, FeatParser::RuleGdefAttach);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(680);
    match(FeatParser::ATTACH);
    setState(681);
    lookupPattern();
    setState(683); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(682);
      match(FeatParser::NUM);
      setState(685); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GdefLigCaretPosContext ------------------------------------------------------------------

FeatParser::GdefLigCaretPosContext::GdefLigCaretPosContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GdefLigCaretPosContext::LIG_CARET_BY_POS() {
  return getToken(FeatParser::LIG_CARET_BY_POS, 0);
}

FeatParser::LookupPatternContext* FeatParser::GdefLigCaretPosContext::lookupPattern() {
  return getRuleContext<FeatParser::LookupPatternContext>(0);
}

std::vector<tree::TerminalNode *> FeatParser::GdefLigCaretPosContext::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::GdefLigCaretPosContext::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}


size_t FeatParser::GdefLigCaretPosContext::getRuleIndex() const {
  return FeatParser::RuleGdefLigCaretPos;
}


antlrcpp::Any FeatParser::GdefLigCaretPosContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGdefLigCaretPos(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GdefLigCaretPosContext* FeatParser::gdefLigCaretPos() {
  GdefLigCaretPosContext *_localctx = _tracker.createInstance<GdefLigCaretPosContext>(_ctx, getState());
  enterRule(_localctx, 90, FeatParser::RuleGdefLigCaretPos);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(687);
    match(FeatParser::LIG_CARET_BY_POS);
    setState(688);
    lookupPattern();
    setState(690); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(689);
      match(FeatParser::NUM);
      setState(692); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GdefLigCaretIndexContext ------------------------------------------------------------------

FeatParser::GdefLigCaretIndexContext::GdefLigCaretIndexContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GdefLigCaretIndexContext::LIG_CARET_BY_IDX() {
  return getToken(FeatParser::LIG_CARET_BY_IDX, 0);
}

FeatParser::LookupPatternContext* FeatParser::GdefLigCaretIndexContext::lookupPattern() {
  return getRuleContext<FeatParser::LookupPatternContext>(0);
}

std::vector<tree::TerminalNode *> FeatParser::GdefLigCaretIndexContext::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::GdefLigCaretIndexContext::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}


size_t FeatParser::GdefLigCaretIndexContext::getRuleIndex() const {
  return FeatParser::RuleGdefLigCaretIndex;
}


antlrcpp::Any FeatParser::GdefLigCaretIndexContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGdefLigCaretIndex(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GdefLigCaretIndexContext* FeatParser::gdefLigCaretIndex() {
  GdefLigCaretIndexContext *_localctx = _tracker.createInstance<GdefLigCaretIndexContext>(_ctx, getState());
  enterRule(_localctx, 92, FeatParser::RuleGdefLigCaretIndex);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(694);
    match(FeatParser::LIG_CARET_BY_IDX);
    setState(695);
    lookupPattern();
    setState(697); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(696);
      match(FeatParser::NUM);
      setState(699); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_headContext ------------------------------------------------------------------

FeatParser::Table_headContext::Table_headContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_headContext::HEAD() {
  return getTokens(FeatParser::HEAD);
}

tree::TerminalNode* FeatParser::Table_headContext::HEAD(size_t i) {
  return getToken(FeatParser::HEAD, i);
}

tree::TerminalNode* FeatParser::Table_headContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_headContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_headContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::HeadStatementContext *> FeatParser::Table_headContext::headStatement() {
  return getRuleContexts<FeatParser::HeadStatementContext>();
}

FeatParser::HeadStatementContext* FeatParser::Table_headContext::headStatement(size_t i) {
  return getRuleContext<FeatParser::HeadStatementContext>(i);
}


size_t FeatParser::Table_headContext::getRuleIndex() const {
  return FeatParser::RuleTable_head;
}


antlrcpp::Any FeatParser::Table_headContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_head(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_headContext* FeatParser::table_head() {
  Table_headContext *_localctx = _tracker.createInstance<Table_headContext>(_ctx, getState());
  enterRule(_localctx, 94, FeatParser::RuleTable_head);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(701);
    match(FeatParser::HEAD);
    setState(702);
    match(FeatParser::LCBRACE);
    setState(704); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(703);
      headStatement();
      setState(706); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || _la == FeatParser::FONT_REVISION);
    setState(708);
    match(FeatParser::RCBRACE);
    setState(709);
    match(FeatParser::HEAD);
    setState(710);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- HeadStatementContext ------------------------------------------------------------------

FeatParser::HeadStatementContext::HeadStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::HeadStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::HeadContext* FeatParser::HeadStatementContext::head() {
  return getRuleContext<FeatParser::HeadContext>(0);
}

FeatParser::IncludeContext* FeatParser::HeadStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::HeadStatementContext::getRuleIndex() const {
  return FeatParser::RuleHeadStatement;
}


antlrcpp::Any FeatParser::HeadStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitHeadStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::HeadStatementContext* FeatParser::headStatement() {
  HeadStatementContext *_localctx = _tracker.createInstance<HeadStatementContext>(_ctx, getState());
  enterRule(_localctx, 96, FeatParser::RuleHeadStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(714);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::FONT_REVISION: {
        setState(712);
        head();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(713);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(716);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- HeadContext ------------------------------------------------------------------

FeatParser::HeadContext::HeadContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::HeadContext::FONT_REVISION() {
  return getToken(FeatParser::FONT_REVISION, 0);
}

tree::TerminalNode* FeatParser::HeadContext::POINTNUM() {
  return getToken(FeatParser::POINTNUM, 0);
}


size_t FeatParser::HeadContext::getRuleIndex() const {
  return FeatParser::RuleHead;
}


antlrcpp::Any FeatParser::HeadContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitHead(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::HeadContext* FeatParser::head() {
  HeadContext *_localctx = _tracker.createInstance<HeadContext>(_ctx, getState());
  enterRule(_localctx, 98, FeatParser::RuleHead);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(718);
    match(FeatParser::FONT_REVISION);
    setState(719);
    match(FeatParser::POINTNUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_hheaContext ------------------------------------------------------------------

FeatParser::Table_hheaContext::Table_hheaContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_hheaContext::HHEA() {
  return getTokens(FeatParser::HHEA);
}

tree::TerminalNode* FeatParser::Table_hheaContext::HHEA(size_t i) {
  return getToken(FeatParser::HHEA, i);
}

tree::TerminalNode* FeatParser::Table_hheaContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_hheaContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_hheaContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::HheaStatementContext *> FeatParser::Table_hheaContext::hheaStatement() {
  return getRuleContexts<FeatParser::HheaStatementContext>();
}

FeatParser::HheaStatementContext* FeatParser::Table_hheaContext::hheaStatement(size_t i) {
  return getRuleContext<FeatParser::HheaStatementContext>(i);
}


size_t FeatParser::Table_hheaContext::getRuleIndex() const {
  return FeatParser::RuleTable_hhea;
}


antlrcpp::Any FeatParser::Table_hheaContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_hhea(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_hheaContext* FeatParser::table_hhea() {
  Table_hheaContext *_localctx = _tracker.createInstance<Table_hheaContext>(_ctx, getState());
  enterRule(_localctx, 100, FeatParser::RuleTable_hhea);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(721);
    match(FeatParser::HHEA);
    setState(722);
    match(FeatParser::LCBRACE);
    setState(726);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 74) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 74)) & ((1ULL << (FeatParser::ASCENDER - 74))
      | (1ULL << (FeatParser::DESCENDER - 74))
      | (1ULL << (FeatParser::LINE_GAP - 74))
      | (1ULL << (FeatParser::CARET_OFFSET - 74)))) != 0)) {
      setState(723);
      hheaStatement();
      setState(728);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(729);
    match(FeatParser::RCBRACE);
    setState(730);
    match(FeatParser::HHEA);
    setState(731);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- HheaStatementContext ------------------------------------------------------------------

FeatParser::HheaStatementContext::HheaStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::HheaStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::HheaContext* FeatParser::HheaStatementContext::hhea() {
  return getRuleContext<FeatParser::HheaContext>(0);
}

FeatParser::IncludeContext* FeatParser::HheaStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::HheaStatementContext::getRuleIndex() const {
  return FeatParser::RuleHheaStatement;
}


antlrcpp::Any FeatParser::HheaStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitHheaStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::HheaStatementContext* FeatParser::hheaStatement() {
  HheaStatementContext *_localctx = _tracker.createInstance<HheaStatementContext>(_ctx, getState());
  enterRule(_localctx, 102, FeatParser::RuleHheaStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(735);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::ASCENDER:
      case FeatParser::DESCENDER:
      case FeatParser::LINE_GAP:
      case FeatParser::CARET_OFFSET: {
        setState(733);
        hhea();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(734);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(737);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- HheaContext ------------------------------------------------------------------

FeatParser::HheaContext::HheaContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::HheaContext::NUM() {
  return getToken(FeatParser::NUM, 0);
}

tree::TerminalNode* FeatParser::HheaContext::CARET_OFFSET() {
  return getToken(FeatParser::CARET_OFFSET, 0);
}

tree::TerminalNode* FeatParser::HheaContext::ASCENDER() {
  return getToken(FeatParser::ASCENDER, 0);
}

tree::TerminalNode* FeatParser::HheaContext::DESCENDER() {
  return getToken(FeatParser::DESCENDER, 0);
}

tree::TerminalNode* FeatParser::HheaContext::LINE_GAP() {
  return getToken(FeatParser::LINE_GAP, 0);
}


size_t FeatParser::HheaContext::getRuleIndex() const {
  return FeatParser::RuleHhea;
}


antlrcpp::Any FeatParser::HheaContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitHhea(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::HheaContext* FeatParser::hhea() {
  HheaContext *_localctx = _tracker.createInstance<HheaContext>(_ctx, getState());
  enterRule(_localctx, 104, FeatParser::RuleHhea);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(739);
    _la = _input->LA(1);
    if (!(((((_la - 74) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 74)) & ((1ULL << (FeatParser::ASCENDER - 74))
      | (1ULL << (FeatParser::DESCENDER - 74))
      | (1ULL << (FeatParser::LINE_GAP - 74))
      | (1ULL << (FeatParser::CARET_OFFSET - 74)))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(740);
    match(FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_vheaContext ------------------------------------------------------------------

FeatParser::Table_vheaContext::Table_vheaContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_vheaContext::VHEA() {
  return getTokens(FeatParser::VHEA);
}

tree::TerminalNode* FeatParser::Table_vheaContext::VHEA(size_t i) {
  return getToken(FeatParser::VHEA, i);
}

tree::TerminalNode* FeatParser::Table_vheaContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_vheaContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_vheaContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::VheaStatementContext *> FeatParser::Table_vheaContext::vheaStatement() {
  return getRuleContexts<FeatParser::VheaStatementContext>();
}

FeatParser::VheaStatementContext* FeatParser::Table_vheaContext::vheaStatement(size_t i) {
  return getRuleContext<FeatParser::VheaStatementContext>(i);
}


size_t FeatParser::Table_vheaContext::getRuleIndex() const {
  return FeatParser::RuleTable_vhea;
}


antlrcpp::Any FeatParser::Table_vheaContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_vhea(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_vheaContext* FeatParser::table_vhea() {
  Table_vheaContext *_localctx = _tracker.createInstance<Table_vheaContext>(_ctx, getState());
  enterRule(_localctx, 106, FeatParser::RuleTable_vhea);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(742);
    match(FeatParser::VHEA);
    setState(743);
    match(FeatParser::LCBRACE);
    setState(747);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 109) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 109)) & ((1ULL << (FeatParser::VERT_TYPO_ASCENDER - 109))
      | (1ULL << (FeatParser::VERT_TYPO_DESCENDER - 109))
      | (1ULL << (FeatParser::VERT_TYPO_LINE_GAP - 109)))) != 0)) {
      setState(744);
      vheaStatement();
      setState(749);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(750);
    match(FeatParser::RCBRACE);
    setState(751);
    match(FeatParser::VHEA);
    setState(752);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VheaStatementContext ------------------------------------------------------------------

FeatParser::VheaStatementContext::VheaStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::VheaStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::VheaContext* FeatParser::VheaStatementContext::vhea() {
  return getRuleContext<FeatParser::VheaContext>(0);
}

FeatParser::IncludeContext* FeatParser::VheaStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::VheaStatementContext::getRuleIndex() const {
  return FeatParser::RuleVheaStatement;
}


antlrcpp::Any FeatParser::VheaStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitVheaStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::VheaStatementContext* FeatParser::vheaStatement() {
  VheaStatementContext *_localctx = _tracker.createInstance<VheaStatementContext>(_ctx, getState());
  enterRule(_localctx, 108, FeatParser::RuleVheaStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(756);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::VERT_TYPO_ASCENDER:
      case FeatParser::VERT_TYPO_DESCENDER:
      case FeatParser::VERT_TYPO_LINE_GAP: {
        setState(754);
        vhea();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(755);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(758);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VheaContext ------------------------------------------------------------------

FeatParser::VheaContext::VheaContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::VheaContext::NUM() {
  return getToken(FeatParser::NUM, 0);
}

tree::TerminalNode* FeatParser::VheaContext::VERT_TYPO_ASCENDER() {
  return getToken(FeatParser::VERT_TYPO_ASCENDER, 0);
}

tree::TerminalNode* FeatParser::VheaContext::VERT_TYPO_DESCENDER() {
  return getToken(FeatParser::VERT_TYPO_DESCENDER, 0);
}

tree::TerminalNode* FeatParser::VheaContext::VERT_TYPO_LINE_GAP() {
  return getToken(FeatParser::VERT_TYPO_LINE_GAP, 0);
}


size_t FeatParser::VheaContext::getRuleIndex() const {
  return FeatParser::RuleVhea;
}


antlrcpp::Any FeatParser::VheaContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitVhea(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::VheaContext* FeatParser::vhea() {
  VheaContext *_localctx = _tracker.createInstance<VheaContext>(_ctx, getState());
  enterRule(_localctx, 110, FeatParser::RuleVhea);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(760);
    _la = _input->LA(1);
    if (!(((((_la - 109) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 109)) & ((1ULL << (FeatParser::VERT_TYPO_ASCENDER - 109))
      | (1ULL << (FeatParser::VERT_TYPO_DESCENDER - 109))
      | (1ULL << (FeatParser::VERT_TYPO_LINE_GAP - 109)))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(761);
    match(FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_nameContext ------------------------------------------------------------------

FeatParser::Table_nameContext::Table_nameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_nameContext::NAME() {
  return getTokens(FeatParser::NAME);
}

tree::TerminalNode* FeatParser::Table_nameContext::NAME(size_t i) {
  return getToken(FeatParser::NAME, i);
}

tree::TerminalNode* FeatParser::Table_nameContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_nameContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_nameContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::NameStatementContext *> FeatParser::Table_nameContext::nameStatement() {
  return getRuleContexts<FeatParser::NameStatementContext>();
}

FeatParser::NameStatementContext* FeatParser::Table_nameContext::nameStatement(size_t i) {
  return getRuleContext<FeatParser::NameStatementContext>(i);
}


size_t FeatParser::Table_nameContext::getRuleIndex() const {
  return FeatParser::RuleTable_name;
}


antlrcpp::Any FeatParser::Table_nameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_name(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_nameContext* FeatParser::table_name() {
  Table_nameContext *_localctx = _tracker.createInstance<Table_nameContext>(_ctx, getState());
  enterRule(_localctx, 112, FeatParser::RuleTable_name);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(763);
    match(FeatParser::NAME);
    setState(764);
    match(FeatParser::LCBRACE);
    setState(766); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(765);
      nameStatement();
      setState(768); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || _la == FeatParser::NAMEID);
    setState(770);
    match(FeatParser::RCBRACE);
    setState(771);
    match(FeatParser::NAME);
    setState(772);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NameStatementContext ------------------------------------------------------------------

FeatParser::NameStatementContext::NameStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::NameStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::NameIDContext* FeatParser::NameStatementContext::nameID() {
  return getRuleContext<FeatParser::NameIDContext>(0);
}

FeatParser::IncludeContext* FeatParser::NameStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::NameStatementContext::getRuleIndex() const {
  return FeatParser::RuleNameStatement;
}


antlrcpp::Any FeatParser::NameStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitNameStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::NameStatementContext* FeatParser::nameStatement() {
  NameStatementContext *_localctx = _tracker.createInstance<NameStatementContext>(_ctx, getState());
  enterRule(_localctx, 114, FeatParser::RuleNameStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(776);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NAMEID: {
        setState(774);
        nameID();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(775);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(778);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NameIDContext ------------------------------------------------------------------

FeatParser::NameIDContext::NameIDContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::NameIDContext::NAMEID() {
  return getToken(FeatParser::NAMEID, 0);
}

tree::TerminalNode* FeatParser::NameIDContext::QUOTE() {
  return getToken(FeatParser::QUOTE, 0);
}

tree::TerminalNode* FeatParser::NameIDContext::STRVAL() {
  return getToken(FeatParser::STRVAL, 0);
}

tree::TerminalNode* FeatParser::NameIDContext::EQUOTE() {
  return getToken(FeatParser::EQUOTE, 0);
}

std::vector<FeatParser::GenNumContext *> FeatParser::NameIDContext::genNum() {
  return getRuleContexts<FeatParser::GenNumContext>();
}

FeatParser::GenNumContext* FeatParser::NameIDContext::genNum(size_t i) {
  return getRuleContext<FeatParser::GenNumContext>(i);
}


size_t FeatParser::NameIDContext::getRuleIndex() const {
  return FeatParser::RuleNameID;
}


antlrcpp::Any FeatParser::NameIDContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitNameID(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::NameIDContext* FeatParser::nameID() {
  NameIDContext *_localctx = _tracker.createInstance<NameIDContext>(_ctx, getState());
  enterRule(_localctx, 116, FeatParser::RuleNameID);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(780);
    match(FeatParser::NAMEID);
    setState(781);
    antlrcpp::downCast<NameIDContext *>(_localctx)->id = genNum();
    setState(788);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 131) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 131)) & ((1ULL << (FeatParser::NUMEXT - 131))
      | (1ULL << (FeatParser::NUMOCT - 131))
      | (1ULL << (FeatParser::NUM - 131)))) != 0)) {
      setState(782);
      antlrcpp::downCast<NameIDContext *>(_localctx)->plat = genNum();
      setState(786);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (((((_la - 131) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 131)) & ((1ULL << (FeatParser::NUMEXT - 131))
        | (1ULL << (FeatParser::NUMOCT - 131))
        | (1ULL << (FeatParser::NUM - 131)))) != 0)) {
        setState(783);
        antlrcpp::downCast<NameIDContext *>(_localctx)->spec = genNum();
        setState(784);
        antlrcpp::downCast<NameIDContext *>(_localctx)->lang = genNum();
      }
    }
    setState(790);
    match(FeatParser::QUOTE);
    setState(791);
    match(FeatParser::STRVAL);
    setState(792);
    match(FeatParser::EQUOTE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_OS_2Context ------------------------------------------------------------------

FeatParser::Table_OS_2Context::Table_OS_2Context(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_OS_2Context::OS_2() {
  return getTokens(FeatParser::OS_2);
}

tree::TerminalNode* FeatParser::Table_OS_2Context::OS_2(size_t i) {
  return getToken(FeatParser::OS_2, i);
}

tree::TerminalNode* FeatParser::Table_OS_2Context::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_OS_2Context::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_OS_2Context::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::Os_2StatementContext *> FeatParser::Table_OS_2Context::os_2Statement() {
  return getRuleContexts<FeatParser::Os_2StatementContext>();
}

FeatParser::Os_2StatementContext* FeatParser::Table_OS_2Context::os_2Statement(size_t i) {
  return getRuleContext<FeatParser::Os_2StatementContext>(i);
}


size_t FeatParser::Table_OS_2Context::getRuleIndex() const {
  return FeatParser::RuleTable_OS_2;
}


antlrcpp::Any FeatParser::Table_OS_2Context::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_OS_2(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_OS_2Context* FeatParser::table_OS_2() {
  Table_OS_2Context *_localctx = _tracker.createInstance<Table_OS_2Context>(_ctx, getState());
  enterRule(_localctx, 118, FeatParser::RuleTable_OS_2);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(794);
    match(FeatParser::OS_2);
    setState(795);
    match(FeatParser::LCBRACE);
    setState(797); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(796);
      os_2Statement();
      setState(799); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || ((((_la - 81) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 81)) & ((1ULL << (FeatParser::FS_TYPE - 81))
      | (1ULL << (FeatParser::FS_TYPE_v - 81))
      | (1ULL << (FeatParser::OS2_LOWER_OP_SIZE - 81))
      | (1ULL << (FeatParser::OS2_UPPER_OP_SIZE - 81))
      | (1ULL << (FeatParser::PANOSE - 81))
      | (1ULL << (FeatParser::TYPO_ASCENDER - 81))
      | (1ULL << (FeatParser::TYPO_DESCENDER - 81))
      | (1ULL << (FeatParser::TYPO_LINE_GAP - 81))
      | (1ULL << (FeatParser::WIN_ASCENT - 81))
      | (1ULL << (FeatParser::WIN_DESCENT - 81))
      | (1ULL << (FeatParser::X_HEIGHT - 81))
      | (1ULL << (FeatParser::CAP_HEIGHT - 81))
      | (1ULL << (FeatParser::WEIGHT_CLASS - 81))
      | (1ULL << (FeatParser::WIDTH_CLASS - 81))
      | (1ULL << (FeatParser::VENDOR - 81))
      | (1ULL << (FeatParser::UNICODE_RANGE - 81))
      | (1ULL << (FeatParser::CODE_PAGE_RANGE - 81))
      | (1ULL << (FeatParser::FAMILY_CLASS - 81)))) != 0));
    setState(801);
    match(FeatParser::RCBRACE);
    setState(802);
    match(FeatParser::OS_2);
    setState(803);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Os_2StatementContext ------------------------------------------------------------------

FeatParser::Os_2StatementContext::Os_2StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::Os_2StatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::Os_2Context* FeatParser::Os_2StatementContext::os_2() {
  return getRuleContext<FeatParser::Os_2Context>(0);
}

FeatParser::IncludeContext* FeatParser::Os_2StatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::Os_2StatementContext::getRuleIndex() const {
  return FeatParser::RuleOs_2Statement;
}


antlrcpp::Any FeatParser::Os_2StatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitOs_2Statement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Os_2StatementContext* FeatParser::os_2Statement() {
  Os_2StatementContext *_localctx = _tracker.createInstance<Os_2StatementContext>(_ctx, getState());
  enterRule(_localctx, 120, FeatParser::RuleOs_2Statement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(807);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::FS_TYPE:
      case FeatParser::FS_TYPE_v:
      case FeatParser::OS2_LOWER_OP_SIZE:
      case FeatParser::OS2_UPPER_OP_SIZE:
      case FeatParser::PANOSE:
      case FeatParser::TYPO_ASCENDER:
      case FeatParser::TYPO_DESCENDER:
      case FeatParser::TYPO_LINE_GAP:
      case FeatParser::WIN_ASCENT:
      case FeatParser::WIN_DESCENT:
      case FeatParser::X_HEIGHT:
      case FeatParser::CAP_HEIGHT:
      case FeatParser::WEIGHT_CLASS:
      case FeatParser::WIDTH_CLASS:
      case FeatParser::VENDOR:
      case FeatParser::UNICODE_RANGE:
      case FeatParser::CODE_PAGE_RANGE:
      case FeatParser::FAMILY_CLASS: {
        setState(805);
        os_2();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(806);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(809);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Os_2Context ------------------------------------------------------------------

FeatParser::Os_2Context::Os_2Context(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::Os_2Context::TYPO_ASCENDER() {
  return getToken(FeatParser::TYPO_ASCENDER, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::TYPO_DESCENDER() {
  return getToken(FeatParser::TYPO_DESCENDER, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::TYPO_LINE_GAP() {
  return getToken(FeatParser::TYPO_LINE_GAP, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::WIN_ASCENT() {
  return getToken(FeatParser::WIN_ASCENT, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::WIN_DESCENT() {
  return getToken(FeatParser::WIN_DESCENT, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::X_HEIGHT() {
  return getToken(FeatParser::X_HEIGHT, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::CAP_HEIGHT() {
  return getToken(FeatParser::CAP_HEIGHT, 0);
}

std::vector<tree::TerminalNode *> FeatParser::Os_2Context::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::Os_2Context::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}

tree::TerminalNode* FeatParser::Os_2Context::FS_TYPE() {
  return getToken(FeatParser::FS_TYPE, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::FS_TYPE_v() {
  return getToken(FeatParser::FS_TYPE_v, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::WEIGHT_CLASS() {
  return getToken(FeatParser::WEIGHT_CLASS, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::WIDTH_CLASS() {
  return getToken(FeatParser::WIDTH_CLASS, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::OS2_LOWER_OP_SIZE() {
  return getToken(FeatParser::OS2_LOWER_OP_SIZE, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::OS2_UPPER_OP_SIZE() {
  return getToken(FeatParser::OS2_UPPER_OP_SIZE, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::FAMILY_CLASS() {
  return getToken(FeatParser::FAMILY_CLASS, 0);
}

FeatParser::GenNumContext* FeatParser::Os_2Context::genNum() {
  return getRuleContext<FeatParser::GenNumContext>(0);
}

tree::TerminalNode* FeatParser::Os_2Context::VENDOR() {
  return getToken(FeatParser::VENDOR, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::QUOTE() {
  return getToken(FeatParser::QUOTE, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::STRVAL() {
  return getToken(FeatParser::STRVAL, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::EQUOTE() {
  return getToken(FeatParser::EQUOTE, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::PANOSE() {
  return getToken(FeatParser::PANOSE, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::UNICODE_RANGE() {
  return getToken(FeatParser::UNICODE_RANGE, 0);
}

tree::TerminalNode* FeatParser::Os_2Context::CODE_PAGE_RANGE() {
  return getToken(FeatParser::CODE_PAGE_RANGE, 0);
}


size_t FeatParser::Os_2Context::getRuleIndex() const {
  return FeatParser::RuleOs_2;
}


antlrcpp::Any FeatParser::Os_2Context::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitOs_2(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Os_2Context* FeatParser::os_2() {
  Os_2Context *_localctx = _tracker.createInstance<Os_2Context>(_ctx, getState());
  enterRule(_localctx, 122, FeatParser::RuleOs_2);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(838);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::TYPO_ASCENDER:
      case FeatParser::TYPO_DESCENDER:
      case FeatParser::TYPO_LINE_GAP:
      case FeatParser::WIN_ASCENT:
      case FeatParser::WIN_DESCENT:
      case FeatParser::X_HEIGHT:
      case FeatParser::CAP_HEIGHT: {
        enterOuterAlt(_localctx, 1);
        setState(811);
        _la = _input->LA(1);
        if (!(((((_la - 86) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 86)) & ((1ULL << (FeatParser::TYPO_ASCENDER - 86))
          | (1ULL << (FeatParser::TYPO_DESCENDER - 86))
          | (1ULL << (FeatParser::TYPO_LINE_GAP - 86))
          | (1ULL << (FeatParser::WIN_ASCENT - 86))
          | (1ULL << (FeatParser::WIN_DESCENT - 86))
          | (1ULL << (FeatParser::X_HEIGHT - 86))
          | (1ULL << (FeatParser::CAP_HEIGHT - 86)))) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(812);
        antlrcpp::downCast<Os_2Context *>(_localctx)->num = match(FeatParser::NUM);
        break;
      }

      case FeatParser::FS_TYPE:
      case FeatParser::FS_TYPE_v:
      case FeatParser::OS2_LOWER_OP_SIZE:
      case FeatParser::OS2_UPPER_OP_SIZE:
      case FeatParser::WEIGHT_CLASS:
      case FeatParser::WIDTH_CLASS: {
        enterOuterAlt(_localctx, 2);
        setState(813);
        _la = _input->LA(1);
        if (!(((((_la - 81) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 81)) & ((1ULL << (FeatParser::FS_TYPE - 81))
          | (1ULL << (FeatParser::FS_TYPE_v - 81))
          | (1ULL << (FeatParser::OS2_LOWER_OP_SIZE - 81))
          | (1ULL << (FeatParser::OS2_UPPER_OP_SIZE - 81))
          | (1ULL << (FeatParser::WEIGHT_CLASS - 81))
          | (1ULL << (FeatParser::WIDTH_CLASS - 81)))) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(814);
        antlrcpp::downCast<Os_2Context *>(_localctx)->unum = match(FeatParser::NUM);
        break;
      }

      case FeatParser::FAMILY_CLASS: {
        enterOuterAlt(_localctx, 3);
        setState(815);
        match(FeatParser::FAMILY_CLASS);
        setState(816);
        antlrcpp::downCast<Os_2Context *>(_localctx)->gnum = genNum();
        break;
      }

      case FeatParser::VENDOR: {
        enterOuterAlt(_localctx, 4);
        setState(817);
        match(FeatParser::VENDOR);
        setState(818);
        match(FeatParser::QUOTE);
        setState(819);
        match(FeatParser::STRVAL);
        setState(820);
        match(FeatParser::EQUOTE);
        break;
      }

      case FeatParser::PANOSE: {
        enterOuterAlt(_localctx, 5);
        setState(821);
        match(FeatParser::PANOSE);
        setState(822);
        match(FeatParser::NUM);
        setState(823);
        match(FeatParser::NUM);
        setState(824);
        match(FeatParser::NUM);
        setState(825);
        match(FeatParser::NUM);
        setState(826);
        match(FeatParser::NUM);
        setState(827);
        match(FeatParser::NUM);
        setState(828);
        match(FeatParser::NUM);
        setState(829);
        match(FeatParser::NUM);
        setState(830);
        match(FeatParser::NUM);
        setState(831);
        match(FeatParser::NUM);
        break;
      }

      case FeatParser::UNICODE_RANGE:
      case FeatParser::CODE_PAGE_RANGE: {
        enterOuterAlt(_localctx, 6);
        setState(832);
        _la = _input->LA(1);
        if (!(_la == FeatParser::UNICODE_RANGE

        || _la == FeatParser::CODE_PAGE_RANGE)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(834); 
        _errHandler->sync(this);
        _la = _input->LA(1);
        do {
          setState(833);
          match(FeatParser::NUM);
          setState(836); 
          _errHandler->sync(this);
          _la = _input->LA(1);
        } while (_la == FeatParser::NUM);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_STATContext ------------------------------------------------------------------

FeatParser::Table_STATContext::Table_STATContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_STATContext::STAT() {
  return getTokens(FeatParser::STAT);
}

tree::TerminalNode* FeatParser::Table_STATContext::STAT(size_t i) {
  return getToken(FeatParser::STAT, i);
}

tree::TerminalNode* FeatParser::Table_STATContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_STATContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_STATContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::StatStatementContext *> FeatParser::Table_STATContext::statStatement() {
  return getRuleContexts<FeatParser::StatStatementContext>();
}

FeatParser::StatStatementContext* FeatParser::Table_STATContext::statStatement(size_t i) {
  return getRuleContext<FeatParser::StatStatementContext>(i);
}


size_t FeatParser::Table_STATContext::getRuleIndex() const {
  return FeatParser::RuleTable_STAT;
}


antlrcpp::Any FeatParser::Table_STATContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_STAT(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_STATContext* FeatParser::table_STAT() {
  Table_STATContext *_localctx = _tracker.createInstance<Table_STATContext>(_ctx, getState());
  enterRule(_localctx, 124, FeatParser::RuleTable_STAT);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(840);
    match(FeatParser::STAT);
    setState(841);
    match(FeatParser::LCBRACE);
    setState(843); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(842);
      statStatement();
      setState(845); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || ((((_la - 100) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 100)) & ((1ULL << (FeatParser::ELIDED_FALLBACK_NAME - 100))
      | (1ULL << (FeatParser::ELIDED_FALLBACK_NAME_ID - 100))
      | (1ULL << (FeatParser::DESIGN_AXIS - 100))
      | (1ULL << (FeatParser::AXIS_VALUE - 100)))) != 0));
    setState(847);
    match(FeatParser::RCBRACE);
    setState(848);
    match(FeatParser::STAT);
    setState(849);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatStatementContext ------------------------------------------------------------------

FeatParser::StatStatementContext::StatStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::StatStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::DesignAxisContext* FeatParser::StatStatementContext::designAxis() {
  return getRuleContext<FeatParser::DesignAxisContext>(0);
}

FeatParser::AxisValueContext* FeatParser::StatStatementContext::axisValue() {
  return getRuleContext<FeatParser::AxisValueContext>(0);
}

FeatParser::ElidedFallbackNameContext* FeatParser::StatStatementContext::elidedFallbackName() {
  return getRuleContext<FeatParser::ElidedFallbackNameContext>(0);
}

FeatParser::ElidedFallbackNameIDContext* FeatParser::StatStatementContext::elidedFallbackNameID() {
  return getRuleContext<FeatParser::ElidedFallbackNameIDContext>(0);
}

FeatParser::IncludeContext* FeatParser::StatStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::StatStatementContext::getRuleIndex() const {
  return FeatParser::RuleStatStatement;
}


antlrcpp::Any FeatParser::StatStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitStatStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::StatStatementContext* FeatParser::statStatement() {
  StatStatementContext *_localctx = _tracker.createInstance<StatStatementContext>(_ctx, getState());
  enterRule(_localctx, 126, FeatParser::RuleStatStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(856);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::DESIGN_AXIS: {
        setState(851);
        designAxis();
        break;
      }

      case FeatParser::AXIS_VALUE: {
        setState(852);
        axisValue();
        break;
      }

      case FeatParser::ELIDED_FALLBACK_NAME: {
        setState(853);
        elidedFallbackName();
        break;
      }

      case FeatParser::ELIDED_FALLBACK_NAME_ID: {
        setState(854);
        elidedFallbackNameID();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(855);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(858);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DesignAxisContext ------------------------------------------------------------------

FeatParser::DesignAxisContext::DesignAxisContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::DesignAxisContext::DESIGN_AXIS() {
  return getToken(FeatParser::DESIGN_AXIS, 0);
}

FeatParser::TagContext* FeatParser::DesignAxisContext::tag() {
  return getRuleContext<FeatParser::TagContext>(0);
}

tree::TerminalNode* FeatParser::DesignAxisContext::NUM() {
  return getToken(FeatParser::NUM, 0);
}

tree::TerminalNode* FeatParser::DesignAxisContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::DesignAxisContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

std::vector<FeatParser::NameEntryStatementContext *> FeatParser::DesignAxisContext::nameEntryStatement() {
  return getRuleContexts<FeatParser::NameEntryStatementContext>();
}

FeatParser::NameEntryStatementContext* FeatParser::DesignAxisContext::nameEntryStatement(size_t i) {
  return getRuleContext<FeatParser::NameEntryStatementContext>(i);
}


size_t FeatParser::DesignAxisContext::getRuleIndex() const {
  return FeatParser::RuleDesignAxis;
}


antlrcpp::Any FeatParser::DesignAxisContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitDesignAxis(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::DesignAxisContext* FeatParser::designAxis() {
  DesignAxisContext *_localctx = _tracker.createInstance<DesignAxisContext>(_ctx, getState());
  enterRule(_localctx, 128, FeatParser::RuleDesignAxis);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(860);
    match(FeatParser::DESIGN_AXIS);
    setState(861);
    tag();
    setState(862);
    match(FeatParser::NUM);
    setState(863);
    match(FeatParser::LCBRACE);
    setState(865); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(864);
      nameEntryStatement();
      setState(867); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || _la == FeatParser::NAME);
    setState(869);
    match(FeatParser::RCBRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AxisValueContext ------------------------------------------------------------------

FeatParser::AxisValueContext::AxisValueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AxisValueContext::AXIS_VALUE() {
  return getToken(FeatParser::AXIS_VALUE, 0);
}

tree::TerminalNode* FeatParser::AxisValueContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::AxisValueContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

std::vector<FeatParser::AxisValueStatementContext *> FeatParser::AxisValueContext::axisValueStatement() {
  return getRuleContexts<FeatParser::AxisValueStatementContext>();
}

FeatParser::AxisValueStatementContext* FeatParser::AxisValueContext::axisValueStatement(size_t i) {
  return getRuleContext<FeatParser::AxisValueStatementContext>(i);
}


size_t FeatParser::AxisValueContext::getRuleIndex() const {
  return FeatParser::RuleAxisValue;
}


antlrcpp::Any FeatParser::AxisValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAxisValue(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AxisValueContext* FeatParser::axisValue() {
  AxisValueContext *_localctx = _tracker.createInstance<AxisValueContext>(_ctx, getState());
  enterRule(_localctx, 130, FeatParser::RuleAxisValue);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(871);
    match(FeatParser::AXIS_VALUE);
    setState(872);
    match(FeatParser::LCBRACE);
    setState(874); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(873);
      axisValueStatement();
      setState(876); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || ((((_la - 78) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 78)) & ((1ULL << (FeatParser::NAME - 78))
      | (1ULL << (FeatParser::FLAG - 78))
      | (1ULL << (FeatParser::LOCATION - 78)))) != 0));
    setState(878);
    match(FeatParser::RCBRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AxisValueStatementContext ------------------------------------------------------------------

FeatParser::AxisValueStatementContext::AxisValueStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AxisValueStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::NameEntryContext* FeatParser::AxisValueStatementContext::nameEntry() {
  return getRuleContext<FeatParser::NameEntryContext>(0);
}

FeatParser::AxisValueLocationContext* FeatParser::AxisValueStatementContext::axisValueLocation() {
  return getRuleContext<FeatParser::AxisValueLocationContext>(0);
}

FeatParser::AxisValueFlagsContext* FeatParser::AxisValueStatementContext::axisValueFlags() {
  return getRuleContext<FeatParser::AxisValueFlagsContext>(0);
}

FeatParser::IncludeContext* FeatParser::AxisValueStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::AxisValueStatementContext::getRuleIndex() const {
  return FeatParser::RuleAxisValueStatement;
}


antlrcpp::Any FeatParser::AxisValueStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAxisValueStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AxisValueStatementContext* FeatParser::axisValueStatement() {
  AxisValueStatementContext *_localctx = _tracker.createInstance<AxisValueStatementContext>(_ctx, getState());
  enterRule(_localctx, 132, FeatParser::RuleAxisValueStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(884);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NAME: {
        setState(880);
        nameEntry();
        break;
      }

      case FeatParser::LOCATION: {
        setState(881);
        axisValueLocation();
        break;
      }

      case FeatParser::FLAG: {
        setState(882);
        axisValueFlags();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(883);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(886);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AxisValueLocationContext ------------------------------------------------------------------

FeatParser::AxisValueLocationContext::AxisValueLocationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AxisValueLocationContext::LOCATION() {
  return getToken(FeatParser::LOCATION, 0);
}

FeatParser::TagContext* FeatParser::AxisValueLocationContext::tag() {
  return getRuleContext<FeatParser::TagContext>(0);
}

std::vector<FeatParser::FixedNumContext *> FeatParser::AxisValueLocationContext::fixedNum() {
  return getRuleContexts<FeatParser::FixedNumContext>();
}

FeatParser::FixedNumContext* FeatParser::AxisValueLocationContext::fixedNum(size_t i) {
  return getRuleContext<FeatParser::FixedNumContext>(i);
}


size_t FeatParser::AxisValueLocationContext::getRuleIndex() const {
  return FeatParser::RuleAxisValueLocation;
}


antlrcpp::Any FeatParser::AxisValueLocationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAxisValueLocation(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AxisValueLocationContext* FeatParser::axisValueLocation() {
  AxisValueLocationContext *_localctx = _tracker.createInstance<AxisValueLocationContext>(_ctx, getState());
  enterRule(_localctx, 134, FeatParser::RuleAxisValueLocation);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(888);
    match(FeatParser::LOCATION);
    setState(889);
    tag();
    setState(890);
    fixedNum();
    setState(895);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::POINTNUM

    || _la == FeatParser::NUM) {
      setState(891);
      fixedNum();
      setState(893);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == FeatParser::POINTNUM

      || _la == FeatParser::NUM) {
        setState(892);
        fixedNum();
      }
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AxisValueFlagsContext ------------------------------------------------------------------

FeatParser::AxisValueFlagsContext::AxisValueFlagsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AxisValueFlagsContext::FLAG() {
  return getToken(FeatParser::FLAG, 0);
}

std::vector<tree::TerminalNode *> FeatParser::AxisValueFlagsContext::AXIS_OSFA() {
  return getTokens(FeatParser::AXIS_OSFA);
}

tree::TerminalNode* FeatParser::AxisValueFlagsContext::AXIS_OSFA(size_t i) {
  return getToken(FeatParser::AXIS_OSFA, i);
}

std::vector<tree::TerminalNode *> FeatParser::AxisValueFlagsContext::AXIS_EAVN() {
  return getTokens(FeatParser::AXIS_EAVN);
}

tree::TerminalNode* FeatParser::AxisValueFlagsContext::AXIS_EAVN(size_t i) {
  return getToken(FeatParser::AXIS_EAVN, i);
}


size_t FeatParser::AxisValueFlagsContext::getRuleIndex() const {
  return FeatParser::RuleAxisValueFlags;
}


antlrcpp::Any FeatParser::AxisValueFlagsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAxisValueFlags(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AxisValueFlagsContext* FeatParser::axisValueFlags() {
  AxisValueFlagsContext *_localctx = _tracker.createInstance<AxisValueFlagsContext>(_ctx, getState());
  enterRule(_localctx, 136, FeatParser::RuleAxisValueFlags);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(897);
    match(FeatParser::FLAG);
    setState(899); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(898);
      _la = _input->LA(1);
      if (!(_la == FeatParser::AXIS_EAVN

      || _la == FeatParser::AXIS_OSFA)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(901); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::AXIS_EAVN

    || _la == FeatParser::AXIS_OSFA);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ElidedFallbackNameContext ------------------------------------------------------------------

FeatParser::ElidedFallbackNameContext::ElidedFallbackNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::ElidedFallbackNameContext::ELIDED_FALLBACK_NAME() {
  return getToken(FeatParser::ELIDED_FALLBACK_NAME, 0);
}

tree::TerminalNode* FeatParser::ElidedFallbackNameContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::ElidedFallbackNameContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

std::vector<FeatParser::NameEntryStatementContext *> FeatParser::ElidedFallbackNameContext::nameEntryStatement() {
  return getRuleContexts<FeatParser::NameEntryStatementContext>();
}

FeatParser::NameEntryStatementContext* FeatParser::ElidedFallbackNameContext::nameEntryStatement(size_t i) {
  return getRuleContext<FeatParser::NameEntryStatementContext>(i);
}


size_t FeatParser::ElidedFallbackNameContext::getRuleIndex() const {
  return FeatParser::RuleElidedFallbackName;
}


antlrcpp::Any FeatParser::ElidedFallbackNameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitElidedFallbackName(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ElidedFallbackNameContext* FeatParser::elidedFallbackName() {
  ElidedFallbackNameContext *_localctx = _tracker.createInstance<ElidedFallbackNameContext>(_ctx, getState());
  enterRule(_localctx, 138, FeatParser::RuleElidedFallbackName);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(903);
    match(FeatParser::ELIDED_FALLBACK_NAME);
    setState(904);
    match(FeatParser::LCBRACE);
    setState(906); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(905);
      nameEntryStatement();
      setState(908); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || _la == FeatParser::NAME);
    setState(910);
    match(FeatParser::RCBRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NameEntryStatementContext ------------------------------------------------------------------

FeatParser::NameEntryStatementContext::NameEntryStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::NameEntryStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::NameEntryContext* FeatParser::NameEntryStatementContext::nameEntry() {
  return getRuleContext<FeatParser::NameEntryContext>(0);
}

FeatParser::IncludeContext* FeatParser::NameEntryStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::NameEntryStatementContext::getRuleIndex() const {
  return FeatParser::RuleNameEntryStatement;
}


antlrcpp::Any FeatParser::NameEntryStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitNameEntryStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::NameEntryStatementContext* FeatParser::nameEntryStatement() {
  NameEntryStatementContext *_localctx = _tracker.createInstance<NameEntryStatementContext>(_ctx, getState());
  enterRule(_localctx, 140, FeatParser::RuleNameEntryStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(914);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NAME: {
        setState(912);
        nameEntry();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(913);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(916);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ElidedFallbackNameIDContext ------------------------------------------------------------------

FeatParser::ElidedFallbackNameIDContext::ElidedFallbackNameIDContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::ElidedFallbackNameIDContext::ELIDED_FALLBACK_NAME_ID() {
  return getToken(FeatParser::ELIDED_FALLBACK_NAME_ID, 0);
}

FeatParser::GenNumContext* FeatParser::ElidedFallbackNameIDContext::genNum() {
  return getRuleContext<FeatParser::GenNumContext>(0);
}


size_t FeatParser::ElidedFallbackNameIDContext::getRuleIndex() const {
  return FeatParser::RuleElidedFallbackNameID;
}


antlrcpp::Any FeatParser::ElidedFallbackNameIDContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitElidedFallbackNameID(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::ElidedFallbackNameIDContext* FeatParser::elidedFallbackNameID() {
  ElidedFallbackNameIDContext *_localctx = _tracker.createInstance<ElidedFallbackNameIDContext>(_ctx, getState());
  enterRule(_localctx, 142, FeatParser::RuleElidedFallbackNameID);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(918);
    match(FeatParser::ELIDED_FALLBACK_NAME_ID);
    setState(919);
    genNum();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NameEntryContext ------------------------------------------------------------------

FeatParser::NameEntryContext::NameEntryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::NameEntryContext::NAME() {
  return getToken(FeatParser::NAME, 0);
}

tree::TerminalNode* FeatParser::NameEntryContext::QUOTE() {
  return getToken(FeatParser::QUOTE, 0);
}

tree::TerminalNode* FeatParser::NameEntryContext::STRVAL() {
  return getToken(FeatParser::STRVAL, 0);
}

tree::TerminalNode* FeatParser::NameEntryContext::EQUOTE() {
  return getToken(FeatParser::EQUOTE, 0);
}

std::vector<FeatParser::GenNumContext *> FeatParser::NameEntryContext::genNum() {
  return getRuleContexts<FeatParser::GenNumContext>();
}

FeatParser::GenNumContext* FeatParser::NameEntryContext::genNum(size_t i) {
  return getRuleContext<FeatParser::GenNumContext>(i);
}


size_t FeatParser::NameEntryContext::getRuleIndex() const {
  return FeatParser::RuleNameEntry;
}


antlrcpp::Any FeatParser::NameEntryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitNameEntry(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::NameEntryContext* FeatParser::nameEntry() {
  NameEntryContext *_localctx = _tracker.createInstance<NameEntryContext>(_ctx, getState());
  enterRule(_localctx, 144, FeatParser::RuleNameEntry);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(921);
    match(FeatParser::NAME);
    setState(928);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (((((_la - 131) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 131)) & ((1ULL << (FeatParser::NUMEXT - 131))
      | (1ULL << (FeatParser::NUMOCT - 131))
      | (1ULL << (FeatParser::NUM - 131)))) != 0)) {
      setState(922);
      genNum();
      setState(926);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (((((_la - 131) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 131)) & ((1ULL << (FeatParser::NUMEXT - 131))
        | (1ULL << (FeatParser::NUMOCT - 131))
        | (1ULL << (FeatParser::NUM - 131)))) != 0)) {
        setState(923);
        genNum();
        setState(924);
        genNum();
      }
    }
    setState(930);
    match(FeatParser::QUOTE);
    setState(931);
    match(FeatParser::STRVAL);
    setState(932);
    match(FeatParser::EQUOTE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Table_vmtxContext ------------------------------------------------------------------

FeatParser::Table_vmtxContext::Table_vmtxContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> FeatParser::Table_vmtxContext::VMTX() {
  return getTokens(FeatParser::VMTX);
}

tree::TerminalNode* FeatParser::Table_vmtxContext::VMTX(size_t i) {
  return getToken(FeatParser::VMTX, i);
}

tree::TerminalNode* FeatParser::Table_vmtxContext::LCBRACE() {
  return getToken(FeatParser::LCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_vmtxContext::RCBRACE() {
  return getToken(FeatParser::RCBRACE, 0);
}

tree::TerminalNode* FeatParser::Table_vmtxContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

std::vector<FeatParser::VmtxStatementContext *> FeatParser::Table_vmtxContext::vmtxStatement() {
  return getRuleContexts<FeatParser::VmtxStatementContext>();
}

FeatParser::VmtxStatementContext* FeatParser::Table_vmtxContext::vmtxStatement(size_t i) {
  return getRuleContext<FeatParser::VmtxStatementContext>(i);
}


size_t FeatParser::Table_vmtxContext::getRuleIndex() const {
  return FeatParser::RuleTable_vmtx;
}


antlrcpp::Any FeatParser::Table_vmtxContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTable_vmtx(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Table_vmtxContext* FeatParser::table_vmtx() {
  Table_vmtxContext *_localctx = _tracker.createInstance<Table_vmtxContext>(_ctx, getState());
  enterRule(_localctx, 146, FeatParser::RuleTable_vmtx);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(934);
    match(FeatParser::VMTX);
    setState(935);
    match(FeatParser::LCBRACE);
    setState(937); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(936);
      vmtxStatement();
      setState(939); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::INCLUDE || _la == FeatParser::VERT_ORIGIN_Y

    || _la == FeatParser::VERT_ADVANCE_Y);
    setState(941);
    match(FeatParser::RCBRACE);
    setState(942);
    match(FeatParser::VMTX);
    setState(943);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VmtxStatementContext ------------------------------------------------------------------

FeatParser::VmtxStatementContext::VmtxStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::VmtxStatementContext::SEMI() {
  return getToken(FeatParser::SEMI, 0);
}

FeatParser::VmtxContext* FeatParser::VmtxStatementContext::vmtx() {
  return getRuleContext<FeatParser::VmtxContext>(0);
}

FeatParser::IncludeContext* FeatParser::VmtxStatementContext::include() {
  return getRuleContext<FeatParser::IncludeContext>(0);
}


size_t FeatParser::VmtxStatementContext::getRuleIndex() const {
  return FeatParser::RuleVmtxStatement;
}


antlrcpp::Any FeatParser::VmtxStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitVmtxStatement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::VmtxStatementContext* FeatParser::vmtxStatement() {
  VmtxStatementContext *_localctx = _tracker.createInstance<VmtxStatementContext>(_ctx, getState());
  enterRule(_localctx, 148, FeatParser::RuleVmtxStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(947);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::VERT_ORIGIN_Y:
      case FeatParser::VERT_ADVANCE_Y: {
        setState(945);
        vmtx();
        break;
      }

      case FeatParser::INCLUDE: {
        setState(946);
        include();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(949);
    match(FeatParser::SEMI);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VmtxContext ------------------------------------------------------------------

FeatParser::VmtxContext::VmtxContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::GlyphContext* FeatParser::VmtxContext::glyph() {
  return getRuleContext<FeatParser::GlyphContext>(0);
}

tree::TerminalNode* FeatParser::VmtxContext::NUM() {
  return getToken(FeatParser::NUM, 0);
}

tree::TerminalNode* FeatParser::VmtxContext::VERT_ORIGIN_Y() {
  return getToken(FeatParser::VERT_ORIGIN_Y, 0);
}

tree::TerminalNode* FeatParser::VmtxContext::VERT_ADVANCE_Y() {
  return getToken(FeatParser::VERT_ADVANCE_Y, 0);
}


size_t FeatParser::VmtxContext::getRuleIndex() const {
  return FeatParser::RuleVmtx;
}


antlrcpp::Any FeatParser::VmtxContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitVmtx(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::VmtxContext* FeatParser::vmtx() {
  VmtxContext *_localctx = _tracker.createInstance<VmtxContext>(_ctx, getState());
  enterRule(_localctx, 150, FeatParser::RuleVmtx);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(951);
    _la = _input->LA(1);
    if (!(_la == FeatParser::VERT_ORIGIN_Y

    || _la == FeatParser::VERT_ADVANCE_Y)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(952);
    glyph();
    setState(953);
    match(FeatParser::NUM);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AnchorContext ------------------------------------------------------------------

FeatParser::AnchorContext::AnchorContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AnchorContext::BEGINVALUE() {
  return getToken(FeatParser::BEGINVALUE, 0);
}

tree::TerminalNode* FeatParser::AnchorContext::ANCHOR() {
  return getToken(FeatParser::ANCHOR, 0);
}

tree::TerminalNode* FeatParser::AnchorContext::ENDVALUE() {
  return getToken(FeatParser::ENDVALUE, 0);
}

tree::TerminalNode* FeatParser::AnchorContext::KNULL() {
  return getToken(FeatParser::KNULL, 0);
}

FeatParser::LabelContext* FeatParser::AnchorContext::label() {
  return getRuleContext<FeatParser::LabelContext>(0);
}

std::vector<tree::TerminalNode *> FeatParser::AnchorContext::NUM() {
  return getTokens(FeatParser::NUM);
}

tree::TerminalNode* FeatParser::AnchorContext::NUM(size_t i) {
  return getToken(FeatParser::NUM, i);
}

tree::TerminalNode* FeatParser::AnchorContext::CONTOURPOINT() {
  return getToken(FeatParser::CONTOURPOINT, 0);
}


size_t FeatParser::AnchorContext::getRuleIndex() const {
  return FeatParser::RuleAnchor;
}


antlrcpp::Any FeatParser::AnchorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAnchor(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AnchorContext* FeatParser::anchor() {
  AnchorContext *_localctx = _tracker.createInstance<AnchorContext>(_ctx, getState());
  enterRule(_localctx, 152, FeatParser::RuleAnchor);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(955);
    match(FeatParser::BEGINVALUE);
    setState(956);
    match(FeatParser::ANCHOR);
    setState(965);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NUM: {
        setState(957);
        antlrcpp::downCast<AnchorContext *>(_localctx)->xval = match(FeatParser::NUM);
        setState(958);
        antlrcpp::downCast<AnchorContext *>(_localctx)->yval = match(FeatParser::NUM);
        setState(961);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::CONTOURPOINT) {
          setState(959);
          match(FeatParser::CONTOURPOINT);
          setState(960);
          antlrcpp::downCast<AnchorContext *>(_localctx)->cp = match(FeatParser::NUM);
        }
        break;
      }

      case FeatParser::KNULL: {
        setState(963);
        match(FeatParser::KNULL);
        break;
      }

      case FeatParser::MARK:
      case FeatParser::NAMELABEL: {
        setState(964);
        antlrcpp::downCast<AnchorContext *>(_localctx)->name = label();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(967);
    match(FeatParser::ENDVALUE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LookupPatternContext ------------------------------------------------------------------

FeatParser::LookupPatternContext::LookupPatternContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<FeatParser::LookupPatternElementContext *> FeatParser::LookupPatternContext::lookupPatternElement() {
  return getRuleContexts<FeatParser::LookupPatternElementContext>();
}

FeatParser::LookupPatternElementContext* FeatParser::LookupPatternContext::lookupPatternElement(size_t i) {
  return getRuleContext<FeatParser::LookupPatternElementContext>(i);
}


size_t FeatParser::LookupPatternContext::getRuleIndex() const {
  return FeatParser::RuleLookupPattern;
}


antlrcpp::Any FeatParser::LookupPatternContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLookupPattern(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LookupPatternContext* FeatParser::lookupPattern() {
  LookupPatternContext *_localctx = _tracker.createInstance<LookupPatternContext>(_ctx, getState());
  enterRule(_localctx, 154, FeatParser::RuleLookupPattern);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(970); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(969);
      lookupPatternElement();
      setState(972); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
      | (1ULL << (FeatParser::GCLASS - 117))
      | (1ULL << (FeatParser::CID - 117))
      | (1ULL << (FeatParser::ESCGNAME - 117))
      | (1ULL << (FeatParser::NAMELABEL - 117))
      | (1ULL << (FeatParser::EXTNAME - 117)))) != 0));
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LookupPatternElementContext ------------------------------------------------------------------

FeatParser::LookupPatternElementContext::LookupPatternElementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::PatternElementContext* FeatParser::LookupPatternElementContext::patternElement() {
  return getRuleContext<FeatParser::PatternElementContext>(0);
}

std::vector<tree::TerminalNode *> FeatParser::LookupPatternElementContext::LOOKUP() {
  return getTokens(FeatParser::LOOKUP);
}

tree::TerminalNode* FeatParser::LookupPatternElementContext::LOOKUP(size_t i) {
  return getToken(FeatParser::LOOKUP, i);
}

std::vector<FeatParser::LabelContext *> FeatParser::LookupPatternElementContext::label() {
  return getRuleContexts<FeatParser::LabelContext>();
}

FeatParser::LabelContext* FeatParser::LookupPatternElementContext::label(size_t i) {
  return getRuleContext<FeatParser::LabelContext>(i);
}


size_t FeatParser::LookupPatternElementContext::getRuleIndex() const {
  return FeatParser::RuleLookupPatternElement;
}


antlrcpp::Any FeatParser::LookupPatternElementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLookupPatternElement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LookupPatternElementContext* FeatParser::lookupPatternElement() {
  LookupPatternElementContext *_localctx = _tracker.createInstance<LookupPatternElementContext>(_ctx, getState());
  enterRule(_localctx, 156, FeatParser::RuleLookupPatternElement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(974);
    patternElement();
    setState(979);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::LOOKUP) {
      setState(975);
      match(FeatParser::LOOKUP);
      setState(976);
      label();
      setState(981);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PatternContext ------------------------------------------------------------------

FeatParser::PatternContext::PatternContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<FeatParser::PatternElementContext *> FeatParser::PatternContext::patternElement() {
  return getRuleContexts<FeatParser::PatternElementContext>();
}

FeatParser::PatternElementContext* FeatParser::PatternContext::patternElement(size_t i) {
  return getRuleContext<FeatParser::PatternElementContext>(i);
}


size_t FeatParser::PatternContext::getRuleIndex() const {
  return FeatParser::RulePattern;
}


antlrcpp::Any FeatParser::PatternContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitPattern(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::PatternContext* FeatParser::pattern() {
  PatternContext *_localctx = _tracker.createInstance<PatternContext>(_ctx, getState());
  enterRule(_localctx, 158, FeatParser::RulePattern);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(983); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(982);
      patternElement();
      setState(985); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::NOTDEF || ((((_la - 117) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 117)) & ((1ULL << (FeatParser::LBRACKET - 117))
      | (1ULL << (FeatParser::GCLASS - 117))
      | (1ULL << (FeatParser::CID - 117))
      | (1ULL << (FeatParser::ESCGNAME - 117))
      | (1ULL << (FeatParser::NAMELABEL - 117))
      | (1ULL << (FeatParser::EXTNAME - 117)))) != 0));
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PatternElementContext ------------------------------------------------------------------

FeatParser::PatternElementContext::PatternElementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::GlyphClassContext* FeatParser::PatternElementContext::glyphClass() {
  return getRuleContext<FeatParser::GlyphClassContext>(0);
}

FeatParser::GlyphContext* FeatParser::PatternElementContext::glyph() {
  return getRuleContext<FeatParser::GlyphContext>(0);
}

tree::TerminalNode* FeatParser::PatternElementContext::MARKER() {
  return getToken(FeatParser::MARKER, 0);
}


size_t FeatParser::PatternElementContext::getRuleIndex() const {
  return FeatParser::RulePatternElement;
}


antlrcpp::Any FeatParser::PatternElementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitPatternElement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::PatternElementContext* FeatParser::patternElement() {
  PatternElementContext *_localctx = _tracker.createInstance<PatternElementContext>(_ctx, getState());
  enterRule(_localctx, 160, FeatParser::RulePatternElement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(989);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::LBRACKET:
      case FeatParser::GCLASS: {
        setState(987);
        glyphClass();
        break;
      }

      case FeatParser::NOTDEF:
      case FeatParser::CID:
      case FeatParser::ESCGNAME:
      case FeatParser::NAMELABEL:
      case FeatParser::EXTNAME: {
        setState(988);
        glyph();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
    setState(992);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::MARKER) {
      setState(991);
      match(FeatParser::MARKER);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GlyphClassOptionalContext ------------------------------------------------------------------

FeatParser::GlyphClassOptionalContext::GlyphClassOptionalContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::GlyphClassContext* FeatParser::GlyphClassOptionalContext::glyphClass() {
  return getRuleContext<FeatParser::GlyphClassContext>(0);
}


size_t FeatParser::GlyphClassOptionalContext::getRuleIndex() const {
  return FeatParser::RuleGlyphClassOptional;
}


antlrcpp::Any FeatParser::GlyphClassOptionalContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGlyphClassOptional(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GlyphClassOptionalContext* FeatParser::glyphClassOptional() {
  GlyphClassOptionalContext *_localctx = _tracker.createInstance<GlyphClassOptionalContext>(_ctx, getState());
  enterRule(_localctx, 162, FeatParser::RuleGlyphClassOptional);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(995);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == FeatParser::LBRACKET

    || _la == FeatParser::GCLASS) {
      setState(994);
      glyphClass();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GlyphClassContext ------------------------------------------------------------------

FeatParser::GlyphClassContext::GlyphClassContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GlyphClassContext::GCLASS() {
  return getToken(FeatParser::GCLASS, 0);
}

FeatParser::GcLiteralContext* FeatParser::GlyphClassContext::gcLiteral() {
  return getRuleContext<FeatParser::GcLiteralContext>(0);
}


size_t FeatParser::GlyphClassContext::getRuleIndex() const {
  return FeatParser::RuleGlyphClass;
}


antlrcpp::Any FeatParser::GlyphClassContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGlyphClass(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GlyphClassContext* FeatParser::glyphClass() {
  GlyphClassContext *_localctx = _tracker.createInstance<GlyphClassContext>(_ctx, getState());
  enterRule(_localctx, 164, FeatParser::RuleGlyphClass);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(999);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::GCLASS: {
        enterOuterAlt(_localctx, 1);
        setState(997);
        match(FeatParser::GCLASS);
        break;
      }

      case FeatParser::LBRACKET: {
        enterOuterAlt(_localctx, 2);
        setState(998);
        gcLiteral();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GcLiteralContext ------------------------------------------------------------------

FeatParser::GcLiteralContext::GcLiteralContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GcLiteralContext::LBRACKET() {
  return getToken(FeatParser::LBRACKET, 0);
}

tree::TerminalNode* FeatParser::GcLiteralContext::RBRACKET() {
  return getToken(FeatParser::RBRACKET, 0);
}

std::vector<FeatParser::GcLiteralElementContext *> FeatParser::GcLiteralContext::gcLiteralElement() {
  return getRuleContexts<FeatParser::GcLiteralElementContext>();
}

FeatParser::GcLiteralElementContext* FeatParser::GcLiteralContext::gcLiteralElement(size_t i) {
  return getRuleContext<FeatParser::GcLiteralElementContext>(i);
}


size_t FeatParser::GcLiteralContext::getRuleIndex() const {
  return FeatParser::RuleGcLiteral;
}


antlrcpp::Any FeatParser::GcLiteralContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGcLiteral(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GcLiteralContext* FeatParser::gcLiteral() {
  GcLiteralContext *_localctx = _tracker.createInstance<GcLiteralContext>(_ctx, getState());
  enterRule(_localctx, 166, FeatParser::RuleGcLiteral);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1001);
    match(FeatParser::LBRACKET);
    setState(1003); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(1002);
      gcLiteralElement();
      setState(1005); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == FeatParser::NOTDEF || ((((_la - 125) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 125)) & ((1ULL << (FeatParser::GCLASS - 125))
      | (1ULL << (FeatParser::CID - 125))
      | (1ULL << (FeatParser::ESCGNAME - 125))
      | (1ULL << (FeatParser::NAMELABEL - 125))
      | (1ULL << (FeatParser::EXTNAME - 125)))) != 0));
    setState(1007);
    match(FeatParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GcLiteralElementContext ------------------------------------------------------------------

FeatParser::GcLiteralElementContext::GcLiteralElementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<FeatParser::GlyphContext *> FeatParser::GcLiteralElementContext::glyph() {
  return getRuleContexts<FeatParser::GlyphContext>();
}

FeatParser::GlyphContext* FeatParser::GcLiteralElementContext::glyph(size_t i) {
  return getRuleContext<FeatParser::GlyphContext>(i);
}

tree::TerminalNode* FeatParser::GcLiteralElementContext::HYPHEN() {
  return getToken(FeatParser::HYPHEN, 0);
}

tree::TerminalNode* FeatParser::GcLiteralElementContext::GCLASS() {
  return getToken(FeatParser::GCLASS, 0);
}


size_t FeatParser::GcLiteralElementContext::getRuleIndex() const {
  return FeatParser::RuleGcLiteralElement;
}


antlrcpp::Any FeatParser::GcLiteralElementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGcLiteralElement(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GcLiteralElementContext* FeatParser::gcLiteralElement() {
  GcLiteralElementContext *_localctx = _tracker.createInstance<GcLiteralElementContext>(_ctx, getState());
  enterRule(_localctx, 168, FeatParser::RuleGcLiteralElement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(1015);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NOTDEF:
      case FeatParser::CID:
      case FeatParser::ESCGNAME:
      case FeatParser::NAMELABEL:
      case FeatParser::EXTNAME: {
        enterOuterAlt(_localctx, 1);
        setState(1009);
        antlrcpp::downCast<GcLiteralElementContext *>(_localctx)->startg = glyph();
        setState(1012);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == FeatParser::HYPHEN) {
          setState(1010);
          match(FeatParser::HYPHEN);
          setState(1011);
          antlrcpp::downCast<GcLiteralElementContext *>(_localctx)->endg = glyph();
        }
        break;
      }

      case FeatParser::GCLASS: {
        enterOuterAlt(_localctx, 2);
        setState(1014);
        match(FeatParser::GCLASS);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GlyphContext ------------------------------------------------------------------

FeatParser::GlyphContext::GlyphContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

FeatParser::GlyphNameContext* FeatParser::GlyphContext::glyphName() {
  return getRuleContext<FeatParser::GlyphNameContext>(0);
}

tree::TerminalNode* FeatParser::GlyphContext::CID() {
  return getToken(FeatParser::CID, 0);
}


size_t FeatParser::GlyphContext::getRuleIndex() const {
  return FeatParser::RuleGlyph;
}


antlrcpp::Any FeatParser::GlyphContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGlyph(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GlyphContext* FeatParser::glyph() {
  GlyphContext *_localctx = _tracker.createInstance<GlyphContext>(_ctx, getState());
  enterRule(_localctx, 170, FeatParser::RuleGlyph);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(1019);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case FeatParser::NOTDEF:
      case FeatParser::ESCGNAME:
      case FeatParser::NAMELABEL:
      case FeatParser::EXTNAME: {
        enterOuterAlt(_localctx, 1);
        setState(1017);
        glyphName();
        break;
      }

      case FeatParser::CID: {
        enterOuterAlt(_localctx, 2);
        setState(1018);
        match(FeatParser::CID);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GlyphNameContext ------------------------------------------------------------------

FeatParser::GlyphNameContext::GlyphNameContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GlyphNameContext::ESCGNAME() {
  return getToken(FeatParser::ESCGNAME, 0);
}

tree::TerminalNode* FeatParser::GlyphNameContext::NAMELABEL() {
  return getToken(FeatParser::NAMELABEL, 0);
}

tree::TerminalNode* FeatParser::GlyphNameContext::EXTNAME() {
  return getToken(FeatParser::EXTNAME, 0);
}

tree::TerminalNode* FeatParser::GlyphNameContext::NOTDEF() {
  return getToken(FeatParser::NOTDEF, 0);
}


size_t FeatParser::GlyphNameContext::getRuleIndex() const {
  return FeatParser::RuleGlyphName;
}


antlrcpp::Any FeatParser::GlyphNameContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGlyphName(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GlyphNameContext* FeatParser::glyphName() {
  GlyphNameContext *_localctx = _tracker.createInstance<GlyphNameContext>(_ctx, getState());
  enterRule(_localctx, 172, FeatParser::RuleGlyphName);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1021);
    _la = _input->LA(1);
    if (!(_la == FeatParser::NOTDEF || ((((_la - 127) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 127)) & ((1ULL << (FeatParser::ESCGNAME - 127))
      | (1ULL << (FeatParser::NAMELABEL - 127))
      | (1ULL << (FeatParser::EXTNAME - 127)))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LabelContext ------------------------------------------------------------------

FeatParser::LabelContext::LabelContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::LabelContext::NAMELABEL() {
  return getToken(FeatParser::NAMELABEL, 0);
}

tree::TerminalNode* FeatParser::LabelContext::MARK() {
  return getToken(FeatParser::MARK, 0);
}


size_t FeatParser::LabelContext::getRuleIndex() const {
  return FeatParser::RuleLabel;
}


antlrcpp::Any FeatParser::LabelContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitLabel(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::LabelContext* FeatParser::label() {
  LabelContext *_localctx = _tracker.createInstance<LabelContext>(_ctx, getState());
  enterRule(_localctx, 174, FeatParser::RuleLabel);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1023);
    _la = _input->LA(1);
    if (!(_la == FeatParser::MARK || _la == FeatParser::NAMELABEL)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TagContext ------------------------------------------------------------------

FeatParser::TagContext::TagContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::TagContext::NAMELABEL() {
  return getToken(FeatParser::NAMELABEL, 0);
}

tree::TerminalNode* FeatParser::TagContext::EXTNAME() {
  return getToken(FeatParser::EXTNAME, 0);
}

tree::TerminalNode* FeatParser::TagContext::CATCHTAG() {
  return getToken(FeatParser::CATCHTAG, 0);
}

tree::TerminalNode* FeatParser::TagContext::MARK() {
  return getToken(FeatParser::MARK, 0);
}


size_t FeatParser::TagContext::getRuleIndex() const {
  return FeatParser::RuleTag;
}


antlrcpp::Any FeatParser::TagContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitTag(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::TagContext* FeatParser::tag() {
  TagContext *_localctx = _tracker.createInstance<TagContext>(_ctx, getState());
  enterRule(_localctx, 176, FeatParser::RuleTag);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1025);
    _la = _input->LA(1);
    if (!(_la == FeatParser::MARK || ((((_la - 128) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 128)) & ((1ULL << (FeatParser::NAMELABEL - 128))
      | (1ULL << (FeatParser::EXTNAME - 128))
      | (1ULL << (FeatParser::CATCHTAG - 128)))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FixedNumContext ------------------------------------------------------------------

FeatParser::FixedNumContext::FixedNumContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::FixedNumContext::POINTNUM() {
  return getToken(FeatParser::POINTNUM, 0);
}

tree::TerminalNode* FeatParser::FixedNumContext::NUM() {
  return getToken(FeatParser::NUM, 0);
}


size_t FeatParser::FixedNumContext::getRuleIndex() const {
  return FeatParser::RuleFixedNum;
}


antlrcpp::Any FeatParser::FixedNumContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitFixedNum(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::FixedNumContext* FeatParser::fixedNum() {
  FixedNumContext *_localctx = _tracker.createInstance<FixedNumContext>(_ctx, getState());
  enterRule(_localctx, 178, FeatParser::RuleFixedNum);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1027);
    _la = _input->LA(1);
    if (!(_la == FeatParser::POINTNUM

    || _la == FeatParser::NUM)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GenNumContext ------------------------------------------------------------------

FeatParser::GenNumContext::GenNumContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GenNumContext::NUM() {
  return getToken(FeatParser::NUM, 0);
}

tree::TerminalNode* FeatParser::GenNumContext::NUMOCT() {
  return getToken(FeatParser::NUMOCT, 0);
}

tree::TerminalNode* FeatParser::GenNumContext::NUMEXT() {
  return getToken(FeatParser::NUMEXT, 0);
}


size_t FeatParser::GenNumContext::getRuleIndex() const {
  return FeatParser::RuleGenNum;
}


antlrcpp::Any FeatParser::GenNumContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGenNum(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GenNumContext* FeatParser::genNum() {
  GenNumContext *_localctx = _tracker.createInstance<GenNumContext>(_ctx, getState());
  enterRule(_localctx, 180, FeatParser::RuleGenNum);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1029);
    _la = _input->LA(1);
    if (!(((((_la - 131) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 131)) & ((1ULL << (FeatParser::NUMEXT - 131))
      | (1ULL << (FeatParser::NUMOCT - 131))
      | (1ULL << (FeatParser::NUM - 131)))) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FeatureFileContext ------------------------------------------------------------------

FeatParser::FeatureFileContext::FeatureFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::FeatureFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::FeatureStatementContext *> FeatParser::FeatureFileContext::featureStatement() {
  return getRuleContexts<FeatParser::FeatureStatementContext>();
}

FeatParser::FeatureStatementContext* FeatParser::FeatureFileContext::featureStatement(size_t i) {
  return getRuleContext<FeatParser::FeatureStatementContext>(i);
}


size_t FeatParser::FeatureFileContext::getRuleIndex() const {
  return FeatParser::RuleFeatureFile;
}


antlrcpp::Any FeatParser::FeatureFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitFeatureFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::FeatureFileContext* FeatParser::featureFile() {
  FeatureFileContext *_localctx = _tracker.createInstance<FeatureFileContext>(_ctx, getState());
  enterRule(_localctx, 182, FeatParser::RuleFeatureFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1034);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::INCLUDE)
      | (1ULL << FeatParser::FEATURE)
      | (1ULL << FeatParser::SCRIPT)
      | (1ULL << FeatParser::LANGUAGE)
      | (1ULL << FeatParser::SUBTABLE)
      | (1ULL << FeatParser::LOOKUP)
      | (1ULL << FeatParser::LOOKUPFLAG)
      | (1ULL << FeatParser::ENUMERATE)
      | (1ULL << FeatParser::ENUMERATE_v)
      | (1ULL << FeatParser::EXCEPT)
      | (1ULL << FeatParser::IGNORE)
      | (1ULL << FeatParser::SUBSTITUTE)
      | (1ULL << FeatParser::SUBSTITUTE_v)
      | (1ULL << FeatParser::REVERSE)
      | (1ULL << FeatParser::REVERSE_v)
      | (1ULL << FeatParser::POSITION)
      | (1ULL << FeatParser::POSITION_v)
      | (1ULL << FeatParser::PARAMETERS)
      | (1ULL << FeatParser::FEATURE_NAMES)
      | (1ULL << FeatParser::CV_PARAMETERS)
      | (1ULL << FeatParser::SIZEMENUNAME)
      | (1ULL << FeatParser::MARK_CLASS))) != 0) || _la == FeatParser::GCLASS) {
      setState(1031);
      featureStatement();
      setState(1036);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1037);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementFileContext ------------------------------------------------------------------

FeatParser::StatementFileContext::StatementFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::StatementFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::StatementContext *> FeatParser::StatementFileContext::statement() {
  return getRuleContexts<FeatParser::StatementContext>();
}

FeatParser::StatementContext* FeatParser::StatementFileContext::statement(size_t i) {
  return getRuleContext<FeatParser::StatementContext>(i);
}


size_t FeatParser::StatementFileContext::getRuleIndex() const {
  return FeatParser::RuleStatementFile;
}


antlrcpp::Any FeatParser::StatementFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitStatementFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::StatementFileContext* FeatParser::statementFile() {
  StatementFileContext *_localctx = _tracker.createInstance<StatementFileContext>(_ctx, getState());
  enterRule(_localctx, 184, FeatParser::RuleStatementFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1042);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::INCLUDE)
      | (1ULL << FeatParser::FEATURE)
      | (1ULL << FeatParser::SCRIPT)
      | (1ULL << FeatParser::LANGUAGE)
      | (1ULL << FeatParser::SUBTABLE)
      | (1ULL << FeatParser::LOOKUPFLAG)
      | (1ULL << FeatParser::ENUMERATE)
      | (1ULL << FeatParser::ENUMERATE_v)
      | (1ULL << FeatParser::EXCEPT)
      | (1ULL << FeatParser::IGNORE)
      | (1ULL << FeatParser::SUBSTITUTE)
      | (1ULL << FeatParser::SUBSTITUTE_v)
      | (1ULL << FeatParser::REVERSE)
      | (1ULL << FeatParser::REVERSE_v)
      | (1ULL << FeatParser::POSITION)
      | (1ULL << FeatParser::POSITION_v)
      | (1ULL << FeatParser::PARAMETERS)
      | (1ULL << FeatParser::FEATURE_NAMES)
      | (1ULL << FeatParser::SIZEMENUNAME)
      | (1ULL << FeatParser::MARK_CLASS))) != 0) || _la == FeatParser::GCLASS) {
      setState(1039);
      statement();
      setState(1044);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1045);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CvStatementFileContext ------------------------------------------------------------------

FeatParser::CvStatementFileContext::CvStatementFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::CvStatementFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::CvParameterStatementContext *> FeatParser::CvStatementFileContext::cvParameterStatement() {
  return getRuleContexts<FeatParser::CvParameterStatementContext>();
}

FeatParser::CvParameterStatementContext* FeatParser::CvStatementFileContext::cvParameterStatement(size_t i) {
  return getRuleContext<FeatParser::CvParameterStatementContext>(i);
}


size_t FeatParser::CvStatementFileContext::getRuleIndex() const {
  return FeatParser::RuleCvStatementFile;
}


antlrcpp::Any FeatParser::CvStatementFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitCvStatementFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::CvStatementFileContext* FeatParser::cvStatementFile() {
  CvStatementFileContext *_localctx = _tracker.createInstance<CvStatementFileContext>(_ctx, getState());
  enterRule(_localctx, 186, FeatParser::RuleCvStatementFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1050);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & ((1ULL << FeatParser::INCLUDE)
      | (1ULL << FeatParser::CV_UI_LABEL)
      | (1ULL << FeatParser::CV_TOOLTIP)
      | (1ULL << FeatParser::CV_SAMPLE_TEXT)
      | (1ULL << FeatParser::CV_PARAM_LABEL)
      | (1ULL << FeatParser::CV_CHARACTER))) != 0)) {
      setState(1047);
      cvParameterStatement();
      setState(1052);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1053);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BaseFileContext ------------------------------------------------------------------

FeatParser::BaseFileContext::BaseFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::BaseFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::BaseStatementContext *> FeatParser::BaseFileContext::baseStatement() {
  return getRuleContexts<FeatParser::BaseStatementContext>();
}

FeatParser::BaseStatementContext* FeatParser::BaseFileContext::baseStatement(size_t i) {
  return getRuleContext<FeatParser::BaseStatementContext>(i);
}


size_t FeatParser::BaseFileContext::getRuleIndex() const {
  return FeatParser::RuleBaseFile;
}


antlrcpp::Any FeatParser::BaseFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitBaseFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::BaseFileContext* FeatParser::baseFile() {
  BaseFileContext *_localctx = _tracker.createInstance<BaseFileContext>(_ctx, getState());
  enterRule(_localctx, 188, FeatParser::RuleBaseFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1058);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (((((_la - 5) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 5)) & ((1ULL << (FeatParser::INCLUDE - 5))
      | (1ULL << (FeatParser::HA_BTL - 5))
      | (1ULL << (FeatParser::VA_BTL - 5))
      | (1ULL << (FeatParser::HA_BSL - 5))
      | (1ULL << (FeatParser::VA_BSL - 5)))) != 0)) {
      setState(1055);
      baseStatement();
      setState(1060);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1061);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- HeadFileContext ------------------------------------------------------------------

FeatParser::HeadFileContext::HeadFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::HeadFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::HeadStatementContext *> FeatParser::HeadFileContext::headStatement() {
  return getRuleContexts<FeatParser::HeadStatementContext>();
}

FeatParser::HeadStatementContext* FeatParser::HeadFileContext::headStatement(size_t i) {
  return getRuleContext<FeatParser::HeadStatementContext>(i);
}


size_t FeatParser::HeadFileContext::getRuleIndex() const {
  return FeatParser::RuleHeadFile;
}


antlrcpp::Any FeatParser::HeadFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitHeadFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::HeadFileContext* FeatParser::headFile() {
  HeadFileContext *_localctx = _tracker.createInstance<HeadFileContext>(_ctx, getState());
  enterRule(_localctx, 190, FeatParser::RuleHeadFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1066);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || _la == FeatParser::FONT_REVISION) {
      setState(1063);
      headStatement();
      setState(1068);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1069);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- HheaFileContext ------------------------------------------------------------------

FeatParser::HheaFileContext::HheaFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::HheaFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::HheaStatementContext *> FeatParser::HheaFileContext::hheaStatement() {
  return getRuleContexts<FeatParser::HheaStatementContext>();
}

FeatParser::HheaStatementContext* FeatParser::HheaFileContext::hheaStatement(size_t i) {
  return getRuleContext<FeatParser::HheaStatementContext>(i);
}


size_t FeatParser::HheaFileContext::getRuleIndex() const {
  return FeatParser::RuleHheaFile;
}


antlrcpp::Any FeatParser::HheaFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitHheaFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::HheaFileContext* FeatParser::hheaFile() {
  HheaFileContext *_localctx = _tracker.createInstance<HheaFileContext>(_ctx, getState());
  enterRule(_localctx, 192, FeatParser::RuleHheaFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1074);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 74) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 74)) & ((1ULL << (FeatParser::ASCENDER - 74))
      | (1ULL << (FeatParser::DESCENDER - 74))
      | (1ULL << (FeatParser::LINE_GAP - 74))
      | (1ULL << (FeatParser::CARET_OFFSET - 74)))) != 0)) {
      setState(1071);
      hheaStatement();
      setState(1076);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1077);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VheaFileContext ------------------------------------------------------------------

FeatParser::VheaFileContext::VheaFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::VheaFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::VheaStatementContext *> FeatParser::VheaFileContext::vheaStatement() {
  return getRuleContexts<FeatParser::VheaStatementContext>();
}

FeatParser::VheaStatementContext* FeatParser::VheaFileContext::vheaStatement(size_t i) {
  return getRuleContext<FeatParser::VheaStatementContext>(i);
}


size_t FeatParser::VheaFileContext::getRuleIndex() const {
  return FeatParser::RuleVheaFile;
}


antlrcpp::Any FeatParser::VheaFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitVheaFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::VheaFileContext* FeatParser::vheaFile() {
  VheaFileContext *_localctx = _tracker.createInstance<VheaFileContext>(_ctx, getState());
  enterRule(_localctx, 194, FeatParser::RuleVheaFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1082);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 109) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 109)) & ((1ULL << (FeatParser::VERT_TYPO_ASCENDER - 109))
      | (1ULL << (FeatParser::VERT_TYPO_DESCENDER - 109))
      | (1ULL << (FeatParser::VERT_TYPO_LINE_GAP - 109)))) != 0)) {
      setState(1079);
      vheaStatement();
      setState(1084);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1085);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GdefFileContext ------------------------------------------------------------------

FeatParser::GdefFileContext::GdefFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::GdefFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::GdefStatementContext *> FeatParser::GdefFileContext::gdefStatement() {
  return getRuleContexts<FeatParser::GdefStatementContext>();
}

FeatParser::GdefStatementContext* FeatParser::GdefFileContext::gdefStatement(size_t i) {
  return getRuleContext<FeatParser::GdefStatementContext>(i);
}


size_t FeatParser::GdefFileContext::getRuleIndex() const {
  return FeatParser::RuleGdefFile;
}


antlrcpp::Any FeatParser::GdefFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitGdefFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::GdefFileContext* FeatParser::gdefFile() {
  GdefFileContext *_localctx = _tracker.createInstance<GdefFileContext>(_ctx, getState());
  enterRule(_localctx, 196, FeatParser::RuleGdefFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1090);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 67) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 67)) & ((1ULL << (FeatParser::GLYPH_CLASS_DEF - 67))
      | (1ULL << (FeatParser::ATTACH - 67))
      | (1ULL << (FeatParser::LIG_CARET_BY_POS - 67))
      | (1ULL << (FeatParser::LIG_CARET_BY_IDX - 67)))) != 0)) {
      setState(1087);
      gdefStatement();
      setState(1092);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1093);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NameFileContext ------------------------------------------------------------------

FeatParser::NameFileContext::NameFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::NameFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::NameStatementContext *> FeatParser::NameFileContext::nameStatement() {
  return getRuleContexts<FeatParser::NameStatementContext>();
}

FeatParser::NameStatementContext* FeatParser::NameFileContext::nameStatement(size_t i) {
  return getRuleContext<FeatParser::NameStatementContext>(i);
}


size_t FeatParser::NameFileContext::getRuleIndex() const {
  return FeatParser::RuleNameFile;
}


antlrcpp::Any FeatParser::NameFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitNameFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::NameFileContext* FeatParser::nameFile() {
  NameFileContext *_localctx = _tracker.createInstance<NameFileContext>(_ctx, getState());
  enterRule(_localctx, 198, FeatParser::RuleNameFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1098);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || _la == FeatParser::NAMEID) {
      setState(1095);
      nameStatement();
      setState(1100);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1101);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VmtxFileContext ------------------------------------------------------------------

FeatParser::VmtxFileContext::VmtxFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::VmtxFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::VmtxStatementContext *> FeatParser::VmtxFileContext::vmtxStatement() {
  return getRuleContexts<FeatParser::VmtxStatementContext>();
}

FeatParser::VmtxStatementContext* FeatParser::VmtxFileContext::vmtxStatement(size_t i) {
  return getRuleContext<FeatParser::VmtxStatementContext>(i);
}


size_t FeatParser::VmtxFileContext::getRuleIndex() const {
  return FeatParser::RuleVmtxFile;
}


antlrcpp::Any FeatParser::VmtxFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitVmtxFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::VmtxFileContext* FeatParser::vmtxFile() {
  VmtxFileContext *_localctx = _tracker.createInstance<VmtxFileContext>(_ctx, getState());
  enterRule(_localctx, 200, FeatParser::RuleVmtxFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1106);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || _la == FeatParser::VERT_ORIGIN_Y

    || _la == FeatParser::VERT_ADVANCE_Y) {
      setState(1103);
      vmtxStatement();
      setState(1108);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1109);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- Os_2FileContext ------------------------------------------------------------------

FeatParser::Os_2FileContext::Os_2FileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::Os_2FileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::Os_2StatementContext *> FeatParser::Os_2FileContext::os_2Statement() {
  return getRuleContexts<FeatParser::Os_2StatementContext>();
}

FeatParser::Os_2StatementContext* FeatParser::Os_2FileContext::os_2Statement(size_t i) {
  return getRuleContext<FeatParser::Os_2StatementContext>(i);
}


size_t FeatParser::Os_2FileContext::getRuleIndex() const {
  return FeatParser::RuleOs_2File;
}


antlrcpp::Any FeatParser::Os_2FileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitOs_2File(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::Os_2FileContext* FeatParser::os_2File() {
  Os_2FileContext *_localctx = _tracker.createInstance<Os_2FileContext>(_ctx, getState());
  enterRule(_localctx, 202, FeatParser::RuleOs_2File);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1114);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 81) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 81)) & ((1ULL << (FeatParser::FS_TYPE - 81))
      | (1ULL << (FeatParser::FS_TYPE_v - 81))
      | (1ULL << (FeatParser::OS2_LOWER_OP_SIZE - 81))
      | (1ULL << (FeatParser::OS2_UPPER_OP_SIZE - 81))
      | (1ULL << (FeatParser::PANOSE - 81))
      | (1ULL << (FeatParser::TYPO_ASCENDER - 81))
      | (1ULL << (FeatParser::TYPO_DESCENDER - 81))
      | (1ULL << (FeatParser::TYPO_LINE_GAP - 81))
      | (1ULL << (FeatParser::WIN_ASCENT - 81))
      | (1ULL << (FeatParser::WIN_DESCENT - 81))
      | (1ULL << (FeatParser::X_HEIGHT - 81))
      | (1ULL << (FeatParser::CAP_HEIGHT - 81))
      | (1ULL << (FeatParser::WEIGHT_CLASS - 81))
      | (1ULL << (FeatParser::WIDTH_CLASS - 81))
      | (1ULL << (FeatParser::VENDOR - 81))
      | (1ULL << (FeatParser::UNICODE_RANGE - 81))
      | (1ULL << (FeatParser::CODE_PAGE_RANGE - 81))
      | (1ULL << (FeatParser::FAMILY_CLASS - 81)))) != 0)) {
      setState(1111);
      os_2Statement();
      setState(1116);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1117);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatFileContext ------------------------------------------------------------------

FeatParser::StatFileContext::StatFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::StatFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::StatStatementContext *> FeatParser::StatFileContext::statStatement() {
  return getRuleContexts<FeatParser::StatStatementContext>();
}

FeatParser::StatStatementContext* FeatParser::StatFileContext::statStatement(size_t i) {
  return getRuleContext<FeatParser::StatStatementContext>(i);
}


size_t FeatParser::StatFileContext::getRuleIndex() const {
  return FeatParser::RuleStatFile;
}


antlrcpp::Any FeatParser::StatFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitStatFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::StatFileContext* FeatParser::statFile() {
  StatFileContext *_localctx = _tracker.createInstance<StatFileContext>(_ctx, getState());
  enterRule(_localctx, 204, FeatParser::RuleStatFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1122);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 100) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 100)) & ((1ULL << (FeatParser::ELIDED_FALLBACK_NAME - 100))
      | (1ULL << (FeatParser::ELIDED_FALLBACK_NAME_ID - 100))
      | (1ULL << (FeatParser::DESIGN_AXIS - 100))
      | (1ULL << (FeatParser::AXIS_VALUE - 100)))) != 0)) {
      setState(1119);
      statStatement();
      setState(1124);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1125);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AxisValueFileContext ------------------------------------------------------------------

FeatParser::AxisValueFileContext::AxisValueFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AxisValueFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::AxisValueStatementContext *> FeatParser::AxisValueFileContext::axisValueStatement() {
  return getRuleContexts<FeatParser::AxisValueStatementContext>();
}

FeatParser::AxisValueStatementContext* FeatParser::AxisValueFileContext::axisValueStatement(size_t i) {
  return getRuleContext<FeatParser::AxisValueStatementContext>(i);
}


size_t FeatParser::AxisValueFileContext::getRuleIndex() const {
  return FeatParser::RuleAxisValueFile;
}


antlrcpp::Any FeatParser::AxisValueFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAxisValueFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AxisValueFileContext* FeatParser::axisValueFile() {
  AxisValueFileContext *_localctx = _tracker.createInstance<AxisValueFileContext>(_ctx, getState());
  enterRule(_localctx, 206, FeatParser::RuleAxisValueFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1130);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || ((((_la - 78) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 78)) & ((1ULL << (FeatParser::NAME - 78))
      | (1ULL << (FeatParser::FLAG - 78))
      | (1ULL << (FeatParser::LOCATION - 78)))) != 0)) {
      setState(1127);
      axisValueStatement();
      setState(1132);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1133);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NameEntryFileContext ------------------------------------------------------------------

FeatParser::NameEntryFileContext::NameEntryFileContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::NameEntryFileContext::EOF() {
  return getToken(FeatParser::EOF, 0);
}

std::vector<FeatParser::NameEntryStatementContext *> FeatParser::NameEntryFileContext::nameEntryStatement() {
  return getRuleContexts<FeatParser::NameEntryStatementContext>();
}

FeatParser::NameEntryStatementContext* FeatParser::NameEntryFileContext::nameEntryStatement(size_t i) {
  return getRuleContext<FeatParser::NameEntryStatementContext>(i);
}


size_t FeatParser::NameEntryFileContext::getRuleIndex() const {
  return FeatParser::RuleNameEntryFile;
}


antlrcpp::Any FeatParser::NameEntryFileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitNameEntryFile(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::NameEntryFileContext* FeatParser::nameEntryFile() {
  NameEntryFileContext *_localctx = _tracker.createInstance<NameEntryFileContext>(_ctx, getState());
  enterRule(_localctx, 208, FeatParser::RuleNameEntryFile);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1138);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == FeatParser::INCLUDE || _la == FeatParser::NAME) {
      setState(1135);
      nameEntryStatement();
      setState(1140);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(1141);
    match(FeatParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- SubtokContext ------------------------------------------------------------------

FeatParser::SubtokContext::SubtokContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::SubtokContext::SUBSTITUTE() {
  return getToken(FeatParser::SUBSTITUTE, 0);
}

tree::TerminalNode* FeatParser::SubtokContext::SUBSTITUTE_v() {
  return getToken(FeatParser::SUBSTITUTE_v, 0);
}


size_t FeatParser::SubtokContext::getRuleIndex() const {
  return FeatParser::RuleSubtok;
}


antlrcpp::Any FeatParser::SubtokContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitSubtok(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::SubtokContext* FeatParser::subtok() {
  SubtokContext *_localctx = _tracker.createInstance<SubtokContext>(_ctx, getState());
  enterRule(_localctx, 210, FeatParser::RuleSubtok);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1143);
    _la = _input->LA(1);
    if (!(_la == FeatParser::SUBSTITUTE

    || _la == FeatParser::SUBSTITUTE_v)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- RevtokContext ------------------------------------------------------------------

FeatParser::RevtokContext::RevtokContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::RevtokContext::REVERSE() {
  return getToken(FeatParser::REVERSE, 0);
}

tree::TerminalNode* FeatParser::RevtokContext::REVERSE_v() {
  return getToken(FeatParser::REVERSE_v, 0);
}


size_t FeatParser::RevtokContext::getRuleIndex() const {
  return FeatParser::RuleRevtok;
}


antlrcpp::Any FeatParser::RevtokContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitRevtok(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::RevtokContext* FeatParser::revtok() {
  RevtokContext *_localctx = _tracker.createInstance<RevtokContext>(_ctx, getState());
  enterRule(_localctx, 212, FeatParser::RuleRevtok);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1145);
    _la = _input->LA(1);
    if (!(_la == FeatParser::REVERSE

    || _la == FeatParser::REVERSE_v)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AnontokContext ------------------------------------------------------------------

FeatParser::AnontokContext::AnontokContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::AnontokContext::ANON() {
  return getToken(FeatParser::ANON, 0);
}

tree::TerminalNode* FeatParser::AnontokContext::ANON_v() {
  return getToken(FeatParser::ANON_v, 0);
}


size_t FeatParser::AnontokContext::getRuleIndex() const {
  return FeatParser::RuleAnontok;
}


antlrcpp::Any FeatParser::AnontokContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitAnontok(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::AnontokContext* FeatParser::anontok() {
  AnontokContext *_localctx = _tracker.createInstance<AnontokContext>(_ctx, getState());
  enterRule(_localctx, 214, FeatParser::RuleAnontok);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1147);
    _la = _input->LA(1);
    if (!(_la == FeatParser::ANON

    || _la == FeatParser::ANON_v)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EnumtokContext ------------------------------------------------------------------

FeatParser::EnumtokContext::EnumtokContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::EnumtokContext::ENUMERATE() {
  return getToken(FeatParser::ENUMERATE, 0);
}

tree::TerminalNode* FeatParser::EnumtokContext::ENUMERATE_v() {
  return getToken(FeatParser::ENUMERATE_v, 0);
}


size_t FeatParser::EnumtokContext::getRuleIndex() const {
  return FeatParser::RuleEnumtok;
}


antlrcpp::Any FeatParser::EnumtokContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitEnumtok(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::EnumtokContext* FeatParser::enumtok() {
  EnumtokContext *_localctx = _tracker.createInstance<EnumtokContext>(_ctx, getState());
  enterRule(_localctx, 216, FeatParser::RuleEnumtok);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1149);
    _la = _input->LA(1);
    if (!(_la == FeatParser::ENUMERATE

    || _la == FeatParser::ENUMERATE_v)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PostokContext ------------------------------------------------------------------

FeatParser::PostokContext::PostokContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::PostokContext::POSITION() {
  return getToken(FeatParser::POSITION, 0);
}

tree::TerminalNode* FeatParser::PostokContext::POSITION_v() {
  return getToken(FeatParser::POSITION_v, 0);
}


size_t FeatParser::PostokContext::getRuleIndex() const {
  return FeatParser::RulePostok;
}


antlrcpp::Any FeatParser::PostokContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitPostok(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::PostokContext* FeatParser::postok() {
  PostokContext *_localctx = _tracker.createInstance<PostokContext>(_ctx, getState());
  enterRule(_localctx, 218, FeatParser::RulePostok);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1151);
    _la = _input->LA(1);
    if (!(_la == FeatParser::POSITION

    || _la == FeatParser::POSITION_v)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MarkligtokContext ------------------------------------------------------------------

FeatParser::MarkligtokContext::MarkligtokContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* FeatParser::MarkligtokContext::MARKLIG() {
  return getToken(FeatParser::MARKLIG, 0);
}

tree::TerminalNode* FeatParser::MarkligtokContext::MARKLIG_v() {
  return getToken(FeatParser::MARKLIG_v, 0);
}


size_t FeatParser::MarkligtokContext::getRuleIndex() const {
  return FeatParser::RuleMarkligtok;
}


antlrcpp::Any FeatParser::MarkligtokContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<FeatParserVisitor*>(visitor))
    return parserVisitor->visitMarkligtok(this);
  else
    return visitor->visitChildren(this);
}

FeatParser::MarkligtokContext* FeatParser::markligtok() {
  MarkligtokContext *_localctx = _tracker.createInstance<MarkligtokContext>(_ctx, getState());
  enterRule(_localctx, 220, FeatParser::RuleMarkligtok);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(1153);
    _la = _input->LA(1);
    if (!(_la == FeatParser::MARKLIG

    || _la == FeatParser::MARKLIG_v)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

// Static vars and initialization.
std::vector<dfa::DFA> FeatParser::_decisionToDFA;
atn::PredictionContextCache FeatParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN FeatParser::_atn;
std::vector<uint16_t> FeatParser::_serializedATN;

std::vector<std::string> FeatParser::_ruleNames = {
  "file", "topLevelStatement", "include", "glyphClassAssign", "langsysAssign", 
  "mark_statement", "anchorDef", "valueRecordDef", "featureBlock", "tableBlock", 
  "anonBlock", "lookupBlockTopLevel", "featureStatement", "lookupBlockOrUse", 
  "cvParameterBlock", "cvParameterStatement", "cvParameter", "statement", 
  "featureUse", "scriptAssign", "langAssign", "lookupflagAssign", "lookupflagElement", 
  "ignoreSubOrPos", "substitute", "position", "valuePattern", "valueRecord", 
  "valueLiteral", "cursiveElement", "baseToMarkElement", "ligatureMarkElement", 
  "parameters", "sizemenuname", "featureNames", "subtable", "table_BASE", 
  "baseStatement", "axisTags", "axisScripts", "baseScript", "table_GDEF", 
  "gdefStatement", "gdefGlyphClass", "gdefAttach", "gdefLigCaretPos", "gdefLigCaretIndex", 
  "table_head", "headStatement", "head", "table_hhea", "hheaStatement", 
  "hhea", "table_vhea", "vheaStatement", "vhea", "table_name", "nameStatement", 
  "nameID", "table_OS_2", "os_2Statement", "os_2", "table_STAT", "statStatement", 
  "designAxis", "axisValue", "axisValueStatement", "axisValueLocation", 
  "axisValueFlags", "elidedFallbackName", "nameEntryStatement", "elidedFallbackNameID", 
  "nameEntry", "table_vmtx", "vmtxStatement", "vmtx", "anchor", "lookupPattern", 
  "lookupPatternElement", "pattern", "patternElement", "glyphClassOptional", 
  "glyphClass", "gcLiteral", "gcLiteralElement", "glyph", "glyphName", "label", 
  "tag", "fixedNum", "genNum", "featureFile", "statementFile", "cvStatementFile", 
  "baseFile", "headFile", "hheaFile", "vheaFile", "gdefFile", "nameFile", 
  "vmtxFile", "os_2File", "statFile", "axisValueFile", "nameEntryFile", 
  "subtok", "revtok", "anontok", "enumtok", "postok", "markligtok"
};

std::vector<std::string> FeatParser::_literalNames = {
  "", "'anon'", "'anonymous'", "", "", "'include'", "'feature'", "'table'", 
  "'script'", "'language'", "'languagesystem'", "'subtable'", "'lookup'", 
  "'lookupflag'", "'.notdef'", "'RightToLeft'", "'IgnoreBaseGlyphs'", "'IgnoreLigatures'", 
  "'IgnoreMarks'", "'UseMarkFilteringSet'", "'MarkAttachmentType'", "'excludeDFLT'", 
  "'includeDFLT'", "'exclude_dflt'", "'include_dflt'", "'useExtension'", 
  "'<'", "'>'", "'enumerate'", "'enum'", "'except'", "'ignore'", "'substitute'", 
  "'sub'", "'reversesub'", "'rsub'", "'by'", "'from'", "'position'", "'pos'", 
  "'parameters'", "'featureNames'", "'cvParameters'", "'FeatUILabelNameID'", 
  "'FeatUITooltipTextNameID'", "'SampleTextNameID'", "'ParamUILabelNameID'", 
  "'Character'", "'sizemenuname'", "'contourpoint'", "'anchor'", "'anchorDef'", 
  "'valueRecordDef'", "'mark'", "'markClass'", "'cursive'", "'base'", "'ligature'", 
  "'lig'", "'ligComponent'", "'NULL'", "'BASE'", "'HorizAxis.BaseTagList'", 
  "'VertAxis.BaseTagList'", "'HorizAxis.BaseScriptList'", "'VertAxis.BaseScriptList'", 
  "'GDEF'", "'GlyphClassDef'", "'Attach'", "'LigatureCaretByPos'", "'LigatureCaretByIndex'", 
  "'head'", "'FontRevision'", "'hhea'", "'Ascender'", "'Descender'", "'LineGap'", 
  "'CaretOffset'", "'name'", "'nameid'", "'OS/2'", "'FSType'", "'fsType'", 
  "'LowerOpSize'", "'UpperOpSize'", "'Panose'", "'TypoAscender'", "'TypoDescender'", 
  "'TypoLineGap'", "'winAscent'", "'winDescent'", "'XHeight'", "'CapHeight'", 
  "'WeightClass'", "'WidthClass'", "'Vendor'", "'UnicodeRange'", "'CodePageRange'", 
  "'FamilyClass'", "'STAT'", "'ElidedFallbackName'", "'ElidedFallbackNameID'", 
  "'DesignAxis'", "'AxisValue'", "'flag'", "'location'", "'ElidableAxisValueName'", 
  "'OlderSiblingFontAttribute'", "'vhea'", "'VertTypoAscender'", "'VertTypoDescender'", 
  "'VertTypoLineGap'", "'vmtx'", "'VertOriginY'", "'VertAdvanceY'", "", 
  "'}'", "'['", "']'", "'-'", "';'", "'='", "'''", "','", "", "", "", "", 
  "", "", "", "", "", "", "", "", "", "", "", "", "", "'('", "", "')'"
};

std::vector<std::string> FeatParser::_symbolicNames = {
  "", "ANON", "ANON_v", "COMMENT", "WHITESPACE", "INCLUDE", "FEATURE", "TABLE", 
  "SCRIPT", "LANGUAGE", "LANGSYS", "SUBTABLE", "LOOKUP", "LOOKUPFLAG", "NOTDEF", 
  "RIGHT_TO_LEFT", "IGNORE_BASE_GLYPHS", "IGNORE_LIGATURES", "IGNORE_MARKS", 
  "USE_MARK_FILTERING_SET", "MARK_ATTACHMENT_TYPE", "EXCLUDE_DFLT", "INCLUDE_DFLT", 
  "EXCLUDE_dflt", "INCLUDE_dflt", "USE_EXTENSION", "BEGINVALUE", "ENDVALUE", 
  "ENUMERATE", "ENUMERATE_v", "EXCEPT", "IGNORE", "SUBSTITUTE", "SUBSTITUTE_v", 
  "REVERSE", "REVERSE_v", "BY", "FROM", "POSITION", "POSITION_v", "PARAMETERS", 
  "FEATURE_NAMES", "CV_PARAMETERS", "CV_UI_LABEL", "CV_TOOLTIP", "CV_SAMPLE_TEXT", 
  "CV_PARAM_LABEL", "CV_CHARACTER", "SIZEMENUNAME", "CONTOURPOINT", "ANCHOR", 
  "ANCHOR_DEF", "VALUE_RECORD_DEF", "MARK", "MARK_CLASS", "CURSIVE", "MARKBASE", 
  "MARKLIG", "MARKLIG_v", "LIG_COMPONENT", "KNULL", "BASE", "HA_BTL", "VA_BTL", 
  "HA_BSL", "VA_BSL", "GDEF", "GLYPH_CLASS_DEF", "ATTACH", "LIG_CARET_BY_POS", 
  "LIG_CARET_BY_IDX", "HEAD", "FONT_REVISION", "HHEA", "ASCENDER", "DESCENDER", 
  "LINE_GAP", "CARET_OFFSET", "NAME", "NAMEID", "OS_2", "FS_TYPE", "FS_TYPE_v", 
  "OS2_LOWER_OP_SIZE", "OS2_UPPER_OP_SIZE", "PANOSE", "TYPO_ASCENDER", "TYPO_DESCENDER", 
  "TYPO_LINE_GAP", "WIN_ASCENT", "WIN_DESCENT", "X_HEIGHT", "CAP_HEIGHT", 
  "WEIGHT_CLASS", "WIDTH_CLASS", "VENDOR", "UNICODE_RANGE", "CODE_PAGE_RANGE", 
  "FAMILY_CLASS", "STAT", "ELIDED_FALLBACK_NAME", "ELIDED_FALLBACK_NAME_ID", 
  "DESIGN_AXIS", "AXIS_VALUE", "FLAG", "LOCATION", "AXIS_EAVN", "AXIS_OSFA", 
  "VHEA", "VERT_TYPO_ASCENDER", "VERT_TYPO_DESCENDER", "VERT_TYPO_LINE_GAP", 
  "VMTX", "VERT_ORIGIN_Y", "VERT_ADVANCE_Y", "LCBRACE", "RCBRACE", "LBRACKET", 
  "RBRACKET", "HYPHEN", "SEMI", "EQUALS", "MARKER", "COMMA", "QUOTE", "GCLASS", 
  "CID", "ESCGNAME", "NAMELABEL", "EXTNAME", "POINTNUM", "NUMEXT", "NUMOCT", 
  "NUM", "CATCHTAG", "A_WHITESPACE", "A_LABEL", "A_LBRACE", "A_CLOSE", "A_LINE", 
  "I_WHITESPACE", "I_RPAREN", "IFILE", "I_LPAREN", "STRVAL", "EQUOTE"
};

dfa::Vocabulary FeatParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> FeatParser::_tokenNames;

FeatParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  static const uint16_t serializedATNSegment0[] = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
       0x3, 0x93, 0x486, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 
       0x9, 0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 
       0x7, 0x4, 0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 
       0x4, 0xb, 0x9, 0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 
       0xe, 0x9, 0xe, 0x4, 0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 
       0x9, 0x11, 0x4, 0x12, 0x9, 0x12, 0x4, 0x13, 0x9, 0x13, 0x4, 0x14, 
       0x9, 0x14, 0x4, 0x15, 0x9, 0x15, 0x4, 0x16, 0x9, 0x16, 0x4, 0x17, 
       0x9, 0x17, 0x4, 0x18, 0x9, 0x18, 0x4, 0x19, 0x9, 0x19, 0x4, 0x1a, 
       0x9, 0x1a, 0x4, 0x1b, 0x9, 0x1b, 0x4, 0x1c, 0x9, 0x1c, 0x4, 0x1d, 
       0x9, 0x1d, 0x4, 0x1e, 0x9, 0x1e, 0x4, 0x1f, 0x9, 0x1f, 0x4, 0x20, 
       0x9, 0x20, 0x4, 0x21, 0x9, 0x21, 0x4, 0x22, 0x9, 0x22, 0x4, 0x23, 
       0x9, 0x23, 0x4, 0x24, 0x9, 0x24, 0x4, 0x25, 0x9, 0x25, 0x4, 0x26, 
       0x9, 0x26, 0x4, 0x27, 0x9, 0x27, 0x4, 0x28, 0x9, 0x28, 0x4, 0x29, 
       0x9, 0x29, 0x4, 0x2a, 0x9, 0x2a, 0x4, 0x2b, 0x9, 0x2b, 0x4, 0x2c, 
       0x9, 0x2c, 0x4, 0x2d, 0x9, 0x2d, 0x4, 0x2e, 0x9, 0x2e, 0x4, 0x2f, 
       0x9, 0x2f, 0x4, 0x30, 0x9, 0x30, 0x4, 0x31, 0x9, 0x31, 0x4, 0x32, 
       0x9, 0x32, 0x4, 0x33, 0x9, 0x33, 0x4, 0x34, 0x9, 0x34, 0x4, 0x35, 
       0x9, 0x35, 0x4, 0x36, 0x9, 0x36, 0x4, 0x37, 0x9, 0x37, 0x4, 0x38, 
       0x9, 0x38, 0x4, 0x39, 0x9, 0x39, 0x4, 0x3a, 0x9, 0x3a, 0x4, 0x3b, 
       0x9, 0x3b, 0x4, 0x3c, 0x9, 0x3c, 0x4, 0x3d, 0x9, 0x3d, 0x4, 0x3e, 
       0x9, 0x3e, 0x4, 0x3f, 0x9, 0x3f, 0x4, 0x40, 0x9, 0x40, 0x4, 0x41, 
       0x9, 0x41, 0x4, 0x42, 0x9, 0x42, 0x4, 0x43, 0x9, 0x43, 0x4, 0x44, 
       0x9, 0x44, 0x4, 0x45, 0x9, 0x45, 0x4, 0x46, 0x9, 0x46, 0x4, 0x47, 
       0x9, 0x47, 0x4, 0x48, 0x9, 0x48, 0x4, 0x49, 0x9, 0x49, 0x4, 0x4a, 
       0x9, 0x4a, 0x4, 0x4b, 0x9, 0x4b, 0x4, 0x4c, 0x9, 0x4c, 0x4, 0x4d, 
       0x9, 0x4d, 0x4, 0x4e, 0x9, 0x4e, 0x4, 0x4f, 0x9, 0x4f, 0x4, 0x50, 
       0x9, 0x50, 0x4, 0x51, 0x9, 0x51, 0x4, 0x52, 0x9, 0x52, 0x4, 0x53, 
       0x9, 0x53, 0x4, 0x54, 0x9, 0x54, 0x4, 0x55, 0x9, 0x55, 0x4, 0x56, 
       0x9, 0x56, 0x4, 0x57, 0x9, 0x57, 0x4, 0x58, 0x9, 0x58, 0x4, 0x59, 
       0x9, 0x59, 0x4, 0x5a, 0x9, 0x5a, 0x4, 0x5b, 0x9, 0x5b, 0x4, 0x5c, 
       0x9, 0x5c, 0x4, 0x5d, 0x9, 0x5d, 0x4, 0x5e, 0x9, 0x5e, 0x4, 0x5f, 
       0x9, 0x5f, 0x4, 0x60, 0x9, 0x60, 0x4, 0x61, 0x9, 0x61, 0x4, 0x62, 
       0x9, 0x62, 0x4, 0x63, 0x9, 0x63, 0x4, 0x64, 0x9, 0x64, 0x4, 0x65, 
       0x9, 0x65, 0x4, 0x66, 0x9, 0x66, 0x4, 0x67, 0x9, 0x67, 0x4, 0x68, 
       0x9, 0x68, 0x4, 0x69, 0x9, 0x69, 0x4, 0x6a, 0x9, 0x6a, 0x4, 0x6b, 
       0x9, 0x6b, 0x4, 0x6c, 0x9, 0x6c, 0x4, 0x6d, 0x9, 0x6d, 0x4, 0x6e, 
       0x9, 0x6e, 0x4, 0x6f, 0x9, 0x6f, 0x4, 0x70, 0x9, 0x70, 0x3, 0x2, 
       0x3, 0x2, 0x3, 0x2, 0x3, 0x2, 0x3, 0x2, 0x7, 0x2, 0xe6, 0xa, 0x2, 
       0xc, 0x2, 0xe, 0x2, 0xe9, 0xb, 0x2, 0x3, 0x2, 0x3, 0x2, 0x3, 0x3, 
       0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x5, 0x3, 0xf3, 
       0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x4, 0x3, 0x4, 0x3, 
       0x4, 0x3, 0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x6, 
       0x3, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x5, 
       0x7, 0x107, 0xa, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x8, 0x3, 
       0x8, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x5, 0x8, 0x111, 0xa, 0x8, 0x3, 
       0x8, 0x3, 0x8, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0x9, 0x3, 0xa, 
       0x3, 0xa, 0x3, 0xa, 0x5, 0xa, 0x11c, 0xa, 0xa, 0x3, 0xa, 0x3, 0xa, 
       0x6, 0xa, 0x120, 0xa, 0xa, 0xd, 0xa, 0xe, 0xa, 0x121, 0x3, 0xa, 0x3, 
       0xa, 0x3, 0xa, 0x3, 0xa, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 
       0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x5, 
       0xb, 0x132, 0xa, 0xb, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x3, 0xc, 0x7, 
       0xc, 0x138, 0xa, 0xc, 0xc, 0xc, 0xe, 0xc, 0x13b, 0xb, 0xc, 0x3, 0xc, 
       0x3, 0xc, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x5, 0xd, 0x142, 0xa, 0xd, 
       0x3, 0xd, 0x3, 0xd, 0x6, 0xd, 0x146, 0xa, 0xd, 0xd, 0xd, 0xe, 0xd, 
       0x147, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x3, 0xe, 0x3, 0xe, 
       0x3, 0xe, 0x5, 0xe, 0x151, 0xa, 0xe, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 
       0x5, 0xf, 0x156, 0xa, 0xf, 0x3, 0xf, 0x3, 0xf, 0x6, 0xf, 0x15a, 0xa, 
       0xf, 0xd, 0xf, 0xe, 0xf, 0x15b, 0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x5, 
       0xf, 0x161, 0xa, 0xf, 0x3, 0xf, 0x3, 0xf, 0x3, 0x10, 0x3, 0x10, 0x3, 
       0x10, 0x7, 0x10, 0x168, 0xa, 0x10, 0xc, 0x10, 0xe, 0x10, 0x16b, 0xb, 
       0x10, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x3, 0x11, 0x3, 0x11, 0x5, 
       0x11, 0x172, 0xa, 0x11, 0x3, 0x11, 0x3, 0x11, 0x3, 0x12, 0x3, 0x12, 
       0x3, 0x12, 0x6, 0x12, 0x179, 0xa, 0x12, 0xd, 0x12, 0xe, 0x12, 0x17a, 
       0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x5, 0x12, 0x181, 0xa, 
       0x12, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 
       0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 
       0x13, 0x3, 0x13, 0x3, 0x13, 0x5, 0x13, 0x191, 0xa, 0x13, 0x3, 0x13, 
       0x3, 0x13, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x15, 0x3, 0x15, 
       0x3, 0x15, 0x3, 0x16, 0x3, 0x16, 0x3, 0x16, 0x5, 0x16, 0x19e, 0xa, 
       0x16, 0x3, 0x17, 0x3, 0x17, 0x3, 0x17, 0x6, 0x17, 0x1a3, 0xa, 0x17, 
       0xd, 0x17, 0xe, 0x17, 0x1a4, 0x5, 0x17, 0x1a7, 0xa, 0x17, 0x3, 0x18, 
       0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 0x3, 0x18, 
       0x3, 0x18, 0x5, 0x18, 0x1b1, 0xa, 0x18, 0x3, 0x19, 0x3, 0x19, 0x3, 
       0x19, 0x3, 0x19, 0x5, 0x19, 0x1b7, 0xa, 0x19, 0x3, 0x19, 0x3, 0x19, 
       0x3, 0x19, 0x7, 0x19, 0x1bc, 0xa, 0x19, 0xc, 0x19, 0xe, 0x19, 0x1bf, 
       0xb, 0x19, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x7, 0x1a, 
       0x1c5, 0xa, 0x1a, 0xc, 0x1a, 0xe, 0x1a, 0x1c8, 0xb, 0x1a, 0x5, 0x1a, 
       0x1ca, 0xa, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 
       0x1a, 0x5, 0x1a, 0x1d1, 0xa, 0x1a, 0x5, 0x1a, 0x1d3, 0xa, 0x1a, 0x3, 
       0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x3, 0x1a, 0x5, 0x1a, 0x1da, 
       0xa, 0x1a, 0x5, 0x1a, 0x1dc, 0xa, 0x1a, 0x5, 0x1a, 0x1de, 0xa, 0x1a, 
       0x3, 0x1b, 0x5, 0x1b, 0x1e1, 0xa, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 0x5, 
       0x1b, 0x1e5, 0xa, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 0x7, 0x1b, 0x1e9, 0xa, 
       0x1b, 0xc, 0x1b, 0xe, 0x1b, 0x1ec, 0xb, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 
       0x6, 0x1b, 0x1f0, 0xa, 0x1b, 0xd, 0x1b, 0xe, 0x1b, 0x1f1, 0x3, 0x1b, 
       0x7, 0x1b, 0x1f5, 0xa, 0x1b, 0xc, 0x1b, 0xe, 0x1b, 0x1f8, 0xb, 0x1b, 
       0x3, 0x1b, 0x3, 0x1b, 0x3, 0x1b, 0x5, 0x1b, 0x1fd, 0xa, 0x1b, 0x3, 
       0x1b, 0x3, 0x1b, 0x3, 0x1b, 0x6, 0x1b, 0x202, 0xa, 0x1b, 0xd, 0x1b, 
       0xe, 0x1b, 0x203, 0x3, 0x1b, 0x5, 0x1b, 0x207, 0xa, 0x1b, 0x3, 0x1b, 
       0x3, 0x1b, 0x3, 0x1b, 0x6, 0x1b, 0x20c, 0xa, 0x1b, 0xd, 0x1b, 0xe, 
       0x1b, 0x20d, 0x3, 0x1b, 0x5, 0x1b, 0x211, 0xa, 0x1b, 0x3, 0x1b, 0x3, 
       0x1b, 0x3, 0x1b, 0x6, 0x1b, 0x216, 0xa, 0x1b, 0xd, 0x1b, 0xe, 0x1b, 
       0x217, 0x3, 0x1b, 0x5, 0x1b, 0x21b, 0xa, 0x1b, 0x5, 0x1b, 0x21d, 
       0xa, 0x1b, 0x3, 0x1c, 0x3, 0x1c, 0x5, 0x1c, 0x221, 0xa, 0x1c, 0x3, 
       0x1d, 0x3, 0x1d, 0x3, 0x1d, 0x3, 0x1d, 0x3, 0x1d, 0x5, 0x1d, 0x228, 
       0xa, 0x1d, 0x3, 0x1e, 0x3, 0x1e, 0x3, 0x1e, 0x3, 0x1e, 0x3, 0x1e, 
       0x3, 0x1e, 0x3, 0x1e, 0x5, 0x1e, 0x231, 0xa, 0x1e, 0x3, 0x1f, 0x3, 
       0x1f, 0x3, 0x1f, 0x3, 0x1f, 0x3, 0x20, 0x3, 0x20, 0x3, 0x20, 0x3, 
       0x20, 0x5, 0x20, 0x23b, 0xa, 0x20, 0x3, 0x21, 0x3, 0x21, 0x3, 0x21, 
       0x5, 0x21, 0x240, 0xa, 0x21, 0x3, 0x21, 0x5, 0x21, 0x243, 0xa, 0x21, 
       0x3, 0x21, 0x5, 0x21, 0x246, 0xa, 0x21, 0x3, 0x22, 0x3, 0x22, 0x6, 
       0x22, 0x24a, 0xa, 0x22, 0xd, 0x22, 0xe, 0x22, 0x24b, 0x3, 0x23, 0x3, 
       0x23, 0x3, 0x23, 0x3, 0x23, 0x3, 0x23, 0x5, 0x23, 0x253, 0xa, 0x23, 
       0x5, 0x23, 0x255, 0xa, 0x23, 0x3, 0x23, 0x3, 0x23, 0x3, 0x23, 0x3, 
       0x23, 0x3, 0x24, 0x3, 0x24, 0x3, 0x24, 0x6, 0x24, 0x25e, 0xa, 0x24, 
       0xd, 0x24, 0xe, 0x24, 0x25f, 0x3, 0x24, 0x3, 0x24, 0x3, 0x25, 0x3, 
       0x25, 0x3, 0x26, 0x3, 0x26, 0x3, 0x26, 0x6, 0x26, 0x269, 0xa, 0x26, 
       0xd, 0x26, 0xe, 0x26, 0x26a, 0x3, 0x26, 0x3, 0x26, 0x3, 0x26, 0x3, 
       0x26, 0x3, 0x27, 0x3, 0x27, 0x3, 0x27, 0x5, 0x27, 0x274, 0xa, 0x27, 
       0x3, 0x27, 0x3, 0x27, 0x3, 0x28, 0x3, 0x28, 0x6, 0x28, 0x27a, 0xa, 
       0x28, 0xd, 0x28, 0xe, 0x28, 0x27b, 0x3, 0x29, 0x3, 0x29, 0x3, 0x29, 
       0x3, 0x29, 0x7, 0x29, 0x282, 0xa, 0x29, 0xc, 0x29, 0xe, 0x29, 0x285, 
       0xb, 0x29, 0x3, 0x2a, 0x3, 0x2a, 0x3, 0x2a, 0x6, 0x2a, 0x28a, 0xa, 
       0x2a, 0xd, 0x2a, 0xe, 0x2a, 0x28b, 0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2b, 
       0x6, 0x2b, 0x291, 0xa, 0x2b, 0xd, 0x2b, 0xe, 0x2b, 0x292, 0x3, 0x2b, 
       0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2b, 0x3, 0x2c, 0x3, 0x2c, 0x3, 0x2c, 
       0x3, 0x2c, 0x3, 0x2c, 0x5, 0x2c, 0x29e, 0xa, 0x2c, 0x3, 0x2c, 0x3, 
       0x2c, 0x3, 0x2d, 0x3, 0x2d, 0x3, 0x2d, 0x3, 0x2d, 0x3, 0x2d, 0x3, 
       0x2d, 0x3, 0x2d, 0x3, 0x2d, 0x3, 0x2d, 0x3, 0x2e, 0x3, 0x2e, 0x3, 
       0x2e, 0x6, 0x2e, 0x2ae, 0xa, 0x2e, 0xd, 0x2e, 0xe, 0x2e, 0x2af, 0x3, 
       0x2f, 0x3, 0x2f, 0x3, 0x2f, 0x6, 0x2f, 0x2b5, 0xa, 0x2f, 0xd, 0x2f, 
       0xe, 0x2f, 0x2b6, 0x3, 0x30, 0x3, 0x30, 0x3, 0x30, 0x6, 0x30, 0x2bc, 
       0xa, 0x30, 0xd, 0x30, 0xe, 0x30, 0x2bd, 0x3, 0x31, 0x3, 0x31, 0x3, 
       0x31, 0x6, 0x31, 0x2c3, 0xa, 0x31, 0xd, 0x31, 0xe, 0x31, 0x2c4, 0x3, 
       0x31, 0x3, 0x31, 0x3, 0x31, 0x3, 0x31, 0x3, 0x32, 0x3, 0x32, 0x5, 
       0x32, 0x2cd, 0xa, 0x32, 0x3, 0x32, 0x3, 0x32, 0x3, 0x33, 0x3, 0x33, 
       0x3, 0x33, 0x3, 0x34, 0x3, 0x34, 0x3, 0x34, 0x7, 0x34, 0x2d7, 0xa, 
       0x34, 0xc, 0x34, 0xe, 0x34, 0x2da, 0xb, 0x34, 0x3, 0x34, 0x3, 0x34, 
       0x3, 0x34, 0x3, 0x34, 0x3, 0x35, 0x3, 0x35, 0x5, 0x35, 0x2e2, 0xa, 
       0x35, 0x3, 0x35, 0x3, 0x35, 0x3, 0x36, 0x3, 0x36, 0x3, 0x36, 0x3, 
       0x37, 0x3, 0x37, 0x3, 0x37, 0x7, 0x37, 0x2ec, 0xa, 0x37, 0xc, 0x37, 
       0xe, 0x37, 0x2ef, 0xb, 0x37, 0x3, 0x37, 0x3, 0x37, 0x3, 0x37, 0x3, 
       0x37, 0x3, 0x38, 0x3, 0x38, 0x5, 0x38, 0x2f7, 0xa, 0x38, 0x3, 0x38, 
       0x3, 0x38, 0x3, 0x39, 0x3, 0x39, 0x3, 0x39, 0x3, 0x3a, 0x3, 0x3a, 
       0x3, 0x3a, 0x6, 0x3a, 0x301, 0xa, 0x3a, 0xd, 0x3a, 0xe, 0x3a, 0x302, 
       0x3, 0x3a, 0x3, 0x3a, 0x3, 0x3a, 0x3, 0x3a, 0x3, 0x3b, 0x3, 0x3b, 
       0x5, 0x3b, 0x30b, 0xa, 0x3b, 0x3, 0x3b, 0x3, 0x3b, 0x3, 0x3c, 0x3, 
       0x3c, 0x3, 0x3c, 0x3, 0x3c, 0x3, 0x3c, 0x3, 0x3c, 0x5, 0x3c, 0x315, 
       0xa, 0x3c, 0x5, 0x3c, 0x317, 0xa, 0x3c, 0x3, 0x3c, 0x3, 0x3c, 0x3, 
       0x3c, 0x3, 0x3c, 0x3, 0x3d, 0x3, 0x3d, 0x3, 0x3d, 0x6, 0x3d, 0x320, 
       0xa, 0x3d, 0xd, 0x3d, 0xe, 0x3d, 0x321, 0x3, 0x3d, 0x3, 0x3d, 0x3, 
       0x3d, 0x3, 0x3d, 0x3, 0x3e, 0x3, 0x3e, 0x5, 0x3e, 0x32a, 0xa, 0x3e, 
       0x3, 0x3e, 0x3, 0x3e, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 
       0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 
       0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 
       0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 0x3, 0x3f, 
       0x3, 0x3f, 0x6, 0x3f, 0x345, 0xa, 0x3f, 0xd, 0x3f, 0xe, 0x3f, 0x346, 
       0x5, 0x3f, 0x349, 0xa, 0x3f, 0x3, 0x40, 0x3, 0x40, 0x3, 0x40, 0x6, 
       0x40, 0x34e, 0xa, 0x40, 0xd, 0x40, 0xe, 0x40, 0x34f, 0x3, 0x40, 0x3, 
       0x40, 0x3, 0x40, 0x3, 0x40, 0x3, 0x41, 0x3, 0x41, 0x3, 0x41, 0x3, 
       0x41, 0x3, 0x41, 0x5, 0x41, 0x35b, 0xa, 0x41, 0x3, 0x41, 0x3, 0x41, 
       0x3, 0x42, 0x3, 0x42, 0x3, 0x42, 0x3, 0x42, 0x3, 0x42, 0x6, 0x42, 
       0x364, 0xa, 0x42, 0xd, 0x42, 0xe, 0x42, 0x365, 0x3, 0x42, 0x3, 0x42, 
       0x3, 0x43, 0x3, 0x43, 0x3, 0x43, 0x6, 0x43, 0x36d, 0xa, 0x43, 0xd, 
       0x43, 0xe, 0x43, 0x36e, 0x3, 0x43, 0x3, 0x43, 0x3, 0x44, 0x3, 0x44, 
       0x3, 0x44, 0x3, 0x44, 0x5, 0x44, 0x377, 0xa, 0x44, 0x3, 0x44, 0x3, 
       0x44, 0x3, 0x45, 0x3, 0x45, 0x3, 0x45, 0x3, 0x45, 0x3, 0x45, 0x5, 
       0x45, 0x380, 0xa, 0x45, 0x5, 0x45, 0x382, 0xa, 0x45, 0x3, 0x46, 0x3, 
       0x46, 0x6, 0x46, 0x386, 0xa, 0x46, 0xd, 0x46, 0xe, 0x46, 0x387, 0x3, 
       0x47, 0x3, 0x47, 0x3, 0x47, 0x6, 0x47, 0x38d, 0xa, 0x47, 0xd, 0x47, 
       0xe, 0x47, 0x38e, 0x3, 0x47, 0x3, 0x47, 0x3, 0x48, 0x3, 0x48, 0x5, 
       0x48, 0x395, 0xa, 0x48, 0x3, 0x48, 0x3, 0x48, 0x3, 0x49, 0x3, 0x49, 
       0x3, 0x49, 0x3, 0x4a, 0x3, 0x4a, 0x3, 0x4a, 0x3, 0x4a, 0x3, 0x4a, 
       0x5, 0x4a, 0x3a1, 0xa, 0x4a, 0x5, 0x4a, 0x3a3, 0xa, 0x4a, 0x3, 0x4a, 
       0x3, 0x4a, 0x3, 0x4a, 0x3, 0x4a, 0x3, 0x4b, 0x3, 0x4b, 0x3, 0x4b, 
       0x6, 0x4b, 0x3ac, 0xa, 0x4b, 0xd, 0x4b, 0xe, 0x4b, 0x3ad, 0x3, 0x4b, 
       0x3, 0x4b, 0x3, 0x4b, 0x3, 0x4b, 0x3, 0x4c, 0x3, 0x4c, 0x5, 0x4c, 
       0x3b6, 0xa, 0x4c, 0x3, 0x4c, 0x3, 0x4c, 0x3, 0x4d, 0x3, 0x4d, 0x3, 
       0x4d, 0x3, 0x4d, 0x3, 0x4e, 0x3, 0x4e, 0x3, 0x4e, 0x3, 0x4e, 0x3, 
       0x4e, 0x3, 0x4e, 0x5, 0x4e, 0x3c4, 0xa, 0x4e, 0x3, 0x4e, 0x3, 0x4e, 
       0x5, 0x4e, 0x3c8, 0xa, 0x4e, 0x3, 0x4e, 0x3, 0x4e, 0x3, 0x4f, 0x6, 
       0x4f, 0x3cd, 0xa, 0x4f, 0xd, 0x4f, 0xe, 0x4f, 0x3ce, 0x3, 0x50, 0x3, 
       0x50, 0x3, 0x50, 0x7, 0x50, 0x3d4, 0xa, 0x50, 0xc, 0x50, 0xe, 0x50, 
       0x3d7, 0xb, 0x50, 0x3, 0x51, 0x6, 0x51, 0x3da, 0xa, 0x51, 0xd, 0x51, 
       0xe, 0x51, 0x3db, 0x3, 0x52, 0x3, 0x52, 0x5, 0x52, 0x3e0, 0xa, 0x52, 
       0x3, 0x52, 0x5, 0x52, 0x3e3, 0xa, 0x52, 0x3, 0x53, 0x5, 0x53, 0x3e6, 
       0xa, 0x53, 0x3, 0x54, 0x3, 0x54, 0x5, 0x54, 0x3ea, 0xa, 0x54, 0x3, 
       0x55, 0x3, 0x55, 0x6, 0x55, 0x3ee, 0xa, 0x55, 0xd, 0x55, 0xe, 0x55, 
       0x3ef, 0x3, 0x55, 0x3, 0x55, 0x3, 0x56, 0x3, 0x56, 0x3, 0x56, 0x5, 
       0x56, 0x3f7, 0xa, 0x56, 0x3, 0x56, 0x5, 0x56, 0x3fa, 0xa, 0x56, 0x3, 
       0x57, 0x3, 0x57, 0x5, 0x57, 0x3fe, 0xa, 0x57, 0x3, 0x58, 0x3, 0x58, 
       0x3, 0x59, 0x3, 0x59, 0x3, 0x5a, 0x3, 0x5a, 0x3, 0x5b, 0x3, 0x5b, 
       0x3, 0x5c, 0x3, 0x5c, 0x3, 0x5d, 0x7, 0x5d, 0x40b, 0xa, 0x5d, 0xc, 
       0x5d, 0xe, 0x5d, 0x40e, 0xb, 0x5d, 0x3, 0x5d, 0x3, 0x5d, 0x3, 0x5e, 
       0x7, 0x5e, 0x413, 0xa, 0x5e, 0xc, 0x5e, 0xe, 0x5e, 0x416, 0xb, 0x5e, 
       0x3, 0x5e, 0x3, 0x5e, 0x3, 0x5f, 0x7, 0x5f, 0x41b, 0xa, 0x5f, 0xc, 
       0x5f, 0xe, 0x5f, 0x41e, 0xb, 0x5f, 0x3, 0x5f, 0x3, 0x5f, 0x3, 0x60, 
       0x7, 0x60, 0x423, 0xa, 0x60, 0xc, 0x60, 0xe, 0x60, 0x426, 0xb, 0x60, 
       0x3, 0x60, 0x3, 0x60, 0x3, 0x61, 0x7, 0x61, 0x42b, 0xa, 0x61, 0xc, 
       0x61, 0xe, 0x61, 0x42e, 0xb, 0x61, 0x3, 0x61, 0x3, 0x61, 0x3, 0x62, 
       0x7, 0x62, 0x433, 0xa, 0x62, 0xc, 0x62, 0xe, 0x62, 0x436, 0xb, 0x62, 
       0x3, 0x62, 0x3, 0x62, 0x3, 0x63, 0x7, 0x63, 0x43b, 0xa, 0x63, 0xc, 
       0x63, 0xe, 0x63, 0x43e, 0xb, 0x63, 0x3, 0x63, 0x3, 0x63, 0x3, 0x64, 
       0x7, 0x64, 0x443, 0xa, 0x64, 0xc, 0x64, 0xe, 0x64, 0x446, 0xb, 0x64, 
       0x3, 0x64, 0x3, 0x64, 0x3, 0x65, 0x7, 0x65, 0x44b, 0xa, 0x65, 0xc, 
       0x65, 0xe, 0x65, 0x44e, 0xb, 0x65, 0x3, 0x65, 0x3, 0x65, 0x3, 0x66, 
       0x7, 0x66, 0x453, 0xa, 0x66, 0xc, 0x66, 0xe, 0x66, 0x456, 0xb, 0x66, 
       0x3, 0x66, 0x3, 0x66, 0x3, 0x67, 0x7, 0x67, 0x45b, 0xa, 0x67, 0xc, 
       0x67, 0xe, 0x67, 0x45e, 0xb, 0x67, 0x3, 0x67, 0x3, 0x67, 0x3, 0x68, 
       0x7, 0x68, 0x463, 0xa, 0x68, 0xc, 0x68, 0xe, 0x68, 0x466, 0xb, 0x68, 
       0x3, 0x68, 0x3, 0x68, 0x3, 0x69, 0x7, 0x69, 0x46b, 0xa, 0x69, 0xc, 
       0x69, 0xe, 0x69, 0x46e, 0xb, 0x69, 0x3, 0x69, 0x3, 0x69, 0x3, 0x6a, 
       0x7, 0x6a, 0x473, 0xa, 0x6a, 0xc, 0x6a, 0xe, 0x6a, 0x476, 0xb, 0x6a, 
       0x3, 0x6a, 0x3, 0x6a, 0x3, 0x6b, 0x3, 0x6b, 0x3, 0x6c, 0x3, 0x6c, 
       0x3, 0x6d, 0x3, 0x6d, 0x3, 0x6e, 0x3, 0x6e, 0x3, 0x6f, 0x3, 0x6f, 
       0x3, 0x70, 0x3, 0x70, 0x3, 0x70, 0x2, 0x2, 0x71, 0x2, 0x4, 0x6, 0x8, 
       0xa, 0xc, 0xe, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 
       0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 
       0x38, 0x3a, 0x3c, 0x3e, 0x40, 0x42, 0x44, 0x46, 0x48, 0x4a, 0x4c, 
       0x4e, 0x50, 0x52, 0x54, 0x56, 0x58, 0x5a, 0x5c, 0x5e, 0x60, 0x62, 
       0x64, 0x66, 0x68, 0x6a, 0x6c, 0x6e, 0x70, 0x72, 0x74, 0x76, 0x78, 
       0x7a, 0x7c, 0x7e, 0x80, 0x82, 0x84, 0x86, 0x88, 0x8a, 0x8c, 0x8e, 
       0x90, 0x92, 0x94, 0x96, 0x98, 0x9a, 0x9c, 0x9e, 0xa0, 0xa2, 0xa4, 
       0xa6, 0xa8, 0xaa, 0xac, 0xae, 0xb0, 0xb2, 0xb4, 0xb6, 0xb8, 0xba, 
       0xbc, 0xbe, 0xc0, 0xc2, 0xc4, 0xc6, 0xc8, 0xca, 0xcc, 0xce, 0xd0, 
       0xd2, 0xd4, 0xd6, 0xd8, 0xda, 0xdc, 0xde, 0x2, 0x19, 0x3, 0x2, 0x2d, 
       0x30, 0x3, 0x2, 0x17, 0x1a, 0x3, 0x2, 0x26, 0x27, 0x3, 0x2, 0x40, 
       0x41, 0x3, 0x2, 0x42, 0x43, 0x3, 0x2, 0x4c, 0x4f, 0x3, 0x2, 0x6f, 
       0x71, 0x3, 0x2, 0x58, 0x5e, 0x4, 0x2, 0x53, 0x56, 0x5f, 0x60, 0x3, 
       0x2, 0x62, 0x63, 0x3, 0x2, 0x6c, 0x6d, 0x3, 0x2, 0x73, 0x74, 0x4, 
       0x2, 0x10, 0x10, 0x81, 0x83, 0x4, 0x2, 0x37, 0x37, 0x82, 0x82, 0x5, 
       0x2, 0x37, 0x37, 0x82, 0x83, 0x88, 0x88, 0x4, 0x2, 0x84, 0x84, 0x87, 
       0x87, 0x3, 0x2, 0x85, 0x87, 0x3, 0x2, 0x22, 0x23, 0x3, 0x2, 0x24, 
       0x25, 0x3, 0x2, 0x3, 0x4, 0x3, 0x2, 0x1e, 0x1f, 0x3, 0x2, 0x28, 0x29, 
       0x3, 0x2, 0x3b, 0x3c, 0x2, 0x4c2, 0x2, 0xe7, 0x3, 0x2, 0x2, 0x2, 
       0x4, 0xf2, 0x3, 0x2, 0x2, 0x2, 0x6, 0xf6, 0x3, 0x2, 0x2, 0x2, 0x8, 
       0xfb, 0x3, 0x2, 0x2, 0x2, 0xa, 0xff, 0x3, 0x2, 0x2, 0x2, 0xc, 0x103, 
       0x3, 0x2, 0x2, 0x2, 0xe, 0x10b, 0x3, 0x2, 0x2, 0x2, 0x10, 0x114, 
       0x3, 0x2, 0x2, 0x2, 0x12, 0x118, 0x3, 0x2, 0x2, 0x2, 0x14, 0x127, 
       0x3, 0x2, 0x2, 0x2, 0x16, 0x133, 0x3, 0x2, 0x2, 0x2, 0x18, 0x13e, 
       0x3, 0x2, 0x2, 0x2, 0x1a, 0x150, 0x3, 0x2, 0x2, 0x2, 0x1c, 0x152, 
       0x3, 0x2, 0x2, 0x2, 0x1e, 0x164, 0x3, 0x2, 0x2, 0x2, 0x20, 0x171, 
       0x3, 0x2, 0x2, 0x2, 0x22, 0x180, 0x3, 0x2, 0x2, 0x2, 0x24, 0x190, 
       0x3, 0x2, 0x2, 0x2, 0x26, 0x194, 0x3, 0x2, 0x2, 0x2, 0x28, 0x197, 
       0x3, 0x2, 0x2, 0x2, 0x2a, 0x19a, 0x3, 0x2, 0x2, 0x2, 0x2c, 0x19f, 
       0x3, 0x2, 0x2, 0x2, 0x2e, 0x1b0, 0x3, 0x2, 0x2, 0x2, 0x30, 0x1b2, 
       0x3, 0x2, 0x2, 0x2, 0x32, 0x1c9, 0x3, 0x2, 0x2, 0x2, 0x34, 0x1e0, 
       0x3, 0x2, 0x2, 0x2, 0x36, 0x21e, 0x3, 0x2, 0x2, 0x2, 0x38, 0x227, 
       0x3, 0x2, 0x2, 0x2, 0x3a, 0x230, 0x3, 0x2, 0x2, 0x2, 0x3c, 0x232, 
       0x3, 0x2, 0x2, 0x2, 0x3e, 0x236, 0x3, 0x2, 0x2, 0x2, 0x40, 0x23c, 
       0x3, 0x2, 0x2, 0x2, 0x42, 0x247, 0x3, 0x2, 0x2, 0x2, 0x44, 0x24d, 
       0x3, 0x2, 0x2, 0x2, 0x46, 0x25a, 0x3, 0x2, 0x2, 0x2, 0x48, 0x263, 
       0x3, 0x2, 0x2, 0x2, 0x4a, 0x265, 0x3, 0x2, 0x2, 0x2, 0x4c, 0x273, 
       0x3, 0x2, 0x2, 0x2, 0x4e, 0x277, 0x3, 0x2, 0x2, 0x2, 0x50, 0x27d, 
       0x3, 0x2, 0x2, 0x2, 0x52, 0x286, 0x3, 0x2, 0x2, 0x2, 0x54, 0x28d, 
       0x3, 0x2, 0x2, 0x2, 0x56, 0x29d, 0x3, 0x2, 0x2, 0x2, 0x58, 0x2a1, 
       0x3, 0x2, 0x2, 0x2, 0x5a, 0x2aa, 0x3, 0x2, 0x2, 0x2, 0x5c, 0x2b1, 
       0x3, 0x2, 0x2, 0x2, 0x5e, 0x2b8, 0x3, 0x2, 0x2, 0x2, 0x60, 0x2bf, 
       0x3, 0x2, 0x2, 0x2, 0x62, 0x2cc, 0x3, 0x2, 0x2, 0x2, 0x64, 0x2d0, 
       0x3, 0x2, 0x2, 0x2, 0x66, 0x2d3, 0x3, 0x2, 0x2, 0x2, 0x68, 0x2e1, 
       0x3, 0x2, 0x2, 0x2, 0x6a, 0x2e5, 0x3, 0x2, 0x2, 0x2, 0x6c, 0x2e8, 
       0x3, 0x2, 0x2, 0x2, 0x6e, 0x2f6, 0x3, 0x2, 0x2, 0x2, 0x70, 0x2fa, 
       0x3, 0x2, 0x2, 0x2, 0x72, 0x2fd, 0x3, 0x2, 0x2, 0x2, 0x74, 0x30a, 
       0x3, 0x2, 0x2, 0x2, 0x76, 0x30e, 0x3, 0x2, 0x2, 0x2, 0x78, 0x31c, 
       0x3, 0x2, 0x2, 0x2, 0x7a, 0x329, 0x3, 0x2, 0x2, 0x2, 0x7c, 0x348, 
       0x3, 0x2, 0x2, 0x2, 0x7e, 0x34a, 0x3, 0x2, 0x2, 0x2, 0x80, 0x35a, 
       0x3, 0x2, 0x2, 0x2, 0x82, 0x35e, 0x3, 0x2, 0x2, 0x2, 0x84, 0x369, 
       0x3, 0x2, 0x2, 0x2, 0x86, 0x376, 0x3, 0x2, 0x2, 0x2, 0x88, 0x37a, 
       0x3, 0x2, 0x2, 0x2, 0x8a, 0x383, 0x3, 0x2, 0x2, 0x2, 0x8c, 0x389, 
       0x3, 0x2, 0x2, 0x2, 0x8e, 0x394, 0x3, 0x2, 0x2, 0x2, 0x90, 0x398, 
       0x3, 0x2, 0x2, 0x2, 0x92, 0x39b, 0x3, 0x2, 0x2, 0x2, 0x94, 0x3a8, 
       0x3, 0x2, 0x2, 0x2, 0x96, 0x3b5, 0x3, 0x2, 0x2, 0x2, 0x98, 0x3b9, 
       0x3, 0x2, 0x2, 0x2, 0x9a, 0x3bd, 0x3, 0x2, 0x2, 0x2, 0x9c, 0x3cc, 
       0x3, 0x2, 0x2, 0x2, 0x9e, 0x3d0, 0x3, 0x2, 0x2, 0x2, 0xa0, 0x3d9, 
       0x3, 0x2, 0x2, 0x2, 0xa2, 0x3df, 0x3, 0x2, 0x2, 0x2, 0xa4, 0x3e5, 
       0x3, 0x2, 0x2, 0x2, 0xa6, 0x3e9, 0x3, 0x2, 0x2, 0x2, 0xa8, 0x3eb, 
       0x3, 0x2, 0x2, 0x2, 0xaa, 0x3f9, 0x3, 0x2, 0x2, 0x2, 0xac, 0x3fd, 
       0x3, 0x2, 0x2, 0x2, 0xae, 0x3ff, 0x3, 0x2, 0x2, 0x2, 0xb0, 0x401, 
       0x3, 0x2, 0x2, 0x2, 0xb2, 0x403, 0x3, 0x2, 0x2, 0x2, 0xb4, 0x405, 
       0x3, 0x2, 0x2, 0x2, 0xb6, 0x407, 0x3, 0x2, 0x2, 0x2, 0xb8, 0x40c, 
       0x3, 0x2, 0x2, 0x2, 0xba, 0x414, 0x3, 0x2, 0x2, 0x2, 0xbc, 0x41c, 
       0x3, 0x2, 0x2, 0x2, 0xbe, 0x424, 0x3, 0x2, 0x2, 0x2, 0xc0, 0x42c, 
       0x3, 0x2, 0x2, 0x2, 0xc2, 0x434, 0x3, 0x2, 0x2, 0x2, 0xc4, 0x43c, 
       0x3, 0x2, 0x2, 0x2, 0xc6, 0x444, 0x3, 0x2, 0x2, 0x2, 0xc8, 0x44c, 
       0x3, 0x2, 0x2, 0x2, 0xca, 0x454, 0x3, 0x2, 0x2, 0x2, 0xcc, 0x45c, 
       0x3, 0x2, 0x2, 0x2, 0xce, 0x464, 0x3, 0x2, 0x2, 0x2, 0xd0, 0x46c, 
       0x3, 0x2, 0x2, 0x2, 0xd2, 0x474, 0x3, 0x2, 0x2, 0x2, 0xd4, 0x479, 
       0x3, 0x2, 0x2, 0x2, 0xd6, 0x47b, 0x3, 0x2, 0x2, 0x2, 0xd8, 0x47d, 
       0x3, 0x2, 0x2, 0x2, 0xda, 0x47f, 0x3, 0x2, 0x2, 0x2, 0xdc, 0x481, 
       0x3, 0x2, 0x2, 0x2, 0xde, 0x483, 0x3, 0x2, 0x2, 0x2, 0xe0, 0xe6, 
       0x5, 0x4, 0x3, 0x2, 0xe1, 0xe6, 0x5, 0x12, 0xa, 0x2, 0xe2, 0xe6, 
       0x5, 0x14, 0xb, 0x2, 0xe3, 0xe6, 0x5, 0x16, 0xc, 0x2, 0xe4, 0xe6, 
       0x5, 0x18, 0xd, 0x2, 0xe5, 0xe0, 0x3, 0x2, 0x2, 0x2, 0xe5, 0xe1, 
       0x3, 0x2, 0x2, 0x2, 0xe5, 0xe2, 0x3, 0x2, 0x2, 0x2, 0xe5, 0xe3, 0x3, 
       0x2, 0x2, 0x2, 0xe5, 0xe4, 0x3, 0x2, 0x2, 0x2, 0xe6, 0xe9, 0x3, 0x2, 
       0x2, 0x2, 0xe7, 0xe5, 0x3, 0x2, 0x2, 0x2, 0xe7, 0xe8, 0x3, 0x2, 0x2, 
       0x2, 0xe8, 0xea, 0x3, 0x2, 0x2, 0x2, 0xe9, 0xe7, 0x3, 0x2, 0x2, 0x2, 
       0xea, 0xeb, 0x7, 0x2, 0x2, 0x3, 0xeb, 0x3, 0x3, 0x2, 0x2, 0x2, 0xec, 
       0xf3, 0x5, 0x6, 0x4, 0x2, 0xed, 0xf3, 0x5, 0x8, 0x5, 0x2, 0xee, 0xf3, 
       0x5, 0xa, 0x6, 0x2, 0xef, 0xf3, 0x5, 0xc, 0x7, 0x2, 0xf0, 0xf3, 0x5, 
       0xe, 0x8, 0x2, 0xf1, 0xf3, 0x5, 0x10, 0x9, 0x2, 0xf2, 0xec, 0x3, 
       0x2, 0x2, 0x2, 0xf2, 0xed, 0x3, 0x2, 0x2, 0x2, 0xf2, 0xee, 0x3, 0x2, 
       0x2, 0x2, 0xf2, 0xef, 0x3, 0x2, 0x2, 0x2, 0xf2, 0xf0, 0x3, 0x2, 0x2, 
       0x2, 0xf2, 0xf1, 0x3, 0x2, 0x2, 0x2, 0xf3, 0xf4, 0x3, 0x2, 0x2, 0x2, 
       0xf4, 0xf5, 0x7, 0x7a, 0x2, 0x2, 0xf5, 0x5, 0x3, 0x2, 0x2, 0x2, 0xf6, 
       0xf7, 0x7, 0x7, 0x2, 0x2, 0xf7, 0xf8, 0x7, 0x8f, 0x2, 0x2, 0xf8, 
       0xf9, 0x7, 0x90, 0x2, 0x2, 0xf9, 0xfa, 0x7, 0x91, 0x2, 0x2, 0xfa, 
       0x7, 0x3, 0x2, 0x2, 0x2, 0xfb, 0xfc, 0x7, 0x7f, 0x2, 0x2, 0xfc, 0xfd, 
       0x7, 0x7b, 0x2, 0x2, 0xfd, 0xfe, 0x5, 0xa6, 0x54, 0x2, 0xfe, 0x9, 
       0x3, 0x2, 0x2, 0x2, 0xff, 0x100, 0x7, 0xc, 0x2, 0x2, 0x100, 0x101, 
       0x5, 0xb2, 0x5a, 0x2, 0x101, 0x102, 0x5, 0xb2, 0x5a, 0x2, 0x102, 
       0xb, 0x3, 0x2, 0x2, 0x2, 0x103, 0x106, 0x7, 0x38, 0x2, 0x2, 0x104, 
       0x107, 0x5, 0xac, 0x57, 0x2, 0x105, 0x107, 0x5, 0xa6, 0x54, 0x2, 
       0x106, 0x104, 0x3, 0x2, 0x2, 0x2, 0x106, 0x105, 0x3, 0x2, 0x2, 0x2, 
       0x107, 0x108, 0x3, 0x2, 0x2, 0x2, 0x108, 0x109, 0x5, 0x9a, 0x4e, 
       0x2, 0x109, 0x10a, 0x7, 0x7f, 0x2, 0x2, 0x10a, 0xd, 0x3, 0x2, 0x2, 
       0x2, 0x10b, 0x10c, 0x7, 0x35, 0x2, 0x2, 0x10c, 0x10d, 0x7, 0x87, 
       0x2, 0x2, 0x10d, 0x110, 0x7, 0x87, 0x2, 0x2, 0x10e, 0x10f, 0x7, 0x33, 
       0x2, 0x2, 0x10f, 0x111, 0x7, 0x87, 0x2, 0x2, 0x110, 0x10e, 0x3, 0x2, 
       0x2, 0x2, 0x110, 0x111, 0x3, 0x2, 0x2, 0x2, 0x111, 0x112, 0x3, 0x2, 
       0x2, 0x2, 0x112, 0x113, 0x5, 0xb0, 0x59, 0x2, 0x113, 0xf, 0x3, 0x2, 
       0x2, 0x2, 0x114, 0x115, 0x7, 0x36, 0x2, 0x2, 0x115, 0x116, 0x5, 0x3a, 
       0x1e, 0x2, 0x116, 0x117, 0x5, 0xb0, 0x59, 0x2, 0x117, 0x11, 0x3, 
       0x2, 0x2, 0x2, 0x118, 0x119, 0x7, 0x8, 0x2, 0x2, 0x119, 0x11b, 0x5, 
       0xb2, 0x5a, 0x2, 0x11a, 0x11c, 0x7, 0x1b, 0x2, 0x2, 0x11b, 0x11a, 
       0x3, 0x2, 0x2, 0x2, 0x11b, 0x11c, 0x3, 0x2, 0x2, 0x2, 0x11c, 0x11d, 
       0x3, 0x2, 0x2, 0x2, 0x11d, 0x11f, 0x7, 0x75, 0x2, 0x2, 0x11e, 0x120, 
       0x5, 0x1a, 0xe, 0x2, 0x11f, 0x11e, 0x3, 0x2, 0x2, 0x2, 0x120, 0x121, 
       0x3, 0x2, 0x2, 0x2, 0x121, 0x11f, 0x3, 0x2, 0x2, 0x2, 0x121, 0x122, 
       0x3, 0x2, 0x2, 0x2, 0x122, 0x123, 0x3, 0x2, 0x2, 0x2, 0x123, 0x124, 
       0x7, 0x76, 0x2, 0x2, 0x124, 0x125, 0x5, 0xb2, 0x5a, 0x2, 0x125, 0x126, 
       0x7, 0x7a, 0x2, 0x2, 0x126, 0x13, 0x3, 0x2, 0x2, 0x2, 0x127, 0x131, 
       0x7, 0x9, 0x2, 0x2, 0x128, 0x132, 0x5, 0x4a, 0x26, 0x2, 0x129, 0x132, 
       0x5, 0x54, 0x2b, 0x2, 0x12a, 0x132, 0x5, 0x60, 0x31, 0x2, 0x12b, 
       0x132, 0x5, 0x66, 0x34, 0x2, 0x12c, 0x132, 0x5, 0x6c, 0x37, 0x2, 
       0x12d, 0x132, 0x5, 0x72, 0x3a, 0x2, 0x12e, 0x132, 0x5, 0x78, 0x3d, 
       0x2, 0x12f, 0x132, 0x5, 0x7e, 0x40, 0x2, 0x130, 0x132, 0x5, 0x94, 
       0x4b, 0x2, 0x131, 0x128, 0x3, 0x2, 0x2, 0x2, 0x131, 0x129, 0x3, 0x2, 
       0x2, 0x2, 0x131, 0x12a, 0x3, 0x2, 0x2, 0x2, 0x131, 0x12b, 0x3, 0x2, 
       0x2, 0x2, 0x131, 0x12c, 0x3, 0x2, 0x2, 0x2, 0x131, 0x12d, 0x3, 0x2, 
       0x2, 0x2, 0x131, 0x12e, 0x3, 0x2, 0x2, 0x2, 0x131, 0x12f, 0x3, 0x2, 
       0x2, 0x2, 0x131, 0x130, 0x3, 0x2, 0x2, 0x2, 0x132, 0x15, 0x3, 0x2, 
       0x2, 0x2, 0x133, 0x134, 0x5, 0xd8, 0x6d, 0x2, 0x134, 0x135, 0x7, 
       0x8a, 0x2, 0x2, 0x135, 0x139, 0x7, 0x8b, 0x2, 0x2, 0x136, 0x138, 
       0x7, 0x8d, 0x2, 0x2, 0x137, 0x136, 0x3, 0x2, 0x2, 0x2, 0x138, 0x13b, 
       0x3, 0x2, 0x2, 0x2, 0x139, 0x137, 0x3, 0x2, 0x2, 0x2, 0x139, 0x13a, 
       0x3, 0x2, 0x2, 0x2, 0x13a, 0x13c, 0x3, 0x2, 0x2, 0x2, 0x13b, 0x139, 
       0x3, 0x2, 0x2, 0x2, 0x13c, 0x13d, 0x7, 0x8c, 0x2, 0x2, 0x13d, 0x17, 
       0x3, 0x2, 0x2, 0x2, 0x13e, 0x13f, 0x7, 0xe, 0x2, 0x2, 0x13f, 0x141, 
       0x5, 0xb0, 0x59, 0x2, 0x140, 0x142, 0x7, 0x1b, 0x2, 0x2, 0x141, 0x140, 
       0x3, 0x2, 0x2, 0x2, 0x141, 0x142, 0x3, 0x2, 0x2, 0x2, 0x142, 0x143, 
       0x3, 0x2, 0x2, 0x2, 0x143, 0x145, 0x7, 0x75, 0x2, 0x2, 0x144, 0x146, 
       0x5, 0x24, 0x13, 0x2, 0x145, 0x144, 0x3, 0x2, 0x2, 0x2, 0x146, 0x147, 
       0x3, 0x2, 0x2, 0x2, 0x147, 0x145, 0x3, 0x2, 0x2, 0x2, 0x147, 0x148, 
       0x3, 0x2, 0x2, 0x2, 0x148, 0x149, 0x3, 0x2, 0x2, 0x2, 0x149, 0x14a, 
       0x7, 0x76, 0x2, 0x2, 0x14a, 0x14b, 0x5, 0xb0, 0x59, 0x2, 0x14b, 0x14c, 
       0x7, 0x7a, 0x2, 0x2, 0x14c, 0x19, 0x3, 0x2, 0x2, 0x2, 0x14d, 0x151, 
       0x5, 0x24, 0x13, 0x2, 0x14e, 0x151, 0x5, 0x1c, 0xf, 0x2, 0x14f, 0x151, 
       0x5, 0x1e, 0x10, 0x2, 0x150, 0x14d, 0x3, 0x2, 0x2, 0x2, 0x150, 0x14e, 
       0x3, 0x2, 0x2, 0x2, 0x150, 0x14f, 0x3, 0x2, 0x2, 0x2, 0x151, 0x1b, 
       0x3, 0x2, 0x2, 0x2, 0x152, 0x153, 0x7, 0xe, 0x2, 0x2, 0x153, 0x160, 
       0x5, 0xb0, 0x59, 0x2, 0x154, 0x156, 0x7, 0x1b, 0x2, 0x2, 0x155, 0x154, 
       0x3, 0x2, 0x2, 0x2, 0x155, 0x156, 0x3, 0x2, 0x2, 0x2, 0x156, 0x157, 
       0x3, 0x2, 0x2, 0x2, 0x157, 0x159, 0x7, 0x75, 0x2, 0x2, 0x158, 0x15a, 
       0x5, 0x24, 0x13, 0x2, 0x159, 0x158, 0x3, 0x2, 0x2, 0x2, 0x15a, 0x15b, 
       0x3, 0x2, 0x2, 0x2, 0x15b, 0x159, 0x3, 0x2, 0x2, 0x2, 0x15b, 0x15c, 
       0x3, 0x2, 0x2, 0x2, 0x15c, 0x15d, 0x3, 0x2, 0x2, 0x2, 0x15d, 0x15e, 
       0x7, 0x76, 0x2, 0x2, 0x15e, 0x15f, 0x5, 0xb0, 0x59, 0x2, 0x15f, 0x161, 
       0x3, 0x2, 0x2, 0x2, 0x160, 0x155, 0x3, 0x2, 0x2, 0x2, 0x160, 0x161, 
       0x3, 0x2, 0x2, 0x2, 0x161, 0x162, 0x3, 0x2, 0x2, 0x2, 0x162, 0x163, 
       0x7, 0x7a, 0x2, 0x2, 0x163, 0x1d, 0x3, 0x2, 0x2, 0x2, 0x164, 0x165, 
       0x7, 0x2c, 0x2, 0x2, 0x165, 0x169, 0x7, 0x75, 0x2, 0x2, 0x166, 0x168, 
       0x5, 0x20, 0x11, 0x2, 0x167, 0x166, 0x3, 0x2, 0x2, 0x2, 0x168, 0x16b, 
       0x3, 0x2, 0x2, 0x2, 0x169, 0x167, 0x3, 0x2, 0x2, 0x2, 0x169, 0x16a, 
       0x3, 0x2, 0x2, 0x2, 0x16a, 0x16c, 0x3, 0x2, 0x2, 0x2, 0x16b, 0x169, 
       0x3, 0x2, 0x2, 0x2, 0x16c, 0x16d, 0x7, 0x76, 0x2, 0x2, 0x16d, 0x16e, 
       0x7, 0x7a, 0x2, 0x2, 0x16e, 0x1f, 0x3, 0x2, 0x2, 0x2, 0x16f, 0x172, 
       0x5, 0x22, 0x12, 0x2, 0x170, 0x172, 0x5, 0x6, 0x4, 0x2, 0x171, 0x16f, 
       0x3, 0x2, 0x2, 0x2, 0x171, 0x170, 0x3, 0x2, 0x2, 0x2, 0x172, 0x173, 
       0x3, 0x2, 0x2, 0x2, 0x173, 0x174, 0x7, 0x7a, 0x2, 0x2, 0x174, 0x21, 
       0x3, 0x2, 0x2, 0x2, 0x175, 0x176, 0x9, 0x2, 0x2, 0x2, 0x176, 0x178, 
       0x7, 0x75, 0x2, 0x2, 0x177, 0x179, 0x5, 0x8e, 0x48, 0x2, 0x178, 0x177, 
       0x3, 0x2, 0x2, 0x2, 0x179, 0x17a, 0x3, 0x2, 0x2, 0x2, 0x17a, 0x178, 
       0x3, 0x2, 0x2, 0x2, 0x17a, 0x17b, 0x3, 0x2, 0x2, 0x2, 0x17b, 0x17c, 
       0x3, 0x2, 0x2, 0x2, 0x17c, 0x17d, 0x7, 0x76, 0x2, 0x2, 0x17d, 0x181, 
       0x3, 0x2, 0x2, 0x2, 0x17e, 0x17f, 0x7, 0x31, 0x2, 0x2, 0x17f, 0x181, 
       0x5, 0xb6, 0x5c, 0x2, 0x180, 0x175, 0x3, 0x2, 0x2, 0x2, 0x180, 0x17e, 
       0x3, 0x2, 0x2, 0x2, 0x181, 0x23, 0x3, 0x2, 0x2, 0x2, 0x182, 0x191, 
       0x5, 0x26, 0x14, 0x2, 0x183, 0x191, 0x5, 0x28, 0x15, 0x2, 0x184, 
       0x191, 0x5, 0x2a, 0x16, 0x2, 0x185, 0x191, 0x5, 0x2c, 0x17, 0x2, 
       0x186, 0x191, 0x5, 0x8, 0x5, 0x2, 0x187, 0x191, 0x5, 0x30, 0x19, 
       0x2, 0x188, 0x191, 0x5, 0x32, 0x1a, 0x2, 0x189, 0x191, 0x5, 0xc, 
       0x7, 0x2, 0x18a, 0x191, 0x5, 0x34, 0x1b, 0x2, 0x18b, 0x191, 0x5, 
       0x42, 0x22, 0x2, 0x18c, 0x191, 0x5, 0x44, 0x23, 0x2, 0x18d, 0x191, 
       0x5, 0x46, 0x24, 0x2, 0x18e, 0x191, 0x5, 0x48, 0x25, 0x2, 0x18f, 
       0x191, 0x5, 0x6, 0x4, 0x2, 0x190, 0x182, 0x3, 0x2, 0x2, 0x2, 0x190, 
       0x183, 0x3, 0x2, 0x2, 0x2, 0x190, 0x184, 0x3, 0x2, 0x2, 0x2, 0x190, 
       0x185, 0x3, 0x2, 0x2, 0x2, 0x190, 0x186, 0x3, 0x2, 0x2, 0x2, 0x190, 
       0x187, 0x3, 0x2, 0x2, 0x2, 0x190, 0x188, 0x3, 0x2, 0x2, 0x2, 0x190, 
       0x189, 0x3, 0x2, 0x2, 0x2, 0x190, 0x18a, 0x3, 0x2, 0x2, 0x2, 0x190, 
       0x18b, 0x3, 0x2, 0x2, 0x2, 0x190, 0x18c, 0x3, 0x2, 0x2, 0x2, 0x190, 
       0x18d, 0x3, 0x2, 0x2, 0x2, 0x190, 0x18e, 0x3, 0x2, 0x2, 0x2, 0x190, 
       0x18f, 0x3, 0x2, 0x2, 0x2, 0x191, 0x192, 0x3, 0x2, 0x2, 0x2, 0x192, 
       0x193, 0x7, 0x7a, 0x2, 0x2, 0x193, 0x25, 0x3, 0x2, 0x2, 0x2, 0x194, 
       0x195, 0x7, 0x8, 0x2, 0x2, 0x195, 0x196, 0x5, 0xb2, 0x5a, 0x2, 0x196, 
       0x27, 0x3, 0x2, 0x2, 0x2, 0x197, 0x198, 0x7, 0xa, 0x2, 0x2, 0x198, 
       0x199, 0x5, 0xb2, 0x5a, 0x2, 0x199, 0x29, 0x3, 0x2, 0x2, 0x2, 0x19a, 
       0x19b, 0x7, 0xb, 0x2, 0x2, 0x19b, 0x19d, 0x5, 0xb2, 0x5a, 0x2, 0x19c, 
       0x19e, 0x9, 0x3, 0x2, 0x2, 0x19d, 0x19c, 0x3, 0x2, 0x2, 0x2, 0x19d, 
       0x19e, 0x3, 0x2, 0x2, 0x2, 0x19e, 0x2b, 0x3, 0x2, 0x2, 0x2, 0x19f, 
       0x1a6, 0x7, 0xf, 0x2, 0x2, 0x1a0, 0x1a7, 0x7, 0x87, 0x2, 0x2, 0x1a1, 
       0x1a3, 0x5, 0x2e, 0x18, 0x2, 0x1a2, 0x1a1, 0x3, 0x2, 0x2, 0x2, 0x1a3, 
       0x1a4, 0x3, 0x2, 0x2, 0x2, 0x1a4, 0x1a2, 0x3, 0x2, 0x2, 0x2, 0x1a4, 
       0x1a5, 0x3, 0x2, 0x2, 0x2, 0x1a5, 0x1a7, 0x3, 0x2, 0x2, 0x2, 0x1a6, 
       0x1a0, 0x3, 0x2, 0x2, 0x2, 0x1a6, 0x1a2, 0x3, 0x2, 0x2, 0x2, 0x1a7, 
       0x2d, 0x3, 0x2, 0x2, 0x2, 0x1a8, 0x1b1, 0x7, 0x11, 0x2, 0x2, 0x1a9, 
       0x1b1, 0x7, 0x12, 0x2, 0x2, 0x1aa, 0x1b1, 0x7, 0x13, 0x2, 0x2, 0x1ab, 
       0x1b1, 0x7, 0x14, 0x2, 0x2, 0x1ac, 0x1ad, 0x7, 0x16, 0x2, 0x2, 0x1ad, 
       0x1b1, 0x5, 0xa6, 0x54, 0x2, 0x1ae, 0x1af, 0x7, 0x15, 0x2, 0x2, 0x1af, 
       0x1b1, 0x5, 0xa6, 0x54, 0x2, 0x1b0, 0x1a8, 0x3, 0x2, 0x2, 0x2, 0x1b0, 
       0x1a9, 0x3, 0x2, 0x2, 0x2, 0x1b0, 0x1aa, 0x3, 0x2, 0x2, 0x2, 0x1b0, 
       0x1ab, 0x3, 0x2, 0x2, 0x2, 0x1b0, 0x1ac, 0x3, 0x2, 0x2, 0x2, 0x1b0, 
       0x1ae, 0x3, 0x2, 0x2, 0x2, 0x1b1, 0x2f, 0x3, 0x2, 0x2, 0x2, 0x1b2, 
       0x1b6, 0x7, 0x21, 0x2, 0x2, 0x1b3, 0x1b7, 0x5, 0xd4, 0x6b, 0x2, 0x1b4, 
       0x1b7, 0x5, 0xd6, 0x6c, 0x2, 0x1b5, 0x1b7, 0x5, 0xdc, 0x6f, 0x2, 
       0x1b6, 0x1b3, 0x3, 0x2, 0x2, 0x2, 0x1b6, 0x1b4, 0x3, 0x2, 0x2, 0x2, 
       0x1b6, 0x1b5, 0x3, 0x2, 0x2, 0x2, 0x1b7, 0x1b8, 0x3, 0x2, 0x2, 0x2, 
       0x1b8, 0x1bd, 0x5, 0x9c, 0x4f, 0x2, 0x1b9, 0x1ba, 0x7, 0x7d, 0x2, 
       0x2, 0x1ba, 0x1bc, 0x5, 0x9c, 0x4f, 0x2, 0x1bb, 0x1b9, 0x3, 0x2, 
       0x2, 0x2, 0x1bc, 0x1bf, 0x3, 0x2, 0x2, 0x2, 0x1bd, 0x1bb, 0x3, 0x2, 
       0x2, 0x2, 0x1bd, 0x1be, 0x3, 0x2, 0x2, 0x2, 0x1be, 0x31, 0x3, 0x2, 
       0x2, 0x2, 0x1bf, 0x1bd, 0x3, 0x2, 0x2, 0x2, 0x1c0, 0x1c1, 0x7, 0x20, 
       0x2, 0x2, 0x1c1, 0x1c6, 0x5, 0x9c, 0x4f, 0x2, 0x1c2, 0x1c3, 0x7, 
       0x7d, 0x2, 0x2, 0x1c3, 0x1c5, 0x5, 0x9c, 0x4f, 0x2, 0x1c4, 0x1c2, 
       0x3, 0x2, 0x2, 0x2, 0x1c5, 0x1c8, 0x3, 0x2, 0x2, 0x2, 0x1c6, 0x1c4, 
       0x3, 0x2, 0x2, 0x2, 0x1c6, 0x1c7, 0x3, 0x2, 0x2, 0x2, 0x1c7, 0x1ca, 
       0x3, 0x2, 0x2, 0x2, 0x1c8, 0x1c6, 0x3, 0x2, 0x2, 0x2, 0x1c9, 0x1c0, 
       0x3, 0x2, 0x2, 0x2, 0x1c9, 0x1ca, 0x3, 0x2, 0x2, 0x2, 0x1ca, 0x1dd, 
       0x3, 0x2, 0x2, 0x2, 0x1cb, 0x1cc, 0x5, 0xd6, 0x6c, 0x2, 0x1cc, 0x1d2, 
       0x5, 0x9c, 0x4f, 0x2, 0x1cd, 0x1d0, 0x7, 0x26, 0x2, 0x2, 0x1ce, 0x1d1, 
       0x7, 0x3e, 0x2, 0x2, 0x1cf, 0x1d1, 0x5, 0x9c, 0x4f, 0x2, 0x1d0, 0x1ce, 
       0x3, 0x2, 0x2, 0x2, 0x1d0, 0x1cf, 0x3, 0x2, 0x2, 0x2, 0x1d1, 0x1d3, 
       0x3, 0x2, 0x2, 0x2, 0x1d2, 0x1cd, 0x3, 0x2, 0x2, 0x2, 0x1d2, 0x1d3, 
       0x3, 0x2, 0x2, 0x2, 0x1d3, 0x1de, 0x3, 0x2, 0x2, 0x2, 0x1d4, 0x1d5, 
       0x5, 0xd4, 0x6b, 0x2, 0x1d5, 0x1db, 0x5, 0x9c, 0x4f, 0x2, 0x1d6, 
       0x1d9, 0x9, 0x4, 0x2, 0x2, 0x1d7, 0x1da, 0x7, 0x3e, 0x2, 0x2, 0x1d8, 
       0x1da, 0x5, 0x9c, 0x4f, 0x2, 0x1d9, 0x1d7, 0x3, 0x2, 0x2, 0x2, 0x1d9, 
       0x1d8, 0x3, 0x2, 0x2, 0x2, 0x1da, 0x1dc, 0x3, 0x2, 0x2, 0x2, 0x1db, 
       0x1d6, 0x3, 0x2, 0x2, 0x2, 0x1db, 0x1dc, 0x3, 0x2, 0x2, 0x2, 0x1dc, 
       0x1de, 0x3, 0x2, 0x2, 0x2, 0x1dd, 0x1cb, 0x3, 0x2, 0x2, 0x2, 0x1dd, 
       0x1d4, 0x3, 0x2, 0x2, 0x2, 0x1de, 0x33, 0x3, 0x2, 0x2, 0x2, 0x1df, 
       0x1e1, 0x5, 0xda, 0x6e, 0x2, 0x1e0, 0x1df, 0x3, 0x2, 0x2, 0x2, 0x1e0, 
       0x1e1, 0x3, 0x2, 0x2, 0x2, 0x1e1, 0x1e2, 0x3, 0x2, 0x2, 0x2, 0x1e2, 
       0x1e4, 0x5, 0xdc, 0x6f, 0x2, 0x1e3, 0x1e5, 0x5, 0xa0, 0x51, 0x2, 
       0x1e4, 0x1e3, 0x3, 0x2, 0x2, 0x2, 0x1e4, 0x1e5, 0x3, 0x2, 0x2, 0x2, 
       0x1e5, 0x21c, 0x3, 0x2, 0x2, 0x2, 0x1e6, 0x1ea, 0x5, 0x38, 0x1d, 
       0x2, 0x1e7, 0x1e9, 0x5, 0x36, 0x1c, 0x2, 0x1e8, 0x1e7, 0x3, 0x2, 
       0x2, 0x2, 0x1e9, 0x1ec, 0x3, 0x2, 0x2, 0x2, 0x1ea, 0x1e8, 0x3, 0x2, 
       0x2, 0x2, 0x1ea, 0x1eb, 0x3, 0x2, 0x2, 0x2, 0x1eb, 0x21d, 0x3, 0x2, 
       0x2, 0x2, 0x1ec, 0x1ea, 0x3, 0x2, 0x2, 0x2, 0x1ed, 0x1ee, 0x7, 0xe, 
       0x2, 0x2, 0x1ee, 0x1f0, 0x5, 0xb0, 0x59, 0x2, 0x1ef, 0x1ed, 0x3, 
       0x2, 0x2, 0x2, 0x1f0, 0x1f1, 0x3, 0x2, 0x2, 0x2, 0x1f1, 0x1ef, 0x3, 
       0x2, 0x2, 0x2, 0x1f1, 0x1f2, 0x3, 0x2, 0x2, 0x2, 0x1f2, 0x1f6, 0x3, 
       0x2, 0x2, 0x2, 0x1f3, 0x1f5, 0x5, 0x9e, 0x50, 0x2, 0x1f4, 0x1f3, 
       0x3, 0x2, 0x2, 0x2, 0x1f5, 0x1f8, 0x3, 0x2, 0x2, 0x2, 0x1f6, 0x1f4, 
       0x3, 0x2, 0x2, 0x2, 0x1f6, 0x1f7, 0x3, 0x2, 0x2, 0x2, 0x1f7, 0x21d, 
       0x3, 0x2, 0x2, 0x2, 0x1f8, 0x1f6, 0x3, 0x2, 0x2, 0x2, 0x1f9, 0x1fa, 
       0x7, 0x39, 0x2, 0x2, 0x1fa, 0x1fc, 0x5, 0x3c, 0x1f, 0x2, 0x1fb, 0x1fd, 
       0x5, 0xa0, 0x51, 0x2, 0x1fc, 0x1fb, 0x3, 0x2, 0x2, 0x2, 0x1fc, 0x1fd, 
       0x3, 0x2, 0x2, 0x2, 0x1fd, 0x21d, 0x3, 0x2, 0x2, 0x2, 0x1fe, 0x1ff, 
       0x7, 0x3a, 0x2, 0x2, 0x1ff, 0x201, 0x5, 0xa0, 0x51, 0x2, 0x200, 0x202, 
       0x5, 0x3e, 0x20, 0x2, 0x201, 0x200, 0x3, 0x2, 0x2, 0x2, 0x202, 0x203, 
       0x3, 0x2, 0x2, 0x2, 0x203, 0x201, 0x3, 0x2, 0x2, 0x2, 0x203, 0x204, 
       0x3, 0x2, 0x2, 0x2, 0x204, 0x206, 0x3, 0x2, 0x2, 0x2, 0x205, 0x207, 
       0x5, 0xa0, 0x51, 0x2, 0x206, 0x205, 0x3, 0x2, 0x2, 0x2, 0x206, 0x207, 
       0x3, 0x2, 0x2, 0x2, 0x207, 0x21d, 0x3, 0x2, 0x2, 0x2, 0x208, 0x209, 
       0x5, 0xde, 0x70, 0x2, 0x209, 0x20b, 0x5, 0xa0, 0x51, 0x2, 0x20a, 
       0x20c, 0x5, 0x40, 0x21, 0x2, 0x20b, 0x20a, 0x3, 0x2, 0x2, 0x2, 0x20c, 
       0x20d, 0x3, 0x2, 0x2, 0x2, 0x20d, 0x20b, 0x3, 0x2, 0x2, 0x2, 0x20d, 
       0x20e, 0x3, 0x2, 0x2, 0x2, 0x20e, 0x210, 0x3, 0x2, 0x2, 0x2, 0x20f, 
       0x211, 0x5, 0xa0, 0x51, 0x2, 0x210, 0x20f, 0x3, 0x2, 0x2, 0x2, 0x210, 
       0x211, 0x3, 0x2, 0x2, 0x2, 0x211, 0x21d, 0x3, 0x2, 0x2, 0x2, 0x212, 
       0x213, 0x7, 0x37, 0x2, 0x2, 0x213, 0x215, 0x5, 0xa0, 0x51, 0x2, 0x214, 
       0x216, 0x5, 0x3e, 0x20, 0x2, 0x215, 0x214, 0x3, 0x2, 0x2, 0x2, 0x216, 
       0x217, 0x3, 0x2, 0x2, 0x2, 0x217, 0x215, 0x3, 0x2, 0x2, 0x2, 0x217, 
       0x218, 0x3, 0x2, 0x2, 0x2, 0x218, 0x21a, 0x3, 0x2, 0x2, 0x2, 0x219, 
       0x21b, 0x5, 0xa0, 0x51, 0x2, 0x21a, 0x219, 0x3, 0x2, 0x2, 0x2, 0x21a, 
       0x21b, 0x3, 0x2, 0x2, 0x2, 0x21b, 0x21d, 0x3, 0x2, 0x2, 0x2, 0x21c, 
       0x1e6, 0x3, 0x2, 0x2, 0x2, 0x21c, 0x1ef, 0x3, 0x2, 0x2, 0x2, 0x21c, 
       0x1f9, 0x3, 0x2, 0x2, 0x2, 0x21c, 0x1fe, 0x3, 0x2, 0x2, 0x2, 0x21c, 
       0x208, 0x3, 0x2, 0x2, 0x2, 0x21c, 0x212, 0x3, 0x2, 0x2, 0x2, 0x21d, 
       0x35, 0x3, 0x2, 0x2, 0x2, 0x21e, 0x220, 0x5, 0xa2, 0x52, 0x2, 0x21f, 
       0x221, 0x5, 0x38, 0x1d, 0x2, 0x220, 0x21f, 0x3, 0x2, 0x2, 0x2, 0x220, 
       0x221, 0x3, 0x2, 0x2, 0x2, 0x221, 0x37, 0x3, 0x2, 0x2, 0x2, 0x222, 
       0x223, 0x7, 0x1c, 0x2, 0x2, 0x223, 0x224, 0x5, 0xb0, 0x59, 0x2, 0x224, 
       0x225, 0x7, 0x1d, 0x2, 0x2, 0x225, 0x228, 0x3, 0x2, 0x2, 0x2, 0x226, 
       0x228, 0x5, 0x3a, 0x1e, 0x2, 0x227, 0x222, 0x3, 0x2, 0x2, 0x2, 0x227, 
       0x226, 0x3, 0x2, 0x2, 0x2, 0x228, 0x39, 0x3, 0x2, 0x2, 0x2, 0x229, 
       0x22a, 0x7, 0x1c, 0x2, 0x2, 0x22a, 0x22b, 0x7, 0x87, 0x2, 0x2, 0x22b, 
       0x22c, 0x7, 0x87, 0x2, 0x2, 0x22c, 0x22d, 0x7, 0x87, 0x2, 0x2, 0x22d, 
       0x22e, 0x7, 0x87, 0x2, 0x2, 0x22e, 0x231, 0x7, 0x1d, 0x2, 0x2, 0x22f, 
       0x231, 0x7, 0x87, 0x2, 0x2, 0x230, 0x229, 0x3, 0x2, 0x2, 0x2, 0x230, 
       0x22f, 0x3, 0x2, 0x2, 0x2, 0x231, 0x3b, 0x3, 0x2, 0x2, 0x2, 0x232, 
       0x233, 0x5, 0xa2, 0x52, 0x2, 0x233, 0x234, 0x5, 0x9a, 0x4e, 0x2, 
       0x234, 0x235, 0x5, 0x9a, 0x4e, 0x2, 0x235, 0x3d, 0x3, 0x2, 0x2, 0x2, 
       0x236, 0x237, 0x5, 0x9a, 0x4e, 0x2, 0x237, 0x238, 0x7, 0x37, 0x2, 
       0x2, 0x238, 0x23a, 0x7, 0x7f, 0x2, 0x2, 0x239, 0x23b, 0x7, 0x7c, 
       0x2, 0x2, 0x23a, 0x239, 0x3, 0x2, 0x2, 0x2, 0x23a, 0x23b, 0x3, 0x2, 
       0x2, 0x2, 0x23b, 0x3f, 0x3, 0x2, 0x2, 0x2, 0x23c, 0x23f, 0x5, 0x9a, 
       0x4e, 0x2, 0x23d, 0x23e, 0x7, 0x37, 0x2, 0x2, 0x23e, 0x240, 0x7, 
       0x7f, 0x2, 0x2, 0x23f, 0x23d, 0x3, 0x2, 0x2, 0x2, 0x23f, 0x240, 0x3, 
       0x2, 0x2, 0x2, 0x240, 0x242, 0x3, 0x2, 0x2, 0x2, 0x241, 0x243, 0x7, 
       0x3d, 0x2, 0x2, 0x242, 0x241, 0x3, 0x2, 0x2, 0x2, 0x242, 0x243, 0x3, 
       0x2, 0x2, 0x2, 0x243, 0x245, 0x3, 0x2, 0x2, 0x2, 0x244, 0x246, 0x7, 
       0x7c, 0x2, 0x2, 0x245, 0x244, 0x3, 0x2, 0x2, 0x2, 0x245, 0x246, 0x3, 
       0x2, 0x2, 0x2, 0x246, 0x41, 0x3, 0x2, 0x2, 0x2, 0x247, 0x249, 0x7, 
       0x2a, 0x2, 0x2, 0x248, 0x24a, 0x5, 0xb4, 0x5b, 0x2, 0x249, 0x248, 
       0x3, 0x2, 0x2, 0x2, 0x24a, 0x24b, 0x3, 0x2, 0x2, 0x2, 0x24b, 0x249, 
       0x3, 0x2, 0x2, 0x2, 0x24b, 0x24c, 0x3, 0x2, 0x2, 0x2, 0x24c, 0x43, 
       0x3, 0x2, 0x2, 0x2, 0x24d, 0x254, 0x7, 0x32, 0x2, 0x2, 0x24e, 0x252, 
       0x5, 0xb6, 0x5c, 0x2, 0x24f, 0x250, 0x5, 0xb6, 0x5c, 0x2, 0x250, 
       0x251, 0x5, 0xb6, 0x5c, 0x2, 0x251, 0x253, 0x3, 0x2, 0x2, 0x2, 0x252, 
       0x24f, 0x3, 0x2, 0x2, 0x2, 0x252, 0x253, 0x3, 0x2, 0x2, 0x2, 0x253, 
       0x255, 0x3, 0x2, 0x2, 0x2, 0x254, 0x24e, 0x3, 0x2, 0x2, 0x2, 0x254, 
       0x255, 0x3, 0x2, 0x2, 0x2, 0x255, 0x256, 0x3, 0x2, 0x2, 0x2, 0x256, 
       0x257, 0x7, 0x7e, 0x2, 0x2, 0x257, 0x258, 0x7, 0x92, 0x2, 0x2, 0x258, 
       0x259, 0x7, 0x93, 0x2, 0x2, 0x259, 0x45, 0x3, 0x2, 0x2, 0x2, 0x25a, 
       0x25b, 0x7, 0x2b, 0x2, 0x2, 0x25b, 0x25d, 0x7, 0x75, 0x2, 0x2, 0x25c, 
       0x25e, 0x5, 0x8e, 0x48, 0x2, 0x25d, 0x25c, 0x3, 0x2, 0x2, 0x2, 0x25e, 
       0x25f, 0x3, 0x2, 0x2, 0x2, 0x25f, 0x25d, 0x3, 0x2, 0x2, 0x2, 0x25f, 
       0x260, 0x3, 0x2, 0x2, 0x2, 0x260, 0x261, 0x3, 0x2, 0x2, 0x2, 0x261, 
       0x262, 0x7, 0x76, 0x2, 0x2, 0x262, 0x47, 0x3, 0x2, 0x2, 0x2, 0x263, 
       0x264, 0x7, 0xd, 0x2, 0x2, 0x264, 0x49, 0x3, 0x2, 0x2, 0x2, 0x265, 
       0x266, 0x7, 0x3f, 0x2, 0x2, 0x266, 0x268, 0x7, 0x75, 0x2, 0x2, 0x267, 
       0x269, 0x5, 0x4c, 0x27, 0x2, 0x268, 0x267, 0x3, 0x2, 0x2, 0x2, 0x269, 
       0x26a, 0x3, 0x2, 0x2, 0x2, 0x26a, 0x268, 0x3, 0x2, 0x2, 0x2, 0x26a, 
       0x26b, 0x3, 0x2, 0x2, 0x2, 0x26b, 0x26c, 0x3, 0x2, 0x2, 0x2, 0x26c, 
       0x26d, 0x7, 0x76, 0x2, 0x2, 0x26d, 0x26e, 0x7, 0x3f, 0x2, 0x2, 0x26e, 
       0x26f, 0x7, 0x7a, 0x2, 0x2, 0x26f, 0x4b, 0x3, 0x2, 0x2, 0x2, 0x270, 
       0x274, 0x5, 0x4e, 0x28, 0x2, 0x271, 0x274, 0x5, 0x50, 0x29, 0x2, 
       0x272, 0x274, 0x5, 0x6, 0x4, 0x2, 0x273, 0x270, 0x3, 0x2, 0x2, 0x2, 
       0x273, 0x271, 0x3, 0x2, 0x2, 0x2, 0x273, 0x272, 0x3, 0x2, 0x2, 0x2, 
       0x274, 0x275, 0x3, 0x2, 0x2, 0x2, 0x275, 0x276, 0x7, 0x7a, 0x2, 0x2, 
       0x276, 0x4d, 0x3, 0x2, 0x2, 0x2, 0x277, 0x279, 0x9, 0x5, 0x2, 0x2, 
       0x278, 0x27a, 0x5, 0xb2, 0x5a, 0x2, 0x279, 0x278, 0x3, 0x2, 0x2, 
       0x2, 0x27a, 0x27b, 0x3, 0x2, 0x2, 0x2, 0x27b, 0x279, 0x3, 0x2, 0x2, 
       0x2, 0x27b, 0x27c, 0x3, 0x2, 0x2, 0x2, 0x27c, 0x4f, 0x3, 0x2, 0x2, 
       0x2, 0x27d, 0x27e, 0x9, 0x6, 0x2, 0x2, 0x27e, 0x283, 0x5, 0x52, 0x2a, 
       0x2, 0x27f, 0x280, 0x7, 0x7d, 0x2, 0x2, 0x280, 0x282, 0x5, 0x52, 
       0x2a, 0x2, 0x281, 0x27f, 0x3, 0x2, 0x2, 0x2, 0x282, 0x285, 0x3, 0x2, 
       0x2, 0x2, 0x283, 0x281, 0x3, 0x2, 0x2, 0x2, 0x283, 0x284, 0x3, 0x2, 
       0x2, 0x2, 0x284, 0x51, 0x3, 0x2, 0x2, 0x2, 0x285, 0x283, 0x3, 0x2, 
       0x2, 0x2, 0x286, 0x287, 0x5, 0xb2, 0x5a, 0x2, 0x287, 0x289, 0x5, 
       0xb2, 0x5a, 0x2, 0x288, 0x28a, 0x7, 0x87, 0x2, 0x2, 0x289, 0x288, 
       0x3, 0x2, 0x2, 0x2, 0x28a, 0x28b, 0x3, 0x2, 0x2, 0x2, 0x28b, 0x289, 
       0x3, 0x2, 0x2, 0x2, 0x28b, 0x28c, 0x3, 0x2, 0x2, 0x2, 0x28c, 0x53, 
       0x3, 0x2, 0x2, 0x2, 0x28d, 0x28e, 0x7, 0x44, 0x2, 0x2, 0x28e, 0x290, 
       0x7, 0x75, 0x2, 0x2, 0x28f, 0x291, 0x5, 0x56, 0x2c, 0x2, 0x290, 0x28f, 
       0x3, 0x2, 0x2, 0x2, 0x291, 0x292, 0x3, 0x2, 0x2, 0x2, 0x292, 0x290, 
       0x3, 0x2, 0x2, 0x2, 0x292, 0x293, 0x3, 0x2, 0x2, 0x2, 0x293, 0x294, 
       0x3, 0x2, 0x2, 0x2, 0x294, 0x295, 0x7, 0x76, 0x2, 0x2, 0x295, 0x296, 
       0x7, 0x44, 0x2, 0x2, 0x296, 0x297, 0x7, 0x7a, 0x2, 0x2, 0x297, 0x55, 
       0x3, 0x2, 0x2, 0x2, 0x298, 0x29e, 0x5, 0x58, 0x2d, 0x2, 0x299, 0x29e, 
       0x5, 0x5a, 0x2e, 0x2, 0x29a, 0x29e, 0x5, 0x5c, 0x2f, 0x2, 0x29b, 
       0x29e, 0x5, 0x5e, 0x30, 0x2, 0x29c, 0x29e, 0x5, 0x6, 0x4, 0x2, 0x29d, 
       0x298, 0x3, 0x2, 0x2, 0x2, 0x29d, 0x299, 0x3, 0x2, 0x2, 0x2, 0x29d, 
       0x29a, 0x3, 0x2, 0x2, 0x2, 0x29d, 0x29b, 0x3, 0x2, 0x2, 0x2, 0x29d, 
       0x29c, 0x3, 0x2, 0x2, 0x2, 0x29e, 0x29f, 0x3, 0x2, 0x2, 0x2, 0x29f, 
       0x2a0, 0x7, 0x7a, 0x2, 0x2, 0x2a0, 0x57, 0x3, 0x2, 0x2, 0x2, 0x2a1, 
       0x2a2, 0x7, 0x45, 0x2, 0x2, 0x2a2, 0x2a3, 0x5, 0xa4, 0x53, 0x2, 0x2a3, 
       0x2a4, 0x7, 0x7d, 0x2, 0x2, 0x2a4, 0x2a5, 0x5, 0xa4, 0x53, 0x2, 0x2a5, 
       0x2a6, 0x7, 0x7d, 0x2, 0x2, 0x2a6, 0x2a7, 0x5, 0xa4, 0x53, 0x2, 0x2a7, 
       0x2a8, 0x7, 0x7d, 0x2, 0x2, 0x2a8, 0x2a9, 0x5, 0xa4, 0x53, 0x2, 0x2a9, 
       0x59, 0x3, 0x2, 0x2, 0x2, 0x2aa, 0x2ab, 0x7, 0x46, 0x2, 0x2, 0x2ab, 
       0x2ad, 0x5, 0x9c, 0x4f, 0x2, 0x2ac, 0x2ae, 0x7, 0x87, 0x2, 0x2, 0x2ad, 
       0x2ac, 0x3, 0x2, 0x2, 0x2, 0x2ae, 0x2af, 0x3, 0x2, 0x2, 0x2, 0x2af, 
       0x2ad, 0x3, 0x2, 0x2, 0x2, 0x2af, 0x2b0, 0x3, 0x2, 0x2, 0x2, 0x2b0, 
       0x5b, 0x3, 0x2, 0x2, 0x2, 0x2b1, 0x2b2, 0x7, 0x47, 0x2, 0x2, 0x2b2, 
       0x2b4, 0x5, 0x9c, 0x4f, 0x2, 0x2b3, 0x2b5, 0x7, 0x87, 0x2, 0x2, 0x2b4, 
       0x2b3, 0x3, 0x2, 0x2, 0x2, 0x2b5, 0x2b6, 0x3, 0x2, 0x2, 0x2, 0x2b6, 
       0x2b4, 0x3, 0x2, 0x2, 0x2, 0x2b6, 0x2b7, 0x3, 0x2, 0x2, 0x2, 0x2b7, 
       0x5d, 0x3, 0x2, 0x2, 0x2, 0x2b8, 0x2b9, 0x7, 0x48, 0x2, 0x2, 0x2b9, 
       0x2bb, 0x5, 0x9c, 0x4f, 0x2, 0x2ba, 0x2bc, 0x7, 0x87, 0x2, 0x2, 0x2bb, 
       0x2ba, 0x3, 0x2, 0x2, 0x2, 0x2bc, 0x2bd, 0x3, 0x2, 0x2, 0x2, 0x2bd, 
       0x2bb, 0x3, 0x2, 0x2, 0x2, 0x2bd, 0x2be, 0x3, 0x2, 0x2, 0x2, 0x2be, 
       0x5f, 0x3, 0x2, 0x2, 0x2, 0x2bf, 0x2c0, 0x7, 0x49, 0x2, 0x2, 0x2c0, 
       0x2c2, 0x7, 0x75, 0x2, 0x2, 0x2c1, 0x2c3, 0x5, 0x62, 0x32, 0x2, 0x2c2, 
       0x2c1, 0x3, 0x2, 0x2, 0x2, 0x2c3, 0x2c4, 0x3, 0x2, 0x2, 0x2, 0x2c4, 
       0x2c2, 0x3, 0x2, 0x2, 0x2, 0x2c4, 0x2c5, 0x3, 0x2, 0x2, 0x2, 0x2c5, 
       0x2c6, 0x3, 0x2, 0x2, 0x2, 0x2c6, 0x2c7, 0x7, 0x76, 0x2, 0x2, 0x2c7, 
       0x2c8, 0x7, 0x49, 0x2, 0x2, 0x2c8, 0x2c9, 0x7, 0x7a, 0x2, 0x2, 0x2c9, 
       0x61, 0x3, 0x2, 0x2, 0x2, 0x2ca, 0x2cd, 0x5, 0x64, 0x33, 0x2, 0x2cb, 
       0x2cd, 0x5, 0x6, 0x4, 0x2, 0x2cc, 0x2ca, 0x3, 0x2, 0x2, 0x2, 0x2cc, 
       0x2cb, 0x3, 0x2, 0x2, 0x2, 0x2cd, 0x2ce, 0x3, 0x2, 0x2, 0x2, 0x2ce, 
       0x2cf, 0x7, 0x7a, 0x2, 0x2, 0x2cf, 0x63, 0x3, 0x2, 0x2, 0x2, 0x2d0, 
       0x2d1, 0x7, 0x4a, 0x2, 0x2, 0x2d1, 0x2d2, 0x7, 0x84, 0x2, 0x2, 0x2d2, 
       0x65, 0x3, 0x2, 0x2, 0x2, 0x2d3, 0x2d4, 0x7, 0x4b, 0x2, 0x2, 0x2d4, 
       0x2d8, 0x7, 0x75, 0x2, 0x2, 0x2d5, 0x2d7, 0x5, 0x68, 0x35, 0x2, 0x2d6, 
       0x2d5, 0x3, 0x2, 0x2, 0x2, 0x2d7, 0x2da, 0x3, 0x2, 0x2, 0x2, 0x2d8, 
       0x2d6, 0x3, 0x2, 0x2, 0x2, 0x2d8, 0x2d9, 0x3, 0x2, 0x2, 0x2, 0x2d9, 
       0x2db, 0x3, 0x2, 0x2, 0x2, 0x2da, 0x2d8, 0x3, 0x2, 0x2, 0x2, 0x2db, 
       0x2dc, 0x7, 0x76, 0x2, 0x2, 0x2dc, 0x2dd, 0x7, 0x4b, 0x2, 0x2, 0x2dd, 
       0x2de, 0x7, 0x7a, 0x2, 0x2, 0x2de, 0x67, 0x3, 0x2, 0x2, 0x2, 0x2df, 
       0x2e2, 0x5, 0x6a, 0x36, 0x2, 0x2e0, 0x2e2, 0x5, 0x6, 0x4, 0x2, 0x2e1, 
       0x2df, 0x3, 0x2, 0x2, 0x2, 0x2e1, 0x2e0, 0x3, 0x2, 0x2, 0x2, 0x2e2, 
       0x2e3, 0x3, 0x2, 0x2, 0x2, 0x2e3, 0x2e4, 0x7, 0x7a, 0x2, 0x2, 0x2e4, 
       0x69, 0x3, 0x2, 0x2, 0x2, 0x2e5, 0x2e6, 0x9, 0x7, 0x2, 0x2, 0x2e6, 
       0x2e7, 0x7, 0x87, 0x2, 0x2, 0x2e7, 0x6b, 0x3, 0x2, 0x2, 0x2, 0x2e8, 
       0x2e9, 0x7, 0x6e, 0x2, 0x2, 0x2e9, 0x2ed, 0x7, 0x75, 0x2, 0x2, 0x2ea, 
       0x2ec, 0x5, 0x6e, 0x38, 0x2, 0x2eb, 0x2ea, 0x3, 0x2, 0x2, 0x2, 0x2ec, 
       0x2ef, 0x3, 0x2, 0x2, 0x2, 0x2ed, 0x2eb, 0x3, 0x2, 0x2, 0x2, 0x2ed, 
       0x2ee, 0x3, 0x2, 0x2, 0x2, 0x2ee, 0x2f0, 0x3, 0x2, 0x2, 0x2, 0x2ef, 
       0x2ed, 0x3, 0x2, 0x2, 0x2, 0x2f0, 0x2f1, 0x7, 0x76, 0x2, 0x2, 0x2f1, 
       0x2f2, 0x7, 0x6e, 0x2, 0x2, 0x2f2, 0x2f3, 0x7, 0x7a, 0x2, 0x2, 0x2f3, 
       0x6d, 0x3, 0x2, 0x2, 0x2, 0x2f4, 0x2f7, 0x5, 0x70, 0x39, 0x2, 0x2f5, 
       0x2f7, 0x5, 0x6, 0x4, 0x2, 0x2f6, 0x2f4, 0x3, 0x2, 0x2, 0x2, 0x2f6, 
       0x2f5, 0x3, 0x2, 0x2, 0x2, 0x2f7, 0x2f8, 0x3, 0x2, 0x2, 0x2, 0x2f8, 
       0x2f9, 0x7, 0x7a, 0x2, 0x2, 0x2f9, 0x6f, 0x3, 0x2, 0x2, 0x2, 0x2fa, 
       0x2fb, 0x9, 0x8, 0x2, 0x2, 0x2fb, 0x2fc, 0x7, 0x87, 0x2, 0x2, 0x2fc, 
       0x71, 0x3, 0x2, 0x2, 0x2, 0x2fd, 0x2fe, 0x7, 0x50, 0x2, 0x2, 0x2fe, 
       0x300, 0x7, 0x75, 0x2, 0x2, 0x2ff, 0x301, 0x5, 0x74, 0x3b, 0x2, 0x300, 
       0x2ff, 0x3, 0x2, 0x2, 0x2, 0x301, 0x302, 0x3, 0x2, 0x2, 0x2, 0x302, 
       0x300, 0x3, 0x2, 0x2, 0x2, 0x302, 0x303, 0x3, 0x2, 0x2, 0x2, 0x303, 
       0x304, 0x3, 0x2, 0x2, 0x2, 0x304, 0x305, 0x7, 0x76, 0x2, 0x2, 0x305, 
       0x306, 0x7, 0x50, 0x2, 0x2, 0x306, 0x307, 0x7, 0x7a, 0x2, 0x2, 0x307, 
       0x73, 0x3, 0x2, 0x2, 0x2, 0x308, 0x30b, 0x5, 0x76, 0x3c, 0x2, 0x309, 
       0x30b, 0x5, 0x6, 0x4, 0x2, 0x30a, 0x308, 0x3, 0x2, 0x2, 0x2, 0x30a, 
       0x309, 0x3, 0x2, 0x2, 0x2, 0x30b, 0x30c, 0x3, 0x2, 0x2, 0x2, 0x30c, 
       0x30d, 0x7, 0x7a, 0x2, 0x2, 0x30d, 0x75, 0x3, 0x2, 0x2, 0x2, 0x30e, 
       0x30f, 0x7, 0x51, 0x2, 0x2, 0x30f, 0x316, 0x5, 0xb6, 0x5c, 0x2, 0x310, 
       0x314, 0x5, 0xb6, 0x5c, 0x2, 0x311, 0x312, 0x5, 0xb6, 0x5c, 0x2, 
       0x312, 0x313, 0x5, 0xb6, 0x5c, 0x2, 0x313, 0x315, 0x3, 0x2, 0x2, 
       0x2, 0x314, 0x311, 0x3, 0x2, 0x2, 0x2, 0x314, 0x315, 0x3, 0x2, 0x2, 
       0x2, 0x315, 0x317, 0x3, 0x2, 0x2, 0x2, 0x316, 0x310, 0x3, 0x2, 0x2, 
       0x2, 0x316, 0x317, 0x3, 0x2, 0x2, 0x2, 0x317, 0x318, 0x3, 0x2, 0x2, 
       0x2, 0x318, 0x319, 0x7, 0x7e, 0x2, 0x2, 0x319, 0x31a, 0x7, 0x92, 
       0x2, 0x2, 0x31a, 0x31b, 0x7, 0x93, 0x2, 0x2, 0x31b, 0x77, 0x3, 0x2, 
       0x2, 0x2, 0x31c, 0x31d, 0x7, 0x52, 0x2, 0x2, 0x31d, 0x31f, 0x7, 0x75, 
       0x2, 0x2, 0x31e, 0x320, 0x5, 0x7a, 0x3e, 0x2, 0x31f, 0x31e, 0x3, 
       0x2, 0x2, 0x2, 0x320, 0x321, 0x3, 0x2, 0x2, 0x2, 0x321, 0x31f, 0x3, 
       0x2, 0x2, 0x2, 0x321, 0x322, 0x3, 0x2, 0x2, 0x2, 0x322, 0x323, 0x3, 
       0x2, 0x2, 0x2, 0x323, 0x324, 0x7, 0x76, 0x2, 0x2, 0x324, 0x325, 0x7, 
       0x52, 0x2, 0x2, 0x325, 0x326, 0x7, 0x7a, 0x2, 0x2, 0x326, 0x79, 0x3, 
       0x2, 0x2, 0x2, 0x327, 0x32a, 0x5, 0x7c, 0x3f, 0x2, 0x328, 0x32a, 
       0x5, 0x6, 0x4, 0x2, 0x329, 0x327, 0x3, 0x2, 0x2, 0x2, 0x329, 0x328, 
       0x3, 0x2, 0x2, 0x2, 0x32a, 0x32b, 0x3, 0x2, 0x2, 0x2, 0x32b, 0x32c, 
       0x7, 0x7a, 0x2, 0x2, 0x32c, 0x7b, 0x3, 0x2, 0x2, 0x2, 0x32d, 0x32e, 
       0x9, 0x9, 0x2, 0x2, 0x32e, 0x349, 0x7, 0x87, 0x2, 0x2, 0x32f, 0x330, 
       0x9, 0xa, 0x2, 0x2, 0x330, 0x349, 0x7, 0x87, 0x2, 0x2, 0x331, 0x332, 
       0x7, 0x64, 0x2, 0x2, 0x332, 0x349, 0x5, 0xb6, 0x5c, 0x2, 0x333, 0x334, 
       0x7, 0x61, 0x2, 0x2, 0x334, 0x335, 0x7, 0x7e, 0x2, 0x2, 0x335, 0x336, 
       0x7, 0x92, 0x2, 0x2, 0x336, 0x349, 0x7, 0x93, 0x2, 0x2, 0x337, 0x338, 
       0x7, 0x57, 0x2, 0x2, 0x338, 0x339, 0x7, 0x87, 0x2, 0x2, 0x339, 0x33a, 
       0x7, 0x87, 0x2, 0x2, 0x33a, 0x33b, 0x7, 0x87, 0x2, 0x2, 0x33b, 0x33c, 
       0x7, 0x87, 0x2, 0x2, 0x33c, 0x33d, 0x7, 0x87, 0x2, 0x2, 0x33d, 0x33e, 
       0x7, 0x87, 0x2, 0x2, 0x33e, 0x33f, 0x7, 0x87, 0x2, 0x2, 0x33f, 0x340, 
       0x7, 0x87, 0x2, 0x2, 0x340, 0x341, 0x7, 0x87, 0x2, 0x2, 0x341, 0x349, 
       0x7, 0x87, 0x2, 0x2, 0x342, 0x344, 0x9, 0xb, 0x2, 0x2, 0x343, 0x345, 
       0x7, 0x87, 0x2, 0x2, 0x344, 0x343, 0x3, 0x2, 0x2, 0x2, 0x345, 0x346, 
       0x3, 0x2, 0x2, 0x2, 0x346, 0x344, 0x3, 0x2, 0x2, 0x2, 0x346, 0x347, 
       0x3, 0x2, 0x2, 0x2, 0x347, 0x349, 0x3, 0x2, 0x2, 0x2, 0x348, 0x32d, 
       0x3, 0x2, 0x2, 0x2, 0x348, 0x32f, 0x3, 0x2, 0x2, 0x2, 0x348, 0x331, 
       0x3, 0x2, 0x2, 0x2, 0x348, 0x333, 0x3, 0x2, 0x2, 0x2, 0x348, 0x337, 
       0x3, 0x2, 0x2, 0x2, 0x348, 0x342, 0x3, 0x2, 0x2, 0x2, 0x349, 0x7d, 
       0x3, 0x2, 0x2, 0x2, 0x34a, 0x34b, 0x7, 0x65, 0x2, 0x2, 0x34b, 0x34d, 
       0x7, 0x75, 0x2, 0x2, 0x34c, 0x34e, 0x5, 0x80, 0x41, 0x2, 0x34d, 0x34c, 
       0x3, 0x2, 0x2, 0x2, 0x34e, 0x34f, 0x3, 0x2, 0x2, 0x2, 0x34f, 0x34d, 
       0x3, 0x2, 0x2, 0x2, 0x34f, 0x350, 0x3, 0x2, 0x2, 0x2, 0x350, 0x351, 
       0x3, 0x2, 0x2, 0x2, 0x351, 0x352, 0x7, 0x76, 0x2, 0x2, 0x352, 0x353, 
       0x7, 0x65, 0x2, 0x2, 0x353, 0x354, 0x7, 0x7a, 0x2, 0x2, 0x354, 0x7f, 
       0x3, 0x2, 0x2, 0x2, 0x355, 0x35b, 0x5, 0x82, 0x42, 0x2, 0x356, 0x35b, 
       0x5, 0x84, 0x43, 0x2, 0x357, 0x35b, 0x5, 0x8c, 0x47, 0x2, 0x358, 
       0x35b, 0x5, 0x90, 0x49, 0x2, 0x359, 0x35b, 0x5, 0x6, 0x4, 0x2, 0x35a, 
       0x355, 0x3, 0x2, 0x2, 0x2, 0x35a, 0x356, 0x3, 0x2, 0x2, 0x2, 0x35a, 
       0x357, 0x3, 0x2, 0x2, 0x2, 0x35a, 0x358, 0x3, 0x2, 0x2, 0x2, 0x35a, 
       0x359, 0x3, 0x2, 0x2, 0x2, 0x35b, 0x35c, 0x3, 0x2, 0x2, 0x2, 0x35c, 
       0x35d, 0x7, 0x7a, 0x2, 0x2, 0x35d, 0x81, 0x3, 0x2, 0x2, 0x2, 0x35e, 
       0x35f, 0x7, 0x68, 0x2, 0x2, 0x35f, 0x360, 0x5, 0xb2, 0x5a, 0x2, 0x360, 
       0x361, 0x7, 0x87, 0x2, 0x2, 0x361, 0x363, 0x7, 0x75, 0x2, 0x2, 0x362, 
       0x364, 0x5, 0x8e, 0x48, 0x2, 0x363, 0x362, 0x3, 0x2, 0x2, 0x2, 0x364, 
       0x365, 0x3, 0x2, 0x2, 0x2, 0x365, 0x363, 0x3, 0x2, 0x2, 0x2, 0x365, 
       0x366, 0x3, 0x2, 0x2, 0x2, 0x366, 0x367, 0x3, 0x2, 0x2, 0x2, 0x367, 
       0x368, 0x7, 0x76, 0x2, 0x2, 0x368, 0x83, 0x3, 0x2, 0x2, 0x2, 0x369, 
       0x36a, 0x7, 0x69, 0x2, 0x2, 0x36a, 0x36c, 0x7, 0x75, 0x2, 0x2, 0x36b, 
       0x36d, 0x5, 0x86, 0x44, 0x2, 0x36c, 0x36b, 0x3, 0x2, 0x2, 0x2, 0x36d, 
       0x36e, 0x3, 0x2, 0x2, 0x2, 0x36e, 0x36c, 0x3, 0x2, 0x2, 0x2, 0x36e, 
       0x36f, 0x3, 0x2, 0x2, 0x2, 0x36f, 0x370, 0x3, 0x2, 0x2, 0x2, 0x370, 
       0x371, 0x7, 0x76, 0x2, 0x2, 0x371, 0x85, 0x3, 0x2, 0x2, 0x2, 0x372, 
       0x377, 0x5, 0x92, 0x4a, 0x2, 0x373, 0x377, 0x5, 0x88, 0x45, 0x2, 
       0x374, 0x377, 0x5, 0x8a, 0x46, 0x2, 0x375, 0x377, 0x5, 0x6, 0x4, 
       0x2, 0x376, 0x372, 0x3, 0x2, 0x2, 0x2, 0x376, 0x373, 0x3, 0x2, 0x2, 
       0x2, 0x376, 0x374, 0x3, 0x2, 0x2, 0x2, 0x376, 0x375, 0x3, 0x2, 0x2, 
       0x2, 0x377, 0x378, 0x3, 0x2, 0x2, 0x2, 0x378, 0x379, 0x7, 0x7a, 0x2, 
       0x2, 0x379, 0x87, 0x3, 0x2, 0x2, 0x2, 0x37a, 0x37b, 0x7, 0x6b, 0x2, 
       0x2, 0x37b, 0x37c, 0x5, 0xb2, 0x5a, 0x2, 0x37c, 0x381, 0x5, 0xb4, 
       0x5b, 0x2, 0x37d, 0x37f, 0x5, 0xb4, 0x5b, 0x2, 0x37e, 0x380, 0x5, 
       0xb4, 0x5b, 0x2, 0x37f, 0x37e, 0x3, 0x2, 0x2, 0x2, 0x37f, 0x380, 
       0x3, 0x2, 0x2, 0x2, 0x380, 0x382, 0x3, 0x2, 0x2, 0x2, 0x381, 0x37d, 
       0x3, 0x2, 0x2, 0x2, 0x381, 0x382, 0x3, 0x2, 0x2, 0x2, 0x382, 0x89, 
       0x3, 0x2, 0x2, 0x2, 0x383, 0x385, 0x7, 0x6a, 0x2, 0x2, 0x384, 0x386, 
       0x9, 0xc, 0x2, 0x2, 0x385, 0x384, 0x3, 0x2, 0x2, 0x2, 0x386, 0x387, 
       0x3, 0x2, 0x2, 0x2, 0x387, 0x385, 0x3, 0x2, 0x2, 0x2, 0x387, 0x388, 
       0x3, 0x2, 0x2, 0x2, 0x388, 0x8b, 0x3, 0x2, 0x2, 0x2, 0x389, 0x38a, 
       0x7, 0x66, 0x2, 0x2, 0x38a, 0x38c, 0x7, 0x75, 0x2, 0x2, 0x38b, 0x38d, 
       0x5, 0x8e, 0x48, 0x2, 0x38c, 0x38b, 0x3, 0x2, 0x2, 0x2, 0x38d, 0x38e, 
       0x3, 0x2, 0x2, 0x2, 0x38e, 0x38c, 0x3, 0x2, 0x2, 0x2, 0x38e, 0x38f, 
       0x3, 0x2, 0x2, 0x2, 0x38f, 0x390, 0x3, 0x2, 0x2, 0x2, 0x390, 0x391, 
       0x7, 0x76, 0x2, 0x2, 0x391, 0x8d, 0x3, 0x2, 0x2, 0x2, 0x392, 0x395, 
       0x5, 0x92, 0x4a, 0x2, 0x393, 0x395, 0x5, 0x6, 0x4, 0x2, 0x394, 0x392, 
       0x3, 0x2, 0x2, 0x2, 0x394, 0x393, 0x3, 0x2, 0x2, 0x2, 0x395, 0x396, 
       0x3, 0x2, 0x2, 0x2, 0x396, 0x397, 0x7, 0x7a, 0x2, 0x2, 0x397, 0x8f, 
       0x3, 0x2, 0x2, 0x2, 0x398, 0x399, 0x7, 0x67, 0x2, 0x2, 0x399, 0x39a, 
       0x5, 0xb6, 0x5c, 0x2, 0x39a, 0x91, 0x3, 0x2, 0x2, 0x2, 0x39b, 0x3a2, 
       0x7, 0x50, 0x2, 0x2, 0x39c, 0x3a0, 0x5, 0xb6, 0x5c, 0x2, 0x39d, 0x39e, 
       0x5, 0xb6, 0x5c, 0x2, 0x39e, 0x39f, 0x5, 0xb6, 0x5c, 0x2, 0x39f, 
       0x3a1, 0x3, 0x2, 0x2, 0x2, 0x3a0, 0x39d, 0x3, 0x2, 0x2, 0x2, 0x3a0, 
       0x3a1, 0x3, 0x2, 0x2, 0x2, 0x3a1, 0x3a3, 0x3, 0x2, 0x2, 0x2, 0x3a2, 
       0x39c, 0x3, 0x2, 0x2, 0x2, 0x3a2, 0x3a3, 0x3, 0x2, 0x2, 0x2, 0x3a3, 
       0x3a4, 0x3, 0x2, 0x2, 0x2, 0x3a4, 0x3a5, 0x7, 0x7e, 0x2, 0x2, 0x3a5, 
       0x3a6, 0x7, 0x92, 0x2, 0x2, 0x3a6, 0x3a7, 0x7, 0x93, 0x2, 0x2, 0x3a7, 
       0x93, 0x3, 0x2, 0x2, 0x2, 0x3a8, 0x3a9, 0x7, 0x72, 0x2, 0x2, 0x3a9, 
       0x3ab, 0x7, 0x75, 0x2, 0x2, 0x3aa, 0x3ac, 0x5, 0x96, 0x4c, 0x2, 0x3ab, 
       0x3aa, 0x3, 0x2, 0x2, 0x2, 0x3ac, 0x3ad, 0x3, 0x2, 0x2, 0x2, 0x3ad, 
       0x3ab, 0x3, 0x2, 0x2, 0x2, 0x3ad, 0x3ae, 0x3, 0x2, 0x2, 0x2, 0x3ae, 
       0x3af, 0x3, 0x2, 0x2, 0x2, 0x3af, 0x3b0, 0x7, 0x76, 0x2, 0x2, 0x3b0, 
       0x3b1, 0x7, 0x72, 0x2, 0x2, 0x3b1, 0x3b2, 0x7, 0x7a, 0x2, 0x2, 0x3b2, 
       0x95, 0x3, 0x2, 0x2, 0x2, 0x3b3, 0x3b6, 0x5, 0x98, 0x4d, 0x2, 0x3b4, 
       0x3b6, 0x5, 0x6, 0x4, 0x2, 0x3b5, 0x3b3, 0x3, 0x2, 0x2, 0x2, 0x3b5, 
       0x3b4, 0x3, 0x2, 0x2, 0x2, 0x3b6, 0x3b7, 0x3, 0x2, 0x2, 0x2, 0x3b7, 
       0x3b8, 0x7, 0x7a, 0x2, 0x2, 0x3b8, 0x97, 0x3, 0x2, 0x2, 0x2, 0x3b9, 
       0x3ba, 0x9, 0xd, 0x2, 0x2, 0x3ba, 0x3bb, 0x5, 0xac, 0x57, 0x2, 0x3bb, 
       0x3bc, 0x7, 0x87, 0x2, 0x2, 0x3bc, 0x99, 0x3, 0x2, 0x2, 0x2, 0x3bd, 
       0x3be, 0x7, 0x1c, 0x2, 0x2, 0x3be, 0x3c7, 0x7, 0x34, 0x2, 0x2, 0x3bf, 
       0x3c0, 0x7, 0x87, 0x2, 0x2, 0x3c0, 0x3c3, 0x7, 0x87, 0x2, 0x2, 0x3c1, 
       0x3c2, 0x7, 0x33, 0x2, 0x2, 0x3c2, 0x3c4, 0x7, 0x87, 0x2, 0x2, 0x3c3, 
       0x3c1, 0x3, 0x2, 0x2, 0x2, 0x3c3, 0x3c4, 0x3, 0x2, 0x2, 0x2, 0x3c4, 
       0x3c8, 0x3, 0x2, 0x2, 0x2, 0x3c5, 0x3c8, 0x7, 0x3e, 0x2, 0x2, 0x3c6, 
       0x3c8, 0x5, 0xb0, 0x59, 0x2, 0x3c7, 0x3bf, 0x3, 0x2, 0x2, 0x2, 0x3c7, 
       0x3c5, 0x3, 0x2, 0x2, 0x2, 0x3c7, 0x3c6, 0x3, 0x2, 0x2, 0x2, 0x3c8, 
       0x3c9, 0x3, 0x2, 0x2, 0x2, 0x3c9, 0x3ca, 0x7, 0x1d, 0x2, 0x2, 0x3ca, 
       0x9b, 0x3, 0x2, 0x2, 0x2, 0x3cb, 0x3cd, 0x5, 0x9e, 0x50, 0x2, 0x3cc, 
       0x3cb, 0x3, 0x2, 0x2, 0x2, 0x3cd, 0x3ce, 0x3, 0x2, 0x2, 0x2, 0x3ce, 
       0x3cc, 0x3, 0x2, 0x2, 0x2, 0x3ce, 0x3cf, 0x3, 0x2, 0x2, 0x2, 0x3cf, 
       0x9d, 0x3, 0x2, 0x2, 0x2, 0x3d0, 0x3d5, 0x5, 0xa2, 0x52, 0x2, 0x3d1, 
       0x3d2, 0x7, 0xe, 0x2, 0x2, 0x3d2, 0x3d4, 0x5, 0xb0, 0x59, 0x2, 0x3d3, 
       0x3d1, 0x3, 0x2, 0x2, 0x2, 0x3d4, 0x3d7, 0x3, 0x2, 0x2, 0x2, 0x3d5, 
       0x3d3, 0x3, 0x2, 0x2, 0x2, 0x3d5, 0x3d6, 0x3, 0x2, 0x2, 0x2, 0x3d6, 
       0x9f, 0x3, 0x2, 0x2, 0x2, 0x3d7, 0x3d5, 0x3, 0x2, 0x2, 0x2, 0x3d8, 
       0x3da, 0x5, 0xa2, 0x52, 0x2, 0x3d9, 0x3d8, 0x3, 0x2, 0x2, 0x2, 0x3da, 
       0x3db, 0x3, 0x2, 0x2, 0x2, 0x3db, 0x3d9, 0x3, 0x2, 0x2, 0x2, 0x3db, 
       0x3dc, 0x3, 0x2, 0x2, 0x2, 0x3dc, 0xa1, 0x3, 0x2, 0x2, 0x2, 0x3dd, 
       0x3e0, 0x5, 0xa6, 0x54, 0x2, 0x3de, 0x3e0, 0x5, 0xac, 0x57, 0x2, 
       0x3df, 0x3dd, 0x3, 0x2, 0x2, 0x2, 0x3df, 0x3de, 0x3, 0x2, 0x2, 0x2, 
       0x3e0, 0x3e2, 0x3, 0x2, 0x2, 0x2, 0x3e1, 0x3e3, 0x7, 0x7c, 0x2, 0x2, 
       0x3e2, 0x3e1, 0x3, 0x2, 0x2, 0x2, 0x3e2, 0x3e3, 0x3, 0x2, 0x2, 0x2, 
       0x3e3, 0xa3, 0x3, 0x2, 0x2, 0x2, 0x3e4, 0x3e6, 0x5, 0xa6, 0x54, 0x2, 
       0x3e5, 0x3e4, 0x3, 0x2, 0x2, 0x2, 0x3e5, 0x3e6, 0x3, 0x2, 0x2, 0x2, 
       0x3e6, 0xa5, 0x3, 0x2, 0x2, 0x2, 0x3e7, 0x3ea, 0x7, 0x7f, 0x2, 0x2, 
       0x3e8, 0x3ea, 0x5, 0xa8, 0x55, 0x2, 0x3e9, 0x3e7, 0x3, 0x2, 0x2, 
       0x2, 0x3e9, 0x3e8, 0x3, 0x2, 0x2, 0x2, 0x3ea, 0xa7, 0x3, 0x2, 0x2, 
       0x2, 0x3eb, 0x3ed, 0x7, 0x77, 0x2, 0x2, 0x3ec, 0x3ee, 0x5, 0xaa, 
       0x56, 0x2, 0x3ed, 0x3ec, 0x3, 0x2, 0x2, 0x2, 0x3ee, 0x3ef, 0x3, 0x2, 
       0x2, 0x2, 0x3ef, 0x3ed, 0x3, 0x2, 0x2, 0x2, 0x3ef, 0x3f0, 0x3, 0x2, 
       0x2, 0x2, 0x3f0, 0x3f1, 0x3, 0x2, 0x2, 0x2, 0x3f1, 0x3f2, 0x7, 0x78, 
       0x2, 0x2, 0x3f2, 0xa9, 0x3, 0x2, 0x2, 0x2, 0x3f3, 0x3f6, 0x5, 0xac, 
       0x57, 0x2, 0x3f4, 0x3f5, 0x7, 0x79, 0x2, 0x2, 0x3f5, 0x3f7, 0x5, 
       0xac, 0x57, 0x2, 0x3f6, 0x3f4, 0x3, 0x2, 0x2, 0x2, 0x3f6, 0x3f7, 
       0x3, 0x2, 0x2, 0x2, 0x3f7, 0x3fa, 0x3, 0x2, 0x2, 0x2, 0x3f8, 0x3fa, 
       0x7, 0x7f, 0x2, 0x2, 0x3f9, 0x3f3, 0x3, 0x2, 0x2, 0x2, 0x3f9, 0x3f8, 
       0x3, 0x2, 0x2, 0x2, 0x3fa, 0xab, 0x3, 0x2, 0x2, 0x2, 0x3fb, 0x3fe, 
       0x5, 0xae, 0x58, 0x2, 0x3fc, 0x3fe, 0x7, 0x80, 0x2, 0x2, 0x3fd, 0x3fb, 
       0x3, 0x2, 0x2, 0x2, 0x3fd, 0x3fc, 0x3, 0x2, 0x2, 0x2, 0x3fe, 0xad, 
       0x3, 0x2, 0x2, 0x2, 0x3ff, 0x400, 0x9, 0xe, 0x2, 0x2, 0x400, 0xaf, 
       0x3, 0x2, 0x2, 0x2, 0x401, 0x402, 0x9, 0xf, 0x2, 0x2, 0x402, 0xb1, 
       0x3, 0x2, 0x2, 0x2, 0x403, 0x404, 0x9, 0x10, 0x2, 0x2, 0x404, 0xb3, 
       0x3, 0x2, 0x2, 0x2, 0x405, 0x406, 0x9, 0x11, 0x2, 0x2, 0x406, 0xb5, 
       0x3, 0x2, 0x2, 0x2, 0x407, 0x408, 0x9, 0x12, 0x2, 0x2, 0x408, 0xb7, 
       0x3, 0x2, 0x2, 0x2, 0x409, 0x40b, 0x5, 0x1a, 0xe, 0x2, 0x40a, 0x409, 
       0x3, 0x2, 0x2, 0x2, 0x40b, 0x40e, 0x3, 0x2, 0x2, 0x2, 0x40c, 0x40a, 
       0x3, 0x2, 0x2, 0x2, 0x40c, 0x40d, 0x3, 0x2, 0x2, 0x2, 0x40d, 0x40f, 
       0x3, 0x2, 0x2, 0x2, 0x40e, 0x40c, 0x3, 0x2, 0x2, 0x2, 0x40f, 0x410, 
       0x7, 0x2, 0x2, 0x3, 0x410, 0xb9, 0x3, 0x2, 0x2, 0x2, 0x411, 0x413, 
       0x5, 0x24, 0x13, 0x2, 0x412, 0x411, 0x3, 0x2, 0x2, 0x2, 0x413, 0x416, 
       0x3, 0x2, 0x2, 0x2, 0x414, 0x412, 0x3, 0x2, 0x2, 0x2, 0x414, 0x415, 
       0x3, 0x2, 0x2, 0x2, 0x415, 0x417, 0x3, 0x2, 0x2, 0x2, 0x416, 0x414, 
       0x3, 0x2, 0x2, 0x2, 0x417, 0x418, 0x7, 0x2, 0x2, 0x3, 0x418, 0xbb, 
       0x3, 0x2, 0x2, 0x2, 0x419, 0x41b, 0x5, 0x20, 0x11, 0x2, 0x41a, 0x419, 
       0x3, 0x2, 0x2, 0x2, 0x41b, 0x41e, 0x3, 0x2, 0x2, 0x2, 0x41c, 0x41a, 
       0x3, 0x2, 0x2, 0x2, 0x41c, 0x41d, 0x3, 0x2, 0x2, 0x2, 0x41d, 0x41f, 
       0x3, 0x2, 0x2, 0x2, 0x41e, 0x41c, 0x3, 0x2, 0x2, 0x2, 0x41f, 0x420, 
       0x7, 0x2, 0x2, 0x3, 0x420, 0xbd, 0x3, 0x2, 0x2, 0x2, 0x421, 0x423, 
       0x5, 0x4c, 0x27, 0x2, 0x422, 0x421, 0x3, 0x2, 0x2, 0x2, 0x423, 0x426, 
       0x3, 0x2, 0x2, 0x2, 0x424, 0x422, 0x3, 0x2, 0x2, 0x2, 0x424, 0x425, 
       0x3, 0x2, 0x2, 0x2, 0x425, 0x427, 0x3, 0x2, 0x2, 0x2, 0x426, 0x424, 
       0x3, 0x2, 0x2, 0x2, 0x427, 0x428, 0x7, 0x2, 0x2, 0x3, 0x428, 0xbf, 
       0x3, 0x2, 0x2, 0x2, 0x429, 0x42b, 0x5, 0x62, 0x32, 0x2, 0x42a, 0x429, 
       0x3, 0x2, 0x2, 0x2, 0x42b, 0x42e, 0x3, 0x2, 0x2, 0x2, 0x42c, 0x42a, 
       0x3, 0x2, 0x2, 0x2, 0x42c, 0x42d, 0x3, 0x2, 0x2, 0x2, 0x42d, 0x42f, 
       0x3, 0x2, 0x2, 0x2, 0x42e, 0x42c, 0x3, 0x2, 0x2, 0x2, 0x42f, 0x430, 
       0x7, 0x2, 0x2, 0x3, 0x430, 0xc1, 0x3, 0x2, 0x2, 0x2, 0x431, 0x433, 
       0x5, 0x68, 0x35, 0x2, 0x432, 0x431, 0x3, 0x2, 0x2, 0x2, 0x433, 0x436, 
       0x3, 0x2, 0x2, 0x2, 0x434, 0x432, 0x3, 0x2, 0x2, 0x2, 0x434, 0x435, 
       0x3, 0x2, 0x2, 0x2, 0x435, 0x437, 0x3, 0x2, 0x2, 0x2, 0x436, 0x434, 
       0x3, 0x2, 0x2, 0x2, 0x437, 0x438, 0x7, 0x2, 0x2, 0x3, 0x438, 0xc3, 
       0x3, 0x2, 0x2, 0x2, 0x439, 0x43b, 0x5, 0x6e, 0x38, 0x2, 0x43a, 0x439, 
       0x3, 0x2, 0x2, 0x2, 0x43b, 0x43e, 0x3, 0x2, 0x2, 0x2, 0x43c, 0x43a, 
       0x3, 0x2, 0x2, 0x2, 0x43c, 0x43d, 0x3, 0x2, 0x2, 0x2, 0x43d, 0x43f, 
       0x3, 0x2, 0x2, 0x2, 0x43e, 0x43c, 0x3, 0x2, 0x2, 0x2, 0x43f, 0x440, 
       0x7, 0x2, 0x2, 0x3, 0x440, 0xc5, 0x3, 0x2, 0x2, 0x2, 0x441, 0x443, 
       0x5, 0x56, 0x2c, 0x2, 0x442, 0x441, 0x3, 0x2, 0x2, 0x2, 0x443, 0x446, 
       0x3, 0x2, 0x2, 0x2, 0x444, 0x442, 0x3, 0x2, 0x2, 0x2, 0x444, 0x445, 
       0x3, 0x2, 0x2, 0x2, 0x445, 0x447, 0x3, 0x2, 0x2, 0x2, 0x446, 0x444, 
       0x3, 0x2, 0x2, 0x2, 0x447, 0x448, 0x7, 0x2, 0x2, 0x3, 0x448, 0xc7, 
       0x3, 0x2, 0x2, 0x2, 0x449, 0x44b, 0x5, 0x74, 0x3b, 0x2, 0x44a, 0x449, 
       0x3, 0x2, 0x2, 0x2, 0x44b, 0x44e, 0x3, 0x2, 0x2, 0x2, 0x44c, 0x44a, 
       0x3, 0x2, 0x2, 0x2, 0x44c, 0x44d, 0x3, 0x2, 0x2, 0x2, 0x44d, 0x44f, 
       0x3, 0x2, 0x2, 0x2, 0x44e, 0x44c, 0x3, 0x2, 0x2, 0x2, 0x44f, 0x450, 
       0x7, 0x2, 0x2, 0x3, 0x450, 0xc9, 0x3, 0x2, 0x2, 0x2, 0x451, 0x453, 
       0x5, 0x96, 0x4c, 0x2, 0x452, 0x451, 0x3, 0x2, 0x2, 0x2, 0x453, 0x456, 
       0x3, 0x2, 0x2, 0x2, 0x454, 0x452, 0x3, 0x2, 0x2, 0x2, 0x454, 0x455, 
       0x3, 0x2, 0x2, 0x2, 0x455, 0x457, 0x3, 0x2, 0x2, 0x2, 0x456, 0x454, 
       0x3, 0x2, 0x2, 0x2, 0x457, 0x458, 0x7, 0x2, 0x2, 0x3, 0x458, 0xcb, 
       0x3, 0x2, 0x2, 0x2, 0x459, 0x45b, 0x5, 0x7a, 0x3e, 0x2, 0x45a, 0x459, 
       0x3, 0x2, 0x2, 0x2, 0x45b, 0x45e, 0x3, 0x2, 0x2, 0x2, 0x45c, 0x45a, 
       0x3, 0x2, 0x2, 0x2, 0x45c, 0x45d, 0x3, 0x2, 0x2, 0x2, 0x45d, 0x45f, 
       0x3, 0x2, 0x2, 0x2, 0x45e, 0x45c, 0x3, 0x2, 0x2, 0x2, 0x45f, 0x460, 
       0x7, 0x2, 0x2, 0x3, 0x460, 0xcd, 0x3, 0x2, 0x2, 0x2, 0x461, 0x463, 
       0x5, 0x80, 0x41, 0x2, 0x462, 0x461, 0x3, 0x2, 0x2, 0x2, 0x463, 0x466, 
       0x3, 0x2, 0x2, 0x2, 0x464, 0x462, 0x3, 0x2, 0x2, 0x2, 0x464, 0x465, 
       0x3, 0x2, 0x2, 0x2, 0x465, 0x467, 0x3, 0x2, 0x2, 0x2, 0x466, 0x464, 
       0x3, 0x2, 0x2, 0x2, 0x467, 0x468, 0x7, 0x2, 0x2, 0x3, 0x468, 0xcf, 
       0x3, 0x2, 0x2, 0x2, 0x469, 0x46b, 0x5, 0x86, 0x44, 0x2, 0x46a, 0x469, 
       0x3, 0x2, 0x2, 0x2, 0x46b, 0x46e, 0x3, 0x2, 0x2, 0x2, 0x46c, 0x46a, 
       0x3, 0x2, 0x2, 0x2, 0x46c, 0x46d, 0x3, 0x2, 0x2, 0x2, 0x46d, 0x46f, 
       0x3, 0x2, 0x2, 0x2, 0x46e, 0x46c, 0x3, 0x2, 0x2, 0x2, 0x46f, 0x470, 
       0x7, 0x2, 0x2, 0x3, 0x470, 0xd1, 0x3, 0x2, 0x2, 0x2, 0x471, 0x473, 
       0x5, 0x8e, 0x48, 0x2, 0x472, 0x471, 0x3, 0x2, 0x2, 0x2, 0x473, 0x476, 
       0x3, 0x2, 0x2, 0x2, 0x474, 0x472, 0x3, 0x2, 0x2, 0x2, 0x474, 0x475, 
       0x3, 0x2, 0x2, 0x2, 0x475, 0x477, 0x3, 0x2, 0x2, 0x2, 0x476, 0x474, 
       0x3, 0x2, 0x2, 0x2, 0x477, 0x478, 0x7, 0x2, 0x2, 0x3, 0x478, 0xd3, 
       0x3, 0x2, 0x2, 0x2, 0x479, 0x47a, 0x9, 0x13, 0x2, 0x2, 0x47a, 0xd5, 
       0x3, 0x2, 0x2, 0x2, 0x47b, 0x47c, 0x9, 0x14, 0x2, 0x2, 0x47c, 0xd7, 
       0x3, 0x2, 0x2, 0x2, 0x47d, 0x47e, 0x9, 0x15, 0x2, 0x2, 0x47e, 0xd9, 
       0x3, 0x2, 0x2, 0x2, 0x47f, 0x480, 0x9, 0x16, 0x2, 0x2, 0x480, 0xdb, 
       0x3, 0x2, 0x2, 0x2, 0x481, 0x482, 0x9, 0x17, 0x2, 0x2, 0x482, 0xdd, 
       0x3, 0x2, 0x2, 0x2, 0x483, 0x484, 0x9, 0x18, 0x2, 0x2, 0x484, 0xdf, 
       0x3, 0x2, 0x2, 0x2, 0x7c, 0xe5, 0xe7, 0xf2, 0x106, 0x110, 0x11b, 
       0x121, 0x131, 0x139, 0x141, 0x147, 0x150, 0x155, 0x15b, 0x160, 0x169, 
       0x171, 0x17a, 0x180, 0x190, 0x19d, 0x1a4, 0x1a6, 0x1b0, 0x1b6, 0x1bd, 
       0x1c6, 0x1c9, 0x1d0, 0x1d2, 0x1d9, 0x1db, 0x1dd, 0x1e0, 0x1e4, 0x1ea, 
       0x1f1, 0x1f6, 0x1fc, 0x203, 0x206, 0x20d, 0x210, 0x217, 0x21a, 0x21c, 
       0x220, 0x227, 0x230, 0x23a, 0x23f, 0x242, 0x245, 0x24b, 0x252, 0x254, 
       0x25f, 0x26a, 0x273, 0x27b, 0x283, 0x28b, 0x292, 0x29d, 0x2af, 0x2b6, 
       0x2bd, 0x2c4, 0x2cc, 0x2d8, 0x2e1, 0x2ed, 0x2f6, 0x302, 0x30a, 0x314, 
       0x316, 0x321, 0x329, 0x346, 0x348, 0x34f, 0x35a, 0x365, 0x36e, 0x376, 
       0x37f, 0x381, 0x387, 0x38e, 0x394, 0x3a0, 0x3a2, 0x3ad, 0x3b5, 0x3c3, 
       0x3c7, 0x3ce, 0x3d5, 0x3db, 0x3df, 0x3e2, 0x3e5, 0x3e9, 0x3ef, 0x3f6, 
       0x3f9, 0x3fd, 0x40c, 0x414, 0x41c, 0x424, 0x42c, 0x434, 0x43c, 0x444, 
       0x44c, 0x454, 0x45c, 0x464, 0x46c, 0x474, 
  };

  _serializedATN.insert(_serializedATN.end(), serializedATNSegment0,
    serializedATNSegment0 + sizeof(serializedATNSegment0) / sizeof(serializedATNSegment0[0]));


  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

FeatParser::Initializer FeatParser::_init;
