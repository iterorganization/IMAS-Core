project( imas-lowlevel )
cmake_minimum_required( VERSION 2.8 )

set( CPPSRC
  ual_backend.cpp ual_lowlevel.cpp ual_context.cpp ual_const.cpp
  mdsplus_backend.cpp memory_backend.cpp ual_exception.cpp no_backend.cpp
  ual_exception.h
)
# uda_backend.cpp
set( CSRC
  ual_low_level.c matlab_adapter.c
)

set( CMAKE_CXX_FLAGS -std=c++11 )

set( UDA_DIR /Users/jhollocombe/uda-2.2.6 )

add_library( low_level ${CPPSRC} ${CSRC} )
target_compile_definitions( low_level PRIVATE MDSPLUS )

#target_link_libraries( low_level PUBLIC ${UDA_DIR}/lib/libfatuda_client.dylib )
target_include_directories( low_level PRIVATE /opt/local/include /usr/local/include /usr/local/mdsplus/include )
# ${UDA_DIR}/include/uda