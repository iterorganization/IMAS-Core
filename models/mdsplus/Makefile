include ../../../Makefile.common

ifneq ("yes","$(strip $(IMAS_MDSPLUS))")
all sources sources_install sources_uninstall install uninstall clean clean-src:
	$(warning "Ignoring models/mdsplus (IMAS_MDSPLUS=no).")
else

FC_g95=g95
COPTS_g95 = -r8 -ftrace=full -fPIC -fno-second-underscore
INCDIR_g95=-Iamd64_g95

FC_nvfortran=nvfortran
COPTS_nvfortran = -r8  -Mnosecond_underscore -fPIC -module=./amd64_nvfortran
INCDIR_nvfortran=-Iamd64_nvfortran

## Adding DEBUG=yes to make command to print additional debug info
DBGFLAGS= -g
ifeq (${DEBUG},yes)
DBGFLAGS+= -g3 -ggdb
DBGFLAGS+= -DDEBUG
OPTFLAG=-O0
else
OPTFLAG=-O3
endif

ifneq ($(SYSTEM),MacOS)
CC=gcc
CXX=g++
else
CC=clang
CXX=clang++
endif
LD=$(CXX)
CFLAGS=-std=c11 --pedantic -Wall -fPIC ${OPTFLAG} ${DBGFLAGS}
CXXFLAGS=-std=c++11 --pedantic -Wall -fPIC ${OPTFLAG} ${DBGFLAGS} -fno-inline-functions
LDFLAGS=${DBGFLAGS} #-pthread

# Ensure _JAVA_OPTIONS does not override/conflict with JFLAGS
_JAVA_OPTIONS=
JFLAGS=-Xms1g -Xmx8g -XX:+UseG1GC

IDSDEF= ../../../xml/IDSDef.xml
XSDDIR= $(dir $(IDSDEF))

# ids_path equals to this dir:
ids_path=$(AL)/lowlevel/models/mdsplus

# Shorthand for ids_model.* files
IDS_MODEL=ids_model.characteristics ids_model.datafile ids_model.tree

# N.B. SAXON, JTRAVERSERJAR and COMPILETREECLASS are obtained in Makefile.classpath

all: models alport

sources: ids.xml alport/ids.xml
sources_install:
sources_uninstall:

# Include OS-specific Makefile, if exists.
ifneq (,$(wildcard Makefile.$(SYSTEM)))
include Makefile.$(SYSTEM)
else
install uninstall:
endif

clean:
	$(RM) $(IDS_MODEL) alport/convert_aos.o alport/convert_aos

clean-src: clean
	$(RM) ids.xml alport/ids.xml

ids.xml: IDSDef2MDSpreTree.xsl $(IDSDEF) | saxonicajar
	$(SAXON) -threads:4 -t -warnings:fatal DD_GIT_DESCRIBE=$(DD_GIT_DESCRIBE) AL_GIT_DESCRIBE=$(AL_GIT_DESCRIBE) -s:$(IDSDEF) -xsl:$< > $@



# All IDS_MODEL files are generated at once, enforce nonparallel execution,
$(filter-out ids_model.characteristics,$(IDS_MODEL)): ids_model.characteristics
ids_model.characteristics: ids.xml | jtraverserjar compiletreeclass
	$(RM) $(IDS_MODEL) -f
ifneq ($(SYSTEM),MacOS)
	ids_path=$(ids_path) $(JAVA) $(JFLAGS) -cp "$(JTRAVERSERJAR)" $(COMPILETREECLASS) ids 2>&1 | tee CompileTree.log
else
	ids_path=$(ids_path) $(JAVA) $(JFLAGS) -Djava.library.path=${MDSPLUS_DIR}/lib:${MDSPLUS_DIR}/lib64 -cp $(JTRAVERSERJAR) $(COMPILETREECLASS) ids 2>&1 | tee CompileTree.log
endif
	test 0 -eq "`grep -c "Error:" CompileTree.log`" && \
	test 0 -eq "`grep -c "Erreur :" CompileTree.log`" && \
	test 0 -eq "`grep -c "Exception" CompileTree.log`" || { $(RM) $(IDS_MODEL) ; exit 1 ;}
	$(RM) CompileTree.log

models: $(IDS_MODEL)

alport: alport/convert_aos alport/ids.xml

alport/convert_aos.o: alport/convert_aos.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

alport/convert_aos: alport/convert_aos.o
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

alport/ids.xml: alport/IDSDef2MDSpreTreeWithAoS.xsl
	$(SAXON) -threads:4 -t -warnings:fatal -s:$(IDSDEF) -xsl:$< > $@

#----------------------- classpath deps ---------------------
include ../../../Makefile.classpath

endif # IMAS_MDSPLUS
