#########################################################################
#                                                                       #
# Copyright 1997-2001 Adobe Systems Incorporated.                       #
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
CONFIG = debug
CFLAGS = $(STD_OPTS) -g

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
