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

# Program
SRC_DIR = $(ROOT_DIR)/source/exe/type1
PRG_SRCS = $(SRC_DIR)/type1.c
PRG_OBJS = type1.o
PRG_TARGET = $(EXE_DIR)/type1
MAKEALL = \
	exe/type1

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
type1.o: 
	$(CC) $(CFLAGS) -c $(SRC_DIR)/type1.c -o $@

# AUTO-GENERATED DEPENDENCIES
type1.o: ../../../../../../../source/exe/type1/type1.c
