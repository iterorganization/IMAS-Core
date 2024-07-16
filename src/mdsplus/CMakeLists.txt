# CMake configuration for the MDSplus backend

find_package( MDSplus REQUIRED )

target_sources( al PRIVATE mdsplus_backend.cpp )
target_include_directories( al PRIVATE ${MDSPLUS_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} )
target_compile_definitions( al PRIVATE -DMDSPLUS )
target_link_libraries( al PRIVATE ${MDSPLUS_LIBRARIES} )

if( WIN32 )
    target_compile_definitions( al PRIVATE -DMDSOBJECTSCPPSHRVS_EXPORTS )
endif()
