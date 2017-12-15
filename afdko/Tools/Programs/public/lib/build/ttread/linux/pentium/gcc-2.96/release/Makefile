# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/ttread
LIB_SRCS = $(SRC_DIR)/ttread.c
LIB_OBJS = ttread.o
LIB_TARGET = $(LIB_DIR)/ttread.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
ttread.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ttread.c -o $@
# AUTO-GENERATED DEPENDENCIES
ttread.o: ../../../../../../../source/lib/ttread/ttread.c \
  ../../../../../../../public/api/ttread.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/ttrerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/api/sfntread.h \
  ../../../../../../../public/api/sfrerr.h \
  ../../../../../../../public/resource/applestd.h \
  ../../../../../../../public/resource/uv2agl.h \
  ../../../../../../../public/resource/dblmapuv.h \
  ../../../../../../../public/resource/macromn0.h
