include ../../Makefile.common

ifeq (,$(wildcard $(UAL)))
$(error $$UAL is unset)
endif

CXX=g++
LD=$(CXX)
ifneq ("no","$(strip $(SYS_WIN))")
CXXFLAGS=-std=c++11 -pthread -g -O0 -Wa,-mbig-obj -fPIC
LDFLAGS=-g -pthread
else
CXXFLAGS=-std=c++11 -pthread -g -O0 -fPIC
LDFLAGS=-g -pthread
endif

ifneq ("no","$(strip $(SYS_WIN))")

INCDIR= -I$(UAL)/lowlevel
LIBS= -L$(UAL)/lowlevel
LIBS+= $(UAL)/lowlevel/libimas.lib
ifneq ("no","$(strip $(IMAS_MDSPLUS))")
LIBS+= -L$(MDSPLUS_DIR)/lib
LIBS+= $(MDSPLUS_DIR)/lib/XTreeShr.a
LIBS+= $(MDSPLUS_DIR)/lib/MdsObjectsCppShr.a
LIBS+= $(MDSPLUS_DIR)/lib/TdiShr.a
LIBS+= $(MDSPLUS_DIR)/lib/TreeShr.a
LIBS+= $(MDSPLUS_DIR)/lib/MdsIpShr.a
LIBS+= $(MDSPLUS_DIR)/lib/MdsShr.a
LIBS+= -lxml2 -lws2_32 -ldl -liphlpapi
endif # IMAS_MDSPLUS
ifneq ("no","$(strip $(IMAS_UDA))")
LIBS+= -L$(UDA_HOME)/lib
LIBS+= $(UDA_HOME)/lib/libuda_cpp.a
LIBS+= $(UDA_HOME)/lib/libportablexdr.a
LIBS+= -lws2_32 -lssl -lcrypto
endif # IMAS_UDA
ifneq ("no","$(strip $(IMAS_HDF5))")
LIBS+= -L$(HDF5_HOME)/lib
LIBS+= $(HDF5_HOME)/lib/libhdf5.a
LIBS+= -ldl -lz
endif # IMAS_HDF5
BINOUT=testlowlevel.exe
CFLAGS+= -DWIN32
CXXFLAGS+= -DWIN32

else # SYS_WIN

INCDIR= -I$(UAL)/lowlevel
LIBS= -L$(UAL)/lowlevel
LIBS+= -limas
ifneq ("no","$(strip $(IMAS_MDSPLUS))")
LIBS+= -L$(MDSPLUS_DIR)/lib
LIBS+= -lTreeShr -lTdiShr -lMdsShr -lMdsIpShr -lXTreeShr
endif # IMAS_MDSPLUS
ifneq ("no","$(strip $(IMAS_UDA))")
LIBS+= -L$(UDA_HOME)/lib
LIBS+= `pkg-config --libs uda-cpp`
LIBS+= -lssl -lcrypto
endif # IMAS_UDA
ifneq ("no","$(strip $(IMAS_HDF5))")
LIBS+= -L$(HDF5_HOME)/lib
LIBS+= -lhdf5 -ldl -lz
endif # IMAS_HDF5
BINOUT=testlowlevel

endif # SYS_WIN


all:$(BINOUT)

clean:
	$(RM) $(BINOUT) *~ *.o

$(BINOUT): testlowlevel.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

testlowlevel.o: testlowlevel.cpp
	$(CXX) $(INCDIR) $(CXXFLAGS) -c $<
	
test: $(BINOUT)
	./$(BINOUT)
