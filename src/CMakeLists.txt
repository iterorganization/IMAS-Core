# Add source files to the al library target

target_sources( al PRIVATE
    al_backend.cpp
    al_lowlevel.cpp
    al_context.cpp
    al_const.cpp
    al_exception.cpp
    no_backend.cpp
    memory_backend.cpp
    ascii_backend.cpp
    access_layer_plugin_manager.cpp
    flexbuffers_backend.cpp
)

target_include_directories( al PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )

# Add optional backends
if( AL_BACKEND_MDSPLUS )
    add_subdirectory( mdsplus )
endif()

if( AL_BACKEND_HDF5 )
    add_subdirectory( hdf5 )
endif()

if( AL_BACKEND_UDA OR AL_BACKEND_UDAFAT )
    add_subdirectory( uda )
endif()
