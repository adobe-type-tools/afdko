#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = release

# Library
ROOT_DIR = ../../../../..
SRC_DIR = $(ROOT_DIR)/source/hotconv
OBJECT_DIR = .
LIB_TARGET = $(LIB_DIR)/hotconv.a

CFLAGS = $(STD_OPTS) -I$(ROOT_DIR)/api \
					 -I$(ROOT_DIR)/resource \
					 -I$(ROOT_DIR)/build/hotpccts/pccts/h \
					 -I$(ROOT_DIR)/../../public/lib/api \
					 -I$(ROOT_DIR)/../../public/lib/resource \
					 -DHOT_FEAT_SUPPORT=1
					 
LIB_SRCS = \
	$(SRCDIR)/anon.c \
	$(SRCDIR)/BASE.c \
	$(SRCDIR)/CFF_.c \
	$(SRCDIR)/cmap.c \
	$(SRCDIR)/featerr.c \
	$(SRCDIR)/featgram.c \
	$(SRCDIR)/featscan.c \
	$(SRCDIR)/fvar.c \
	$(SRCDIR)/GDEF.c \
	$(SRCDIR)/GPOS.c \
	$(SRCDIR)/GSUB.c \
	$(SRCDIR)/head.c \
	$(SRCDIR)/hhea.c \
	$(SRCDIR)/hmtx.c \
	$(SRCDIR)/hot.c \
	$(SRCDIR)/map.c \
	$(SRCDIR)/maxp.c \
	$(SRCDIR)/MMFX.c \
	$(SRCDIR)/MMSD.c \
	$(SRCDIR)/name.c \
	$(SRCDIR)/OS_2.c \
	$(SRCDIR)/otl.c \
	$(SRCDIR)/post.c \
	$(SRCDIR)/sfnt.c \
	$(SRCDIR)/vhea.c \
	$(SRCDIR)/vmtx.c \
	$(SRCDIR)/VORG.c
	
LIB_OBJS = \
	$(OBJECT_DIR)/anon.o \
	$(OBJECT_DIR)/BASE.o \
	$(OBJECT_DIR)/CFF_.o \
	$(OBJECT_DIR)/cmap.o \
	$(OBJECT_DIR)/featerr.o \
	$(OBJECT_DIR)/featgram.o \
	$(OBJECT_DIR)/featscan.o \
	$(OBJECT_DIR)/fvar.o \
	$(OBJECT_DIR)/GDEF.o \
	$(OBJECT_DIR)/GPOS.o \
	$(OBJECT_DIR)/GSUB.o \
	$(OBJECT_DIR)/head.o \
	$(OBJECT_DIR)/hhea.o \
	$(OBJECT_DIR)/hmtx.o \
	$(OBJECT_DIR)/hot.o \
	$(OBJECT_DIR)/map.o \
	$(OBJECT_DIR)/maxp.o \
	$(OBJECT_DIR)/MMFX.o \
	$(OBJECT_DIR)/MMSD.o \
	$(OBJECT_DIR)/name.o \
	$(OBJECT_DIR)/OS_2.o \
	$(OBJECT_DIR)/otl.o \
	$(OBJECT_DIR)/post.o \
	$(OBJECT_DIR)/sfnt.o \
	$(OBJECT_DIR)/vhea.o \
	$(OBJECT_DIR)/vmtx.o \
	$(OBJECT_DIR)/VORG.o

# Standard definitions
include  $(ROOT_DIR)/../../public/lib/config/linux/gcc/gcc.mak

	
# Lib Object rules
anon.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/anon.c -o $@

BASE.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/BASE.c -o $@

CFF_.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/CFF_.c -o $@

cmap.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cmap.c -o $@

featerr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/featerr.c -o $@

featgram.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/featgram.c -o $@

featscan.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/featscan.c -o $@

fvar.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/fvar.c -o $@

GDEF.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/GDEF.c -o $@

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

hot.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/hot.c -o $@

map.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/map.c -o $@

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

otl.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/otl.c -o $@

post.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/post.c -o $@

sfnt.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sfnt.c -o $@

vhea.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/vhea.c -o $@

vmtx.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/vmtx.c -o $@

VORG.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/VORG.c -o $@
