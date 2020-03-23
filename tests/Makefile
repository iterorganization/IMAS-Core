include ../../Makefile.common

ifeq (,$(wildcard $(UAL)))
$(error $$UAL is unset)
endif

## Adding DEBUG=yes to make command to print additional debug info
DBGFLAGS= -g
ifeq (${DEBUG},yes)
DBGFLAGS+= -g3 -ggdb
DBGFLAGS+= -DDEBUG
endif
ifeq (${STOPONEXCEPT},yes)
DBGFLAGS+= -DSOE
endif

# Include OS-specific Makefile, if exists.
ifneq (,$(wildcard Makefile.$(SYSTEM)))
include Makefile.$(SYSTEM)
endif

all: $(BINOUT) $(BINOUT_C)

clean:
	$(RM) $(BINOUT) $(BINOUT_C) *~ *.o

$(BINOUT): testlowlevel.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(BINOUT_C): testlowlevel_c.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

testlowlevel.o: testlowlevel.cpp
	$(CXX) $(INCDIR) $(CXXFLAGS) -c $<

testlowlevel_c.o: testlowlevel_c.c
	$(CC) $(INCDIR) $(CFLAGS) -c $<
	
test: $(BINOUT) $(BINOUT_C)
	./$(BINOUT)
	./$(BINOUT_C)
