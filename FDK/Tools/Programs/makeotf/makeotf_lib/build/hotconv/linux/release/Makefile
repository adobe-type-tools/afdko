#########################################################################
#                                                                       #
# Copyright 1997-1999 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
# Patents Pending                                                       #
#                                                                       #
# NOTICE: All information contained herein is the property of Adobe     #
# Systems Incorporated. Many of the intellectual and technical          #
# concepts contained herein are proprietary to Adobe, are protected     #
# as trade secrets, and are made available only to Adobe licensees      #
# for their internal use. Any reproduction or dissemination of this     #
# software is strictly forbidden unless prior written permission is     #
# obtained from Adobe.                                                  #
#                                                                       #
# PostScript and Display PostScript are trademarks of Adobe Systems     #
# Incorporated or its subsidiaries and may be registered in certain     #
# jurisdictions.                                                        #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2 -DHOT_FEAT_SUPPORT=1

# Library
SRC_DIR = $(ROOT_DIR)/source/hotconv
LIB_SRCS = \
	$(SRC_DIR)/hot.c \
	$(SRC_DIR)/BASE.c \
	$(SRC_DIR)/CFF_.c \
	$(SRC_DIR)/cmap.c \
	$(SRC_DIR)/fvar.c \
	$(SRC_DIR)/GPOS.c \
	$(SRC_DIR)/GSUB.c \
	$(SRC_DIR)/head.c \
	$(SRC_DIR)/hhea.c \
	$(SRC_DIR)/hmtx.c \
	$(SRC_DIR)/maxp.c \
	$(SRC_DIR)/MMFX.c \
	$(SRC_DIR)/MMSD.c \
	$(SRC_DIR)/name.c \
	$(SRC_DIR)/OS_2.c \
	$(SRC_DIR)/post.c \
	$(SRC_DIR)/vhea.c \
	$(SRC_DIR)/vmtx.c \
	$(SRC_DIR)/VORG.c \
	$(SRC_DIR)/anon.c \
	$(SRC_DIR)/map.c \
	$(SRC_DIR)/otl.c \
	$(SRC_DIR)/sfnt.c \
	$(SRC_DIR)/featerr.c \
	$(SRC_DIR)/featgram.c \
	$(SRC_DIR)/featscan.c
LIB_OBJS = hot.o BASE.o CFF_.o cmap.o fvar.o GPOS.o GSUB.o head.o hhea.o \
	hmtx.o maxp.o MMFX.o MMSD.o name.o OS_2.o post.o vhea.o vmtx.o VORG.o \
	anon.o map.o otl.o sfnt.o featerr.o featgram.o featscan.o
LIB_TARGET = $(LIB_DIR)/hotconv.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../standard/linux.mak

# Auto-generated dependencies
include depend.mak

# Object rules
hot.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/hot.c -o $@

BASE.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/BASE.c -o $@

CFF_.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/CFF_.c -o $@

cmap.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cmap.c -o $@

fvar.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fvar.c -o $@

GPOS.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/GPOS.c -o $@

GSUB.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/GSUB.c -o $@

head.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/head.c -o $@

hhea.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/hhea.c -o $@

hmtx.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/hmtx.c -o $@

maxp.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/maxp.c -o $@

MMFX.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/MMFX.c -o $@

MMSD.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/MMSD.c -o $@

name.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/name.c -o $@

OS_2.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/OS_2.c -o $@

post.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/post.c -o $@

vhea.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/vhea.c -o $@

vmtx.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/vmtx.c -o $@

VORG.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/VORG.c -o $@

anon.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/anon.c -o $@

map.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/map.c -o $@

otl.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/otl.c -o $@

sfnt.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sfnt.c -o $@

featerr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/featerr.c -o $@

featgram.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/featgram.c -o $@

featscan.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/featscan.c -o $@

