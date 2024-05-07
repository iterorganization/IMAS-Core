find_package(Python REQUIRED COMPONENTS Interpreter Development.Module NumPy)
find_package(Cython REQUIRED)
cython_compile_pyx(imas_core/_al_lowlevel.pyx OUTPUT_VAR _al_lowlevel_source)
cython_compile_pyx(imas_core/al_defs.pyx OUTPUT_VAR al_defs_source)
python_add_library(_al_lowlevel MODULE ${_al_lowlevel_source} WITH_SOABI)
python_add_library(al_defs MODULE ${al_defs_source} WITH_SOABI)
target_link_libraries(_al_lowlevel PRIVATE Python::NumPy al)
target_link_libraries(al_defs PRIVATE Python::NumPy al)
set_target_properties(al PROPERTIES INSTALL_RPATH $ORIGIN)
set_target_properties(_al_lowlevel al_defs PROPERTIES INSTALL_RPATH $ORIGIN/../imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:al> DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:_al_lowlevel> DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:al_defs> DESTINATION imas_core.libs)
if(DEFINED ENV{LD_LIBRARY_PATH})
  string(REPLACE ":" ";" LIBRARY_DIRS $ENV{LD_LIBRARY_PATH})
else()
  set(LIBRARY_DIRS)
endif()
install(
  TARGETS al
  DESTINATION imas_core.libs
  RUNTIME DESTINATION imas_core.libs
  #RUNTIME_DEPENDENCIES	
  #DIRECTORIES ${LIBRARY_DIRS}
  #  PRE_EXCLUDE_REGEXES   
  #    "api-ms-" # VC Redistibutable DLLs
  #    "ext-ms-" # Windows extension DLLs
  #  POST_INCLUDE_REGEXES 
  #    "libMds" 
  #    "libTdir" 
  #    "libTree" 
  #    "libXTree"
  #    "libboost_filesystem"
  #    "libcapnp"
  #    "libhdf5"
  #    "libkj" 
  #    "libsz" 
  #    "libuda"
  #  POST_EXCLUDE_REGEXES 
  #    ".*system32/.*\\.dll" # Windows system DLLs
  #    "^/(lib|usr/lib|usr/local/lib)" # Unix system libraries
)
install(TARGETS _al_lowlevel al_defs DESTINATION imas_core)
