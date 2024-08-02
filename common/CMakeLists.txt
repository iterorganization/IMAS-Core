# CMake configuration for AL common assets
# Intended for inclusion (using FetchContent_MakeAvailable) in other AL components

# Add ./cmake to the module path in the parent scope:
set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake PARENT_SCOPE )

if ( ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} )
    message( FATAL_ERROR "In-source builds not supported. Please use a separate build directory, for example `cmake -B build [...]`" )
endif()


find_package( Python3 )
set( PYVER "${Python3_VERSION_MAJOR}.${Python3_VERSION_MINOR}" )
configure_file( al_env.sh.in ${CMAKE_CURRENT_BINARY_DIR}/al_env.sh )

# Don't generate the environment file when building modules:
if( AL_DOWNLOAD_DEPENDENCIES OR AL_DEVELOPMENT_LAYOUT )
    install( FILES ${CMAKE_CURRENT_BINARY_DIR}/al_env.sh DESTINATION bin )
endif()
