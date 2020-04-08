# -*- makefile -*- #
include ../Makefile.common

# Library interface number (used as soname suffix)
# If any interfaces have been added, removed, or changed since the last update,
# increment this number. Do not increment if it is certain the changes retain
# ABI compatibility. This may be possible if the changes are only in the
# implementation and do not change any function signatures or data structures.
# N.B. this number is not tied to the AL major version number whatsoever.
SO_NUM=5


SHELL=/bin/sh

## Adding DEBUG=yes to make command to print additional debug info
DBGFLAGS= -g
ifeq (${DEBUG},yes)
DBGFLAGS+= -DDEBUG
endif
ifeq (${STOPONEXCEPT},yes)
DBGFLAGS+= -DSOE
endif


ifeq "$(strip $(CC))" "icc"
## intel compiler should be >= 13 to meet C++11 requirement
CXX=icpc
CFLAGS=-std=c99 -Wall -fPIC -O3 -shared-intel ${DBGFLAGS}
CXXFLAGS=-std=c++11 -pedantic -Wall -fPIC -O3 -fno-inline-functions -shared-intel ${DBGFLAGS}
LDF=ifort -lc -lstdc++
else
CFLAGS=-std=c11 -pedantic -Wall -fPIC -O3 ${DBGFLAGS}
CXXFLAGS=-std=c++11 -pedantic -Wall -fPIC -O3 -fno-inline-functions ${DBGFLAGS}
LDF=gfortran -lc -lstdc++
endif

CXXINCLUDES= -DASCII ${INCLUDES}

CPPSRC= ual_backend.cpp ual_lowlevel.cpp ual_context.cpp \
	ual_const.cpp ual_exception.cpp no_backend.cpp \
	memory_backend.cpp ascii_backend.cpp

CSRC= 

COMMON_OBJECTS= ual_lowlevel.o ual_context.o ual_const.o \
		ual_exception.o ual_backend.o no_backend.o \
		memory_backend.o ascii_backend.o

# Include OS-specific Makefile, if exists.

ifneq (,$(wildcard Makefile.$(SYSTEM)))
include Makefile.$(SYSTEM)
else
all sources sources_install install uninstall:
	$(error No Makefile.$(SYSTEM) found for this system: $(UNAME_S))
endif

clean: pkgconfig_clean test_clean 
	$(RM) *.o *.mod *.a *.so *.so.* *.lib *.dll $(SPECIFIC_TOOLS)

clean-src: clean clean-doc
	$(RM) *.d *~ $(INSTALL)/include/*.h
	$(RM) -r $(INSTALL)/documentation/dev

test: $(TARGETS)
	$(MAKE) -C tests
test_clean:
	$(MAKE) -C tests clean

# Create embedded documentation
doc: latex/files.tex html/files.html
latex/files.tex html/files.html:
	doxygen Doxyfile || $(RM) $@

clean-doc:
	$(RM) -r latex html


# Creates dependency files
%.d: %.c
	@set -e; $(RM) $@; \
	$(CC) -MM $(CFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

%.d: %.cpp
	@set -e; $(RM) $@; \
	$(CXX) -MM $(CXXFLAGS) $(CXXINCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

# Includes all dependency files
-include $(CSRC:.c=.d)
-include $(CPPSRC:.cpp=.d)

printMDSplusFileVersion: printMDSplusFileVersion.cpp libimas.a
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -o $@ $^ $(LIBS) 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -o $@ -c $<

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

# add pkgconfig pkgconfig_install targets
PC_FILES = imas-lowlevel.pc
#----------------------- pkgconfig ---------------------
include ../Makefile.pkgconfig
