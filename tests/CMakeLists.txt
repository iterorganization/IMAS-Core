project( lowlevel-tests )


########################################################################################################################
# Targets
#
# These smoke tests exercise the public C ABI (al_lowlevel.h) end-to-end and,
# by building the same program as both C++ and C, guard that the header stays
# C-compatible. The `al` target exports its include dirs PUBLICly, so linking
# it is enough to resolve al_lowlevel.h / al_defs.h.

set( EXE_FLAG  )
set( EXE_EXT  )

add_executable( testlowlevel ${EXE_FLAG} testlowlevel.cpp )
target_link_libraries( testlowlevel PRIVATE al )
add_dependencies( testlowlevel al )
set_target_properties( testlowlevel PROPERTIES OUTPUT_NAME testlowlevel${EXE_EXT} )

add_executable( testlowlevel_c ${EXE_FLAG} testlowlevel_c.c )
target_link_libraries( testlowlevel_c PRIVATE al )
add_dependencies( testlowlevel_c al )
set_target_properties( testlowlevel_c PROPERTIES OUTPUT_NAME testlowlevel_c${EXE_EXT} )

if(NOT WIN32 AND NOT APPLE)
  # stdc++fs is only needed on Linux with older GCC
  target_link_libraries( testlowlevel   PRIVATE stdc++fs )
  target_link_libraries( testlowlevel_c PRIVATE stdc++fs )
endif()


########################################################################################################################
# CTest registration
#
# The legacy `imas_public` user is not a real account, so the pulse path is
# supplied directly by passing an absolute base directory as the "user" (-u):
# the core then builds <user>/<machine>/<version>/<pulse>/<run>. Each test gets
# its own base dir (created by a fixture) to stay parallel-safe, and uses the
# DD-agnostic ASCII backend so no MDSplus model tree is required.

set( _testdb ${CMAKE_CURRENT_BINARY_DIR}/testdb )

add_test( NAME testlowlevel_c_setup
  COMMAND ${CMAKE_COMMAND} -E make_directory ${_testdb}/c/west/3/12/0 )
set_tests_properties( testlowlevel_c_setup PROPERTIES FIXTURES_SETUP TESTDB_C )

add_test( NAME testlowlevel_cpp_setup
  COMMAND ${CMAKE_COMMAND} -E make_directory ${_testdb}/cpp/west/3/12/0 )
set_tests_properties( testlowlevel_cpp_setup PROPERTIES FIXTURES_SETUP TESTDB_CPP )

add_test( NAME testlowlevel_c
  COMMAND testlowlevel_c -b=A -s=12 -r=0 -o=C -c=C -u=${_testdb}/c )
set_tests_properties( testlowlevel_c PROPERTIES FIXTURES_REQUIRED TESTDB_C )

add_test( NAME testlowlevel_cpp
  COMMAND testlowlevel -b=A -s=12 -r=0 -o=C -c=C -u=${_testdb}/cpp )
set_tests_properties( testlowlevel_cpp PROPERTIES FIXTURES_REQUIRED TESTDB_CPP )


########################################################################################################################
# Install

install(
  TARGETS testlowlevel testlowlevel_c
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
