/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef SHARED_INCLUDE_DESIGNSPACE_H_
#define SHARED_INCLUDE_DESIGNSPACE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <map>
#include <vector>

#include "slogger.h"
#include "varsupport.h"

class Designspace {
 public:
    explicit Designspace(std::shared_ptr<slogger> l) : logger(l) {}
    bool read(const char *filename);
    bool checkAxes(var_axes &va);
    Fixed userizeCoord(uint16_t axisIndex, Fixed v);
    void setLogger(std::shared_ptr<slogger> l) { logger = l; }

 private:
    bool readAxes(xmlDocPtr doc, xmlNodePtr axes);
    bool getFixed(xmlAttr *attr, Fixed &r);
    ctlTag str2tag(const std::string &tagName);
    void toerr();
    struct axis {
        std::string name;
        ctlTag tag {0};
        Fixed minValue {0};
        Fixed defaultValue {0};
        Fixed maxValue {0};
        bool hidden {false};
        bool discrete {false};
        int var_axes_index {-1};
        std::map<Fixed, Fixed> valueMap;
    };

    std::vector<axis> axes;
    std::shared_ptr<slogger> logger;
    bool readFile {false};
};

#endif  // SHARED_INCLUDE_DESIGNSPACE_H_
