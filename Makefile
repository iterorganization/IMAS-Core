# -*- makefile -*- #
include ../Makefile.common

SHELL=/bin/sh

#CC=gcc
#LD=$(CC)
LD=g++

ifeq "$(strip $(CC))" "icc"
 CFLAGS=-g -fPIC -shared-intel
else
 CFLAGS=-g -fPIC
endif
CXXFLAGS = -g -fPIC -D__USE_XOPEN2K8 #-I$(MDSPLUS_DIR)/include

INCDIR=-I$(MDSPLUS_DIR)/include
LIBDIR= -L. -L$(MDSPLUS_DIR)/lib64 -L$(MDSPLUS_DIR)/lib
#-L$(MDSPLUS_DIR)/lib -L$(JAVA_HOME)/jre/lib/i386
#LIBS_create= -lMdsShr  -lhdf5 -lmpi -lz
LIBS=-lTreeShr -lTdiShr -lMdsShr -lXTreeShr -lMdsIpShr -lMdsObjectsCppShr -lpthread

COMMON_OBJECTS=ual_low_level_f77.o ual_low_level.o ual_low_level_mdsplus.o ual_low_level_remote.o ual_low_level_meta.o ual_low_level_mdsobjects.o

TARGETS = timed_struct_array.h libimas.so libimas.a

#---------- Options for the catalog ------------
ifeq "$(strip $(ITM_CATALOG))" "yes"
 CFLAGS+= -DUSE_ITM_CATALOG
 INCDIR+= -I$(ITM_CATALOG_DIR)/include
 LIBDIR+= -L$(ITM_CATALOG_DIR)/lib -L/usr/lib64/mysql
 LIBS+= -lItmCatalog -lmysqlclient
 COMMON_OBJECTS+= ual_catalog.o $(ITM_CATALOG_DIR)/*.o
endif
#-----------------------------------------------

#-------------- Options for UDA ---------------
ifeq "$(strip $(UDA))" "yes"
 CFLAGS+= -DIDAM `pkg-config --cflags uda-client`
 LIBS+= `pkg-config --libs uda-client`
 COMMON_OBJECTS+=ual_low_level_idam.o
else
 COMMON_OBJECTS+=ual_low_level_idam_dummy.o
endif
#-----------------------------------------------

#-------------- Options for HDF5 ---------------
ifeq "$(strip $(HDF5))" "yes"
 CFLAGS+= -DHDF5
 INCDIR+= -I$(HDF5_DIR)/include -I$(MPI_DIR)/include
 LIBDIR+= -L$(HDF5_DIR)/lib -L$(MPI_DIR)/lib
 LIBS+= -lhdf5 $(MPI_LIB)
 COMMON_OBJECTS+= ual_low_level_hdf5.o
endif
#-----------------------------------------------

#-------------- Options for java ---------------
ifeq "$(strip $(JAVA))" "yes"
 INCDIR+= -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
 LIBDIR+= -L$(JAVA_HOME)/jre/lib/amd64
 #LIBS+= -ljava
 COMMON_OBJECTS+= ual_jni.o
endif
#-----------------------------------------------

all: $(TARGETS) pkgconfig
sources:
sources_install: $(wildcard *.c *.h)
	install -d $(INSTALL)/share/src/lowlevel
	install -m 644 $^ $(INSTALL)/share/src/lowlevel

install: all pkgconfig_install sources_install
	mkdir -p $(INSTALL)/lib $(INSTALL)/include
	for OBJECT in *.so ;do \
		cp -vTT $$OBJECT $(INSTALL)/lib/$$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO); \
		ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO)  $(INSTALL)/lib/$$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR); \
		ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO)  $(INSTALL)/lib/$$OBJECT.$(IMAS_MAJOR); \
		ln -svfT $$OBJECT.$(IMAS_MAJOR).$(IMAS_MINOR).$(IMAS_MICRO)  $(INSTALL)/lib/$$OBJECT; \
	done
	cp ual_low_level.h $(INSTALL)/include

clean: pkgconfig_clean
	rm -f *.o *.so *.a *~ ual_low_level_wrap.c ual_low_level.py
	rm -rf build

clean-src: clean

libimas.so: $(COMMON_OBJECTS)
	$(LD) -g -o $@ -Wl,-z,defs -shared -Wl,-soname,$@.$(IMAS_MAJOR).$(IMAS_MINOR) $(COMMON_OBJECTS) $(LIBDIR) $(LIBS)

libimas.a: $(COMMON_OBJECTS)
	ar rs $@ $^

.c.o:
	$(CC) $(INCDIR) $(CFLAGS) -c $<

.cpp.o:
	$(CXX) $(INCDIR) $(CXXFLAGS) -c $<

timed_struct_array.h:
	./timed_struct_array.sh

# add pkgconfig pkgconfig_install targets
PC_FILES = imas-lowlevel.pc
include ../Makefile.pkgconfig
