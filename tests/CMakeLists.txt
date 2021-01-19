project( lowlevel-tests )


########################################################################################################################
# Dependencies


########################################################################################################################
# Sources

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/lowlevel
)


########################################################################################################################
# Targets

set( EXE_FLAG  )
set( EXE_EXT  )

if( BUILD_SHARED_LIBS )
  set( LOWLEVEL_LINK lowlevel-shared )
else()
  set( LOWLEVEL_LINK lowlevel-static )
endif()

add_executable( testlowlevel ${EXE_FLAG} testlowlevel.cpp )
target_link_libraries( testlowlevel PRIVATE
  ${LOWLEVEL_LINK}
)

if( BUILD_SHARED_LIBS )
  add_dependencies( testlowlevel lowlevel-shared )
else()
  add_dependencies( testlowlevel lowlevel-static )
endif()

set_target_properties( testlowlevel
  PROPERTIES
    OUTPUT_NAME testlowlevel${EXE_EXT}
    COMPILE_FLAGS -DNOIMPLIB
)

add_executable( testlowlevel_c ${EXE_FLAG} testlowlevel_c.c )
target_link_libraries( testlowlevel_c PRIVATE
  ${LOWLEVEL_LINK}
)

set_target_properties( testlowlevel_c
  PROPERTIES
    OUTPUT_NAME testlowlevel_c${EXE_EXT}
    COMPILE_FLAGS -DNOIMPLIB
)


########################################################################################################################
# Install

install( TARGETS
  testlowlevel
  DESTINATION bin
)

install( TARGETS
  testlowlevel_c
  DESTINATION bin
)