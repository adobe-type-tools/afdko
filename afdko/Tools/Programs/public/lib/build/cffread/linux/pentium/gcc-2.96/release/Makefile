# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/cffread
LIB_SRCS = $(SRC_DIR)/cffread.c
LIB_OBJS = cffread.o
LIB_TARGET = $(LIB_DIR)/cffread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
cffread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffread.c -o $@
# AUTO-GENERATED DEPENDENCIES
cffread.o: ../../../../../../../source/lib/cffread/cffread.c \
  ../../../../../../../public/api/cffread.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfrerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/t2cstr.h \
  ../../../../../../../public/api/t2cerr.h \
  ../../../../../../../public/resource/dictops.h \
  ../../../../../../../public/resource/txops.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/sfrerr.h \
  ../../../../../../../public/resource/stdenc0.h \
  ../../../../../../../public/resource/exenc0.h \
  ../../../../../../../public/resource/stdstr1.h \
  ../../../../../../../public/resource/isocs0.h \
  ../../../../../../../public/resource/excs0.h \
  ../../../../../../../public/resource/exsubcs0.h
