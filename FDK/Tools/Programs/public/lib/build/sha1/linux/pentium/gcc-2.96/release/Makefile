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
SRC_DIR = $(ROOT_DIR)/source/lib/sha1
LIB_SRCS = $(SRC_DIR)/sha1.c
LIB_OBJS = sha1.o
LIB_TARGET = $(LIB_DIR)/sha1.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
sha1.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/sha1.c -o $@

# AUTO-GENERATED DEPENDENCIES
sha1.o: ../../../../../../../source/lib/sha1/sha1.c \
  ../../../../../../../public/api/sha1.h
