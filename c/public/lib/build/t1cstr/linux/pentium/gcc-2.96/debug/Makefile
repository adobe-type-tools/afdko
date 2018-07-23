# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DT1C_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/t1cstr
LIB_SRCS = $(SRC_DIR)/t1cstr.c
LIB_OBJS = t1cstr.o
LIB_TARGET = $(LIB_DIR)/t1cstr.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
t1cstr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/t1cstr.c -o $@

# AUTO-GENERATED DEPENDENCIES
t1cstr.o: ../../../../../../../source/lib/t1cstr/t1cstr.c \
  ../../../../../../../public/api/t1cstr.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/absfont.h \
  ../../../../../../../public/api/abfdesc.h \
  ../../../../../../../public/api/abferr.h \
  ../../../../../../../public/api/t1cerr.h \
  ../../../../../../../public/resource/txops.h
