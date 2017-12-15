# Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
# This software is licensed as OpenSource, under the Apache License, Version 2.0. 
# This license is available at: http://opensource.org/licenses/Apache-2.0.

# Configuration
CONFIG = debug
CFLAGS = $(STD_OPTS) -g -DPST_DEBUG=1

# Library
SRC_DIR = $(ROOT_DIR)/source/lib/pstoken
LIB_SRCS = $(SRC_DIR)/pstoken.c
LIB_OBJS = pstoken.o
LIB_TARGET = $(LIB_DIR)/pstoken.a

# Build targets
TARGETS = $(LIB_TARGET) $(PRG_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
pstoken.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/pstoken.c -o $@
# AUTO-GENERATED DEPENDENCIES
pstoken.o: ../../../../../../../source/lib/pstoken/pstoken.c \
  ../../../../../../../public/api/pstoken.h \
  ../../../../../../../public/api/ctlshare.h \
  ../../../../../../../public/api/psterr.h \
  ../../../../../../../public/api/dynarr.h
