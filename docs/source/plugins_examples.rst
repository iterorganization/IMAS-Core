Plugins examples
================


The ``debug`` plugin
--------------------

In this first example, we want to display the value of the field
``ids_properties/version_put/access_layer`` for a given IDS during the
execution of a ``get()`` operation.

The debug plugin is a C++ class named ``Debug_plugin``. `Figure 10`_ shows
the header code:

-  The Debug_plugin class inherits from the access_layer_plugin plugin
   interface

-  All operations of the plugin interface are declared

-  The private attributes shot, dataobjectname and occurrence will be
   initialized during the initialization of the plugin

.. literalinclude:: ./static/plugins/sources/debug_plugin.h
   :caption: **Figure 10:** Header of the Debug_plugin class
   :name: Figure 10
   :language: C++

`Figure 10a`_ shows the plugin implementation code:

-  Plugin initialization occurs in the ``begin_global_action(...)`` function.
   However, no initialization is required in this example.

-  ``read_data(...)`` calls the backend using the LL
   ``al_plugin_read_data(...)`` function. If the data type is a string, the
   plugin print its value to the screen.

-  ``begin_arraystruct_action(...)`` creates a new ``ArraystructContext`` and
   calls the backend, then the plugin prints the size of the array of
   structure.

-  The *readback* information are empty in this example (the function
   ``getReadbackName(path, index)`` returns an empty string, meaning that
   the ``debug`` plugin does not define any *readback* plugin.


.. literalinclude:: ./static/plugins/sources/debug_plugin.cpp
   :caption: **Figure 10a:** C++ plugin implementation code
   :name: Figure 10a
   :language: C++


Plugin compilation: creating a shared library
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`Figure 11`_ shows the Makefile for compiling the plugin. This makefile
will also compile the C++ HLI code ``test_debug_plugin.cpp``, which uses
this plugin (`Figure 13`_).

Executing the Makefile generates the shared library ``debug_plugin.so``
and creates an executable ``test_debug_plugin`` for the HLI test
code described later.

The ``AL_BUILD`` variable has to be set to the root directory of the IMAS
Access Layer sources. The dependency ``$AL_BUILD/lowlevel`` is required
since plugins depend on the LL API. The dependency
``$AL_BUILD/cppinterface/src`` is used only for the compilation of the HLI
test code.


.. code-block:: Makefile
   :caption: **Figure 11:** plugin compilation Makefile
   :name: Figure 11

   include ../Makefile.common

   ifeq (,$(AL_BUILD))
   AL_BUILD=../
   $(warning AL_BUILD variable unset, probably not running 'make al_test' from the Installer. Assuming AL_BUILD=$(AL_BUILD).)
   endif

   all sources sources_install install sources_uninstall uninstall clean clean-src test:

   DBGFLAGS= -g
   DBGFLAGS+= -DDEBUG -DBOOST_ALL_DYN_LINK
   NOT_SUPPORTED_COMPILER=
   CC = gcc
   CXX = g++
   ifeq "$(strip $(CC))" "icc"
   CXXFLAGS= -O0 -fPIC -shared-intel ${DBGFLAGS}
   LDFLAGS=
   else ifeq "$(strip $(CC))" "gcc"
   CXXFLAGS= -std=c++11 -pthread -O0 -fPIC ${DBGFLAGS}
   LDFLAGS= -shared -pthread -Wl,--no-undefined
   LDFLAGS_HLI=-pthread -Wl,--no-undefined
   else
   NOT_SUPPORTED_COMPILER=unsupported_compiler
   endif
   INCDIR_PKGCONFIG=`pkg-config blitz --cflags`
   INCDIR= -I$(AL_BUILD)/lowlevel
   LIBS= -lal `pkg-config blitz --libs` -L$(BOOST_ROOT)/lib -L$(AL_BUILD)/lowlevel -lboost_log -lboost_thread
   INCDIR_HLI=$(INCDIR) -I$(AL_BUILD)/cppinterface/src $(INCDIR_PKGCONFIG)
   LIBS_HLI=-L$(AL_BUILD)/cppinterface/lib $(LIBS) -lal-cpp

   OBJ_DEBUG_PLUGIN = debug_plugin.o simple_logger.o
   PLUGINS = debug_plugin

   install: all

   EXE = $(addprefix test_, $(PLUGINS))

   all: $(NOT_SUPPORTED_COMPILER) $(EXE)

   test_debug_plugin: debug_plugin
         @echo Compiling test: $@
         $(CXX) $(INCDIR_HLI) $(CXXFLAGS) -c $@.cpp -o $@.o
         $(CXX) $(LDFLAGS_HLI) -o $@ $@.o $(LIBDIR) $(LIBS_HLI)

   debug_plugin: $(OBJ_DEBUG_PLUGIN)
      @echo Linking $^
      $(CXX) $(LDFLAGS) -o $@.so $^ $(LIBDIR) $(LIBS)

   .cpp.o:
      @echo compiling $<
      $(CXX) $(INCDIR) $(INCDIR_PKGCONFIG) $(CXXFLAGS) -c $< -o $@

   clean:
      $(RM) *.log *.o *.so $(OBJ) $(EXE) $(PLUGINS)

   unsupported_compiler:
      @echo Makefile does not support $(CC) compiler \(try CC=gcc or icc\).
      exit 1


Execution of the Makefile gives the following output:

.. code-block:: console

   $ make -f Makefile_debug_plugin
   g++ `pkg-config blitz --cflags` -I/ZONE_TRAVAIL/LF218007/installer/src/3.35.0/ual/feature/al_plugins/lowlevel -std=c++11 -pthread -O0 -fPIC -g -DDEBUG -DBOOST_ALL_DYN_LINK -c debug_plugin.cpp -o debug_plugin.o
   g++ `pkg-config blitz --cflags` -I/ZONE_TRAVAIL/LF218007/installer/src/3.35.0/ual/feature/al_plugins/lowlevel -std=c++11 -pthread -O0 -fPIC -g -DDEBUG -DBOOST_ALL_DYN_LINK -c simple_logger.cpp -o simple_logger.o
   g++ -shared -pthread -Wl,--no-undefined -o debug_plugin.so debug_plugin.o simple_logger.o  -L/ZONE_TRAVAIL/LF218007/installer/src/3.35.0/ual/feature/al_plugins/lowlevel -L/Applications/libraries/boost/1.76.0/gcc/6.4.0/lib -limas `pkg-config blitz --libs` -lboost_log -lboost_thread


Client code for plugin execution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


.. md-tab-set::

    .. md-tab-item:: Using Python

        `Figure 12`_ shows a python client code to execute the ``debug`` plugin:

        -  The shot is opened using ``open(uri, mode)``

        -  The plugin ``debug`` is registered using ``register_plugin(...)``

        -  The plugin is bound to the node
           ``ids_properties/version_put/access_layer`` of the ``magnetics`` IDS,
           occurrence 0

        -  The ``get()`` operation is called for the IDS name found in the path
           which is the last argument passed to the python script

        -  Finally, the plugin is removed from memory using ``unregister_plugin(...)``


        .. code-block:: python
           :caption: **Figure 12:** Client implementation code using a python HLI
           :name: Figure 12

           import imas
           from imas import imasdef
           from imas.hli_plugins import HLIPlugins
           import sys

           uri = sys.argv[1]

           data_entry = imas.DBEntry(uri, "r")
           data_entry.open()

           HLIPlugins.register_plugin("debug")
           HLIPlugins.bind_plugin("magnetics:0/ids_properties/version_put/access_layer", "debug")
           HLIPlugins.bind_plugin("magnetics:0/flux_loop", "debug")
           data_entry.get('magnetics', occurrence=0)
           HLIPlugins.unregister_plugin("debug")
           data_entry.close()


        Before executing the plugin, we need to specify the location of the
        plugin shared library to the AL plugin framework using the environment
        variable **IMAS_AL_PLUGINS**. For example:

        .. code-block:: console

           $ export IMAS_AL_PLUGINS=/Home/LF218007/access_layer_plugins

        In the example below, we are printing the value of the field
        ``magnetics:0/ids_properties/version_put/access_layer`` for the shot
        specified by the **uri** passed to the python script. The execution of
        the python code shown above gives the following output:

        .. code-block:: console

           $ python test_debug_plugin.py imas:hdf5?path=/Imas_public/public/imasdb/west/3/54914/0/
           0000001 | [debug] : read_data - reading data for field path= ids_properties/version_put/access_layer
           0000002 | [debug] : read_data - ids_properties/access_layer= 4.10.0-2-g367115bb
           visiting AOS=flux_loop with size=17


    .. md-tab-item:: Using C++

        .. code-block:: C++
           :caption: **Figure 13:** C++ test code, file ``test_debug_plugin.cpp``
           :name: Figure 13

            #include <stdlib.h>
            #include "ALClasses.h"

            using namespace IdsNs;

            void execute(char** argv);
            void exitIfError(al_status_t &status);

            void execute(char** argv) {

               int pulse=54;
               char* userName = NULL;

               userName = getenv("USER");
               if(userName == NULL)
               {
                  printf( "PANIC: $USER not found! Exiting...");
                  exit(1);
               }

               /*   Get Full  */
               IdsNs::IDS data_entry(pulse,1,-1,-1);
               data_entry.openEnv(userName, "test", "3");

               al_status_t status = al_register_plugin("debug");
               exitIfError(status);
               printf("Using the magnetics IDS for demo purpose\n");
               al_bind_plugin("magnetics:0/ids_properties/version_put/access_layer", "debug");
               al_bind_plugin("magnetics:0/flux_loop", "debug");

               IDS::magnetics ids = data_entry._magnetics;
               ids.get(0);

               data_entry.close();
               status = al_unregister_plugin("debug");
               exitIfError(status);
            }

            void exitIfError(al_status_t &status) {
               if (status.code != 0) {
                     printf("%s\n", status.message);
                     exit(-1);
               }
            }

            int main(int argc, char** argv){
               execute(argv);
            }

        `Figure 13`_ shows a C++ client implementation test for executing the
        ``debug`` plugin. The output is the same than previously as
        expected:

        .. code-block:: console

           $ ./test_debug_plugin imas:hdf5?path=/Imas_public/public/imasdb/west/3/54914/0/
           Using the magnetics IDS for demo purpose
           0000001 | [debug] : read_data - reading data for field path= ids_properties/version_put/access_layer
           0000002 | [debug] : read_data - ids_properties/creation_date= 4.10.0-2-g367115bb
           visiting AOS=flux_loop with size=17


    .. md-tab-item:: Using gfortran

        .. code-block:: Fortran
           :caption: **Figure 14:** client implementation code using a Fortran HLI
           :name: Figure 14

            program test_debug_plugin

               use ids_routines
               implicit none

               integer :: idx, mode, status

               type (ids_magnetics) :: mag  ! Declaration of the ids
               character(STRMAXLEN) :: uri
               character(len=132):: usr
               integer :: pulse = 54
               integer :: run = 1
               integer :: pulsectx

               call get_environment_variable("USER",usr)

               ! Registering the 'debug' plugin
               call al_register_plugin ('debug', status)

               ! Binding the 'debug' plugin to the access_layer node of the magnetics IDS (as a demo purpose)
               call al_bind_plugin ('magnetics:0/ids_properties/version_put/access_layer', 'debug', status)

               ! Opening the pulse file
               call al_build_uri_from_legacy_parameters(MDSPLUS_BACKEND, pulse, run, usr, "test", "3", "", uri, status)
               call al_begin_dataentry_action(uri, OPEN_PULSE, pulsectx, status);
               write(*,*) 'Opened pulse file, pulsectx = ', pulsectx

               ! Calling 'get' will call the 'debug' plugin
               call ids_get(pulsectx,"magnetics", mag)

               call imas_close(pulsectx)

            end program test_debug_plugin


        `Figure 14`_ shows a Fortran client implementation for executing the
        ``debug`` plugin. The output is the same than previously as expected:

        .. code-block:: console

           $ ./gfortran_test_debug_plugin imas:hdf5?path=/Imas_public/public/imasdb/west/3/54914/0/
           0000001 | [debug] : read_data - reading data for field path= ids_properties/version_put/access_layer
           0000002 | [debug] : read_data - ids_properties/creation_date= 4.10.0-2-g367115bb
           visiting AOS=flux_loop with size=17


.. _`simplifying plugin code`:

Simplifying plugin code: introducing the ``AL_reader_helper_plugin`` class
--------------------------------------------------------------------------

In this section, we show how to simplify the ``debug`` plugin code
presented previously. For this purpose, we introduce the new helper
class ``AL_reader_helper_plugin`` whose part of the header (provenance
feature operations have been removed for clarity) is shown in `Figure 15`_.
`Figure 16`_ depicts its implementation code.

.. literalinclude:: ./static/plugins/sources/al_reader_helper_plugin.h
   :caption: **Figure 15:** the ``AL_reader_helper_plugin`` class header
   :name: Figure 15
   :language: C++

By inheriting the helper class, we obtain the header of the ``Debug_plugin``
class depicted in `Figure 17`_. The header code declares only the
``read_data(..)`` function (from the plugin interface) whose implementation
is overridden in the simplified implementation code of the ``Debug_plugin``
class (`Figure 18`_).


.. literalinclude:: ./static/plugins/sources/al_reader_helper_plugin.cpp
   :caption: **Figure 16:** the ``AL_reader_helper_plugin`` class implementation
   :name: Figure 16
   :language: C++

.. code-block:: C++
   :caption: **Figure 17:** the simplified ``Debug_plugin`` class header
   :name: Figure 17

   #ifndef DEBUG_PLUGIN_H
   #define DEBUG_PLUGIN_H 1
   #include "al_reader_helper_plugin.h"
   #include "access_layer_plugin.h"

   class Debug_plugin: public AL_reader_helper_plugin
   {
   public:
      Debug_plugin();
      ~Debug_plugin();

      int read_data(int ctx, const char* fieldPath, const char* timeBasePath,
   void **data, int datatype, int dim, int *size);
   };

   extern "C" access_layer_plugin* create() {
   return new Debug_plugin;
   }
   extern "C" void destroy (access_layer_plugin* al_plugin) {
   delete al_plugin;
   }
   #endif

.. code-block:: C++
   :caption: **Figure 18:** the simplified ``Debug_plugin`` class implementation
   :name: Figure 18

   #include "debug_plugin.h"
   #include "simple_logger.h"

   Debug_plugin::Debug_plugin()
   {}

   Debug_plugin::~Debug_plugin()
   {}

   void Debug_plugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
      if (*arraySize != 0)
         printf("visiting AOS=%s with size=%d\n", fieldPath, *arraySize);
   }

   int Debug_plugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath,
                  void **data, int datatype, int dim, int *size) {
      al_plugin_read_data(ctx, fieldPath, timeBasePath, data, datatype, dim, size);
      if (datatype == CHAR_DATA) {
         LOG_DEBUG << "reading data for field path= " << fieldPath;
         char buff[100];
         snprintf(buff, sizeof(buff), "%s", (char*) *data);
         std::string buffAsStdStr = buff;
         LOG_DEBUG << "ids_properties/access_layer= " << buffAsStdStr;
      }
      else {
         LOG_DEBUG << "debug plugin prints only STRING data" << fieldPath;
      }
      return 1;
   }


The ``patch_reader`` plugin
---------------------------

The patch_reader plugin is another plugin example, which shows how a
plugin is able to patch the value (on the fly) of an IDS field while it
is read using a call to the AL API ``get()`` operation.

Let us consider the field ``camera_ir/frame/surface_temperature`` of
the ``camera_ir`` IDS. This is a 2D array (representing the pixels of the
``i``\ th image of the ``frame[i]`` where frame is an array of structure). In
this example, we will patch the value of the previous field by
multiplying all pixels by a scalar (which is a parameter of the plugin)
during the read of the IDS using a call to ``get()``.

In `Figure 19`_, the ``PatchReaderPlugin`` class inherits from the
``AL_reader_helper_plugin`` class (see :ref:`Simplifying plugin code`).

.. literalinclude:: ./static/plugins/sources/patch_reader_plugin.h
   :caption: **Figure 19:** the ``PatchReaderPlugin`` class header
   :name: Figure 19
   :language: C++

`Figure 20`_ shows the implementation of the ``PatchReaderPlugin`` class:

-  In the ``read_data()`` implementation, the plugin first reads the 2D data
   from the backend using the LL ``al_plugin_read_data(...)`` function

-  The plugin multiplies each pixel of the 2D image by ``op`` which is a
   parameter (scalar integer type) of the ``PatchReaderPlugin`` class

.. literalinclude:: ./static/plugins/sources/patch_reader_plugin.cpp
   :caption: **Figure 20:** the ``PatchReaderPlugin`` class implementation
   :name: Figure 20
   :language: C++


Executing the plugin
~~~~~~~~~~~~~~~~~~~~

.. md-tab-set::

    .. md-tab-item:: Using a Python client

        .. code-block:: python
           :caption: **Figure 23:** python client code using the ``PatchReaderPlugin`` plugin
           :name: Figure 23

           import imas
           from imas import imasdef
           import matplotlib.pyplot as plt
           from imas.hli_plugins import HLIPlugins
           import numpy as np
           import sys

           uri = sys.argv[1]
           data_entry = imas.DBEntry(uri, "w")

           #Creating a new pulse file and populating with some data
           data_entry.create()
           ids = imas.camera_ir()
           ids.ids_properties.homogeneous_time = 1
           ids.time.resize(1)
           ids.time[0] = 1
           ids.frame.resize(2)
           NX = 5
           NY = 15
           for k in range(2):
              ids.frame[k].surface_temperature.resize(NX, NY);
              for j in range(NY):
              for i in range(NX):
                 ids.frame[k].surface_temperature[i,j] = float (i + j + k)
           for i in range(2):
           print("displaying frame ", i)
           print(ids.frame[i].surface_temperature)

           data_entry.put(ids)

           HLIPlugins.register_plugin("patch_reader")
           HLIPlugins.setvalue_parameter_plugin("op", 2, "patch_reader")

           HLIPlugins.bind_plugin("camera_ir:0/frame/surface_temperature", "patch_reader")

           ids = data_entry.get('camera_ir')

           for i in range(2):
           print("displaying patched frame ", i)
           print(ids.frame[i].surface_temperature)

           data_entry.close()

        The execution of the python client code gives the following output:

        .. code-block:: console

           $ python test_patch_reader_plugin imas:hdf5?path=/Home/LF218007/public/imasdb/test/3/55000/0

           displaying frame  0
           [[ 0.  1.  2.  3.  4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14.]
            [ 1.  2.  3.  4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15.]
            [ 2.  3.  4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15. 16.]
            [ 3.  4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15. 16. 17.]
            [ 4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15. 16. 17. 18.]]
           displaying frame  1
           [[ 1.  2.  3.  4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15.]
            [ 2.  3.  4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15. 16.]
            [ 3.  4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15. 16. 17.]
            [ 4.  5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15. 16. 17. 18.]
            [ 5.  6.  7.  8.  9. 10. 11. 12. 13. 14. 15. 16. 17. 18. 19.]]

           0000001 | [debug] : setParameter - setting op parameter :op to: 2
           displaying patched frame  0
           [[ 0.  2.  4.  6.  8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28.]
            [ 2.  4.  6.  8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30.]
            [ 4.  6.  8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30. 32.]
            [ 6.  8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30. 32. 34.]
            [ 8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30. 32. 34. 36.]]
           displaying patched frame  1
           [[ 2.  4.  6.  8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30.]
            [ 4.  6.  8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30. 32.]
            [ 6.  8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30. 32. 34.]
            [ 8. 10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30. 32. 34. 36.]
            [10. 12. 14. 16. 18. 20. 22. 24. 26. 28. 30. 32. 34. 36. 38.]]


    .. md-tab-item:: Using a C++ client

        .. code-block:: C++
           :caption: **Figure 21:** C++ client code using the ``PatchReaderPlugin`` plugin
           :name: Figure 21

           #include <stdlib.h>
           #include "ALClasses.h"

           using namespace IdsNs;

           void execute(char** argv);
           void exitIfError(al_status_t &status);

           void execute(char** argv) {

             int pulse=60;
             char* userName = NULL;

             userName = getenv("USER");
             if(userName == NULL)
             {
                 printf( "PANIC: $USER not found! Exiting...");
                 exit(1);
             }

             /*   Get Full  */
             IdsNs::IDS data_entry(pulse,1,-1,-1);
             data_entry.createEnv(userName, "test", "3");

             IDS::camera_ir ids = data_entry._camera_ir;

             ids.ids_properties.homogeneous_time = 1;
             ids.time.resize(2);
             ids.time(0) = 1.0;
             ids.time(1) = 1.1;
             ids.frame.resize(2);
             for (int k = 0; k < 2; k++) {
               ids.frame(k).surface_temperature.resize(5,15);
               for (int j = 0; j < 15; j++)
                 for (int i = 0; i< 5; i++)
                    ids.frame(k).surface_temperature(i,j) = (double) (i + j + k);
	            }

             ids.put();

             //displays first 2 frames before multiplying pixels by op parameter
             for (int i = 0; i < 2; i++) {
                printf("displaying frame %d:\n", i);
                std::cout << ids.frame(i).surface_temperature << std::endl;
             }

             al_status_t status = al_register_plugin("patch_reader");
             status = al_bind_plugin("camera_ir:0/frame/surface_temperature", "patch_reader");
             exitIfError(status);

             //setting the value of the ‘op’ plugin parameter to 2
             status = al_setvalue_int_scalar_parameter_plugin("op", 2, "patch_reader");
             exitIfError(status);

             ids.get();

             //displays first 2 frames after multiplying pixels by op parameter
             for (int i = 0; i < 2; i++) {
                printf("displaying patched frame %d:\n", i);
                std::cout << ids.frame(i).surface_temperature << std::endl;
             }
             data_entry.close();
           }

           void exitIfError(al_status_t &status) {
             if (status.code != 0) {
                  printf("%s\n", status.message);
                  exit(-1);
             }
           }

           int main(int argc, char** argv){
               execute(argv);
           }

        To execute the ``PatchReaderPlugin`` plugin, we are using the C++ code shown
        in `Figure 21`_:

        1. A ``camera_ir`` IDS is first populated with some data in the first 2
           frames of the ``surface_temperature`` field, then written to the disk
           using a call to ``put()``

        2. The first 2 (unpatched) frames are printed to the screen

        3. The ``patch_reader`` plugin is then registered and bound to the node
           with path ``camera_ir/frame/surface_temperature``

        4. **The plugin parameter ``op`` is set to 2**

        5. The ``get()`` operation is called

        6. The first 2 patched frames are printed again to the screen

        As expected, the plugin multiplies all pixels of the 2 (5x15pixels)
        frames by ``op`` as shown in the output of the C++ client code (`Figure
        22`_).

        .. code-block:: console
           :caption: **Figure 22:** output of the C++ client code
           :name: Figure 22

           $ ./test_patch_reader_plugin imas:hdf5?path=/Home/LF218007/public/imasdb/test/3/55000/0

           displaying frame 0:
           (0,4) x (0,14)
           [ 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14
           1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
           2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
           3 4 5 6 7 8 9 10 11 12 13 14 15 16 17
           4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 ]

           displaying frame 1:
           (0,4) x (0,14)
           [ 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
           2 3 4 5 6 7 8 9 10 11 12 13 14 15 16
           3 4 5 6 7 8 9 10 11 12 13 14 15 16 17
           4 5 6 7 8 9 10 11 12 13 14 15 16 17 18
           5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 ]

           0000001 | [debug] : setParameter - setting op parameter :op to: 2
           displaying patched frame 0:
           (0,4) x (0,14)
           [ 0 2 4 6 8 10 12 14 16 18 20 22 24 26 28
           2 4 6 8 10 12 14 16 18 20 22 24 26 28 30
           4 6 8 10 12 14 16 18 20 22 24 26 28 30 32
           6 8 10 12 14 16 18 20 22 24 26 28 30 32 34
           8 10 12 14 16 18 20 22 24 26 28 30 32 34 36 ]

           displaying patched frame 1:
           (0,4) x (0,14)
           [ 2 4 6 8 10 12 14 16 18 20 22 24 26 28 30
           4 6 8 10 12 14 16 18 20 22 24 26 28 30 32
           6 8 10 12 14 16 18 20 22 24 26 28 30 32 34
           8 10 12 14 16 18 20 22 24 26 28 30 32 34 36
           10 12 14 16 18 20 22 24 26 28 30 32 34 36 38 ]


    .. md-tab-item:: Using a Fortran client

        `Figure 24`_ depicts an example of fortran code to execute the
        ``patch_reader`` plugin.

        .. code-block:: Fortran
           :caption: **Figure 24:** Fortran client code using the ``patch_reader`` plugin
           :name: Figure 24

           program test_patch_reader_plugin

             use ids_routines
             implicit none

             integer :: i, j, k, idx, mode, status, nx, ny

             type (ids_camera_ir) :: cir  ! Declaration of the empty ids to be filled
             character(STRMAXLEN) :: uri
             character(len=132):: usr

             integer :: pulse = 500
             integer :: run = 1
             call get_environment_variable("USER",usr)

             cir%ids_properties%homogeneous_time = 1 ! Mandatory to define this property
             allocate(cir%time(2))
             cir%time = (/ 0.0, 0.1 /)

             allocate(cir%frame(2))

             nx = 15
             ny = 5

             allocate(cir%frame(2))

             do k=1,2
                allocate(cir%frame(k)%surface_temperature(nx,ny))
                do j=1,ny
                   do i=1,nx
                      cir%frame(k)%surface_temperature(i,j) = i + j + k - 3
                   enddo
                enddo
             enddo

             ! Creating the pulse file

            write(*,*) 'Creating pulse file, idx = ', idx
            call al_build_uri_from_legacy_parameters(MDSPLUS_BACKEND, pulse, run, usr, "test", "3", "", uri, status)
            call al_begin_dataentry_action(uri, FORCE_CREATE_PULSE, idx, status)

             call ids_put(idx,"camera_ir",cir)

             write(*,*) 'displaying frame 1', cir%frame(1)%surface_temperature
             write(*,*) 'displaying frame 2', cir%frame(2)%surface_temperature

             write(*,*) 'Closing pulse file, idx = ', idx
             call imas_close(idx)

             ! Registering the 'debug' plugin
             call al_register_plugin ('patch_reader', status)

             ! Binding the 'surface_temperature' node of the camera_ir IDS to the 'patch_reader' plugin (only for demo purpose)
             call al_bind_plugin ('camera_ir:0/frame/surface_temperature', 'patch_reader', status)

             call al_setvalue_int_scalar_parameter_plugin("op", 2, 'patch_reader', status);

             ! Opening the pulse file
             write(*,*) 'Opening pulse file, idx = ', idx
             call al_begin_dataentry_action(uri, OPEN_PULSE, idx, status)

             call ids_get(idx,"camera_ir",cir)

             write(*,*) 'displaying patched frame 1', cir%frame(1)%surface_temperature
             write(*,*) 'displaying patched frame 2', cir%frame(2)%surface_temperature

             call imas_close(idx)

             call ids_deallocate(cir)

             ! Unregistering the plugin since we do not need it anymore
             call al_unregister_plugin ('patch_reader', status)

           end program test_patch_reader_plugin

        Executing the Fortran code gives the output shown below.

        .. code-block:: console

           ./gfortran_test_patch_reader_plugin imas:hdf5?path=/Home/LF218007/public/imasdb/test/3/55000/0
            Creating pulse file, idx =            1
            displaying frame 1   0.0000000000000000        1.0000000000000000        2.0000000000000000        3.0000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        1.0000000000000000        2.0000000000000000        3.0000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        2.0000000000000000        3.0000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        16.000000000000000        3.0000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        16.000000000000000        17.000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        16.000000000000000        17.000000000000000        18.000000000000000
            displaying frame 2   1.0000000000000000        2.0000000000000000        3.0000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        2.0000000000000000        3.0000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        16.000000000000000        3.0000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        16.000000000000000        17.000000000000000        4.0000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        16.000000000000000        17.000000000000000        18.000000000000000        5.0000000000000000        6.0000000000000000        7.0000000000000000        8.0000000000000000        9.0000000000000000        10.000000000000000        11.000000000000000        12.000000000000000        13.000000000000000        14.000000000000000        15.000000000000000        16.000000000000000        17.000000000000000        18.000000000000000        19.000000000000000
            Closing pulse file, idx =            1
           0000001 | [debug] : setParameter - setting op parameter :op to: 2
            Opening pulse file, idx =            1
           displaying patched frame 1   0.0000000000000000        2.0000000000000000        4.0000000000000000        6.0000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        2.0000000000000000        4.0000000000000000        6.0000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        4.0000000000000000        6.0000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        32.000000000000000        6.0000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        32.000000000000000        34.000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        32.000000000000000        34.000000000000000        36.000000000000000
            displaying patched frame 2   2.0000000000000000        4.0000000000000000        6.0000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        4.0000000000000000        6.0000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        32.000000000000000        6.0000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        32.000000000000000        34.000000000000000        8.0000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        32.000000000000000        34.000000000000000        36.000000000000000        10.000000000000000        12.000000000000000        14.000000000000000        16.000000000000000        18.000000000000000        20.000000000000000        22.000000000000000        24.000000000000000        26.000000000000000        28.000000000000000        30.000000000000000        32.000000000000000        34.000000000000000        36.000000000000000        38.000000000000000


A plugin to automatically fill ``ids_properties/creation_date``
---------------------------------------------------------------

The requirement of IMAS-3121 ITER JIRA ticket is to fill in the
``ids_properties/creation_date`` node during a ``put()`` operation with the
current date in the form YYYY-MM-DD.

.. #TODO replace below code once we have IMAS-Plugins repository
.. - `creation_date_plugin.cpp <https://github.com/iterorganization/IMAS-Plugins/blob/main/creation_date_plugin.cpp>`_

The ``Creation_date_plugin`` class whose implementation is depicted in
`Figure 25`_ implements this feature. `Figure 26`_ displays the header file
content. Provenance feature operations have been removed for clarity in
these files.

.. literalinclude:: ./static/plugins/sources/creation_date_plugin.cpp
   :caption: **Figure 25:** Creation_date_plugin class implementation
   :name: Figure 25
   :language: C++

.. literalinclude:: ./static/plugins/sources/creation_date_plugin.cpp
   :caption: **Figure 26:** Creation_date_plugin class header
   :name: Figure 26
   :language: C++

.. code-block:: python
   :caption: **Figure 27:** python client of the ``creation_date`` plugin
   :name: Figure 27

   import imas
   import matplotlib.pyplot as plt
   from imas.hli_plugins import HLIPlugins
   from imas import imasdef
   import sys

   uri = sys.argv[1]

   data_entry = imas.DBEntry(uri, "w")
   data_entry.create()

   m= imas.magnetics()
   m.ids_properties.homogeneous_time=1
   m.time.resize(1)
   m.time[0]=0.

   HLIPlugins.register_plugin("creation_date")

   path= "magnetics:0/ids_properties/creation_date"
   HLIPlugins.bind_plugin(path, "creation_date")

   data_entry.put(m)

   HLIPlugins.unregister_plugin("creation_date")

   data_entry.close()

   print("checking value of creation_date...")
   data_entry.open(uri=uri, mode=imasdef.FORCE_OPEN_PULSE)
   ids = data_entry.get('magnetics')
   print(ids.ids_properties.creation_date)
   data_entry.close()

.. code-block:: console
   :caption: **Figure 28:** executing the ``creation_date`` plugin and checking the result
   :name: Figure 28

   $ python test_creation_date_plugin.py imas:hdf5?path=/Home/LF218007/public/imasdb/test/3/55000/0
   Patching creation_date...
   checking value of creation_date...
   2022-03-11

After execution of the ``creation_date`` plugin using the python client code
depicted in `Figure 27`_, we check that the ``ids_properties/creation_date`` has
been successfully updated as expected (`Figure 28`_).

The C++ code below (`Figure 29`_) uses the same plugin. The test is making
a little bit more than the previous python test, it prints at the end
if some *readback* plugins have been called during ``get()``. Since no
*readback* plugin has been defined in the ``creation_date``
plugin code, the test prints “No readback plugins have been called
during ``get()``.”. We will see later an example, which shows how to
define a *readback* plugin.

.. code-block:: C++
   :caption: **Figure 29:** executing the ``Creation_date`` plugin
   :name: Figure 29

   #include "creation_date_plugin.h"

   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
   #include <iostream>
   #include <iomanip>
   #include <ctime>
   #include <sstream>

   Creation_date_plugin::Creation_date_plugin()
   {
   }

   Creation_date_plugin::~Creation_date_plugin()
   {
   }

   /**
   The following functions are used to provide metadata about the plugin to identify and manage the plugin.

   getName() returns the name of the plugin, which is "creation_date" in this example.
   getCommit() returns a unique identifier for the version of the plugin, which is "8f2e7cd64daf9e35a6e6c5850dd80fc198f11d86" in this example.
   getVersion() returns the version number of the plugin, which is "1.0.0" in this example.
   getRepository() returns the URI for the Git repository where the plugin is located, which is "ssh://git@git.iter.org/imas/access-layer-plugins.git" in this example.
   getParameters() returns a string representing the parameters for the plugin.
   */

   std::string Creation_date_plugin::getName() {
       return "creation_date";
   }

   std::string Creation_date_plugin::getDescription() {
       return "";
   }

   std::string Creation_date_plugin::getCommit() {
       return "8f2e7cd64daf9e35a6e6c5850dd80fc198f11d86";
   }

   std::string Creation_date_plugin::getVersion() {
       return "1.0.0";
   }
   std::string Creation_date_plugin::getRepository() {
       return "ssh://git@git.iter.org/imas/access-layer-plugins.git";
   }
   std::string Creation_date_plugin::getParameters() {
       return "creation_date plugin parameters";
   }

   /**
   The following functions are used by the Access Layer to determine which readback plugin to use for a given field path.

   The getReadbackName() function returns the name of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns the string "debug". Otherwise, it returns an empty string.

   The getReadbackCommit() function returns the commit hash of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns a specific commit hash. Otherwise, it returns an empty string.

   The getReadbackVersion() function returns the version of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns the string "1.1.0". Otherwise, it returns an empty string.

   The getReadbackRepository() function returns the repository location of the readback plugin to use for a given field path. If the field path contains the string "creation_date", it returns a specific repository location. Otherwise, it returns an empty string.

   The getReadbackParameters() function returns the parameters to pass to the readback plugin for a given field path. If the field path contains the string "creation_date", it returns a specific string. Otherwise, it returns an empty string.
   */

   std::string Creation_date_plugin::getReadbackName(const std::string &path, int* index) {
          return "";
   }

   std::string Creation_date_plugin::getReadbackDescription(const std::string &path) {
          return "";
   }

   std::string Creation_date_plugin::getReadbackCommit(const std::string &path) {
       if (path.rfind("creation_date"))
          return "d92bb2f30384bc618574508b56e9542d00f0e97a";
       return "";
   }

   std::string Creation_date_plugin::getReadbackVersion(const std::string &path) {
       if (path.rfind("creation_date"))
          return "1.1.0";
       return "";
   }

   std::string Creation_date_plugin::getReadbackRepository(const std::string &path) {
       if (path.rfind("creation_date"))
          return "ssh://git@git.iter.org/imas/access-layer-plugins.git";
       return "";
   }

   std::string Creation_date_plugin::getReadbackParameters(const std::string &path) {
       if (path.rfind("creation_date"))
          return "readback plugin parameters";
       return "";
   }

   plugin::OPERATION Creation_date_plugin::node_operation(const std::string &path) {
       return plugin::OPERATION::PUT_ONLY;
   }

   void Creation_date_plugin::begin_global_action(int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {
   }

   void Creation_date_plugin::begin_slice_action(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
   }

   void Creation_date_plugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
        LLenv lle = Lowlevel::getLLenv(ctx);
        ArraystructContext* actx = lle.create(fieldPath, timeBasePath);
        *aosctx = Lowlevel::addLLenv(lle.backend, actx);
        lle.backend->beginArraystructAction(actx, arraySize);
   }

   int Creation_date_plugin::read_data(int ctx, const char* fieldPath, const char* timeBasePath,
                   void **data, int datatype, int dim, int *size) {
       return 0;
   }


   /**
    This function is used to write data to the specific IDS field 'ids_properties/creation_date'.

   The function takes several parameters, including an integer "ctx", which is a handle to the current low level context, a character array "fieldPath" representing the path to the field being modified, a character array "timeBasePath" representing the path to the timebase for the field, a void pointer "data" containing the data to be written, an integer "datatype" specifying the type of data being written, an integer "dim" specifying the number of dimensions of the data, and an array of integers "size" specifying the size of each dimension.

   The function prints a message indicating that the "Creation_date" plugin is patching the creation date. It then uses the standard library function "std::time" to obtain the current system time and local time, and formats it using the standard library function "std::put_time" to create a string representing the current date in the format of "YYYY-MM-DD". This string is stored in a std::ostringstream object called "oss" and then converted to a C-style string using the "str()" function, and then to a void pointer using the "c_str()" function.

   Finally, the function creates an array of sizes for the data being written, with a single element representing the length of the string. It then calls the "al_plugin_write_data" function to write the data to the specified field, passing in the current context, the field path, the timebase path, the void pointer to the data, the data type, the number of dimensions, and the array of sizes. After writing the data, the function prints a message indicating that the patching is complete.
   */
   void Creation_date_plugin::write_data(int ctx, const char* fieldPath, const char* timeBasePath, void *data, int datatype, int dim, int *size) {
       printf("Patching creation_date from the creation_date plugin... \n");
       auto t = std::time(nullptr);
       auto tm = *std::localtime(&t);
       std::ostringstream oss;
       oss << std::put_time(&tm, "%Y-%m-%d");
       auto text = oss.str();
       void* ptrData = (void *) (text.c_str());
       int arrayOfSizes[1] = {(int)text.size()};
       al_plugin_write_data(ctx, fieldPath, timeBasePath, ptrData, CHAR_DATA, 1, arrayOfSizes);
       printf("End of patching.\n");
   }

   void Creation_date_plugin::setParameter(const char* parameter_name, int datatype, int dim, int *size, void *data) {
   }

   void Creation_date_plugin::end_action(int ctx) {}



.. code-block:: Fortran
   :caption: **Figure 30:** Fortran test code for executing the ``creation_date`` plugin
   :name: Figure 30

   program test

     use ids_routines
     implicit none

     integer :: idx, mode, status

     type (ids_magnetics) :: mag  ! Declaration of the empty ids to be filled
     character(STRMAXLEN) :: uri
     character(len=132):: usr
     integer :: pulse = 54
     integer :: run = 1

     call get_environment_variable("USER",usr)

     call al_build_uri_from_legacy_parameters(MDSPLUS_BACKEND, pulse, run, usr, "test", "3", "", uri, status)
     mag%ids_properties%homogeneous_time = 1 ! Mandatory to define this property
     allocate(mag%time(1))
     mag%time = 0.0


     ! Registering the 'creation_date' plugin
     call al_register_plugin ('creation_date', status)

     write(*,*) 'Using the magnetics IDS for demo purpose'

     ! Binding the 'creation_date' node of the magnetics IDS to the 'creation_date' plugin (only for demo purpose)
     call al_bind_plugin ('magnetics:0/ids_properties/creation_date', 'creation_date', status)

     ! Creating the pulse file

     call al_begin_dataentry_action(uri, FORCE_CREATE_PULSE, idx, status)
     write(*,*) 'Creating pulse file, idx = ', idx
     call ids_put(idx,"magnetics",mag)

     write(*,*) 'Closing pulse file, idx = ', idx
     call imas_close(idx)

     ! Unregistering the plugin since we do not need it anymore
     call al_unregister_plugin ('creation_date', status)

     ! Opening the pulse file to check the value of 'creation_date'

     call al_begin_dataentry_action(uri, OPEN_PULSE, idx, status)
     write(*,*) 'Opening pulse file, idx = ', idx

     write(*,*) 'Reading pulse file, idx = ', idx
     call ids_get(idx,"magnetics",mag)

     print *, 'creation_date = ', mag%ids_properties%creation_date
     call imas_close(idx)

   end program test

Executing the Fortran code above gives the following output:

.. code-block:: console

   $ ./gfortran_test_creation_date_plugin 55000 0 LF218007 test 13

   Using the magnetics IDS for demo purpose

   Creating pulse file, idx = 1

   Patching creation_date...

   End of patching.

   Closing pulse file, idx = 1

   Opening pulse file, idx = 1

   Reading pulse file, idx = 1

   creation_date = 2022-03-16


Binding a *readback* plugin
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Let us consider again the ``creation_date`` plugin. Suppose that
we want to display the value of the field ``ids_properties/creation_date``
(using the ``debug`` plugin) just after it has been patched. In order to
execute the ``debug`` plugin, we replace the *readback* feature
function ``getReadbackName(...)`` of the ``creation_date`` plugin:

.. code-block:: C++

   std::string Creation_date_plugin::getReadbackName(const std::string &path, int* index) {
      return "";
   }

by the following code:

.. code-block:: C++

   std::string Creation_date_plugin::getReadbackName(const std::string &path, int* index) {
      if (path.rfind("ids_properties/creation_date")) {
         *index = 0;
         return "debug";
      }
   }

In the code above, we are binding the ``debug`` plugin to the path
``ids_properties/creation_date``

After recompilation of the plugin, the execution of the C++ test of
`Figure 29`_ gives the following output:

.. code-block:: console

   $ ./test_creation_date_plugin imas:hdf5?path=/Home/LF218007/public/imasdb/test/3/55000/0
   Patching the field 'ids_properties/creation_date' of a magnetics IDS for demo purpose.
   Patching creation_date from the creation_date plugin...
   End of patching.
   Reading IDS...
   0000001 | [debug] : read_data - reading data for field path= ids_properties/creation_date
   0000002 | [debug] : read_data - ids_properties/creation_date= 2023-03-06
   visiting AOS=ids_properties/plugins/node with size=1
   node at path=ids_properties/creation_date
   readback plugin --> name=debug
   --> version=1.1.0
   --> commit=d92bb2f30384bc618574508b56e9542d00f0e97a

The *readback* plugin ``debug`` is executed during the
``get()`` operation (when visiting the node ``ids_properties/creation_date``)
as expected.


Building a partial ``get()`` operation
--------------------------------------

Skipping the read of an array of structure
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. literalinclude:: ./static/plugins/sources/partial_get_plugin.cpp
   :caption: **Figure 31:** the PartialGetPlugin class implementation
   :name: Figure 31
   :language: C++

.. literalinclude:: ./static/plugins/sources/partial_get_plugin.h
   :caption: **Figure 32:** the PartialGetPlugin header class
   :name: Figure 32
   :language: C++

In a first use-case, the user wants to access only few attributes of the
``equilibrium`` IDS for many shots. In order to speed up reading, he
decides to skip the loading of the ``grids_ggd`` array of structures (AOS).
The use of the ``PartialGetPlugin`` class implementation (`Figure 31`_) with
its header (`Figure 32`_) provides an efficient solution. During the ``get()``
operation, the plugin intercepts the HLI call to the function
``al_begin_arraystruct_action(...)`` for the ``grids_ggd`` AOS and sets its size
(using the arraySize pointer) to 0 after displaying a warning to the
user:

.. code-block:: C++

   void PartialGetPlugin::begin_arraystruct_action(int ctx, int *aosctx, const char* fieldPath, const char* timeBasePath, int *arraySize) {
      if (std::string(fieldPath) == "grids_ggd") {
         LOG_WARNING << "ignoring gids_ggd";
         *arraySize = 0;
      }
   }

The value of arraySize (set to 0) is returned to the HLI which will
therefore skip the iteration of the ``grids_ggd`` AOS.


Executing the ``partial_get`` plugin
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. md-tab-set::

    .. md-tab-item:: Using Python

        `Figure 33`_ depicts a HLI python client code to test the plugin.

        .. code-block:: python
           :caption: **Figure 33:** python client code to test the ``PartialGetPlugin`` plugin
           :name: Figure 33

           import imas
           from imas import imasdef
           import matplotlib.pyplot as plt
           from imas.hli_plugins import HLIPlugins
           import sys

           uri = sys.argv[1]

           data_entry = imas.DBEntry(uri)
           data_entry.open()

           HLIPlugins.register_plugin("partial_get");
           HLIPlugins.bind_plugin("equilibrium:0/grids_ggd", "partial_get")

           equilibrium_ids = data_entry.get('equilibrium', occurrence=0)

           print("Grids_ggd AOS size=",len(equilibrium_ids.grids_ggd))

           HLIPlugins.unregister_plugin("partial_get")

        The execution of the plugin gives the following output:

        .. code-block:: console

           $ python test_partial_get_plugin.py imas:hdf5?path=/Imas_public/public/imasdb/west/3/54914/0/
           0000001 | [warning] : begin_arraystruct_action - ignoring grids_ggd
           Grids_ggd AOS size= 0


    .. md-tab-item:: Using C++

        .. code-block:: C++
           :caption: **Figure 34:** C++ client code to test the ``partial_get`` plugin
           :name: Figure 34

            #include <stdlib.h>
            #include "ALClasses.h"

            using namespace IdsNs;

            void execute(char** argv);
            void exitIfError(al_status_t &status);

            void execute(char** argv) {

                int pulse=54;
                char* userName = NULL;

                userName = getenv("USER");
                if(userName == NULL)
                {
                    printf( "PANIC: $USER not found! Exiting...");
                    exit(1);
                }

                /*   Get Full  */
                IdsNs::IDS data_entry(pulse,1,-1,-1);
                data_entry.openEnv(userName, "test", "3");

                al_status_t status = al_register_plugin("partial_get");
                exitIfError(status);
                status = al_bind_plugin("magnetics:0/flux_loop", "partial_get");
                IDS::magnetics ids = data_entry._magnetics;
                ids.get(0);
                printf("magnetics AOS size=%d\n",ids.flux_loop.size());
                data_entry.close();

                status = al_unregister_plugin("partial_get");
                exitIfError(status);
            }

            void exitIfError(al_status_t &status) {
              if (status.code != 0) {
                   printf("%s\n", status.message);
                   exit(-1);
              }
            }


            int main(int argc, char** argv){
                execute(argv);
            }

        Execution of C++ client code gives:

        .. code-block:: console

           $ ./test_partial_get_plugin imas:hdf5?path=/Imas_public/public/imasdb/west/3/54914/0/
           0000001 | [warning] : begin_arraystruct_action - ignoring grids_ggd
           Grids_ggd AOS size=0


    .. md-tab-item:: Using Fortran

        .. code-block:: Fortran
           :caption: **Figure 35:** fortran client code to test the ``partial_get`` plugin
           :name: Figure 35

           program test

             use ids_routines
             implicit none

             integer :: idx, mode, status

             type (ids_magnetics) :: mag  ! Declaration of the ids
             character(STRMAXLEN) :: uri
             character(len=132)::  usr
             integer :: pulse = 54
             integer :: run = 1

             call get_environment_variable("USER",usr)

             call al_build_uri_from_legacy_parameters(MDSPLUS_BACKEND, pulse, run, usr, "test", "3", "", uri, status)

             ! Registering the 'partial_get' plugin
             call al_register_plugin ('partial_get', status)

             ! Binding the 'partial_get' plugin to the 'grids_ggd' node of the equilibrium IDS (as a demo purpose)
             call al_bind_plugin ('magnetics:0/flux_loop', 'partial_get', status)

             ! Opening the pulse file
             call al_begin_dataentry_action(uri, OPEN_PULSE, idx, status);
             write(*,*) 'Opening pulse file, idx = ', idx

             ! Calling 'get' will call the 'partial_get' plugin
             call ids_get(idx,"magnetics", mag)
             print *, 'flux_loop pointer associated = ', associated(mag%flux_loop)
             call imas_close(idx)

           end program test

        .. code-block:: console

           $ ./gfortran_test_partial_get_plugin imas:hdf5?path=/Imas_public/public/imasdb/west/3/54914/0/
           0000001 | [warning] : begin_arraystruct_action - ignoring grids_ggd
           Grids_ggd pointer associated =  F
