/* Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_NAMESUPPORT_H_
#define SHARED_INCLUDE_NAMESUPPORT_H_

#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "ctlshare.h"
#include "sfntread.h"
#include "varsupport.h"

/* Name Table Reader Library
   =======================================
   This library reads the name table.
*/

class nam_name {
 private:
    struct NameRecord {
        NameRecord() {}
        NameRecord(uint16_t platformId, uint16_t platspecId, uint16_t languageId,
                   uint16_t nameId, const std::string &str) : platformId(platformId), platspecId(platspecId),
                                                              languageId(languageId), nameId(nameId), content(str) {}
        NameRecord(const NameRecord &nr) : platformId(nr.platformId), platspecId(nr.platspecId),
                                           languageId(nr.languageId), nameId(nr.nameId), length(nr.length),
                                           offset(nr.offset), fromTable(nr.fromTable), content(nr.content) {}
        NameRecord(NameRecord &&nr) : platformId(nr.platformId), platspecId(nr.platspecId),
                                      languageId(nr.languageId), nameId(nr.nameId), length(nr.length),
                                      offset(nr.offset), fromTable(nr.fromTable), content(std::move(nr.content)) {}
        NameRecord &operator=(const NameRecord &nr) {
            platformId = nr.platformId;
            platspecId = nr.platspecId;
            languageId = nr.languageId;
            nameId = nr.nameId;
            length = nr.length;
            offset = nr.offset;
            fromTable = nr.fromTable;
            content = nr.content;
            return *this;
        }
        NameRecord &operator=(NameRecord &&nr) {
            platformId = nr.platformId;
            platspecId = nr.platspecId;
            languageId = nr.languageId;
            nameId = nr.nameId;
            length = nr.length;
            offset = nr.offset;
            fromTable = nr.fromTable;
            content.swap(nr.content);
            return *this;
        }
        uint16_t platformId {0};
        uint16_t platspecId {0};
        uint16_t languageId {0};
        uint16_t nameId {0};
        uint16_t length {0};
        uint16_t offset {0};
        bool fromTable {false};
        std::string content;
    };

 public:
    static const uint16_t MATCH_ANY = 0xFFFF;

    nam_name() = default;
    nam_name(sfrCtx sfr, ctlSharedStmCallbacks *sscb) { Read(sfr, sscb); }

    /* name and cmap table platform and platform-specific ids */
    enum {
        /* Platform ids */
        NAME_UNI_PLATFORM = 0, /* Unicode */
        NAME_MAC_PLATFORM = 1, /* Macintosh */
        NAME_WIN_PLATFORM = 3, /* Microsoft */

        /* Unicode platform-specific ids */
        NAME_UNI_DEFAULT =   0, /* Default semantics */
        NAME_UNI_VER_1_1 =   1, /* Version 1.1 semantics */
        NAME_UNI_ISO_10646 = 2, /* ISO 10646 semantics */
        NAME_UNI_VER_2_0 =   3, /* Version 2.0 semantics */

        /* Macintosh platform-specific and language ids */
        NAME_MAC_ROMAN =   0, /* Roman */
        NAME_MAC_ENGLISH = 0, /* English language */

        /* Windows platform-specific and language ids */
        NAME_WIN_SYMBOL =  0,    /* Undefined index scheme */
        NAME_WIN_UGL =     1,    /* UGL */
        NAME_WIN_ENGLISH = 0x409 /* English (American) language */
    };

    /* name IDs */
    enum {
        NAME_ID_COPYRIGHT =          0, /* copyright notice */
        NAME_ID_FAMILY =             1, /* family name */
        NAME_ID_SUBFAMILY =          2, /* sub-family name */
        NAME_ID_UNIQUEID =           3, /* unique font identifier */
        NAME_ID_FULL =               4, /* full name */
        NAME_ID_VERSION =            5, /* version */
        NAME_ID_POSTSCRIPT =         6, /* Postscript font name */
        NAME_ID_TRADEMARK =          7, /* trademark */
        NAME_ID_TYPO_FAMILY =       16, /* typographic family name */
        NAME_ID_CIDFONTNAME =       20, /* CID font name */
        NAME_ID_POSTSCRIPT_PREFIX = 25  /* Postscript font name prefix */
    };

    /* functions */
    bool Read(sfrCtx sfr, ctlSharedStmCallbacks *sscb);

    /*  read() loads the name table from an SFNT font.
        If not found, false is returned.

        sfr - context pointer created by calling sfrNew in sfntread library.

        sscb - a pointer to shared stream callback functions.
    */

    std::string getASCIIName(uint16_t nameId, bool isPS);

    /*  getASCIIName() looks up the name table for an ASCII name string specified by a name ID.
        If a name string contains any Unicode characters outside of the ASCII range,
        they are removed from the result.

        nameId - the name ID in the name table to be looked up.
        isPS   - if true the returned name must be Postscript-safe
    */

    std::string getFamilyNamePrefix();

    /*  nam_getFamilyNamePrefix() returns a font name prefix string to be used
        to name a CFF2 variable font instance.
    */

    std::string getNamedInstancePSName(var_axes *axesTbl, float *coords,
                                       uint16_t axisCount, int instanceIndex);

    /*  getNamedInstancePSName() looks up the fvar table for a named instance of a
        CFF2 variable font specified either by its instance index or a designk
        vector of the instance.

        axesTbl       - axes table data loaded by calling var_loadaxes in varread
                        library.
        coords        - the user design vector. May be NULL if the default instance
                        or a named instance is specified.
        axisCount     - the number of axes in the variable font.

        instanceIndex - specifies the named instance. If -1, coords specifies an
                         arbitrary instance.
    */

    std::string generateArbitraryInstancePSSuffix(var_axes *axesTbl, float *coords,
                                                  uint16_t axisCount,
                                                  ctlSharedStmCallbacks *sscb);
    std::string generateArbitraryInstancePSName(var_axes *axesTbl, float *coords,
                                                uint16_t axisCount,
                                                ctlSharedStmCallbacks *sscb);

    /*  generateArbitraryInstancePSName() generate a PS name of an arbitrary instance
        of a CFF2 variable font specified by a design vector of the instance.

        axesTbl       - axes table data loaded by calling var_loadaxes in varread
                        library.
        coords        - the user design vector. May be NULL if the default instance
                        or a named instance is specified.
        axisCount     - the number of axes in the variable font.
    */


    std::string generateLastResortInstancePSName(var_axes *axesTbl, float *coords,
                                                 uint16_t axisCount,
                                                 ctlSharedStmCallbacks *sscb,
                                                 uint16_t maxLength);

    /* generateLastResortInstancePSName() generates a unique PostScript font name
       for the specified instance of a CFF2 variable font by hashing the coordinate values
       as the last resort.

        axesTbl       - axes table data loaded by calling var_loadaxes in varread
                        library.
        coords        - the user design vector. May be NULL if the default instance
                        or a named instance is specified.
        axisCount     - the number of axes in the variable font.
    */

    // uint16_t addUser(std::string &str);
    void addName(uint16_t platformId, uint16_t platspecId, uint16_t languageId,
                 uint16_t nameId, const std::string &str, bool omitMacNames = false);
    uint16_t reserveUserID();
    bool verifyIDExists(uint16_t nameId);
    void Write(VarWriter &vw);
    int Fill();
    std::vector<NameRecord*> allMatches(uint16_t platformId, uint16_t platspecId,
                                        uint16_t languageId, uint16_t nameId) {
        std::vector<NameRecord*> nrv;
        auto i = lowerBound(platformId, platspecId, languageId, nameId);
        auto e = upperBound(platformId, platspecId, languageId, nameId);
        for (; i != e; i++)
            if (nrMatch(i->second, platformId, platspecId, languageId, nameId))
                nrv.push_back(&i->second);
        return nrv;
    }
    std::string getName(uint16_t platformId, uint16_t platspecId,
                        uint16_t languageId, uint16_t nameId) {
        auto i = firstMatch(platformId, platspecId, languageId, nameId);
        if (i == entries.end())
            return "";
        return i->second.content;
    }
    bool noName(uint16_t platformId, uint16_t platspecId, uint16_t languageId,
                uint16_t nameId) {
        return firstMatch(platformId, platspecId, languageId, nameId) == entries.end();
    }
    bool noWinDfltName(uint16_t nameId) {
        return noName(NAME_WIN_PLATFORM, NAME_WIN_UGL, NAME_WIN_ENGLISH, nameId);
    }
    bool noMacDfltName(uint16_t nameId) {
        return noName(NAME_MAC_PLATFORM, NAME_MAC_ROMAN, NAME_MAC_ENGLISH, nameId);
    }
    void deleteDuplicates(uint16_t platformId, uint16_t targetId, uint16_t referenceId);

 private:
    bool checkNameChar(unsigned char ch, bool isPS) {
        if (isPS) {
            switch (ch) {
                case '[':
                case ']':
                case '(':
                case ')':
                case '{':
                case '}':
                case '<':
                case '>':
                case '/':
                case '%':
                    return false;
                default:
                    return (ch >= 33 && ch <= 126);
            }
        } else
            return (ch >= 32) && (ch <= 126);
    }

    typedef std::tuple<uint16_t, uint16_t, uint16_t, uint16_t> EntryKey;
    typedef std::map<EntryKey, NameRecord> EntryMap;

    EntryMap::iterator find(uint16_t platformId, uint16_t platspecId,
                            uint16_t languageId, uint16_t nameId) {
        return entries.find(EntryKey(platformId, platspecId, languageId, nameId));
    }
    uint16_t to0(uint16_t id) { return id == MATCH_ANY ? 0 : id; }
    uint16_t toF(uint16_t id) { return id; }
    EntryMap::iterator lowerBound(uint16_t platformId, uint16_t platspecId,
                                  uint16_t languageId, uint16_t nameId) {
        return entries.lower_bound(EntryKey(to0(platformId), to0(platspecId),
                                            to0(languageId), to0(nameId)));
    }
    EntryMap::iterator upperBound(uint16_t platformId, uint16_t platspecId,
                                  uint16_t languageId, uint16_t nameId) {
        return entries.upper_bound(EntryKey(toF(platformId), toF(platspecId),
                                            toF(languageId), toF(nameId)));
    }
    bool nrMatch(NameRecord &nr, uint16_t platformId, uint16_t platspecId,
                 uint16_t languageId, uint16_t nameId) {
        return (platformId == MATCH_ANY || platformId == nr.platformId) &&
               (platspecId == MATCH_ANY || platspecId == nr.platspecId) &&
               (languageId == MATCH_ANY || languageId == nr.languageId) &&
               (nameId == MATCH_ANY || nameId == nr.nameId);
    }

    EntryMap::iterator firstMatch(uint16_t platformId, uint16_t platspecId,
                                  uint16_t languageId, uint16_t nameId) {
        auto i = lowerBound(platformId, platspecId, languageId, nameId);
        auto e = upperBound(platformId, platspecId, languageId, nameId);
        for (; i != e; i++)
            if (nrMatch(i->second, platformId, platspecId, languageId, nameId))
                return i;
        return entries.end();
    }
    std::string getPSName() { return getASCIIName(NAME_ID_POSTSCRIPT, true); }
    uint16_t hdrSize() { return sizeof(uint16_t) * 3; }
    uint16_t recSize() { return sizeof(uint16_t) * 6 * entries.size(); }

    uint16_t userNameId {256};
    EntryMap entries;
    std::string stringData;
};

#endif  // SHARED_INCLUDE_NAMESUPPORT_H_
