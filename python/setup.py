import os
import logging

from setuptools import setup, Extension
from Cython.Build import cythonize

import numpy

logger = logging.getLogger("imas_core")

# For now this environment variable is set by CMake
# TODO: figure out if this is the best approach
if "AL_PROJECT_VERSION" in os.environ:
    VERSION = os.environ["AL_PROJECT_VERSION"]
else:
    # Fallback version
    VERSION = "0.0.0+unknown"
    logger.warning("Could not determine project version, falling back to '%s'", VERSION)


include_dirs = [numpy.get_include()]
if "AL_INCLUDE_PATH" in os.environ:
    include_dirs.extend(os.environ["AL_INCLUDE_PATH"].split(":"))
library_dirs = []
if "AL_LIBRARY_PATH" in os.environ:
    library_dirs.extend(os.environ["AL_LIBRARY_PATH"].split(":"))

ext_modules = [
    Extension(
        name="imas_core._al_lowlevel",
        sources=["imas_core/_al_lowlevel.pyx"],
        language="c",
        libraries=["al"],
        extra_link_args=[],
        include_dirs=include_dirs,
        library_dirs=library_dirs,
    ),
    Extension(
        name="imas_core.al_defs",
        sources=["imas_core/al_defs.pyx"],
        language="c",
        libraries=["al"],
        extra_link_args=[],
        include_dirs=include_dirs,
        library_dirs=library_dirs,
    ),
]

setup(
    name="imas_core",
    version=VERSION,
    description="Python bindings to the IMAS Access Layer core",
    author="ITER Organization",
    author_email="imas-support@iter.org",
    url="https://imas.iter.org/",
    classifiers=[
        "Development Status :: 5 - Beta",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: Other/Proprietary License",
        "Programming Language :: Python :: 3",
        "Topic :: Scientific/Engineering :: Physics",
    ],
    packages=["imas_core"],
    keywords="imas, access layer, python interface",
    ext_modules=cythonize(ext_modules),
    python_requires=">=3.6, <4",
    setup_requires=["setuptools"],
)
