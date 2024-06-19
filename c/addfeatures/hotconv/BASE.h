/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef ADDFEATURES_HOTCONV_BASE_H_
#define ADDFEATURES_HOTCONV_BASE_H_

#include "common.h"

#define BASE_ TAG('B', 'A', 'S', 'E')

/* Standard functions */
void BASENew(hotCtx g);
int BASEFill(hotCtx g);
void BASEWrite(hotCtx g);
void BASEReuse(hotCtx g);
void BASEFree(hotCtx g);

class BASE {
 private:
    struct Axis {
        struct BaseScriptRecord {
            BaseScriptRecord(Tag t, int16_t bsi) : baseScriptTag(t), baseScriptInx(bsi) {}
            bool operator < (const BaseScriptRecord &rhs) const {
                return baseScriptTag < rhs.baseScriptTag;
            }
            Tag baseScriptTag {0};
            int16_t baseScriptInx {0};
            int32_t baseScriptOffset {0} ; // |-> BaseScriptList
                                           // long instead of Offset for temp -ve value
        };

        Axis() = delete;
        explicit Axis(const char *d) : desc(d) {}
        static Offset size() { return sizeof(uint16_t) * 2; }
        Offset tag_list_size() { return sizeof(uint16_t) + sizeof(uint32_t) * baseTagList.size(); }
        Offset script_list_size() {
            return sizeof(uint16_t) + (sizeof(uint32_t) + sizeof(uint16_t)) * baseScriptList.size(); }
        void addScript(BASE &h, Tag script, Tag dfltBaseline,
                       std::vector<VarValueRecord> &coords);
        void prep(hotCtx g);
        Offset fill(Offset curr);
        void write(Offset shared, BASE *h);
    
        std::vector<Tag> baseTagList;
        Offset baseTagOffset {0};
        std::vector<BaseScriptRecord> baseScriptList;
        Offset baseScriptOffset {0};
        Offset o {NULL_OFFSET};
        const char *desc;
    };

    struct BaseScriptInfo {
        explicit BaseScriptInfo(int16_t dbli) : dfltBaselineInx(dbli) {}
        BaseScriptInfo(BaseScriptInfo &&o) : dfltBaselineInx(o.dfltBaselineInx),
                                             coordInx(std::move(o.coordInx)) {}
        int16_t dfltBaselineInx {0};
        std::vector<int16_t> coordInx;
    };

    struct BaseValues {
        BaseValues() {}
        BaseValues(BaseValues &&other) : DefaultIndex(other.DefaultIndex),
                                         BaseCoordOffset(std::move(other.BaseCoordOffset)),
                                         o(other.o) {}
        static Offset script_size(uint16_t nLangSys) {
            return sizeof(uint16_t) * 3 + (sizeof(uint32_t) + sizeof(uint16_t)) * nLangSys;
        }
        Offset size() { return sizeof(uint16_t) * (2 + BaseCoordOffset.size()); }
        uint16_t DefaultIndex {0};
        std::vector<int32_t> BaseCoordOffset;  // int32_t instead of Offset for temp -ve value
        Offset o {0};
    };

    struct BaseCoord {
        BaseCoord() = delete;
        BaseCoord(VarValueRecord &&vvr, Offset o) : vvr(std::move(vvr)), o(o) {}
        bool operator==(const BaseCoord &rhs) const { return vvr == rhs.vvr; }
        bool operator==(const VarValueRecord &ovvr) { return vvr == ovvr; }
        uint16_t format() { return vvr.isVariable() ? 3 : 1; }
        LOffset size() {
            if (vvr.isVariable()) {
                return sizeof(uint16_t) * 5 + sizeof(int16_t);
            } else {
                return sizeof(uint16_t) + sizeof(int16_t);
            }
        }
        void write(BASE *h) {
            OUT2(format());
            OUT2(vvr.getDefault());
            if (vvr.isVariable()) {
                assert(pair.outerIndex != 0xFFFF);
                OUT2(6);
                OUT2(pair.outerIndex);
                OUT2(pair.innerIndex);
                OUT2(0x8000);
            }
        }
        VarValueRecord vvr;
        Offset o;
        var_indexPair pair {0xFFFF, 0xFFFF};
    };

 public:
    BASE() = delete;
    explicit BASE(hotCtx g) : g(g) {}
    void setBaselineTags(bool doVert, std::vector<Tag> &baselineTag);
    void addScript(bool doVert, Tag script, Tag dfltBaseline,
                   std::vector<VarValueRecord> &coords);
    int Fill();
    void Write();
    void setAxisCount(uint16_t axisCount) { ivs.setAxisCount(axisCount); }
 private:
    int addBaseScript(int dfltInx, size_t nBaseTags, std::vector<VarValueRecord> &coords);
    static Offset hdr_size(bool seenVariable) {
        Offset r = sizeof(int32_t) + sizeof(uint16_t) * 2;
        if (seenVariable)
            r += sizeof(uint32);
        return r;
    }
    Offset fillAxis(bool doVert);
    Offset fillSharedData();
    void writeSharedData();
    int32_t addCoord(VarValueRecord &c);

    std::vector<BaseScriptInfo> baseScript;

    struct {
        Offset curr {0};
        Offset shared {0}; /* Offset to start of shared area */
    } offset;

    // Table data
    Fixed version;
    Axis HorizAxis {"horizontal"};
    Axis VertAxis {"Vertical"};

    // Shared table data
    std::vector<BaseValues> baseValues;
    std::vector<BaseCoord> baseCoords;

    itemVariationStore ivs;
    LOffset ivsOffset {0};
    hotCtx g;    /* Package context */
};

#endif  // ADDFEATURES_HOTCONV_BASE_H_
