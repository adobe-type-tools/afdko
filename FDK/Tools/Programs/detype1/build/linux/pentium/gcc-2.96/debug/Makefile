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

# Program
SRC_DIR = $(ROOT_DIR)/source/exe/detype1
PRG_SRCS = $(SRC_DIR)/detype1.c
PRG_OBJS = detype1.o
PRG_TARGET = $(EXE_DIR)/detype1
MAKEALL = \
	exe/detype1

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../../config/linux/pentium/gcc-2.96.mak

# Object rules
detype1.o: 
	$(CC) $(CFLAGS) -c $(SRC_DIR)/detype1.c -o $@

# AUTO-GENERATED DEPENDENCIES
detype1.o: ../../../../../../../source/exe/detype1/detype1.c
