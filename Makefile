# -*- makefile -*- #
include ../Makefile.common

# Library interface number (used as soname suffix)
# If any interfaces have been added, removed, or changed since the last update,
# increment this number. Do not increment if it is certain the changes retain
# ABI compatibility. This may be possible if the changes are only in the
# implementation and do not change any function signatures or data structures.
# N.B. this number is not tied to the AL major version number whatsoever.
SO_NUM=4


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
 CFLAGS=-std=c99 -Wall -fPIC -O0 -shared-intel ${DBGFLAGS}
 CXXFLAGS=-std=c++11 -pedantic -Wall -fPIC -O0 -fno-inline-functions -shared-intel ${DBGFLAGS}
 LDF=ifort -lc -lstdc++ 
else
 CXX=g++
 CFLAGS=--std=c99 --pedantic -Wall -fPIC -O0 ${DBGFLAGS}
 CXXFLAGS=--std=c++11 --pedantic -Wall -fPIC -O0  -fno-inline-functions ${DBGFLAGS}
 LDF=gfortran -lc -lstdc++ 
endif


## MDSPlus install (require recent alpha tarball)
INCLUDES= -I$(MDSPLUS_DIR)/include -I.
CXXINCLUDES= ${INCLUDES}
LIBS= -L$(MDSPLUS_DIR)/lib -lMdsObjectsCppShr


CPPSRC= ual_backend.cpp ual_lowlevel.cpp ual_context.cpp context_test.cpp ual_const.cpp \
	mdsplus_backend.cpp memory_backend.cpp 
CSRC=   ual_low_level.c 

COMMON_OBJECTS= ual_lowlevel.o ual_context.o ual_const.o \
		ual_low_level.o ual_backend.o \
		mdsplus_backend.o memory_backend.o 


#-------------- Options for UDA ----------------
ifneq ("no","$(strip $(IMAS_UDA))")
 CXXINCLUDES+= -DUDA `pkg-config --cflags uda-fat-cpp`
 LIBS+= `pkg-config --libs uda-fat-cpp`
 COMMON_OBJECTS+= uda_backend.o
 CPPSRC+=uda_backend.cpp
endif

#-------------- Options for Matlab -------------
ifneq ("no","$(strip $(IMAS_MATLAB))")
 COMMON_OBJECTS+= matlab_adapter.o
 CSRC+= matlab_adapter.c
endif


TARGETS = libimas.so libimas.a


all: $(TARGETS) pkgconfig doc
sources:
sources_install: $(wildcard *.c *.h) | $(datadir)/src/lowlevel
	$(INSTALL_DATA) $^ $(datadir)/src/lowlevel

install: all libimas.so_install libimas.a_install pkgconfig_install sources_install | $(libdir) $(includedir) $(docdir)/dev/lowlevel
	$(INSTALL_DATA) ual_low_level.h matlab_adapter.h ual_defs.h ual_lowlevel.h ual_backend.h ual_context.h ual_exception.h ual_const.h \
		$(includedir)
	cp -r latex html $(docdir)/dev/lowlevel

$(libdir) $(includedir) $(docdir)/dev/lowlevel $(datadir)/src/lowlevel:
	$(mkdir_p) $@

clean: pkgconfig_clean
	$(RM) *.o *.mod *.a *.so

clean-src: clean clean-doc
	$(RM) *.d *~ $(INSTALL)/include/*.h
	$(RM) -r $(INSTALL)/documentation/dev


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


%.o: %.cpp 
	$(CXX) $(CXXFLAGS) $(CXXINCLUDES) -o $@ -c $< 

%.o: %.c 
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $< 

# Dynamic library
libimas.so.$(SO_NUM): $(COMMON_OBJECTS)
	$(CXX) -g -o $@ -Wl,-z,defs -shared -Wl,-soname,$@ $^ $(LIBS)
libimas.so: %:%.$(SO_NUM)
	$(LN_S) $< $@
libimas.so_install: %.so_install:%.so.$(SO_NUM) | $(libdir)
	$(INSTALL_DATA) $< $(libdir)
	$(LN_S) $< $(libdir)/$*.so

# Static library
libimas.a: $(COMMON_OBJECTS)
	$(AR) rvs $@ $^
libimas.a_install: %_install:% | $(libdir)
	$(INSTALL_DATA) $< $(libdir)


PC_FILES = imas-lowlevel.pc
#----------------------- pkgconfig ---------------------
include ../Makefile.pkgconfig
