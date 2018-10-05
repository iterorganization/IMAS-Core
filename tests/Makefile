include ../../Makefile.common

ifeq (,$(wildcard $(UAL)))
$(error $$UAL is unset)
endif

CXX=g++
LD=$(CXX)
CXXFLAGS=-std=c++11 -pthread -g -O0 -Wa,-mbig-obj -fPIC
LDFLAGS=-g #-pthread 

ifneq ("no","$(strip $(SYS_WIN))")
	INCDIR= -I$(UAL)/lowlevel
	LIBDIR= -L$(UAL)/lowlevel
	LIBS= $(UAL)/lowlevel/libimas.lib
	ifneq ("no","$(strip $(IMAS_MDSPLUS))")
		LIBDIR+= -L$(MDSPLUS_DIR)/lib
		LIBS+= $(MDSPLUS_DIR)/lib/XTreeShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsObjectsCppShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsIpShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsLib.a
		LIBS+= $(MDSPLUS_DIR)/lib/TdiShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/TreeShr.a
		LIBS+= $(MDSPLUS_DIR)/lib/MdsShr.a
		LIBS+= -lxml2 -lws2_32 -ldl -liphlpapi
	endif
	ifneq ("no","$(strip $(IMAS_UDA))")
		LIBDIR+= -L$(UDA_HOME)/lib
		LIBS+= $(UDA_HOME)/lib/libuda_cpp.a
		LIBS+= $(UDA_HOME)/lib/libportablexdr.a
		LIBS+= -lws2_32 -lssl -lcrypto
	endif
	ifneq ("no","$(strip $(IMAS_HDF5))")
		LIBDIR+= -L$(HDF5_HOME)/lib
		LIBS+= $(HDF5_HOME)/lib/libhdf5.a
		LIBS+= -ldl -lz
	endif
	BINOUT=testlowlevel.exe
	CFLAGS+= -DWIN32
	CXXFLAGS+= -DWIN32
else
    INCDIR= -I$(UAL)/lowlevel
    LIBDIR= -I$(UAL)/lowlevel
    LIBS= -limas
	ifneq ("no","$(strip $(IMAS_MDSPLUS))")
		LIBDIR+= -L$(MDSPLUS_DIR)/lib
		LIBS+= -lTreeShr -lTdiShr -lMdsShr -lMdsIpShr -lXTreeShr
	endif
	ifneq ("no","$(strip $(IMAS_UDA))")
		LIBDIR+= -L$(UDA_HOME)/lib
		LIBS+= -uda_cpp -portablexdr
	endif
	ifneq ("no","$(strip $(IMAS_HDF5))")
		LIBDIR+= -L$(HDF5_HOME)/lib
		LIBS+= -lhdf5 -ldl -lz
	endif
    BINOUT=testlowlevel
endif


all:$(BINOUT)

clean:
	$(RM) $(BINOUT) *~ *.o

$(BINOUT): testlowlevel.o 
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBDIR) $(LIBS)

testlowlevel.o: testlowlevel.cpp
	$(CXX) $(INCDIR) $(CXXFLAGS) -c $<
	
test: $(BINOUT)
	./$(BINOUT)
