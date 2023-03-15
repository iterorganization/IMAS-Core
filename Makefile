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
DBGFLAGS+= -g3 -ggdb
DBGFLAGS+= -DDEBUG
OPTFLAG=-O0
else
OPTFLAG=-O3
endif
ifeq (${STOPONEXCEPT},yes)
DBGFLAGS+= -DSOE
endif


ifeq "$(strip $(CC))" "icc"
## intel compiler should be >= 13 to meet C++11 requirement
CXX=icpc
CFLAGS=-std=c99 -Wall -fPIC ${OPTFLAG} -shared-intel ${DBGFLAGS}
CXXFLAGS=-std=c++11 -pedantic -Wall -fPIC ${OPTFLAG} -fno-inline-functions -shared-intel ${DBGFLAGS}
LDF=ifort -lc -lstdc++
else
CFLAGS=-std=c11 -pedantic -Wall -fPIC ${OPTFLAG} ${DBGFLAGS}
CXXFLAGS=-std=c++11 -pedantic -Wall -fPIC ${OPTFLAG} -fno-inline-functions ${DBGFLAGS}
LDF=gfortran -lc -lstdc++
endif

CXXINCLUDES= -DASCII ${INCLUDES} -I. -I$(BOOST_HOME)/include 
LIBS+= -lboost_filesystem -lboost_system

CPPSRC= ual_utilities.cpp ual_backend.cpp ual_lowlevel.cpp ual_context.cpp access_layer_plugin_manager.cpp \
	ual_const.cpp ual_exception.cpp no_backend.cpp \
	memory_backend.cpp ascii_backend.cpp

CSRC= 

COMMON_OBJECTS= ual_lowlevel.o ual_context.o ual_const.o access_layer_plugin_manager.o \
		ual_exception.o ual_utilities.o ual_backend.o no_backend.o \
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
	$(RM) *.d *~ ual_defs.h $(INSTALL)/include/*.h
	$(RM) -r $(INSTALL)/documentation/dev

test: $(TARGETS)
	@echo "Do nothing"
#	$(MAKE) -C tests
test_clean:
	@echo "Do nothing"
#	$(MAKE) -C tests clean

# Create embedded documentation
doc: latex/files.tex html/files.html
latex/files.tex html/files.html:
	doxygen Doxyfile || $(RM) $@

clean-doc:
	$(RM) -r latex html

# If only sources generation is requested
sources: ual_defs.h

# Create ual_defs.h
ual_defs.h: ual_defs.h.in
	sed \
		-e "s|@@UAL_VERSION@@|$(AL_SHORT_DESCRIBE)|g" \
		-e "s|@@DD_VERSION@@|$(DD_SHORT_DESCRIBE)|g" \
		$< > $@ 

# Create dependency files
%.d: %.c | sources
	@set -e; $(RM) $@; \
	$(CC) -MM $(CFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

%.d: %.cpp | sources
	@set -e; $(RM) $@; \
	$(CXX) -MM $(CXXFLAGS) $(CXXINCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

# Includes all dependency files
-include $(CSRC:.c=.d)
-include $(CPPSRC:.cpp=.d)


%.o: %.cpp | sources
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -o $@ -c $<

%.o: %.c | sources
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

# add pkgconfig pkgconfig_install targets
PC_FILES = al-lowlevel.pc

#----------------------- pkgconfig ---------------------
include ../Makefile.pkgconfig
