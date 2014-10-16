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
CXXFLAGS = -g -fPIC -I$(MDSPLUS_DIR)/include

INCDIR=-I$(MDSPLUS_DIR)/include
LIBDIR= -L. -L$(MDSPLUS_DIR)/lib64 -L$(MDSPLUS_DIR)/lib
#-L$(MDSPLUS_DIR)/lib -L$(JAVA_HOME)/jre/lib/i386
#LIBS_create= -lMdsShr  -lhdf5 -lmpi -lz
LIBS=-lTreeShr -lTdiShr -lMdsShr -lXTreeShr -lMdsIpShr -lMdsObjectsCppShr


COMMON_OBJECTS=ual_low_level_f77.o ual_low_level.o ual_low_level_mdsplus.o ual_low_level_remote.o ual_low_level_meta.o ual_low_level_mdsobjects.o

TARGETS = timed_struct_array.h libUALLowLevel.so libUALLowLevel.a

#---------- Options for the catalog ------------
ifeq "$(strip $(ITM_CATALOG))" "yes"
 CFLAGS+= -DUSE_ITM_CATALOG
 INCDIR+= -I$(ITM_CATALOG_DIR)/include
 LIBDIR+= -L$(ITM_CATALOG_DIR)/lib -L/usr/lib64/mysql
 LIBS+= -lItmCatalog -lmysqlclient
 COMMON_OBJECTS+= ual_catalog.o $(ITM_CATALOG_DIR)/*.o
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

all: $(TARGETS)

install: all
	cp *.so $(INSTALL)/lib
	cp ual_low_level.h $(INSTALL)/include

clean:
	rm -f *.o *.so *.a *~ ual_low_level_wrap.c ual_low_level.py
	rm -rf build

clean-src: clean

libUALLowLevel.so: $(COMMON_OBJECTS)  
	$(LD) -g -o $@ -shared $(COMMON_OBJECTS) $(LIBDIR) $(LIBS)

libUALLowLevel.a: $(COMMON_OBJECTS)
	ar rs $@ $^

.c.o:
	$(CC) $(INCDIR) $(CFLAGS) -c $< 

timed_struct_array.h:
	./timed_struct_array.sh
