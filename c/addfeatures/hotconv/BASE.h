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
        void addScript(BASE &h, Tag script, Tag dfltBaseline, std::vector<int16_t> &coords);
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
                                         BaseCoord(std::move(other.BaseCoord)),
                                         o(other.o) {}
        static Offset script_size(uint16_t nLangSys) {
            return sizeof(uint16_t) * 3 + (sizeof(uint32_t) + sizeof(uint16_t)) * nLangSys;
        }
        Offset size() { return sizeof(uint16_t) * (2 + BaseCoord.size()); }
        uint16_t DefaultIndex {0};
        std::vector<int32_t> BaseCoord;  // int32_t instead of Offset for temp -ve value
        Offset o {0};
    };

    struct BaseCoordFormat1 {
        BaseCoordFormat1() = delete;
        explicit BaseCoordFormat1(int16_t c) : Coordinate(c) {}
        uint16_t BaseCoordFormat {1};
        int16_t Coordinate;
        static Offset size() { return sizeof(uint16_t) * 2; }
    };

 public:
    BASE() = delete;
    explicit BASE(hotCtx g) : g(g) {}
    void setBaselineTags(bool doVert, std::vector<Tag> &baselineTag);
    void addScript(bool doVert, Tag script, Tag dfltBaseline,
                   std::vector<int16_t> &coords);
    int Fill();
    void Write();
 private:
    int addBaseScript(int dfltInx, size_t nBaseTags, std::vector<int16_t> &coords);
    static Offset hdr_size() { return sizeof(int32_t) + sizeof(uint16_t) * 2; }
    Offset fillAxis(bool doVert);
    Offset fillSharedData();
    void writeSharedData();
    int32_t addCoord(int16_t c);

    /* Shared data */
    std::vector<BaseScriptInfo> baseScript;
    std::vector<int16_t> coord;

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
    std::vector<BaseCoordFormat1> bcf1;

    hotCtx g;    /* Package context */
};

#endif  // ADDFEATURES_HOTCONV_BASE_H_
