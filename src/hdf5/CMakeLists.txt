# CMake configuration for the HDF5 backend

find_package( HDF5 COMPONENTS C HL REQUIRED )

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

target_include_directories( al PRIVATE ${HDF5_C_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( al PRIVATE ${HDF5_C_LIBRARIES} )
target_compile_definitions( al PRIVATE ${HDF5_C_DEFINITIONS} )

