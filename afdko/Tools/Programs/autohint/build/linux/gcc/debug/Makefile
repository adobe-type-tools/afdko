#########################################################################
#                                                                       #
# Copyright 1997-2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
#########################################################################

# Configuration
CONFIG = debug
ROOT_DIR = ../../../..
OBJECT_DIR = .
LIB_DIR = $(ROOT_DIR)/autohintlib/lib/$(PLATFORM)/$(CONFIG)

CFLAGS = $(STD_OPTS) -I$(ROOT_DIR)/autohintlib/source/public/ac \
				-I$(ROOT_DIR)/autohintlib/source/public/extras \
				-I$(ROOT_DIR)/autohintlib/source/ac \
				-I$(ROOT_DIR)/autohintlib/source/bf \
				-DEXECUTABLE=1 -g

# Program
PRG_SRCS = $(SRC_DIR)/main.c
PRG_OBJS = main.o
PRG_LIBS = $(LIB_DIR)/autohintlib.a
PRG_TARGET = $(EXE_DIR)/autohintexe

# Build targets
TARGETS = $(PRG_TARGET)

# Standard definitions
include ../../../../../public/config/linux/gcc/gcc.mak

# program rule is in gcc.mak

# Lib rule
 $(LIB_DIR)/autohintlib.a:
	cd $(ROOT_DIR)/autohintlib/build/$(PLATFORM)/$(COMPILER)/$(CONFIG); $(MAKE) clean; $(MAKE);


# Object rules
main.o:
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $@

