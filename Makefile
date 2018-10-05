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
	CFLAGS=--std=c11 --pedantic -Wall -fPIC -O0 ${DBGFLAGS}
	CXXFLAGS=--std=c++11 --pedantic -Wall -fPIC -O0 -fno-inline-functions ${DBGFLAGS}
	LDF=gfortran -lc -lstdc++ 
endif


CPPSRC=ual_backend.cpp ual_lowlevel.cpp ual_context.cpp \
		context_test.cpp ual_const.cpp memory_backend.cpp 
CSRC=lowlevel_test.c ual_low_level.c test_lowlevel.c 

COMMON_OBJECTS= ual_lowlevel.o ual_context.o ual_const.o \
		ual_low_level.o ual_backend.o memory_backend.o 

		
#-------------- Options for MDSplus ------------
ifneq ("no","$(strip $(IMAS_MDSPLUS))")
	ifneq ("no","$(strip $(SYS_WIN))")
		INCLUDES+= -DMDSPLUS -I$(MDSPLUS_DIR)/include -I.
		LIBDIR+= -L. -L$(MDSPLUS_DIR)/lib
		#LIBS+= -lMdsShr -lTreeShr -lTdiShr -lMdsLib -lMdsIpShr -lMdsObjectsCppShr -lXTreeShr -lpthread
		LIBS+= $(MDSPLUS_DIR)/lib/XTreeShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsObjectsCppShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsIpShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsLib.a
		LIBS+= $(MDSPLUS_DIR)/lib/TdiShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/TreeShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsShr.a
		LIBS+= -lxml2 -lws2_32 -ldl -liphlpapi
	else
		INCLUDES+= -DMDSPLUS -I$(MDSPLUS_DIR)/include -I.
		LIBDIR+= -L. -L$(MDSPLUS_DIR)/lib64 -L$(MDSPLUS_DIR)/lib
		LIBS+= -lTreeShr -lTdiShr -lMdsShr -lXTreeShr -lMdsIpShr -lMdsObjectsCppShr -lpthread
	endif
	COMMON_OBJECTS+=mdsplus_backend.o
	CPPSRC+=mdsplus_backend.cpp memory_backend.cpp 
endif

#-------------- Options for UDA ----------------
ifneq ("no","$(strip $(IMAS_UDA))")
	ifneq ("no","$(strip $(SYS_WIN))")
		INCLUDES+= -DUDA -I$(UDA_HOME)/include/uda
		LIBS+= -L$(UDA_HOME)/lib
		LIBS+= $(UDA_HOME)/lib/libuda_cpp.a
		LIBS+= $(UDA_HOME)/lib/libportablexdr.a
		LIBS+= -lws2_32 -lssl -lcrypto
	else
		INCLUDES+= -DUDA `pkg-config --cflags uda-cpp`
		LIBS+= `pkg-config --libs uda-cpp`
	endif
	COMMON_OBJECTS+= uda_backend.o
	CPPSRC+=uda_backend.cpp
endif

#-------------- Options for HDF5 ---------------
ifneq ("no","$(strip $(IMAS_HDF5))")
	ifneq ("no","$(strip $(SYS_WIN))")
		INCLUDES+= -DHDF5 -I$(HDF5_HOME)/include
		LIBS+= -L$(HDF5_HOME)/lib
		LIBS+= $(HDF5_HOME)/lib/libhdf5.a -ldl -lz
	else
		INCLUDES+= -DHDF5 `pkg-config --cflags hdf5`
		LIBS+= `pkg-config --libs hdf5`
	endif
	COMMON_OBJECTS+= hdf5_backend.o
	CPPSRC+=hdf5_backend.cpp
endif

#-------------- Options for Matlab -------------
ifneq ("no","$(strip $(IMAS_MATLAB))")
	COMMON_OBJECTS+= matlab_adapter.o
	CSRC+= matlab_adapter.c
endif

#-------------- Options for Windows ------------
ifneq ("no","$(strip $(SYS_WIN))")
	CFLAGS+= -DWIN32
	CXXFLAGS+= -DWIN32
	INCDIR+= -Iwin -I$(MINGW_HOME)/mingw64/include
	TARGETS = libimas.dll libimas.lib
else
	TARGETS = libimas.so libimas.a
endif


all: $(TARGETS) pkgconfig doc
sources:
sources_install: $(wildcard *.c *.h)
	$(mkdir_p) $(datadir)/src/lowlevel
	$(INSTALL_DATA) $^ $(datadir)/src/lowlevel

install: all pkgconfig_install sources_install
	$(mkdir_p) $(libdir) $(includedir) $(docdir)/dev/lowlevel
ifeq ("no","$(strip $(SYS_WIN))")
	for OBJECT in *.so; do \
		$(INSTALL_DATA) -T $$OBJECT $(libdir)/$$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO); \
	   	ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO) $(libdir)/$$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR); \
	   	ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO) $(libdir)/$$OBJECT.$(IMAS_MAJOR); \
	   	ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO) $(libdir)/$$OBJECT; \
	done
endif
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
	$(RM) -f *.o *.mod *.a *.so *.lib *.dll
	$(RM) -rf $(libdir) $(includedir)
	cd tests && $(MAKE) clean

clean-src: clean clean-doc
	$(RM) *.d *~ $(INSTALL)/include/*.h
	$(RM) -r $(INSTALL)/documentation/dev

test: $(TARGETS)
	cd tests && $(MAKE)


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

# Windows dynamic library
libimas.dll: $(COMMON_OBJECTS) 
	$(CXX) -g -o $@ -shared -Wl,-soname,$@.$(IMAS_MAJOR).$(IMAS_MINOR) -Wl,--out-implib,$@.lib $^ $(LIBS)

# Windows static library
libimas.lib: $(COMMON_OBJECTS)
	$(AR) rcvsu $@ $^
	ranlib $@

	
# add pkgconfig pkgconfig_install targets
PC_FILES = imas-lowlevel.pc
#----------------------- pkgconfig ---------------------
include ../Makefile.pkgconfig
