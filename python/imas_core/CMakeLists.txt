cmake_minimum_required(VERSION 3.21)

cython_compile_pyx(_al_lowlevel.pyx OUTPUT_VAR _al_lowlevel_source)
python_add_library(_al_lowlevel MODULE ${_al_lowlevel_source} WITH_SOABI)
target_link_libraries(_al_lowlevel PRIVATE Python::NumPy al)
cython_compile_pyx(al_defs.pyx OUTPUT_VAR al_defs_source)
python_add_library(al_defs MODULE ${al_defs_source} WITH_SOABI)
target_link_libraries(al_defs PRIVATE Python::NumPy al)
set_target_properties(_al_lowlevel al_defs PROPERTIES INSTALL_RPATH
                                                      $ORIGIN/../imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:al> DESTINATION imas_core.libs)
install(
  TARGETS al
  COMPONENT al
  RUNTIME_DEPENDENCIES PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
  POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
  DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:_al_lowlevel> DESTINATION imas_core.libs)
install(FILES $<TARGET_RUNTIME_DLLS:al_defs> DESTINATION imas_core.libs)
install(TARGETS al DESTINATION imas_core.libs)
install(TARGETS _al_lowlevel al_defs DESTINATION imas_core)
