# -*- makefile -*- #
include ../Makefile.common

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
LIBS= -L$(MDSPLUS_DIR)/lib -lMdsObjectsCppShr


CPPSRC= ual_backend.cpp ual_lowlevel.cpp ual_context.cpp context_test.cpp ual_const.cpp \
	mdsplus_backend.cpp memory_backend.cpp 
CSRC=   lowlevel_test.c ual_low_level.c test_lowlevel.c 

COMMON_OBJECTS= ual_lowlevel.o ual_context.o ual_const.o \
		ual_low_level.o ual_backend.o \
		mdsplus_backend.o memory_backend.o 


#-------------- Options for UDA ----------------
ifneq ("no","$(strip $(IMAS_UDA))")
 INCLUDES+= -DUDA `pkg-config --cflags uda-fat-cpp`
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
sources_install: $(wildcard *.c *.h)
	$(mkdir_p) $(datadir)/src/lowlevel
	$(INSTALL_DATA) $^ $(datadir)/src/lowlevel

install: all pkgconfig_install sources_install
	$(mkdir_p) $(libdir) $(includedir) $(docdir)/dev/lowlevel
	for OBJECT in *.so; do \
		$(INSTALL_DATA) -T $$OBJECT $(libdir)/$$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO); \
	   	ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO) $(libdir)/$$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR); \
	   	ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO) $(libdir)/$$OBJECT.$(IMAS_MAJOR); \
	   	ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO) $(libdir)/$$OBJECT; \
	done
	$(INSTALL_DATA) ual_low_level.h $(includedir)
	$(INSTALL_DATA) matlab_adapter.h $(includedir)
	$(INSTALL_DATA) ual_defs.h $(includedir)
	$(INSTALL_DATA) ual_lowlevel.h $(includedir)
	$(INSTALL_DATA) ual_backend.h $(includedir)
	$(INSTALL_DATA) ual_context.h $(includedir)
	$(INSTALL_DATA) ual_exception.h $(includedir)
	$(INSTALL_DATA) ual_const.h $(includedir)
	cp -r latex html $(docdir)/dev/lowlevel

clean: pkgconfig_clean
	$(RM) *.o *.mod *.a *.so

clean-src: clean clean-doc
	$(RM) *.d *~ $(INSTALL)/include/*.h
	$(RM) -r $(INSTALL)/documentation/dev


# Create embedded documentation
doc:
	doxygen Doxyfile

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
	$(CXX) -MM $(CXXFLAGS) $(INCLUDES) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	$(RM) $@.$$$$

# Includes all dependency files
-include $(CSRC:.c=.d)
-include $(CPPSRC:.cpp=.d)


%.o: %.cpp 
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ -c $< 

%.o: %.c 
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $< 



# dynamic library
libimas.so: $(COMMON_OBJECTS) 
	$(CXX) -g -o $@ -Wl,-z,defs -shared -Wl,-soname,$@.$(IMAS_MAJOR).$(IMAS_MINOR) $^ $(LIBS)
	ln -svfT $@ $@.$(IMAS_MAJOR).$(IMAS_MINOR)

# static library
libimas.a: $(COMMON_OBJECTS) 
	$(AR) rvs $@ $^



PC_FILES = imas-lowlevel.pc
#----------------------- pkgconfig ---------------------
include ../Makefile.pkgconfig
