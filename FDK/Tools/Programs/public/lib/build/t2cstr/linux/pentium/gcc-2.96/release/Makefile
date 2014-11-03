# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/t2cstr
LIB_SRCS = $(SRC_DIR)/t2cstr.c
LIB_OBJS = t2cstr.o
LIB_TARGET = $(LIB_DIR)/t2cstr.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
t2cstr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t2cstr.c -o $@
# AUTO-GENERATED DEPENDENCIES
t2cstr.o: ../../../../../../../source/lib/t2cstr/t2cstr.c \
  ../../../../../../../public/api/t2cstr.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/t2cerr.h \
  ../../../../../../../public/resource/txops.h
