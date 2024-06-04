/* Copyright 2024 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "designspace.h"

#include <libxml/tree.h>
#include <libxml/parser.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

#include "supportfp.h"

bool Designspace::checkAxes(var_axes &va) {
    if (!readFile)
        return true;
    if (va.getAxisCount() != axes.size()) {
        logger->log(sFATAL, "designspace file has different number of axes than "
                            "fvar table in font");
        return false;
    }

    for (auto &a : axes) {
        a.var_axes_index = va.getAxisIndex(a.tag);
        if (a.var_axes_index == -1) {
            logger->log(sFATAL, "axis '%s' from designspace file not found in fvar",
                        a.name.c_str());
            return false;
        }
    }
    std::sort(axes.begin(), axes.end(), [](const axis &a, const axis &b) { return a.var_axes_index < b.var_axes_index; });
    for (size_t i = 0; i < axes.size(); i++) {
        auto &vaa = va.axes[i];
        auto &a = axes[i];
        assert(a.var_axes_index == i);
        if (vaa.defaultValue != a.defaultValue || vaa.minValue != a.minValue ||
            vaa.maxValue != a.maxValue) {
            logger->log(sFATAL, "axis '%s' has different values in designspace versus fvar",
                        a.name.c_str());
            return false;
        }
    }
    return true;
}

Fixed Designspace::userizeCoord(uint16_t axisIndex, Fixed v) {
    if (!readFile) {
        logger->log(sERROR, "Design unit value used without designspace file");
        return v;
    }
    assert(axisIndex < axes.size());
    auto &a = axes[axisIndex];
    if (a.valueMap.size() == 0)
        return v;
    auto i = a.valueMap.lower_bound(v);
    if (i == a.valueMap.end()) {
        logger->log(sWARNING, "Design value %g greater than highest map value for axis '%s', "
                              "pinning to maximum axis value",
                    (float) v / 65536.0, a.name.c_str());
        return a.maxValue;
    }
    if (i->first == v)
        return i->second;
    assert(i->first > v);
    if (i == a.valueMap.begin()) {
        logger->log(sWARNING, "Design value %g less than lowest map value for axis '%s', "
                              "pinning to minimum axis value",
                    (float) v / 65536.0, a.name.c_str());
        return a.minValue;
    }
    Fixed highD = i->first, highU = i->second;
    i--;
    Fixed lowD = i->first, lowU = i->second;
    assert(lowD < v);
    float adj = (float)(highU - lowU) * (float) (v - lowD) / (float) (highD - lowD);
    return lowU + roundf(adj);
}

bool Designspace::read(const char *filename) {
    auto doc = xmlReadFile(filename, NULL, 0);
    if (doc == nullptr) {
        logger->log(sERROR, "Error reading designspace file '%s'", filename);
        return false;
    }
    auto root = xmlDocGetRootElement(doc);
    if (root == nullptr) {
        logger->log(sERROR, "Empty designspace file '%s'", filename);
        xmlFreeDoc(doc);
        return false;
    }
    readFile = true;
    auto cur = root->xmlChildrenNode;
    while (cur != nullptr) {
        if (xmlStrcmp(cur->name, (const xmlChar *) "axes") == 0)
            if (!readAxes(doc, cur))
                return false;
        cur = cur->next;
    }
    xmlFreeDoc(doc);
    return true;
}

bool Designspace::readAxes(xmlDocPtr doc, xmlNodePtr axesptr) {
    assert(axesptr != nullptr);
    auto cur = axesptr->xmlChildrenNode;
    while (cur != nullptr) {
        axis a;
        if (xmlStrcmp(cur->name, (const xmlChar *) "axis")) {
            cur = cur->next;
            continue;
        }
        auto attr = cur->properties;
        while (attr != nullptr) {
            if (xmlStrcmp(attr->name, (const xmlChar *) "name") == 0) {
                a.name = (char*) attr->children->content;
            } else if (xmlStrcmp(attr->name, (const xmlChar *) "tag") == 0) {
                a.tag = str2tag((char *) attr->children->content);
                if (a.tag == 0)
                    return false;
            } else if (xmlStrcmp(attr->name, (const xmlChar *) "hidden") == 0) {
                a.hidden = attr->children->content[0] == '1';
            } else if (xmlStrcmp(attr->name, (const xmlChar *) "values") == 0) {
                a.discrete = true;
            } else if (xmlStrcmp(attr->name, (const xmlChar *) "default") == 0) {
                if (!getFixed(attr, a.defaultValue))
                    return false;
            } else if (xmlStrcmp(attr->name, (const xmlChar *) "minimum") == 0) {
                if (!getFixed(attr, a.minValue))
                    return false;
            } else if (xmlStrcmp(attr->name, (const xmlChar *) "maximum") == 0) {
                if (!getFixed(attr, a.maxValue))
                    return false;
            }
            attr = attr->next;
        }
        auto child = cur->xmlChildrenNode;
        while (child != nullptr) {
            if (xmlStrcmp(child->name, (const xmlChar *) "map") == 0) {
                Fixed userV, designV;
                attr = child->properties;
                while (attr != nullptr) {
                    if (xmlStrcmp(attr->name, (const xmlChar *) "input") == 0) {
                        if (!getFixed(attr, userV))
                            return false;
                    } else if (xmlStrcmp(attr->name, (const xmlChar *) "output") == 0) {
                        if (!getFixed(attr, designV))
                            return false;
                    }
                    attr = attr->next;
                }
                a.valueMap.insert({designV, userV});
            }
            child = child->next;
        }
        axes.push_back(std::move(a));
        cur = cur->next;
    }
    toerr();
    return true;
}

void Designspace::toerr() {
    for (auto &a : axes) {
        std::cout << "Designspace Axis " << a.name << "(tag " << a.tag << "):" << std::endl;
        std::cout << "    min: " << a.minValue / 65536.0 << ", default: " << a.defaultValue / 65536.0 << ", max: " << a.maxValue / 65536.0 << std::endl;
        std::cout << "    map:";
        for (auto [designV, userV] : a.valueMap) {
            std::cout << userV / 65536.0 << " -> " << designV / 65536.0 << ", ";
        }
        std::cout << std::endl;
    }
}

bool Designspace::getFixed(xmlAttr *attr, Fixed &r) {
    char *end;
    float f = strtof((char *) attr->children->content, &end);
    if (end == (char *) attr->children->content) {
        logger->log(sERROR, "Invalid numeric attribute in axis '%s'", attr->children->content);
        return false;
    }
    r = pflttofix(&f);
    return true;
}

// XXX Pretty much copied from FeatCtx.cpp
ctlTag Designspace::str2tag(const std::string &tagName) {
    if (tagName.length() == 0) {
        logger->log(sERROR, "Empty tag in designspace axis element");
        return 0;
    }

    if ( tagName.length() > 4 )
        logger->log(sERROR, "Tag %s exceeds 4 characters", tagName.c_str());

    char buf[4];
    size_t l = tagName.length();
    for (size_t i = 0; i < 4; i++) {
        char c {' '};
        if (i < l) {
            c = tagName[i];
            if (c < 0x20 || c > 0x7E) {
                logger->log(sERROR, "Invalid character value %hhx in tag string", c);
                c = 0;
            }
        }
        buf[i] = c;
    }
    return CTL_TAG(buf[0], buf[1], buf[2], buf[3]);
}
