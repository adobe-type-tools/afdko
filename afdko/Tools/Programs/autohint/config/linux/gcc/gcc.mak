#########################################################################
#                                                                       #
# Copyright 2014 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#                                                                       #
# Patents Pending                                                       #
#                                                                       #
#########################################################################

# Make definitions for Linux platform (i86)

# Configuration
PLATFORM = linux
HARDWARE = i86
COMPILER = gcc
SYS_LIBS =  -lm
XFLAGS = -m32

# Directories (relative to build directory)
CT_LIB_DIR = $(ROOT_DIR)/../public/lib/lib/$(PLATFORM)/$(CONFIG)
EXE_DIR = $(ROOT_DIR)/exe/$(PLATFORM)/$(CONFIG)
SRC_DIR = $(ROOT_DIR)/source


STD_OPTS = $(XFLAGS)  \
	-I$(ROOT_DIR)/../public/lib/api \
	-I$(ROOT_DIR)/../public/lib/resource \
	 $(SYS_INCLUDES)
ifneq ($(strip $(OSX)),) # In order to test under Mac OSX, define OSX in the user environment.
	STD_OPTS += -DOSX=1
endif


default: $(TARGETS)

$(PRG_TARGET): $(PRG_OBJS) $(PRG_LIBS) $(PRG_SRCS)  $(PRG_INCLUDES) 
	mkdir -p $(EXE_DIR)
	$(CC) $(CFLAGS) -o $@ $(PRG_OBJS) $(PRG_LIBS) $(SYS_LIBS)

clean:
	if [ "$(LIB_OBJS)" ]; then \
			rm -f $(LIB_OBJS); \
	fi
	
	if [ "$(PRG_OBJS)" ]; then \
			rm -f $(PRG_OBJS); \
	fi

	if [ "$(PRG_LIBS)" ]; then \
			rm -f $(PRG_LIBS); \
	fi

	if [ "$(PRG_TARGET)" ]; then \
			rm -f $(PRG_TARGET); \
	fi
	

