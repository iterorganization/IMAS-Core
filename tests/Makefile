include ../../Makefile.common

ifeq (,$(wildcard $(UAL)))
$(error $$UAL is unset)
endif

CXX=g++
LD=$(CXX)

# Include OS-specific Makefile, if exists.
ifneq (,$(wildcard Makefile.$(SYSTEM)))
include Makefile.$(SYSTEM)
endif

all:$(BINOUT)

clean:
	$(RM) $(BINOUT) *~ *.o

$(BINOUT): testlowlevel.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

testlowlevel.o: testlowlevel.cpp
	$(CXX) $(INCDIR) $(CXXFLAGS) -c $<
	
test: $(BINOUT)
	./$(BINOUT)
