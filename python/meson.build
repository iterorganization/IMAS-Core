add_languages('cython', native: false)

# =============================================================================
# Version File
# =============================================================================
# Get version using setuptools-scm
version = run_command('python', '-m', 'setuptools_scm', check: true).stdout().strip()

# Create tuple: split version like "5.5.2", ignoring dirty suffix etc.
split_ver = version.split('.')
ver_tuple = '(@0@, @1@, @2@)'.format(
  split_ver[0],
  split_ver[1],
  split_ver[2],
)

# Get commit id
git_commit = run_command('git', 'rev-parse', '--short', 'HEAD', check: false).stdout().strip()

if git_commit == ''
  git_commit = 'None'
endif

# Generate _version.py
version_file = configure_file(
  input: '_version.py.in',
  output: '_version.py',
  configuration: {
    'VERSION': version,
    'VERSION_TUPLE': ver_tuple,
    'COMMIT_ID': git_commit,
  },
)

# =============================================================================
# Modules and Dependencies
# =============================================================================
fs = import('fs')
py = import('python').find_installation(pure: false)
py_dep = dependency('python3', native: false)
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
py.install_sources(py_files + pxd_files + version_file, subdir: 'imas_core')