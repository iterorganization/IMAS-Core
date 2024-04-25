"""Access Layer Core Python bindings.
"""

import os 
import pathlib

os.add_dll_directory(pathlib.Path(__file__).parents[1] / "imas_core.libs")

from . import _al_lowlevel
from . import imasdef

from ._version import version as __version__
from ._version import version_tuple
