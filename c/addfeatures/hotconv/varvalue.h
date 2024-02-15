/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <map>
#include <memory>
#include <vector>

#include "common.h"
#include "varsupport.h"

#ifndef ADDFEATURES_HOTCONV_VARVALUE_H
#define ADDFEATURES_HOTCONV_VARVALUE_H

class VarLocation {
 public:
    struct cmpSP {
        bool operator()(const std::shared_ptr<VarLocation> &a,
                        const std::shared_ptr<VarLocation> &b) const {
            if (a == nullptr && b == nullptr)
                return false;
            if (a == nullptr)
                return true;
            if (b == nullptr)
                return false;
            return *a < *b;
        }
    };
    VarLocation() = delete;
    explicit VarLocation(std::vector<var_F2dot14> &l) : alocs(std::move(l)) {}
    explicit VarLocation(const std::vector<var_F2dot14> &l) : alocs(l) {}
    bool operator==(const VarLocation &o) const { return alocs == o.alocs; }
    bool operator<(const VarLocation &o) const { return alocs < o.alocs; }
    auto at(int i) const { return alocs.at(i); }
    auto size() const { return alocs.size(); }
    void toerr() const {
        int i {0};
        for (auto f2d : alocs) {
            if (i++ > 0)
                std::cerr << ", ";
            std::cerr << var_F2dot14ToFloat(f2d);
        }
    }
    std::vector<var_F2dot14> alocs;
};

class VarLocationMap {
 public:
    VarLocationMap() = delete;
    explicit VarLocationMap(uint16_t axis_count) {
        // Store default location first, always has index 0
        std::vector<var_F2dot14> defaxis(axis_count, 0);
        auto defloc = std::make_shared<VarLocation>(std::move(defaxis));
        getIndex(defloc);
    }
    uint32_t getIndex(std::shared_ptr<VarLocation> &l) {
        const auto [it, success] = locmap.emplace(l, static_cast<uint32_t>(locvec.size()));
        if (success)
            locvec.push_back(l);
        return it->second;
    }
    std::shared_ptr<VarLocation> getLocation(uint32_t i) {
        if (i >= locvec.size())
            return nullptr;
        return locvec[i];
    }
    void toerr() {
        int i {0};
        for (auto &loc : locvec) {
            std::cerr << i++ << ":  ";
            loc->toerr();
            std::cerr << std::endl;
        }
    }
 private:
    std::map<std::shared_ptr<VarLocation>, uint32_t, VarLocation::cmpSP> locmap;
    std::vector<std::shared_ptr<VarLocation>> locvec;
};

class VarValueRecord {
 public:
    void addValue(int16_t value) {
        assert(!seenDefault);
        seenDefault = true;
        defaultValue = value;
    }
    bool addValue(uint32_t locIndex, int16_t value, hotCtx g) {
        if (locIndex == 0) {
            if (seenDefault) {
                g->logger->msg(sERROR, "Duplicate values for default location");
                return false;
            } else {
                defaultValue = value;
                seenDefault = true;
                return true;
            }
        } else {
            const auto [it, success] = locationValues.emplace(locIndex, value);
            if (!success)
                g->logger->msg(sERROR, "Duplicate values for location");
            return success;
        }
    }
    void verifyDefault(hotCtx g) {
        if (!seenDefault)
            g->logger->msg(sERROR, "No default entry for variable value");
    }
    int16_t getDefault() { return defaultValue; }
    bool isVariable() { return locationValues.size() > 0; }
 private:
     bool seenDefault {false};
     int16_t defaultValue {0};
     uint32_t model {0};
     uint32_t index {0};
     std::map<uint32_t, int16_t> locationValues;
};

typedef std::vector<VarValueRecord> ValueVector;

#endif  // ADDFEATURES_HOTCONV_VARVALUE_H
