/* Copyright 2016 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "namesupport.h"

#include <unordered_map>

#include "ctlshare.h"
#include "varsupport.h"
#include "sha1.h"

/* --------------------------- Functions  --------------------------- */

/* Load the name table. */
bool nam_name::Read(sfrCtx sfr, ctlSharedStmCallbacks *sscb) {
    sfrTable *table = sfrGetTableByTag(sfr, CTL_TAG('n', 'a', 'm', 'e'));
    if (table == NULL) {
        sscb->message(sscb, "name table missing");
        return false;
    }
    sscb->seek(sscb, table->offset);

    uint16_t format = sscb->read2(sscb);
    if (format != 0) {
        sscb->message(sscb, "invalid name table format");
        return false;
    }

    /* Read rest of header */
    uint16_t count = sscb->read2(sscb);
    uint16_t stringOffset = sscb->read2(sscb);
    uint16_t stringEnd {0};

    /* Read name records */
    for (int i = 0; i < count; i++) {
        NameRecord nr;
        nr.platformId = sscb->read2(sscb);
        nr.platspecId = sscb->read2(sscb);
        nr.languageId = sscb->read2(sscb);
        nr.nameId = sscb->read2(sscb);
        nr.length = sscb->read2(sscb);
        nr.offset = sscb->read2(sscb);
        nr.fromTable = true;
        if (stringEnd < nr.length + nr.offset)
            stringEnd = nr.length + nr.offset;
        entries.insert_or_assign(EntryKey(nr.platformId, nr.platspecId, nr.languageId, nr.nameId), std::move(nr));
    }

    if (stringOffset + stringEnd > table->length) {
        sscb->message(sscb, "name table too short");
        return false;
    }

    for (auto &[_, nr] : entries) {
        sscb->seek(sscb, table->offset + stringOffset + nr.offset);
        nr.content.resize(nr.length);
        sscb->read(sscb, nr.length, nr.content.data());
    }

    return true;
}

std::string nam_name::getASCIIName(uint16_t nameId, bool isPS) {
    unsigned char ch1, ch2;

    auto it = find(NAME_WIN_PLATFORM, NAME_WIN_UGL, NAME_WIN_ENGLISH, nameId);
    if (it != entries.end() && it->second.length > 0) {
        assert(it->second.fromTable);  // This is only safe to use on Read entries
        auto &s = it->second.content;
        std::string r;
        for (size_t i = 0; i + 1 < s.size(); i += 2) {
            ch1 = s[i];
            ch2 = s[i+1];
            if (s[i] == 0 && checkNameChar(s[i+1], isPS))
                r.push_back(s[i+1]);
        }
        return r;
    }
    it = find(NAME_MAC_PLATFORM, NAME_MAC_ROMAN, NAME_MAC_ENGLISH, nameId);
    if (it != entries.end()) {
        assert(it->second.fromTable);  // This is only safe to use on Read entries
        auto &s = it->second.content;
        std::string r;
        for (size_t i = 0; i < s.size(); i++) {
            if (checkNameChar(s[i], isPS))
                r.push_back(s[i+1]);
        }
        return r;
    }
    return "";
}

static void removeNonAlphanumChar(std::string s) {
    uint32_t i, j;

    for (i = j = 0; i < s.size(); i++) {
        if (isalnum((unsigned char)s[i])) {
            s[j++] = s[i];
        }
    }
    s.resize(j);
}

/* get the family name prefix for a variable font instance name */
std::string nam_name::getFamilyNamePrefix() {
    auto s = getASCIIName(NAME_ID_POSTSCRIPT_PREFIX, true);
    if (s.size() == 0)
        s = getASCIIName(NAME_ID_TYPO_FAMILY, true);
    if (s.size() == 0)
        s = getASCIIName(NAME_ID_FAMILY, true);
    if (s.size() > 0)
         removeNonAlphanumChar(s);

    /*
    if (s.size() > MAX_FAMILY_NAME_LENGTH) {
        sscb->message(sscb, "too long family name prefix");
        s.clear();
    }
    */
    return s;
}

std::string nam_name::getNamedInstancePSName(var_axes *axesTbl, float *coords,
                                             uint16_t axisCount, int instanceIndex) {
    uint16_t subfamilyID = 0, postscriptID = 0;

    std::string s;

    /* if the font is not a variable font or no coordinates are specified, return the default PS name */
    if (!axesTbl || !coords || !axisCount)
        return getPSName();

    /* find a matching named instance in the fvar table */
    if (instanceIndex < 0)
        instanceIndex = axesTbl->findInstance(coords, axisCount, subfamilyID, postscriptID);
    if ((instanceIndex >= 0) && (postscriptID == 6 || ((postscriptID > 255) && (postscriptID < 32768)))) {
        s = getASCIIName(postscriptID, true);
        if (s.size() > 0)
            return s;
    }

    /* get the family name prefix */
    s = getFamilyNamePrefix();
    if (s.size() == 0)
        return s;

    /* append the style name from the instance if there is a matching one */
    auto style = getASCIIName(subfamilyID, false);
    if (style.size() == 0)
        return "";

    removeNonAlphanumChar(style);
    s += style;

    return s;
}

static std::string stringizeNum(float value) {
    std::string s;

    unsigned long i = 0;
    float eps = 0.5f / (1 << 16);
    float hi, lo;
    int intPart;
    float fractPart;
    int neg = 0;
    int power10;
    int digit;

    if (value < 0.0f) {
        neg = 1;
        value = -value;
    }

    hi = value + eps;
    lo = value - eps;

    intPart = (int)floor(hi);
    if (intPart != (int)floor(lo)) {
        fractPart = 0.0f;
    } else {
        fractPart = hi - intPart;
    }

    if ((intPart == 0) && (fractPart == 0.0f)) {
        s.push_back('0');
        return s;
    }

    if (neg) {
        s.push_back('-');
    }

    /* integer part */
    for (digit = 0, power10 = 1; intPart >= power10;) {
        digit++;
        power10 *= 10;
    }
    while (digit-- > 0) {
        power10 /= 10;
        s.push_back((char)('0' + (intPart / power10)));
        intPart %= power10;
    }

    /* fractional part */
    if (fractPart >= eps) {
        s.push_back('.');

        do {
            fractPart *= 10;
            eps *= 10;
            intPart = (int)fractPart;
            s.push_back((char)('0' + intPart));
            fractPart -= intPart;
        } while (fractPart >= eps);
    }

    return s;
}

std::string nam_name::generateArbitraryInstancePSSuffix(var_axes *axesTbl,
                                                        float *coords,
                                                        uint16_t axisCount,
                                                        ctlSharedStmCallbacks *sscb) {
    /* if the font is not a variable font or no coordinates are specified, return the default PS name */
    if (!axesTbl || !coords || !axisCount)
        return "";

    std::string s;

    /* create an arbitrary instance name from the coordinates */
    for (uint16_t axis = 0; axis < axisCount; axis++) {
        /* value range check */
        if (fabs(coords[axis]) > 32768.0f) {
            sscb->message(sscb, "coordinates value out of range: %f", coords[axis]);
            return "";
        }

        /* convert a coordinates value to string */
        s.push_back('_');
        s += stringizeNum(coords[axis]);

        /* append a axis name */
        ctlTag tag;
        if (!axesTbl->getAxis(axis, &tag, NULL, NULL, NULL, NULL)) {
            sscb->message(sscb, "failed to get axis information");
            return "";
        }
        for (int i = 3; i >= 0; i--) {
            char ch = (char)((tag >> (i * 8)) & 0xff);
            if (ch < 32 || ch > 126) {
                sscb->message(sscb, "invalid character found in an axis tag");
                continue;
            }
            if (ch == ' ') continue;
            s.push_back(ch);
        }
    }

    return s;
}

std::string nam_name::generateArbitraryInstancePSName(var_axes *axesTbl,
                                                      float *coords,
                                                      uint16_t axisCount,
                                                      ctlSharedStmCallbacks *sscb) {
    if (!axesTbl || !coords || !axisCount)
        return "";

    return getFamilyNamePrefix() +
           generateArbitraryInstancePSSuffix(axesTbl, coords, axisCount, sscb);
}

static void *nam_sha1_malloc(size_t size, void *hook) {
    ctlSharedStmCallbacks *sscb = (ctlSharedStmCallbacks *)hook;
    return sscb->memNew(sscb, size);
}

static void nam_sha1_free(sha1_pctx ctx, void *hook) {
    ctlSharedStmCallbacks *sscb = (ctlSharedStmCallbacks *)hook;
    sscb->memFree(sscb, ctx);
}

std::string nam_name::generateLastResortInstancePSName(var_axes *axesTbl, float *coords,
                                                       uint16_t axisCount,
                                                       ctlSharedStmCallbacks *sscb,
                                                       uint16_t maxLength) {
    /* if the font is not a variable font or no coordinates are specified, return the default PS name */
    if (!axesTbl || !coords || !axisCount)
        return getPSName();

    /* get the family name prefix */
    auto s = getFamilyNamePrefix();
    if (s.size() == 0)
        return "";

    /* generate a SHA1 value of the generated name as the identifier in a last resort name.
     */
    s.push_back('-');

    auto suffix = generateArbitraryInstancePSSuffix(axesTbl, coords, axisCount, sscb);

    {
        sha1_pctx sha1_ctx = sha1_init(nam_sha1_malloc, sscb);
        sha1_hash hash;
        int error;

        if (!sha1_ctx)
            return "";

        error = sha1_update(sha1_ctx, (unsigned char *) suffix.data(), suffix.size());
        error |= sha1_finalize(sha1_ctx, nam_sha1_free, hash, sscb);

        if (error) {
            sscb->message(sscb, "failed to generate hash during a last resort variable font instance name generation");
            return "";
        }

        /* append the hash value has a hex string */
        for (int i = 0; i < 20; i++) {
            static const char hexChar[] = "0123456789ABCDEF";
            unsigned char byte = hash[i];
            s.push_back(hexChar[byte >> 4]);
            s.push_back(hexChar[byte & 0xF]);
        }

        if (s.size() >= maxLength)
            s.resize(maxLength-1);

        sscb->message(sscb, "last resort variable font instance name %s generated for %s",
                      s.c_str(), suffix.c_str());
    }
    return s;
}

void nam_name::addName(uint16_t platformId, uint16_t platspecId,
                       uint16_t languageId, uint16_t nameId,
                       const std::string &str, bool omitMacNames) {
    if (omitMacNames && platformId == NAME_MAC_PLATFORM)
        return;

    NameRecord nr {platformId, platspecId, languageId, nameId, str};
    entries.insert_or_assign(EntryKey(nr.platformId, nr.platspecId, nr.languageId, nr.nameId), std::move(nr));
}

/* Delete a target name from list if the same as reference name. */
void nam_name::deleteDuplicates(uint16_t platformId, uint16_t targetId, uint16_t referenceId) {
    std::vector<EntryKey> ekv;

    auto i = lowerBound(platformId, MATCH_ANY, MATCH_ANY, targetId);
    auto e = upperBound(platformId, MATCH_ANY, MATCH_ANY, targetId);
    for (; i != e; i++) {
        if (nrMatch(i->second, platformId, MATCH_ANY, MATCH_ANY, targetId)) {
            auto d = firstMatch(platformId, i->second.platspecId, i->second.languageId, referenceId);
            if (d != entries.end() && d->second.content == i->second.content)
                ekv.push_back(i->first);
        }
    }
    for (auto &ek : ekv)
        entries.erase(ek);
}

/* Translate UTF-8 strings to target platform. */
static std::string translate(uint16_t platformId, std::string &s) {
    switch (platformId) {
        case nam_name::NAME_WIN_PLATFORM: {  // Convert UTF-8 to 16-bit
            std::string dst;
            auto src = s.begin();
            auto end = s.end();
            while (src < end) {
                unsigned s0 = *src++ & 0xff;
                if (s0 < 0xc0) {
                    // 1-byte
                    dst.push_back(0);
                    dst.push_back(s0);
                } else {
                    unsigned s1 = *src++ & 0xff;
                    if (s0 < 0xe0) {
                        // 2-byte
                        dst.push_back(s0 >> 2 & 0x07);
                        dst.push_back(s0 << 6 | (s1 & 0x3f));
                    } else {
                        // 3-byte
                        unsigned s2 = *src++;
                        dst.push_back(s0 << 4 | (s1 >> 2 & 0x0f));
                        dst.push_back(s1 << 6 | (s2 & 0x3f));
                    }
                }
            }
            return dst;
        }

        case nam_name::NAME_MAC_PLATFORM:
            // pass thru - Mac strings are expected to already be translated to the target script/lang-id.
            break;
    }
    return s;
}

int nam_name::Fill() {
    std::unordered_map<std::string, uint16_t> offsets;
    stringData.clear();
    for (auto &[k, nr] : entries) {
        /* Update record to point to new string */
        auto str = nr.fromTable ? nr.content : translate(nr.platformId, nr.content);
        auto [oi, b] = offsets.insert({str, (uint16_t)stringData.size()});
        if (b)
            stringData += str;
        nr.offset = oi->second;
        nr.length = str.size();
    }
    return entries.size() > 0;
}

void nam_name::Write(VarWriter &vw) {
    vw.w2(0);                                 // Format
    vw.w2((uint16_t)entries.size());          // Count
    vw.w2((uint16_t) hdrSize() + recSize());  // stringOffset

    for (auto &[k, nr] : entries) {
        vw.w2(nr.platformId);
        vw.w2(nr.platspecId);
        vw.w2(nr.languageId);
        vw.w2(nr.nameId);
        vw.w2(nr.length);
        vw.w2(nr.offset);
    }

    vw.w(stringData.size(), stringData.data());
}

/* Add user-defined name with specified attributes. */
uint16_t nam_name::reserveUserID() {
    auto nameId = userNameId++;
    if (verifyIDExists(nameId) && nameId > 255) {
        while (verifyIDExists(++nameId)) {}
    }
    return nameId;
}

bool nam_name::verifyIDExists(uint16_t nameId) {
    return !noName(MATCH_ANY, MATCH_ANY, MATCH_ANY, nameId);
}
