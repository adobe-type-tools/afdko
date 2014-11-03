# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DABF_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/absfont
LIB_SRCS = \
	$(SRC_DIR)/absfont.c \
	$(SRC_DIR)/absfont_compare.c \
	$(SRC_DIR)/absfont_dump.c \
	$(SRC_DIR)/absfont_draw.c \
	$(SRC_DIR)/absfont_metrics.c \
	$(SRC_DIR)/absfont_path.c \
	$(SRC_DIR)/absfont_afm.c \
	$(SRC_DIR)/absfont_desc.c
LIB_OBJS = \
	absfont.o \
	absfont_compare.o \
	absfont_dump.o \
	absfont_draw.o \
	absfont_metrics.o \
	absfont_path.o \
	absfont_afm.o \
	absfont_desc.o
LIB_TARGET = $(LIB_DIR)/absfont.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
absfont.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont.c -o $@

absfont_compare.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_compare.c -o $@

absfont_dump.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_dump.c -o $@

absfont_draw.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_draw.c -o $@

absfont_metrics.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_metrics.c -o $@

absfont_path.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_path.c -o $@

absfont_afm.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_afm.c -o $@

absfont_desc.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/absfont_desc.c -o $@

# AUTO-GENERATED DEPENDENCIES
absfont.o: ../../../../../../../source/lib/absfont/absfont.c \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/resource/dictops.h
absfont_dump.o: ../../../../../../../source/lib/absfont/absfont_dump.c \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/resource/dictops.h \
  ../../../../../../../public/resource/txops.h
absfont_draw.o: ../../../../../../../source/lib/absfont/absfont_draw.c \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h
absfont_metrics.o:  \
 ../../../../../../../source/lib/absfont/absfont_metrics.c \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h
absfont_path.o: ../../../../../../../source/lib/absfont/absfont_path.c \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/dynarr.h
absfont_afm.o: ../../../../../../../source/lib/absfont/absfont_afm.c \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h
absfont_desc.o: ../../../../../../../source/lib/absfont/absfont_desc.c \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/resource/dictops.h
