
// Generated from FeatLexer.g4 by ANTLR 4.13.1

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
    CV_PARAM_LABEL = 46, DEF_AXIS_UNIT = 47, CV_CHARACTER = 48, SIZEMENUNAME = 49, 
    CONTOURPOINT = 50, ANCHOR = 51, ANCHOR_DEF = 52, VALUE_RECORD_DEF = 53, 
    LOCATION_DEF = 54, MARK = 55, MARK_CLASS = 56, CURSIVE = 57, MARKBASE = 58, 
    MARKLIG = 59, MARKLIG_v = 60, LIG_COMPONENT = 61, KNULL = 62, BASE = 63, 
    HA_BTL = 64, VA_BTL = 65, HA_BSL = 66, VA_BSL = 67, GDEF = 68, GLYPH_CLASS_DEF = 69, 
    ATTACH = 70, LIG_CARET_BY_POS = 71, LIG_CARET_BY_IDX = 72, HEAD = 73, 
    FONT_REVISION = 74, HHEA = 75, ASCENDER = 76, DESCENDER = 77, LINE_GAP = 78, 
    CARET_OFFSET = 79, CARET_SLOPE_RISE = 80, CARET_SLOPE_RUN = 81, NAME = 82, 
    NAMEID = 83, OS_2 = 84, FS_TYPE = 85, FS_TYPE_v = 86, OS2_LOWER_OP_SIZE = 87, 
    OS2_UPPER_OP_SIZE = 88, PANOSE = 89, TYPO_ASCENDER = 90, TYPO_DESCENDER = 91, 
    TYPO_LINE_GAP = 92, WIN_ASCENT = 93, WIN_DESCENT = 94, X_HEIGHT = 95, 
    CAP_HEIGHT = 96, SUBSCRIPT_X_SIZE = 97, SUBSCRIPT_X_OFFSET = 98, SUBSCRIPT_Y_SIZE = 99, 
    SUBSCRIPT_Y_OFFSET = 100, SUPERSCRIPT_X_SIZE = 101, SUPERSCRIPT_X_OFFSET = 102, 
    SUPERSCRIPT_Y_SIZE = 103, SUPERSCRIPT_Y_OFFSET = 104, STRIKEOUT_SIZE = 105, 
    STRIKEOUT_POSITION = 106, WEIGHT_CLASS = 107, WIDTH_CLASS = 108, VENDOR = 109, 
    UNICODE_RANGE = 110, CODE_PAGE_RANGE = 111, FAMILY_CLASS = 112, STAT = 113, 
    ELIDED_FALLBACK_NAME = 114, ELIDED_FALLBACK_NAME_ID = 115, DESIGN_AXIS = 116, 
    AXIS_VALUE = 117, FLAG = 118, LOCATION = 119, AXIS_EAVN = 120, AXIS_OSFA = 121, 
    VHEA = 122, VERT_TYPO_ASCENDER = 123, VERT_TYPO_DESCENDER = 124, VERT_TYPO_LINE_GAP = 125, 
    VMTX = 126, VERT_ORIGIN_Y = 127, VERT_ADVANCE_Y = 128, LCBRACE = 129, 
    RCBRACE = 130, LBRACKET = 131, RBRACKET = 132, LPAREN = 133, RPAREN = 134, 
    HYPHEN = 135, PLUS = 136, SEMI = 137, EQUALS = 138, MARKER = 139, COMMA = 140, 
    COLON = 141, STRVAL = 142, LNAME = 143, GCLASS = 144, AXISUNIT = 145, 
    CID = 146, ESCGNAME = 147, NAMELABEL = 148, EXTNAME = 149, POINTNUM = 150, 
    NUMEXT = 151, NUMOCT = 152, NUM = 153, A_WHITESPACE = 154, A_LABEL = 155, 
    A_LBRACE = 156, A_CLOSE = 157, A_LINE = 158, I_WHITESPACE = 159, I_RPAREN = 160, 
    IFILE = 161, I_LPAREN = 162
  };

  enum {
    Anon = 1, AnonContent = 2, Include = 3, Ifile = 4
  };

  explicit FeatLexer(antlr4::CharStream *input);

  ~FeatLexer() override;


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


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  void action(antlr4::RuleContext *context, size_t ruleIndex, size_t actionIndex) override;

  bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.
  void A_LABELAction(antlr4::RuleContext *context, size_t actionIndex);

  // Individual semantic predicate functions triggered by sempred() above.
  bool A_CLOSESempred(antlr4::RuleContext *_localctx, size_t predicateIndex);

};

