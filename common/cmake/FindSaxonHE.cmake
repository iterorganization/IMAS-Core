# Find Saxon-HE XSLT processor
#
# Sets the following variables
# - SaxonHE_FOUND
# - SaxonHE_CLASSPATH
# - SaxonHE_VERSION

find_package( Java COMPONENTS Runtime )
include( FindPackageHandleStandardArgs )

macro( TestSaxon CLASSPATH )
  execute_process(
    COMMAND ${Java_JAVA_EXECUTABLE} -cp "${CLASSPATH}" net.sf.saxon.Transform -?
    ERROR_VARIABLE _Saxon_OUTPUT
    RESULT_VARIABLE _Saxon_RESULT
    OUTPUT_QUIET
  )
  if( _Saxon_RESULT EQUAL 0 )
    set( SaxonHE_CLASSPATH "${CLASSPATH}" CACHE STRING "Java classpath containing Saxon-HE" FORCE )
    if( _Saxon_OUTPUT MATCHES "Saxon(-HE)? ([^ ]*) from" )
      set( SaxonHE_VERSION ${CMAKE_MATCH_2} )
    else()
      set( SaxonHE_VERSION "Unknown" )
    endif()
  endif()
endmacro()

if( Java_FOUND AND NOT SaxonHE_CLASSPATH )
  # Check if saxon is already on the classpath
  TestSaxon( "$ENV{CLASSPATH}" )
  if( NOT SaxonHE_CLASSPATH )
    # Try to find Saxon in /usr/share/java:
    find_file( SaxonHE_JAR
      NAMES Saxon-HE.jar saxon-he.jar
      PATHS
        /usr/share/java/
        /usr/local/share/java/
    )
    if( SaxonHE_JAR )
      TestSaxon( "${SaxonHE_JAR}" )
    endif()
  endif()
endif()

if( SaxonHE_CLASSPATH )
  # Saxon found (or classpath set by user)
  if( NOT SaxonHE_VERSION )
    TestSaxon( "${SaxonHE_CLASSPATH}" )
  endif()
endif()

find_package_handle_standard_args(
  SaxonHE
  REQUIRED_VARS SaxonHE_CLASSPATH SaxonHE_VERSION 
  VERSION_VAR SaxonHE_VERSION
)
