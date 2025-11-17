find_package(Python REQUIRED COMPONENTS Interpreter Development.Module NumPy)
find_package(Cython MODULE REQUIRED VERSION 3.0)
include(UseCython)
cython_transpile(imas_core/_al_lowlevel.pyx LANGUAGE C OUTPUT_VARIABLE _al_lowlevel_source)
cython_transpile(imas_core/al_defs.pyx LANGUAGE C OUTPUT_VARIABLE al_defs_source)
python_add_library(_al_lowlevel MODULE ${_al_lowlevel_source} WITH_SOABI)
python_add_library(al_defs MODULE ${al_defs_source} WITH_SOABI)
target_link_libraries(_al_lowlevel PRIVATE Python::NumPy al)
target_link_libraries(al_defs PRIVATE Python::NumPy al)
set_target_properties(al PROPERTIES INSTALL_RPATH $ORIGIN)
set_target_properties(_al_lowlevel al_defs PROPERTIES INSTALL_RPATH $ORIGIN/../imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:al> DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:_al_lowlevel> DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:al_defs> DESTINATION imas_core.libs)
install(
  TARGETS al
  DESTINATION imas_core.libs
  RUNTIME_DEPENDENCIES
    PRE_EXCLUDE_REGEXES   
      "api-ms-" # VC Redistributable DLLs
      "ext-ms-" # Windows extension DLLs
    POST_EXCLUDE_REGEXES 
      ".*system32/.*\\.dll" # Windows system DLLs
      "^/(lib|usr/lib|usr/local/lib|lib64|usr/lib64|usr/local/lib64)" # Unix system libraries
)
install(TARGETS _al_lowlevel al_defs DESTINATION imas_core)
