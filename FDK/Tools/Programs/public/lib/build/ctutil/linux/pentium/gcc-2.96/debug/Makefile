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
SRC_DIR = $(ROOT_DIR)/source/lib/ctutil
LIB_SRCS = $(SRC_DIR)/ctutil.c
LIB_OBJS = ctutil.o
LIB_TARGET = $(LIB_DIR)/ctutil.a

# Build targets
TARGETS = $(LIB_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
ctutil.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/ctutil.c -o $@
# AUTO-GENERATED DEPENDENCIES
ctutil.o: ../../../../../../../source/lib/ctutil/ctutil.c \
  ../../../../../../../public/api/ctutil.h \
  ../../../../../../../public/api/ctlshare.h
