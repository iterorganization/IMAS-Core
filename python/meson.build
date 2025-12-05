add_languages('cython', native: false)

# =============================================================================
# Modules and Dependencies
# =============================================================================
fs = import('fs')
py = import('python').find_installation(pure: false)
py_dep = py.dependency()
numpy_dep = dependency('numpy')

cython_args = ['--annotate']

# =============================================================================
# Source files
# =============================================================================
py_files = files(
  'imas_core/__init__.py',
  'imas_core/exception.py',
  'imas_core/imasdef.py',
)
pyx_files = files(
  'imas_core/_al_lowlevel.pyx',
  'imas_core/al_defs.pyx',
)
pxd_files = files(
  'imas_core/al_defs.pxd',
  'imas_core/al_lowlevel_interface.pxd',
)

# =============================================================================
# Build Python Extension Module
# =============================================================================
# compile cython
foreach pyx_file : pyx_files
  py.extension_module(
    fs.stem(pyx_file),
    pyx_file,
    cython_args: cython_args,
    dependencies: [al_core_dep, py_dep, numpy_dep],
    install: true,
    subdir: 'imas_core',
  )
endforeach

# Install pure python files
py.install_sources(py_files + pxd_files, subdir: 'imas_core')

# =============================================================================
# Version File