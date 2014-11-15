# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/cffwrite
LIB_SRCS = \
	$(SRC_DIR)/cffwrite.c \
	$(SRC_DIR)/cffwrite_charset.c \
	$(SRC_DIR)/cffwrite_dict.c \
	$(SRC_DIR)/cffwrite_encoding.c \
	$(SRC_DIR)/cffwrite_fdselect.c \
	$(SRC_DIR)/cffwrite_sindex.c \
	$(SRC_DIR)/cffwrite_t2cstr.c
LIB_OBJS = \
	cffwrite.o \
	cffwrite_charset.o \
	cffwrite_dict.o \
	cffwrite_encoding.o \
	cffwrite_fdselect.o \
	cffwrite_sindex.o \
	cffwrite_t2cstr.o
LIB_TARGET = $(LIB_DIR)/cffwrite.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
cffwrite.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite.c -o $@

cffwrite_charset.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_charset.c -o $@

cffwrite_dict.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_dict.c -o $@

cffwrite_encoding.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_encoding.c -o $@

cffwrite_fdselect.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_fdselect.c -o $@

cffwrite_sindex.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_sindex.c -o $@

cffwrite_t2cstr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/cffwrite_t2cstr.c -o $@
# AUTO-GENERATED DEPENDENCIES
cffwrite.o: ../../../../../../../source/lib/cffwrite/cffwrite.c \
  ../../../../../../../public/resource/txops.h \
  ../../../../../../../public/resource/dictops.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_charset.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_share.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_encoding.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_fdselect.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_sindex.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_dict.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_t2cstr.h
cffwrite_charset.o:  \
 ../../../../../../../source/lib/cffwrite/cffwrite_charset.c \
  ../../../../../../../source/lib/cffwrite/cffwrite_charset.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_share.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/resource/isocs0.h \
  ../../../../../../../public/resource/excs0.h \
  ../../../../../../../public/resource/exsubcs0.h
cffwrite_dict.o:  \
 ../../../../../../../source/lib/cffwrite/cffwrite_dict.c \
  ../../../../../../../source/lib/cffwrite/cffwrite_dict.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_share.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_sindex.h \
  ../../../../../../../public/resource/dictops.h
cffwrite_encoding.o:  \
 ../../../../../../../source/lib/cffwrite/cffwrite_encoding.c \
  ../../../../../../../source/lib/cffwrite/cffwrite_encoding.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_share.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/resource/stdenc0.h \
  ../../../../../../../public/resource/exenc0.h
cffwrite_fdselect.o:  \
 ../../../../../../../source/lib/cffwrite/cffwrite_fdselect.c \
  ../../../../../../../source/lib/cffwrite/cffwrite_fdselect.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_share.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h
cffwrite_sindex.o:  \
 ../../../../../../../source/lib/cffwrite/cffwrite_sindex.c \
  ../../../../../../../source/lib/cffwrite/cffwrite_sindex.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_share.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/resource/stdstr0.h \
  ../../../../../../../public/resource/stdstr1.h
cffwrite_t2cstr.o:  \
 ../../../../../../../source/lib/cffwrite/cffwrite_t2cstr.c \
  ../../../../../../../source/lib/cffwrite/cffwrite_t2cstr.h \
  ../../../../../../../source/lib/cffwrite/cffwrite_share.h \
  ../../../../../../../public/api/cffwrite.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/cfwerr.h \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/resource/txops.h
