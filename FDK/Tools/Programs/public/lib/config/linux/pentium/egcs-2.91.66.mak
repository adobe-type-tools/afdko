#########################################################################
#                                                                       #
# Copyright 2016 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#########################################################################

# Make definitions for Linux platform (Linux x86)

PLATFORM = linux
HARDWARE = pentium
COMPILER = egcs-2.91.66
CC = gcc 
CPP = gcc -E
MKCSTRS = mkcstrs
STD_OPTS = $(XFLAGS) -Wall -ansi -pedantic \
	-I$(ROOT_DIR)/public/api \
	-I$(ROOT_DIR)/public/resource \
	-I$(ROOT_DIR)/source/shared

# Directories (relative to build directory)
ROOT_DIR = ../../../../../../..
LIB_DIR = $(ROOT_DIR)/public/lib/$(PLATFORM)/$(HARDWARE)/$(COMPILER)/$(CONFIG)
EXE_DIR = $(ROOT_DIR)/public/exe/$(PLATFORM)/$(HARDWARE)/$(COMPILER)/$(CONFIG)
VAL_DIR = $(ROOT_DIR)/public/validate/$(PLATFORM)/$(HARDWARE)/$(COMPILER)/$(CONFIG)

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
