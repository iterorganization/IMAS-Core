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
LIBS+= $(MDSPLUS_DIR)/lib/XTreeShr.a
LIBS+= $(MDSPLUS_DIR)/lib/MdsObjectsCppShr.a
LIBS+= $(MDSPLUS_DIR)/lib/TdiShr.a
LIBS+= $(MDSPLUS_DIR)/lib/TreeShr.a
LIBS+= $(MDSPLUS_DIR)/lib/MdsIpShr.a
LIBS+= $(MDSPLUS_DIR)/lib/MdsShr.a
LIBS+= -lxml2 -lws2_32 -ldl -liphlpapi
else
INCLUDES+= -DMDSPLUS -I$(MDSPLUS_DIR)/include -I.
LIBDIR+= -L. -L$(MDSPLUS_DIR)/lib64 -L$(MDSPLUS_DIR)/lib
LIBS+= -lTreeShr -lTdiShr -lMdsShr -lXTreeShr -lMdsIpShr -lMdsObjectsCppShr -lpthread
endif # SYS_WIN
COMMON_OBJECTS+=mdsplus_backend.o
CPPSRC+=mdsplus_backend.cpp memory_backend.cpp 
endif # IMAS_MDSPLUS

#-------------- Options for UDA ----------------
ifneq ("no","$(strip $(IMAS_UDA))")
ifneq ("no","$(strip $(SYS_WIN))")
INCLUDES+= -DUDA -I$(UDA_HOME)/include/uda
LIBDIR+= -L$(UDA_HOME)/lib
LIBS+= $(UDA_HOME)/lib/libuda_cpp.a
LIBS+= $(UDA_HOME)/lib/libportablexdr.a
LIBS+= -lws2_32 -lssl -lcrypto
else
INCLUDES+= -DUDA `pkg-config --cflags uda-cpp`
LIBS+= `pkg-config --libs uda-cpp`
LIBS+= -lssl -lcrypto
endif # SYS_WIN
COMMON_OBJECTS+= uda_backend.o
CPPSRC+=uda_backend.cpp
endif # IMAS_UDA

#-------------- Options for HDF5 ---------------
ifneq ("no","$(strip $(IMAS_HDF5))")
ifneq ("no","$(strip $(SYS_WIN))")
INCLUDES+= -DHDF5 -I$(HDF5_HOME)/include
LIBDIR+= -L$(HDF5_HOME)/lib
LIBS+= $(HDF5_HOME)/lib/libhdf5.a -ldl -lz
else
INCLUDES+= -DHDF5 -I$(HDF5_HOME)/include
LIBDIR+= -L$(HDF5_HOME)/lib
LIBS+= -lhdf5 -ldl -lz
endif # SYS_WIN
COMMON_OBJECTS+= hdf5_backend.o
CPPSRC+=hdf5_backend.cpp
endif # IMAS_HDF5

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


all: $(TARGETS) pkgconfig doc link
sources:
sources_install: $(wildcard *.c *.h) | $(datadir)/src/lowlevel
ifeq ("no","$(strip $(SYS_WIN))")
	$(INSTALL_DATA) $^ $(datadir)/src/lowlevel
endif

ifeq ("no","$(strip $(SYS_WIN))")
install: all libimas.so_install libimas.a_install pkgconfig_install sources_install | $(libdir) $(includedir) $(docdir)/dev/lowlevel
	$(INSTALL_DATA) ual_low_level.h matlab_adapter.h ual_defs.h ual_lowlevel.h ual_backend.h ual_context.h ual_exception.h ual_const.h \
		$(includedir)
	cp -r latex html $(docdir)/dev/lowlevel
else # SYS_WIN
install: all pkgconfig_install sources_install
	$(mkdir_p) $(packagedir)/lowlevel/lib
	$(mkdir_p) $(packagedir)/lowlevel/include
	# Copy libraries
	for OBJECT in `find . -type f \( -name "*.lib" -or -name "*.dll" \)`; do \
		cp $$OBJECT $(packagedir)/lowlevel/lib; \
	done
ifneq ("no","$(strip $(IMAS_MDSPLUS))")
	# Copy MDSplus libraries
	$(mkdir_p) $(packagedir)/mdsplus/lib
	cp $(MDSPLUS_DIR)/lib/XTreeShr.a $(packagedir)/mdsplus/lib
	cp $(MDSPLUS_DIR)/lib/MdsObjectsCppShr.a $(packagedir)/mdsplus/lib
	cp $(MDSPLUS_DIR)/lib/MdsIpShr.a $(packagedir)/mdsplus/lib
	cp $(MDSPLUS_DIR)/lib/MdsLib.a $(packagedir)/mdsplus/lib
	cp $(MDSPLUS_DIR)/lib/TdiShr.a $(packagedir)/mdsplus/lib
	cp $(MDSPLUS_DIR)/lib/TreeShr.a $(packagedir)/mdsplus/lib
	cp $(MDSPLUS_DIR)/lib/MdsShr.a $(packagedir)/mdsplus/lib
endif # IMAS_MDSPLUS
ifneq ("no","$(strip $(IMAS_UDA))")
	# Copy UDA libraries
	$(mkdir_p) $(packagedir)/uda/lib
	cp $(UDA_HOME)/lib/libuda_cpp.a $(packagedir)/uda/lib
	cp $(UDA_HOME)/lib/libportablexdr.a $(packagedir)/uda/lib
endif # IMAS_UDA
ifneq ("no","$(strip $(IMAS_HDF5))")
	# Copy HDF5 libraries
	$(mkdir_p) $(packagedir)/hdf5/lib
	cp $(HDF5_HOME)/lib/libhdf5.a $(packagedir)/hdf5/lib
ifneq ("no","$(strip $(IMAS_MPI))")
	# Copy Open MPI libraries
	$(mkdir_p) $(packagedir)/openmpi/lib
	cp $(MPI_HOME)/lib/mpi.a $(packagedir)/openmpi/lib
endif # IMAS_MPI
endif # IMAS_HDF5
	# Copy includes
	cp ual_low_level.h $(packagedir)/lowlevel/include
	cp matlab_adapter.h $(packagedir)/lowlevel/include
	cp ual_defs.h $(packagedir)/lowlevel/include
	cp ual_lowlevel.h $(packagedir)/lowlevel/include
	cp ual_backend.h $(packagedir)/lowlevel/include
	cp ual_context.h $(packagedir)/lowlevel/include
	cp ual_exception.h $(packagedir)/lowlevel/include
	cp ual_const.h $(packagedir)/lowlevel/include
	# Copy documentation
	$(mkdir_p) $(packagedir)/doc/lowlevel
	cp -r latex html $(packagedir)/doc/lowlevel
	# Copy system shared libraries
	$(mkdir_p) $(packagedir)/bin
	cp /mingw64/bin/libstdc++-6.dll $(packagedir)/bin
	cp /mingw64/bin/libgcc_s_seh-1.dll $(packagedir)/bin
	cp /mingw64/bin/libwinpthread-1.dll $(packagedir)/bin
	cp /mingw64/bin/libcrypto-1_1-x64.dll $(packagedir)/bin
	cp /mingw64/bin/libdl.dll $(packagedir)/bin
	cp /mingw64/bin/libxml2-2.dll $(packagedir)/bin
	cp /mingw64/bin/libiconv-2.dll $(packagedir)/bin
	cp /mingw64/bin/liblzma-5.dll $(packagedir)/bin
	cp /mingw64/bin/zlib1.dll $(packagedir)/bin
endif # SYS_WIN

$(libdir) $(includedir) $(docdir)/dev/lowlevel $(datadir)/src/lowlevel:
ifeq ("no","$(strip $(SYS_WIN))")
	$(mkdir_p) $@
endif

clean: pkgconfig_clean link_clean test_clean
	$(RM) *.o *.mod *.a *.so *.lib *.dll

clean-src: clean clean-doc
	$(RM) *.d *~ $(INSTALL)/include/*.h
	$(RM) -r $(INSTALL)/documentation/dev

test: $(TARGETS)
	$(MAKE) -C tests
test_clean:
	$(MAKE) -C tests clean

link:
LN_LIB:=$(shell cd .. && test ! -d lib && $(LN_S) lowlevel lib)
LN_INC:=$(shell cd .. && test ! -d include && $(LN_S) lowlevel include)

link_clean:
LN_LIB:=$(shell cd .. && test -d lib && $(RM) lib)
LN_INC:=$(shell cd .. && test -d include && $(RM) include)


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

# Windows dynamic library
libimas.dll: $(COMMON_OBJECTS) 
	$(CXX) -g -o $@ -shared -Wl,-soname,$@.$(SO_NUM) -Wl,--out-implib,$@.lib $^ $(LIBDIR) $(LIBS)

# Windows static library
libimas.lib: $(COMMON_OBJECTS)
	$(AR) rcvsu $@ $^
	ranlib $@

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

	
# add pkgconfig pkgconfig_install targets
PC_FILES = imas-lowlevel.pc
#----------------------- pkgconfig ---------------------
include ../Makefile.pkgconfig
