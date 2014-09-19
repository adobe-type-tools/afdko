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
CONFIG = release
CFLAGS = $(STD_OPTS) -O2

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
