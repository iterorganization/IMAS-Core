#//-*-makefile-*-
#// Temporary makefile, just for quick tests, self-explanatory

include ../Makefile.common


## intel compiler should be >= 13 to meet C++11 requirement
ifeq "$(strip $(INTEL))" "yes"
 CC=icc
 FC=ifort
 CPP=cpp
 CPC=icpc
 LDC=icpc
 LDF=ifort -lc -lstdc++ 
else
 CC=gcc
 FC=gfortran
 CPP=cpp
 CPC=g++
 LDC=g++ 
 LDF=gfortran -lc -lstdc++ 
endif

#HDF5
HDF5LIB= -L/Applications/hdf5-1.8.12-1/lib64 -lhdf5 -lhdf5_hl

## MDSPlus install (require recent alpha tarball)
MDSINC= -I$(MDSPLUS_DIR)/include/ -I.
MDSLIB= -L$(MDSPLUS_DIR)/lib/ -lMdsObjectsCppShr -lMdsLib_client

#-------------- Options for UDA ---------------
UDAINC= $(shell pkg-config --cflags uda-fat-cpp)
UDALIB= $(shell pkg-config --libs uda-fat-cpp)
#UDALIB = -L/work/imas/opt/uda/2.0.0/lib -lfatuda_cpp

## Adding DEBUG=yes to make command to print additional debug info
DEBFLAGS= -g
ifeq (${DEBUG},yes)
DEBFLAGS+= -DDEBUG
endif
ifeq (${STOPONEXCEPT},yes)
DEBFLAGS+= -DSOE
endif


ifeq "$(strip $(INTEL))" "yes"
  CPPFLAGS= 
  CFLAGS= -std=c99 -pedantic -Wall -fPIC -O0 ${DEBFLAGS}
  CPFLAGS= -std=c++11 -pedantic -Wall -fPIC -O0 -fno-inline-functions ${DEBFLAGS} ${MDSINC} ${UDAINC}
  FFLAGS= -fpp -r8 -assume no2underscore -fPIC -shared-intel ${DEBFLAGS}
  LDFLAGS= $(MDSLIB) ${UDALIB}
else
  CPPFLAGS= 
  CFLAGS= --std=c99 --pedantic -Wall -fPIC -g -O0 ${DEBFLAGS}
  CPFLAGS= --std=c++11 --pedantic -Wall -fPIC -g -O0  -fno-inline-functions ${DEBFLAGS} ${MDSINC} ${UDAINC} 
  FFLAGS= -cpp -fdefault-real-8 -fPIC -fno-second-underscore -ffree-line-length-none ${DEBFLAGS}
  LDFLAGS= $(HDF5LIB) $(MDSLIB) ${UDALIB}
endif


CPPSRC= ual_backend.cpp ual_lowlevel.cpp ual_context.cpp context_test.cpp ual_const.cpp \
	mdsplus_backend.cpp memory_backend.cpp
CSRC=   lowlevel_test.c ual_low_level.c test_lowlevel.c matlab_adapter.c

LL_OBJ= ual_lowlevel.o ual_context.o ual_const.o 

BE_OBJ= ual_backend.o mdsplus_backend.o memory_backend.o

COMMON_OBJECTS= ual_lowlevel.o ual_context.o ual_const.o \
		ual_low_level.o ual_backend.o \
		mdsplus_backend.o memory_backend.o matlab_adapter.o uda_backend.o

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
	$(RM) -f *.o *.mod *.a *.so tests/*.o tests/*.mod \
	tests/test-context tests/test-lowlevel tests/test-oldapi \
	tests/test-mdsplus tests/test-c libUALLowLevel.*

clean-src: clean clean-doc
	$(RM) -f *.d *~ $(INSTALL)/include/*.h 
	$(RM) -rf $(INSTALL)/documentation/dev


# Create embedded documentation
doc:
	doxygen Doxyfile

clean-doc:
	$(RM) -rf latex html

tests: context lowlevel mdsplus oldapi testc 


# Creates dependency files
%.d: %.c
	@set -e; rm -f $@; \
	$(CPP) -MM $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

%.d: %.cpp
	@set -e; rm -f $@; \
	$(CPP) -MM --std=c++11 $(CPPFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

# Includes all dependency files
-include $(CSRC:.c=.d)
-include $(CPPSRC:.cpp=.d)


%.o: %.cpp 
	$(CPC) $(CPFLAGS) -I. -o $@ -c $< 

%.o: %.c 
	$(CC) $(CFLAGS) -I. -o $@ -c $< 


# Test program for Context classes usage
context: ual_context.o tests/context_test.o ual_const.o
	$(LDC) $(LDFLAGS) $? -o tests/test-context 

# Test program for new Lowlevel C wrappers usage
lowlevel: ${LL_OBJ} ${BE_OBJ} tests/lowlevel_test.o
	$(LDC) $(LDFLAGS) $? -o tests/test-lowlevel 

# Test program for stack from old low level to MDSPlus backend
testc: ${LL_OBJ} ${BE_OBJ} ual_low_level.o  tests/test_lowlevel.o #matlab_adapter.o
	$(LDC) $(LDFLAGS) $? -o tests/test-c 

# Test program for old API C wrappers
oldapi: ${LL_OBJ} ${BE_OBJ} ual_low_level.o tests/ual_low_level_test.o #matlab_adapter.o
	$(LDC) $(LDFLAGS) $? -o tests/test-oldapi 

# Test program for MDSPlus backend
mdsplus: ual_context.o ual_const.o mdsplus_backend.o tests/test_mdsplus_backend.o
	$(LDC) $(LDFLAGS) $? -o tests/test-mdsplus 


# dynamic library
libimas.so: $(COMMON_OBJECTS) 
	$(LDC) -g -o $@ -Wl,-z,defs -shared -Wl,-soname,$@.$(IMAS_MAJOR).$(IMAS_MINOR) $^ $(LDFLAGS) 

# static library
libimas.a: $(COMMON_OBJECTS) 
	ar rs $@ $^



# add pkgconfig pkgconfig_install targets
PC_FILES = imas-lowlevel.pc
include ../Makefile.pkgconfig
