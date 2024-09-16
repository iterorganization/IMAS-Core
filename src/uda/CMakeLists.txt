# CMake configuration for the UDA backend

# Use pkg config to find uda requirements
find_package( PkgConfig REQUIRED )
if( IMAS_UDAFAT )
    pkg_check_modules( uda REQUIRED IMPORTED_TARGET uda-fat-cpp )
else()
    pkg_check_modules( uda REQUIRED IMPORTED_TARGET uda-cpp )
endif()
# if( IMAS_UDAFAT )
#   find_package( PostgreSQL REQUIRED )
# endif()

target_sources( al PRIVATE
    uda_backend.cpp
    uda_cache.cpp
    uda_path.cpp
    uda_xml.cpp
    pugixml.cpp
)
target_include_directories( al PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} )
if(uda_VERSION VERSION_GREATER_EQUAL "2.7.6")
   message(STATUS "Use UDA legacy API (-DUDA_LEGACY_276)")
   target_compile_definitions( al PRIVATE -DUDA -DUDA_LEGACY_276 -DUDA_CLIENT_FLAGS_API)
else()
   target_compile_definitions( al PRIVATE -DUDA )
endif()

target_link_libraries( al PRIVATE PkgConfig::uda )
