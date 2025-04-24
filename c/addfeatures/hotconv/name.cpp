/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * Naming table.
 */

#include "name.h"

#include <tuple>
#include <vector>
#include <cassert>

#include "GDEF.h"

void nameNew(hotCtx g) {
    if (g->ctx.name == nullptr)
        g->ctx.name = new nam_name;
}

void nameWrite(hotCtx g) {
    g->ctx.name->Write(g->vw);
}

void nameReuse(hotCtx g) {
    delete g->ctx.name;
    g->ctx.name = new nam_name;
}

void nameFree(hotCtx g) {
    delete g->ctx.name;
    g->ctx.name = nullptr;
}

std::string getWinDfltName(hotCtx g, uint16_t nameId) {
    return g->ctx.name->getName(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                                nam_name::NAME_WIN_ENGLISH, nameId);
}

static void addWinDfltName(hotCtx g, uint16_t nameId, std::string &str) {
    g->ctx.name->addName(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                         nam_name::NAME_WIN_ENGLISH, nameId, str,
                         g->convertFlags & HOT_OMIT_MAC_NAMES);
}

static void addMacDfltName(hotCtx g, uint16_t nameId, std::string &str) {
    g->ctx.name->addName(nam_name::NAME_MAC_PLATFORM, nam_name::NAME_MAC_ROMAN,
                         nam_name::NAME_MAC_ENGLISH, nameId, str,
                         g->convertFlags & HOT_OMIT_MAC_NAMES);
}

static void addStdNamePair(hotCtx g, bool win, bool mac,
                           uint16_t nameId, std::string &str) {
    if (win)
        addWinDfltName(g, nameId, str);
    if (mac)
        addMacDfltName(g, nameId, str);
}

/* Add standard Windows and Macintosh default names. */
static void addStdNames(hotCtx g, bool win, bool mac) {
    std::string buf;
    buf.resize(512);

    auto nam = g->ctx.name;

    {
        /* Set font version string. Round to 3 decimal places. Truncated to 3 decimal places by sprintf. */
        double dFontVersion = 0.0005 + FIX2DBL(g->font.version.otf);

        const char *idTag = "addfeatures";

        dFontVersion = ((int)(1000 * dFontVersion)) / 1000.0;

        /* Add Unique name */
        /* xxx is this really needed or just a waste of space? */
        int sz;
        if (!g->font.licenseID.empty()) {
            sz = snprintf(buf.data(), buf.size(),
                          "%.3f;%s;%s;%s",
                          dFontVersion,
                          g->font.vendId.c_str(),
                          g->font.FontName.c_str(),
                          g->font.licenseID.c_str());
        } else {
            sz = snprintf(buf.data(), buf.size(),
                          "%.3f;%s;%s",
                          dFontVersion,
                          g->font.vendId.c_str(),
                          g->font.FontName.c_str());
        }

        buf.resize(sz);
        /* Don't add if one has already been provided from the feature file */
        if (nam->noName(nam_name::MATCH_ANY, nam_name::MATCH_ANY, nam_name::MATCH_ANY,
                        nam_name::NAME_ID_UNIQUEID))
            addStdNamePair(g, win, mac, nam_name::NAME_ID_UNIQUEID, buf);

        buf.erase();
        buf.resize(512);
        /* Make and add version name */
        assert(g->font.version.client != NULL);
        sz = snprintf(buf.data(), buf.size(), "Version %.3f;%s %s",
                      dFontVersion, idTag, g->font.version.client);
        buf.resize(sz);
    }

    if (nam->noName(nam_name::MATCH_ANY, nam_name::MATCH_ANY,
                    nam_name::MATCH_ANY, nam_name::NAME_ID_VERSION))
        addStdNamePair(g, win, mac, nam_name::NAME_ID_VERSION, buf);

    /* Add PostScript name */
    addStdNamePair(g, win, mac, HOT_NAME_FONTNAME, g->font.FontName);
}

/* Translate utf8 string to target mac script/langid. */
/*Depends on hard-coded list of supported Mac encodings. See codepage.h,
   macEnc definitions in map.c */
typedef UV_BMP MacEnc[256];
static MacEnc macRomanEnc = {
#include "macromn0.h" /* 0. Mac Roman */
};

static char mapUV2MacRoman(UV uv) {
    char macChar = 0;
    int i;

    for (i = 0; i < 256; i++) {
        if (macRomanEnc[i] == uv) {
            macChar = i;
        }
    }

    return macChar;
}

/* translate string to mac platform default = macRoman script and language */
/* Does an in-place translation of a UTF-8 string.
   we can do this because the Mac string will be shorter than or equal to the UTF8 string. */
/* NOTE! This uses the h->tmp string to store the translated string, so don't call it again
   until you have copied the temp string contents */
static std::string translate2MacDflt(hotCtx g, const char *src) {
    const char *end;
    short int uv = 0;
    char *uvp = (char *)&uv;
    std::string dst;

    long length = strlen(src);
    end = src + length;
    dst.reserve(length);   /* This translation can only make the string shorter */

    while (src < end) {
        unsigned s0 = *src++ & 0xff;
        uv = 0;
        if (s0 < 0xc0) {
            /* 1-byte */
            uv = s0;
        } else {
            unsigned s1 = *src++ & 0xff;
            if (s0 < 0xe0) {
                /* 2-byte */
#ifdef __LITTLE_ENDIAN__
                uvp[1] = s0 >> 2 & 0x07;
                uvp[0] = s0 << 6 | (s1 & 0x3f);
#else
                uvp[0] = s0 >> 2 & 0x07;
                uvp[1] = s0 << 6 | (s1 & 0x3f);
#endif
            } else {
                /* 3-byte */
                unsigned s2 = *src++;
#ifdef __LITTLE_ENDIAN__
                uvp[1] = s0 << 4 | (s1 >> 2 & 0x0f);
                uvp[0] = s1 << 6 | (s2 & 0x3f);
#else
                uvp[0] = s0 << 4 | (s1 >> 2 & 0x0f);
                uvp[1] = s1 << 6 | (s2 & 0x3f);
#endif
            }
        }
        if (uv) {
            char macChar = mapUV2MacRoman(uv);
            if (macChar == 0) {
                g->logger->log(sFATAL, "[name] Could not translate UTF8 glyph code into Mac Roman in name table name %s", dst.c_str());
            }
            dst.push_back(macChar);
        }
    }
    return dst;
}

int nameFill(hotCtx g) {
    auto nam = g->ctx.name;
    static const char *styles[4] = { "Regular", "Bold", "Italic", "Bold Italic"};
    std::string style {styles[g->font.flags & 0x3]};
    bool doV1Names = (g->convertFlags & HOT_USE_V1_MENU_NAMES);    /* if the Font Menu DB file is using the original V1 syntax, we write the mac Preferred Family */
                                                                     /* and Preferred Subfamily names to id's 1 and 2 rather than 16 and 17                         */
    bool doOTSpecName4 {true};  // !(h->g->convertFlags & HOT_USE_OLD_NAMEID4); write Windows name ID 4 correctly in Win platform.
    bool omitMacNames = (g->convertFlags & HOT_OMIT_MAC_NAMES);    /* omit all Mac platform names. */

    auto tempString = nam->getName(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                                   nam_name::MATCH_ANY, HOT_NAME_PREF_FAMILY);

    if (tempString.empty()) {
        /* Use the family Font name; this is generated from the PS family name if there is no Font Menu Name DB. */
        tempString = nam->getName(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                                  nam_name::MATCH_ANY, nam_name::NAME_ID_FAMILY);

        if (tempString.empty()) {
            g->logger->log(sFATAL, "[name] I can't find a Family name for this font !");
        }
        addWinDfltName(g, nam_name::NAME_ID_FAMILY, tempString);
    }

    /**************************** First fill out all the Windows menu names */

    /* All and only the font menu name specified in the FontMenuNameDB entry are present at this point.     */
    /* Only one  Windows Preferred Family is guaranteed to be defined (name ID 16); the rest may have to be */
    /* derived from these two values. This call is about deriving the missing names                         */

    bool doWarning = !nam->noName(nam_name::MATCH_ANY, nam_name::MATCH_ANY,
                                  nam_name::MATCH_ANY, nam_name::NAME_ID_FAMILY);

    /* Copy nam_name::NAME_PREF_FAMILY entries to nam_name::NAME_FAMILY. */
    /* We need to add a compatible family name for each Preferred Family name which does not already have a nam_name::NAME_FAMILY name. */
    /* That way, the name ID 16's will get eliminated, and we will be left with just name ID 1's, rather than just name ID's 16. */

    auto mv = nam->allMatches(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                              nam_name::MATCH_ANY, HOT_NAME_PREF_FAMILY);
    std::vector<std::tuple<uint16_t, uint16_t, std::string>> matchrecs;
    // We copy the fields because the pointers could be invalidated by the adds
    for (auto nrp : mv)
        matchrecs.emplace_back(nrp->platspecId, nrp->languageId, nrp->content);
    for (auto &[platspec, language, content] : matchrecs) {
        if (nam->noName(nam_name::NAME_WIN_PLATFORM, platspec, language, nam_name::NAME_ID_FAMILY)) {
            nam->addName(nam_name::NAME_WIN_PLATFORM, platspec, language, nam_name::NAME_ID_FAMILY, content);
            if (doWarning) {
                g->logger->log(sWARNING, "[name] The Font Menu Name DB entry for this font is missing an MS Platform Compatible Family Name entry to match the MS Platform Preferred Family Name for language ID %d. Using the Preferred Name only.", language);
            }
        }
    }

    /* Any MS UGL nam_name::NAME_PREF_SUBFAMILY names must be explicitly defined; */
    /* else we will add  only the nam_name::NAME_SUBFAMILY values.                */

    /* Add nam_name::NAME_SUBFAMILY  and nam_name::NAME_FULL for all MS Platform nam_name::NAME_FAMILY names, using the derived style value. */
    /* There is no way to add these from the Font Menu Name DB, so we don't have to check                                  */
    /* if there is already a value matching any particular nam_name::NAME_FAMILY                                                 */
    mv.clear();
    mv = nam->allMatches(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                         nam_name::MATCH_ANY, nam_name::NAME_ID_FAMILY);
    std::vector<uint16_t> langs;
    for (auto nrp : mv)
        langs.push_back(nrp->languageId);
    for (auto language : langs)
        nam->addName(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                     language, nam_name::NAME_ID_SUBFAMILY, style);

    /* Now add Windows name ID 4, Full Name. This to come last as it it requires the SubFamilyName to exist */
    if (!doOTSpecName4) {
        /* add Windows name ID 4 with the PS name as the value. */
        addWinDfltName(g, nam_name::NAME_ID_FULL, g->font.FontName);
    } else {
        if (nam->noName(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                                 nam_name::MATCH_ANY, nam_name::NAME_ID_FULL)) {
            mv.clear();
            mv = nam->allMatches(nam_name::NAME_WIN_PLATFORM, nam_name::NAME_WIN_UGL,
                                 nam_name::MATCH_ANY, nam_name::NAME_ID_FAMILY);
            matchrecs.clear();
            for (auto nrp : mv)
                matchrecs.emplace_back(nrp->platspecId, nrp->languageId, nrp->content);
            for (auto &[platspec, language, family] : matchrecs) {
                /* See if we can get a  subfamily name with the same script/language settings */
                auto subfamily = nam->getName(nam_name::NAME_WIN_PLATFORM, platspec, language, nam_name::NAME_ID_SUBFAMILY);

                if (subfamily.empty())
                    subfamily = getWinDfltName(g, nam_name::NAME_ID_SUBFAMILY);

                if (subfamily == "Regular") {
                    nam->addName(nam_name::NAME_WIN_PLATFORM, platspec, language, nam_name::NAME_ID_FULL, family);
                } else {
                    std::string full = family + " " + subfamily;
                    nam->addName(nam_name::NAME_WIN_PLATFORM, platspec, language, nam_name::NAME_ID_FULL, full);
                }
            }
        }
    }

    /**************************** Now fill out all the Mac menu names. This is slightly different due to V1 vs V2 syntax diffs. */
    if (!omitMacNames) {
        /* If there is no Mac Preferred Family, we get the default Win Preferred Family name. */
        /* This is the only one we copy to the Mac platform, as it is the only one we can     */
        /* translate easily form Win UGL to Mac encoding.                                     */
        if (nam->noMacDfltName(HOT_NAME_PREF_FAMILY)) {
            tempString = getWinDfltName(g, HOT_NAME_PREF_FAMILY);
            if (!tempString.empty()) {
                auto family = translate2MacDflt(g, tempString.c_str());
                addMacDfltName(g, HOT_NAME_PREF_FAMILY, family);

                /* In version 1 syntax, there is never a Mac Compatible Family name entry, */
                /* so just copy the nam_name::NAME_PREF_FAMILY to nam_name::NAME_FAMILY                */
                if (doV1Names)
                    addMacDfltName(g, nam_name::NAME_ID_FAMILY, family);
            }
        }

        /* If we are using V2 syntax, and there is no Mac nam_name::NAME_FAMILY specified, we get it from the */
        /* Windows default nam_name::NAME_FAMILY name. We do NOT do this if we are using v1 syntax.           */

        if (nam->noMacDfltName(nam_name::NAME_ID_FAMILY)) {
            if (!doV1Names) {
                tempString = getWinDfltName(g, nam_name::NAME_ID_FAMILY);
                if (!tempString.empty()) {
                    auto family = translate2MacDflt(g, tempString.c_str());
                    addMacDfltName(g, nam_name::NAME_ID_FAMILY, family);
                }
            }
        }

        /* Copy nam_name::NAME_PREF_FAMILY entries to nam_name::NAME_FAMILY. Same logic as for Windows names.                         */
        /* For V2 syntax, there may be nam_name::NAME_FAMILY entries, so we need to make sure we don't add conflicting entries. */

        mv.clear();
        mv = nam->allMatches(nam_name::NAME_MAC_PLATFORM, nam_name::MATCH_ANY,
                             nam_name::MATCH_ANY, HOT_NAME_PREF_FAMILY);
        matchrecs.clear();
        for (auto nrp : mv)
            matchrecs.emplace_back(nrp->platspecId, nrp->languageId, nrp->content);
        for (auto &[platspec, language, family] : matchrecs) {
            if (nam->noName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_FAMILY)) {
                nam->addName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_FAMILY, family);
                if (doWarning && !doV1Names) {
                    g->logger->log(sWARNING, "[name] The Font Menu Name DB entry for this font is missing a Mac Platform Compatible Family Name entry to match the Mac Platform Preferred Family Name for language ID %d. Using the Preferred Name only.", language);
                }
            }
        }
        doWarning = false;

        /* Now for the Mac sub-family modifiers */

        /* If there are no nam_name::NAME_PREF_SUBFAMILY names at all,                    */
        /* and there is a default MS  nam_name::NAME_PREF_SUBFAMILY, we will use that.    */
        /* Otherwise, Mac nam_name::NAME_PREF_SUBFAMILY names must be explicitly defined. */

        if (nam->noMacDfltName(HOT_NAME_PREF_SUBFAMILY)) {
            tempString = getWinDfltName(g, HOT_NAME_PREF_SUBFAMILY);
            if (!tempString.empty()) {
                auto subfamily = translate2MacDflt(g, tempString.c_str());
                addMacDfltName(g, HOT_NAME_PREF_SUBFAMILY, subfamily);
                if (doV1Names)
                    addMacDfltName(g, nam_name::NAME_ID_SUBFAMILY, subfamily);
            }
        }

        /* For V1 syntax, any nam_name::NAME_PREF_SUBFAMILY must be copied to the nam_name::NAME_SUBFAMILY      */
        /* entries, so that the nam_name::NAME_PREF_SUBFAMILY entries will later be eliminated. We didn't */
        /* do this for Windows, as for Windows, name ID 2 is always the compatible subfamily names. */
        /* However, in V1 syntax, Mac name ID 2 contains the Preferred Sub Family names.            */

        if (doV1Names) {
            mv.clear();
            mv = nam->allMatches(nam_name::NAME_MAC_PLATFORM, nam_name::MATCH_ANY,
                                 nam_name::MATCH_ANY, HOT_NAME_PREF_SUBFAMILY);
            matchrecs.clear();
            for (auto nrp : mv)
                matchrecs.emplace_back(nrp->platspecId, nrp->languageId, nrp->content);
            for (auto &[platspec, language, subfamily] : matchrecs) {
                if (nam->noName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_SUBFAMILY))
                    nam->addName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_SUBFAMILY, subfamily);
            }
        }

        /* Now for all Mac nam_name::NAME_FAMILY names, fill in the nam_name::NAME_SUBFAMILY names.      */
        /* These may not yet be filled in even for V1 syntax, despite the loop above. If the */
        /* user specified Mac nam_name::NAME_PREF_FAMILY names but not nam_name::NAME_PREF_SUBFAMILY     */
        /* names, there will still be no nam_name::NAME_SUBFAMILY entries.                         */

        mv.clear();
        mv = nam->allMatches(nam_name::NAME_MAC_PLATFORM, nam_name::MATCH_ANY,
                             nam_name::MATCH_ANY, nam_name::NAME_ID_FAMILY);
        matchrecs.clear();
        for (auto nrp : mv)
            matchrecs.emplace_back(nrp->platspecId, nrp->languageId, nrp->content);
        for (auto &[platspec, language, subfamily] : matchrecs) {
            if (nam->noName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_SUBFAMILY))
                nam->addName(nam_name::NAME_MAC_PLATFORM, platspec, language,
                             nam_name::NAME_ID_SUBFAMILY, style);
        }

        /* Done with Mac Preferred and Win Compatible Family and SubFamily names */

        /* Add mac.Full (name ID 4) for all mac.Family names. compatible Family name + compatible Subfamily name. */

        uint16_t famId = doOTSpecName4 ? nam_name::NAME_ID_FAMILY : HOT_NAME_PREF_FAMILY;
        uint16_t subFamId = doOTSpecName4 ? nam_name::NAME_ID_SUBFAMILY : HOT_NAME_PREF_SUBFAMILY;

        mv.clear();
        mv = nam->allMatches(nam_name::NAME_MAC_PLATFORM, nam_name::MATCH_ANY,
                             nam_name::MATCH_ANY, famId);
        matchrecs.clear();
        for (auto nrp : mv)
            matchrecs.emplace_back(nrp->platspecId, nrp->languageId, nrp->content);
        for (auto &[platspec, language, family] : matchrecs) {
            if (!nam->noName(nam_name::NAME_MAC_PLATFORM, nam_name::MATCH_ANY,
                             nam_name::MATCH_ANY, nam_name::NAME_ID_FULL))
                break;
            auto subfamily = nam->getName(nam_name::NAME_MAC_PLATFORM, platspec, language, subFamId);
            if (!doOTSpecName4 && subfamily.empty())
                subfamily = nam->getName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_SUBFAMILY);
            if (subfamily.empty()) {
                tempString = getWinDfltName(g, subFamId);
                if (!tempString.empty())
                    subfamily = translate2MacDflt(g, tempString.c_str());
            }
            if (subfamily.empty()) {
                tempString = getWinDfltName(g, nam_name::NAME_ID_SUBFAMILY);
                if (!tempString.empty())
                    subfamily = translate2MacDflt(g, tempString.c_str());
            }
            if (subfamily.empty()) {
                g->logger->log(sFATAL, "[name] no Mac subfamily name specified");
            } else if (subfamily == "Regular") {
                nam->addName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_FULL, family);
            } else {
                std::string full = family + " " + subfamily;
                nam->addName(nam_name::NAME_MAC_PLATFORM, platspec, language, nam_name::NAME_ID_FULL, full);
            }
        }
    }

    /* Remove unnecessary ( i.e. duplicate) names. For example, if the user  */
    /* specified a Mac Compatible Full Name ( name id 18) in the             */
    /* FontMenuNameDB, and it is the same as the derived Preferred Full Name */
    /* (id 4), then we can omit the Mac Compatible Full Name ( name id 18).  */

    nam->deleteDuplicates(nam_name::NAME_WIN_PLATFORM, HOT_NAME_PREF_FAMILY, nam_name::NAME_ID_FAMILY);
    nam->deleteDuplicates(nam_name::NAME_WIN_PLATFORM, HOT_NAME_PREF_SUBFAMILY, nam_name::NAME_ID_SUBFAMILY);

    if (!omitMacNames) {
        nam->deleteDuplicates(nam_name::NAME_MAC_PLATFORM, HOT_NAME_PREF_FAMILY, nam_name::NAME_ID_FAMILY);
        nam->deleteDuplicates(nam_name::NAME_MAC_PLATFORM, HOT_NAME_PREF_SUBFAMILY, nam_name::NAME_ID_SUBFAMILY);
        nam->deleteDuplicates(nam_name::NAME_MAC_PLATFORM, HOT_NAME_COMP_FULL, nam_name::NAME_ID_FULL);
    }
    /* Determine if platform names are present */
    bool win = !nam->noName(nam_name::NAME_WIN_PLATFORM, nam_name::MATCH_ANY,
                            nam_name::MATCH_ANY, nam_name::MATCH_ANY);
    bool mac = !nam->noName(nam_name::NAME_MAC_PLATFORM, nam_name::MATCH_ANY,
                            nam_name::MATCH_ANY, nam_name::MATCH_ANY);

    addStdNames(g, win, mac);

    return g->ctx.name->Fill();
}

int verifyDefaultNames(hotCtx g, uint16_t nameId) {
    int returnVal = 0;

    if (g->ctx.name->noWinDfltName(nameId)) {
        returnVal = MISSING_WIN_DEFAULT_NAME;
    }
    if (g->ctx.name->noMacDfltName(nameId)) {
        returnVal |= MISSING_MAC_DEFAULT_NAME;
    }

    return returnVal;
}

void nameAdd(hotCtx g, uint16_t platformId, uint16_t platspecId,
             uint16_t languageId, uint16_t nameId, const std::string &str) {
    if (g->ctx.name->verifyIDExists(nameId)) {
        auto foundstr = g->ctx.name->getName(platformId, platspecId, languageId, nameId);
        if (foundstr.size() > 0 && foundstr != str)
            g->logger->log(sWARNING, "[name] Overwriting existing nameid %d %s with %s",
                           nameId, foundstr.c_str(), str.c_str());
    }
    g->ctx.name->addName(platformId, platspecId, languageId, nameId, str,
            g->convertFlags & HOT_OMIT_MAC_NAMES);
}
