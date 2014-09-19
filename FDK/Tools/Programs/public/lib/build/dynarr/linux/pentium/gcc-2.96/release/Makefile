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
SRC_DIR = $(ROOT_DIR)/source/lib/dynarr
LIB_SRCS = $(SRC_DIR)/dynarr.c
LIB_OBJS = dynarr.o
LIB_TARGET = $(LIB_DIR)/dynarr.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
dynarr.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/dynarr.c -o $@
# AUTO-GENERATED DEPENDENCIES
dynarr.o: ../../../../../../../source/lib/dynarr/dynarr.c \
  ../../../../../../../public/api/dynarr.h \
  ../../../../../../../public/api/ctlshare.h
