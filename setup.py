#! /usr/bin/env python

# System imports
from numpy.distutils.core import setup, Extension

# Third-party modules - we depend on numpy for everything
import numpy

# Obtain the numpy include directory.  This logic works across numpy versions.
try:
    numpy_include = numpy.get_include()
except AttributeError:
    numpy_include = numpy.get_numpy_include()

numpy_swig_include = '.'
#/afs/efda-itm.eu/euforia/user/haefele/public/Soft/swig'

# _ual_low_level extension module
ual_low_level = Extension('_ual_low_level',
                    ['ual_low_level.i', 'empty.c'],
                    include_dirs = [numpy_include, numpy_swig_include, '.'],
                    library_dirs = ['.'],
                    libraries = ['UALLowLevel'],
                    swig_opts = ['-I'+numpy_include, '-I'+numpy_swig_include ]
                    )



# ual low level setup
setup(name        = "ual_low_level",
      description = "Python bindings for the ual low level library",
      author      = "Matthieu Haefele",
      py_modules  = ["ual_low_level"],
      ext_modules = [ual_low_level]
      )
