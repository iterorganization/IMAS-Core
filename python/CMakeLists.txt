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
if(DEFINED $ENV{LD_LIBRARY_PATH})
  string(REPLACE ":" ";" LIBRARY_DIRS $ENV{LD_LIBRARY_PATH})
else()
  set(LIBRARY_DIRS)
endif()

install(CODE [[
               file(GET_RUNTIME_DEPENDENCIES
                  EXECUTABLES  $<TARGET_FILE:al>
                  RESOLVED_DEPENDENCIES_VAR RESOLVED_DEPS
                  UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPS
                  POST_INCLUDE_REGEXES "/libboost_filesystem" "libm"
                  POST_EXCLUDE_REGEXES "^/lib"
                  )
                 foreach(FILE ${RESOLVED_DEPS})
                  #if(NOT IS_SYMLINK ${FILE})
                  message(STATUS "Resolved: ${FILE}")
                 #endif()
                 endforeach()
                   foreach(FILE ${UNRESOLVED_DEPS})
                   message(STATUS "Unresolved: ${FILE}")
                 endforeach()
         ]] COMPONENT core)

install(TARGETS _al_lowlevel al_defs DESTINATION imas_core)
