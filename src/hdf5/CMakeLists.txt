# CMake configuration for the HDF5 backend
if(WIN32)
	# Try modern CONFIG mode first (vcpkg), fallback to legacy FindHDF5 module
	find_package( hdf5 CONFIG QUIET )
	if( NOT hdf5_FOUND )
	  find_package( HDF5 COMPONENTS C HL REQUIRED )
	endif()
else()
	find_package( HDF5 COMPONENTS C HL REQUIRED )
endif()

target_sources( al PRIVATE
    hdf5_backend.cpp
    hdf5_writer.cpp
    hdf5_reader.cpp
    hdf5_utils.cpp
    hdf5_hs_selection_reader.cpp
    hdf5_hs_selection_writer.cpp
    hdf5_dataset_handler.cpp
    hdf5_events_handler.cpp
    hdf5_backend_factory.cpp
)
target_compile_definitions( al PRIVATE -DHDF5 )
if(WIN32)
	# Use modern CMake targets if available (vcpkg), otherwise legacy variables
	if( TARGET hdf5::hdf5-shared )
	  target_link_libraries( al PRIVATE hdf5::hdf5-shared )
	  if( TARGET hdf5::hdf5_hl-shared )
	    target_link_libraries( al PRIVATE hdf5::hdf5_hl-shared )
	  endif()
	elseif( TARGET hdf5::hdf5-static )
	  target_link_libraries( al PRIVATE hdf5::hdf5-static )
	  if( TARGET hdf5::hdf5_hl-static )
	    target_link_libraries( al PRIVATE hdf5::hdf5_hl-static )
	  endif()
	else()
	  # Legacy FindHDF5 module
	  target_include_directories( al PRIVATE ${HDF5_C_INCLUDE_DIRS} )
	  target_link_libraries( al PRIVATE ${HDF5_C_LIBRARIES} )
	  target_compile_definitions( al PRIVATE ${HDF5_C_DEFINITIONS} )
	endif()
	
	target_include_directories( al PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
else()
  target_include_directories( al PRIVATE ${HDF5_C_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )
  target_link_libraries( al PRIVATE ${HDF5_C_LIBRARIES} )
  target_compile_definitions( al PRIVATE ${HDF5_C_DEFINITIONS} )
endif()

