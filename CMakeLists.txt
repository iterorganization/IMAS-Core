project( lowlevel )

include( ListSharedLibDependencies )


########################################################################################################################
# Dependencies

find_package( OpenSSL REQUIRED )
find_package( LibXml2 REQUIRED )
find_package( pthreads REQUIRED )
find_package( dlfcn-win32 CONFIG REQUIRED )

if( IMAS_MDSPLUS )
  find_package( MDSplus REQUIRED )
endif()

if( IMAS_UDA OR IMAS_FATUDA )
  find_package( UDA REQUIRED )
endif()
if( IMAS_FATUDA )
  find_package( PostgreSQL REQUIRED )
endif()

if( IMAS_HDF5 )
  find_package( HDF5 COMPONENTS C HL REQUIRED )
  find_package( HDF5ex REQUIRED )
  find_package( SZIP REQUIRED )
  find_package( SZIPex REQUIRED )
  find_package( Boost COMPONENTS system filesystem REQUIRED )
  find_package( Boostex REQUIRED )
endif()


########################################################################################################################
# Sources

set( SRC_FILES
  al_backend.cpp
  al_lowlevel.cpp
  al_context.cpp
  al_const.cpp
  al_exception.cpp
  no_backend.cpp
  memory_backend.cpp
  ascii_backend.cpp
  access_layer_plugin_manager.cpp
)

set( HEADER_FILES
  ascii_backend.h
  memory_backend.h
  no_backend.h
  al_backend.h
  al_const.h
  al_context.h
  al_defs.h
  al_exception.h
  al_lowlevel.h
  access_layer_plugin_manager.h
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${OPENSSL_INCLUDE_DIR}
  ${LIBXML2_INCLUDE_DIR}
  ${PThreads4W_INCLUDE_DIR}
)

add_definitions( -DASCII )

if( IMAS_MDSPLUS )
  set( SRC_FILES ${SRC_FILES} mdsplus_backend.cpp )
  set( HEADER_FILES ${HEADER_FILES} mdsplus_backend.h )
  include_directories( ${MDSPLUS_INCLUDES} )
  add_definitions( -DMDSPLUS -DMDSOBJECTSCPPSHRVS_EXPORTS )
endif()

if( IMAS_UDA OR IMAS_FATUDA )
  set( SRC_FILES ${SRC_FILES} uda_backend.cpp )
  set( HEADER_FILES ${HEADER_FILES} uda_backend.h )
  include_directories( ${UDA_INCLUDES} )
  if( IMAS_FATUDA )
    include_directories( ${PostgreSQL_INCLUDE_DIRS} )
  endif()
  add_definitions( -DUDA )
endif()

if( IMAS_HDF5 )
  set( SRC_FILES ${SRC_FILES} hdf5_backend.cpp hdf5_writer.cpp hdf5_reader.cpp hdf5_utils.cpp hdf5_hs_selection_reader.cpp hdf5_hs_selection_writer.cpp hdf5_dataset_handler.cpp hdf5_events_handler.cpp hdf5_backend_factory.cpp )
  set( HEADER_FILES ${HEADER_FILES} hdf5_backend.h hdf5_writer.h hdf5_reader.h hdf5_utils.h hdf5_hs_selection_reader.h hdf5_hs_selection_writer.h hdf5_dataset_handler.h hdf5_events_handler.h hdf5_backend_factory.h)
  include_directories( ${HDF5_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} )
  add_definitions( -DHDF5 )
endif()


########################################################################################################################
# Targets

add_library( lowlevel-objects OBJECT ${SRC_FILES} ${HEADER_FILES} )

set( LOWLEVEL_OBJS
  $<TARGET_OBJECTS:lowlevel-objects>
)


add_library( lowlevel-static STATIC ${LOWLEVEL_OBJS} )
if( BUILD_SHARED_LIBS )
  add_library( lowlevel-shared SHARED ${LOWLEVEL_OBJS} )
endif()

set( LOWLEVEL_LINK ${OPENSSL_LIBRARIES} ${LIBXML2_LIBRARIES} ${PThreads4W_LIBRARY} ws2_32 )
if( IMAS_MDSPLUS )
  set( LOWLEVEL_LINK ${LOWLEVEL_LINK} ${MDSPLUS_LIBRARIES} )
  set( LOWLEVEL_LINK ${LOWLEVEL_LINK} dlfcn-win32::dl )
endif()

if( IMAS_UDA )
  set( LOWLEVEL_LINK ${LOWLEVEL_LINK} ${UDA_LIBRARIES} ${XDR_LIBRARIES} )
endif()
if( IMAS_FATUDA )
  set( LOWLEVEL_LINK ${LOWLEVEL_LINK} ${FATUDA_LIBRARIES} ${XDR_LIBRARIES} ${PostgreSQL_LIBRARY} )
endif()

if( IMAS_HDF5 )
  set( LOWLEVEL_LINK ${LOWLEVEL_LINK} ${HDF5_LIBRARIES} ${HDF5_HL_LIBRARIES} ${Boost_LIBRARIES} )
endif()

target_link_libraries( lowlevel-static PRIVATE ${LOWLEVEL_LINK} )
if( BUILD_SHARED_LIBS )
  target_link_libraries( lowlevel-shared PRIVATE ${LOWLEVEL_LINK} )
endif()


########################################################################################################################
# Set library properties

set_target_properties( lowlevel-static
  PROPERTIES
    OUTPUT_NAME libal
)

if( BUILD_SHARED_LIBS )
  set_target_properties( lowlevel-shared
    PROPERTIES
      OUTPUT_NAME libal
      VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
	  IMPORT_SUFFIX ".dll.lib"
  )
  
  ExportBinDef( lowlevel-shared ${CMAKE_CURRENT_BINARY_DIR} lowlevel )
endif()


########################################################################################################################
# Install

install( TARGETS lowlevel-static
  DESTINATION lib
)

if( BUILD_SHARED_LIBS )
  install( TARGETS lowlevel-shared
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()

install( FILES ${HEADER_FILES}
  DESTINATION include/lowlevel
)

if( BUILD_SHARED_LIBS )
  # Extract Release target path to get all dependent libraries for installation
  #get_target_property( MY_BINARY_LOCATION lowlevel::lowlevel-shared IMPORTED_LOCATION_RELEASE )
  #ListDependencies( ${MY_BINARY_LOCATION} OFF lowlevel-shared_lists )
  
  if( IMAS_MDSPLUS )
    # Install MDSplus runtime libraries
    foreach( MDSPLUS_DLL ${MDSPLUS_RUNTIME_LIB} )
      ResolveDependencyPath( "${MDSPLUS_DLL}" MDSPLUS_PATH "" )
      
      install( FILES ${MDSPLUS_PATH}
        DESTINATION bin
      )
    endforeach()
    
    # Required by MDSplus DLL, provided by MinGW
    ResolveDependencyPath( "libdl.dll" LIBDL_PATH "$ENV{MSYS64_DIR}/mingw64/bin" )
    ResolveDependencyPath( "libiconv-2.DLL" LIBICONV-2_PATH "$ENV{MSYS64_DIR}/mingw64/bin" )
    ResolveDependencyPath( "libwinpthread-1.DLL" LIBWINPTHREAD-1_PATH "$ENV{MSYS64_DIR}/mingw64/bin" )
    
    install( FILES 
      ${LIBDL_PATH}
      ${LIBICONV-2_PATH}
      ${LIBWINPTHREAD-1_PATH}
      DESTINATION bin
    )
  endif()
  
  if( IMAS_UDA OR IMAS_FATUDA )
    # Install UDA runtime libraries
    foreach( UDA_DLL ${UDA_RUNTIME_LIB} )
      ResolveDependencyPath( "${UDA_DLL}" UDA_PATH "" )
      
      install( FILES ${UDA_PATH}
        DESTINATION bin
      )
    endforeach()
  endif()
  
  if( IMAS_HDF5 )
    # Install HDF5 runtime libraries
    foreach( HDF5_DLL ${HDF5_RUNTIME_LIB} )
      ResolveDependencyPath( "${HDF5_DLL}" HDF5_PATH "" )
      
      install( FILES ${HDF5_PATH}
        DESTINATION bin
      )
    endforeach()
    
    # SZIP is required by HDF5 DLL
    foreach( SZIP_DLL ${SZIP_RUNTIME_LIB} )
      ResolveDependencyPath( "${SZIP_DLL}" SZIP_PATH "" )
      
      install( FILES ${SZIP_PATH}
        DESTINATION bin
      )
    endforeach()
    
    # Boost is required by HDF5 DLL
    foreach( Boost_DLL ${Boost_RUNTIME_LIB} )
      ResolveDependencyPath( "${Boost_DLL}" Boost_PATH "" )
      
      install( FILES ${Boost_PATH}
        DESTINATION bin
      )
    endforeach()
  endif()
endif()


########################################################################################################################
# Tests

if( IMAS_TESTS )
  add_subdirectory( tests )
endif()
