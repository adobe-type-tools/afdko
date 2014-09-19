#########################################################################
#                                                                       #
# Copyright 1997-1999 Adobe Systems Incorporated.                       #
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
# Incorporated registered in the U.S.A. and other countries.            #
#                                                                       #
#########################################################################

# Make definitions for Linux platform (Linux x86)

PLATFORM = linux
CC = gcc 
CPP = gcc -E
MKCSTRS = mkcstrs
STD_OPTS = $(XFLAGS) -Wall -ansi -pedantic \
	-I$(ROOT_DIR)/public/api \
	-I$(ROOT_DIR)/public/resource \
	-I$(ROOT_DIR)/source/shared

# Directories (relative to build directory)
ROOT_DIR = ../../../..
LIB_DIR = $(ROOT_DIR)/public/lib/$(PLATFORM)/$(CONFIG)
EXE_DIR = $(ROOT_DIR)/public/exe/$(PLATFORM)/$(CONFIG)

default: $(TARGETS)

$(LIB_TARGET): $(LIB_OBJS)
	$(AR) rv $@ $?

$(PRG_TARGET): $(PRG_OBJS) $(PRG_LIBS)
	$(CC) $(CFLAGS) -o $@ $(PRG_OBJS) $(PRG_LIBS) $(SYS_LIBS)

clean:
	rm -f $(LIB_OBJS) $(PRG_OBJS) $(MISC_CLEAN)

spotless:
	rm -f $(LIB_OBJS) $(PRG_OBJS) $(MISC_CLEAN) $(LIB_TARGET) $(PRG_TARGET)

depend: $(PRG_SRCS) $(LIB_SRCS)
	$(CPP) -MM $(CFLAGS) $(LIB_SRCS) $(PRG_SRCS) >depend.mak
