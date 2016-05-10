#########################################################################
#                                                                       #
# Copyright 2016 Adobe Systems Incorporated.                       #
# All rights reserved.                                                  #
#########################################################################

# Make definitions for Linux platform (Linux pentium)

# Configuration
PLATFORM = linux
HARDWARE = pentium
COMPILER = gcc-2.96
CONTEXT = $(PLATFORM)/$(HARDWARE)/$(COMPILER)/$(CONFIG)
CC = gcc 
CPP = gcc -E
MKCSTRS = mkcstrs
STD_OPTS = $(XFLAGS) -Wall -ansi -pedantic \
	-I$(ROOT_DIR)/public/api \
	-I$(ROOT_DIR)/public/resource \
	-I$(ROOT_DIR)/source/shared \
	-DPLAT_LINUX=1

NONANSI_OPTS = $(XFLAGS) -Wall \
	-I$(ROOT_DIR)/public/api \
	-I$(ROOT_DIR)/public/resource \
	-I$(ROOT_DIR)/source/shared \
	-DPLAT_LINUX=1

# Directories (relative to build directory)
ROOT_DIR = ../../../../../../..
LIB_DIR = $(ROOT_DIR)/public/lib/$(CONTEXT)
EXE_DIR = $(ROOT_DIR)/public/exe/$(CONTEXT)
VAL_DIR = $(ROOT_DIR)/public/validate/$(CONTEXT)

default: $(TARGETS)

$(LIB_TARGET): $(LIB_OBJS)
	$(AR) rv $@ $?

$(PRG_TARGET): $(PRG_OBJS) $(PRG_LIBS)
	$(CC) $(CFLAGS) -o $@ $(PRG_OBJS) $(PRG_LIBS) $(SYS_LIBS)

clean:
	rm -f $(LIB_OBJS) $(PRG_OBJS) $(MISC_CLEAN)

spotless:
	rm -f $(LIB_OBJS) $(PRG_OBJS) $(MISC_CLEAN) $(LIB_TARGET) $(PRG_TARGET)

all:
	@for PROJECT in $(MAKEALL); do \
		echo "--- making $${PROJECT}"; \
		if expr $${PROJECT} : '\.\.' >/dev/null; then \
			cd $${PROJECT}; \
		else \
			cd $(ROOT_DIR)/build/$${PROJECT}/$(CONTEXT); \
		fi; \
		$(MAKE); \
	done

allclean:
	@for PROJECT in $(MAKEALL); do \
		echo "--- cleaning $${PROJECT}"; \
		if expr $${PROJECT} : '\.\.' >/dev/null; then \
			cd $${PROJECT}; \
		else \
			cd $(ROOT_DIR)/build/$${PROJECT}/$(CONTEXT); \
		fi; \
		$(MAKE) clean; \
	done

allspotless:
	@for PROJECT in $(MAKEALL); do \
		echo "--- scouring $${PROJECT}"; \
		if expr $${PROJECT} : '\.\.' >/dev/null; then \
			cd $${PROJECT}; \
		else \
			cd $(ROOT_DIR)/build/$${PROJECT}/$(CONTEXT); \
		fi; \
		$(MAKE) spotless; \
	done

alldepend:
	@rm -f $(ROOT_DIR)/Makefile.edit
	@for PROJECT in $(MAKEALL); do \
		echo "--- depending $${PROJECT}"; \
		if expr $${PROJECT} : '\.\.' >/dev/null; then \
			cd $${PROJECT}; \
		else \
			cd $(ROOT_DIR)/build/$${PROJECT}/$(CONTEXT); \
		fi; \
		$(MAKE) depend; \
	done	

depend: $(PRG_SRCS) $(LIB_SRCS)
	@rm -f Makefile.bak
	@mv -f Makefile Makefile.bak
	@sed '/^# AUTO-GENERATED DEPENDENCIES/,$$d' Makefile.bak >Makefile
	@echo '# AUTO-GENERATED DEPENDENCIES' >>Makefile
	$(CPP) -MM $(CFLAGS) $(LIB_SRCS) $(PRG_SRCS) >>Makefile
	@if cmp -s Makefile Makefile.bak; then \
		echo "dependencies up-to-date; restoring Makefile"; \
		mv -f Makefile.bak Makefile; \
	else \
		echo "dependencies changed; update Makefile on Perforce depot"; \
		echo "saving Perforce path in <clientroot>/build/Makefile.edits"; \
		PROJECT=`expr $(SRC_DIR) : '.*/\(.*/.*\)'`; \
		echo "//coretype/build/$${PROJECT}/$(CONTEXT)/Makefile" >>$(ROOT_DIR)/build/Makefile.edits; \
	fi
