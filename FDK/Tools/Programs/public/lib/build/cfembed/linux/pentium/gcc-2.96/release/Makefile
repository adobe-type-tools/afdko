# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/cfembed
LIB_SRCS = $(SRC_DIR)/cfembed.c
LIB_OBJS = cfembed.o
LIB_TARGET = $(LIB_DIR)/cfembed.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
cfembed.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cfembed.c -o $@
# AUTO-GENERATED DEPENDENCIES
cfembed.o: ../../../../../../../source/lib/cfembed/cfembed.c \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/cfembed.h \
  ../../../../../../../public/api/sfntwrite.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/sfwerr.h \
  ../../../../../../../public/api/ceferr.h \
  ../../../../../../../public/api/t1read.h \
  ../../../../../../../public/api/t1rerr.h \
  ../../../../../../../public/api/cffread.h \
  ../../../../../../../public/api/cfrerr.h \
  ../../../../../../../public/api/ttread.h \
  ../../../../../../../public/api/ttrerr.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/sfrerr.h
