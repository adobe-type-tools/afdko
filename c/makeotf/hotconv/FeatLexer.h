
// Generated from FeatLexer.g4 by ANTLR 4.9.3

#pragma once


#include "antlr4-runtime.h"




class  FeatLexer : public antlr4::Lexer {
public:
  enum {
    ANON = 1, ANON_v = 2, COMMENT = 3, WHITESPACE = 4, INCLUDE = 5, FEATURE = 6, 
    TABLE = 7, SCRIPT = 8, LANGUAGE = 9, LANGSYS = 10, SUBTABLE = 11, LOOKUP = 12, 
    LOOKUPFLAG = 13, NOTDEF = 14, RIGHT_TO_LEFT = 15, IGNORE_BASE_GLYPHS = 16, 
    IGNORE_LIGATURES = 17, IGNORE_MARKS = 18, USE_MARK_FILTERING_SET = 19, 
    MARK_ATTACHMENT_TYPE = 20, EXCLUDE_DFLT = 21, INCLUDE_DFLT = 22, EXCLUDE_dflt = 23, 
    INCLUDE_dflt = 24, USE_EXTENSION = 25, BEGINVALUE = 26, ENDVALUE = 27, 
    ENUMERATE = 28, ENUMERATE_v = 29, EXCEPT = 30, IGNORE = 31, SUBSTITUTE = 32, 
    SUBSTITUTE_v = 33, REVERSE = 34, REVERSE_v = 35, BY = 36, FROM = 37, 
    POSITION = 38, POSITION_v = 39, PARAMETERS = 40, FEATURE_NAMES = 41, 
    CV_PARAMETERS = 42, CV_UI_LABEL = 43, CV_TOOLTIP = 44, CV_SAMPLE_TEXT = 45, 
    CV_PARAM_LABEL = 46, CV_CHARACTER = 47, SIZEMENUNAME = 48, CONTOURPOINT = 49, 
    ANCHOR = 50, ANCHOR_DEF = 51, VALUE_RECORD_DEF = 52, MARK = 53, MARK_CLASS = 54, 
    CURSIVE = 55, MARKBASE = 56, MARKLIG = 57, MARKLIG_v = 58, LIG_COMPONENT = 59, 
    KNULL = 60, BASE = 61, HA_BTL = 62, VA_BTL = 63, HA_BSL = 64, VA_BSL = 65, 
    GDEF = 66, GLYPH_CLASS_DEF = 67, ATTACH = 68, LIG_CARET_BY_POS = 69, 
    LIG_CARET_BY_IDX = 70, HEAD = 71, FONT_REVISION = 72, HHEA = 73, ASCENDER = 74, 
    DESCENDER = 75, LINE_GAP = 76, CARET_OFFSET = 77, NAME = 78, NAMEID = 79, 
    OS_2 = 80, FS_TYPE = 81, FS_TYPE_v = 82, OS2_LOWER_OP_SIZE = 83, OS2_UPPER_OP_SIZE = 84, 
    PANOSE = 85, TYPO_ASCENDER = 86, TYPO_DESCENDER = 87, TYPO_LINE_GAP = 88, 
    WIN_ASCENT = 89, WIN_DESCENT = 90, X_HEIGHT = 91, CAP_HEIGHT = 92, WEIGHT_CLASS = 93, 
    WIDTH_CLASS = 94, VENDOR = 95, UNICODE_RANGE = 96, CODE_PAGE_RANGE = 97, 
    FAMILY_CLASS = 98, STAT = 99, ELIDED_FALLBACK_NAME = 100, ELIDED_FALLBACK_NAME_ID = 101, 
    DESIGN_AXIS = 102, AXIS_VALUE = 103, FLAG = 104, LOCATION = 105, AXIS_EAVN = 106, 
    AXIS_OSFA = 107, VHEA = 108, VERT_TYPO_ASCENDER = 109, VERT_TYPO_DESCENDER = 110, 
    VERT_TYPO_LINE_GAP = 111, VMTX = 112, VERT_ORIGIN_Y = 113, VERT_ADVANCE_Y = 114, 
    LCBRACE = 115, RCBRACE = 116, LBRACKET = 117, RBRACKET = 118, HYPHEN = 119, 
    SEMI = 120, EQUALS = 121, MARKER = 122, COMMA = 123, QUOTE = 124, GCLASS = 125, 
    CID = 126, ESCGNAME = 127, NAMELABEL = 128, EXTNAME = 129, POINTNUM = 130, 
    NUMEXT = 131, NUMOCT = 132, NUM = 133, CATCHTAG = 134, A_WHITESPACE = 135, 
    A_LABEL = 136, A_LBRACE = 137, A_CLOSE = 138, A_LINE = 139, I_WHITESPACE = 140, 
    I_RPAREN = 141, IFILE = 142, I_LPAREN = 143, STRVAL = 144, EQUOTE = 145
  };

  enum {
    Anon = 1, AnonContent = 2, Include = 3, Ifile = 4, String = 5
  };

  explicit FeatLexer(antlr4::CharStream *input);
  ~FeatLexer();


   std::string anon_tag;

   /* All the TSTART/TCHR characters are grouped together, so just
    * look for the string and if its there verify that the characters
    * on either side are from the appropriate set (in case there are
    * "extra" characters).
    */

   bool verify_anon(const std::string &line) {
       auto p = line.find(anon_tag);
       if ( p == std::string::npos )
           return false;
       --p;
       if ( ! ( line[p] == ' ' || line[p] == '\t' || line[p] == '}' ) )
           return false;
       p += anon_tag.size() + 1;
       if ( ! ( line[p] == ' ' || line[p] == '\t' || line[p] == ';' ) )
           return false;
       return true;
   }

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

  virtual void action(antlr4::RuleContext *context, size_t ruleIndex, size_t actionIndex) override;
  virtual bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.
  void A_LABELAction(antlr4::RuleContext *context, size_t actionIndex);

  // Individual semantic predicate functions triggered by sempred() above.
  bool A_CLOSESempred(antlr4::RuleContext *_localctx, size_t predicateIndex);

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

