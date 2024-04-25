"""Access Layer Core Python bindings."""

import os
import pathlib

if os.name == "nt":
    os.add_dll_directory(pathlib.Path(__file__).parents[1] / "imas_core.libs")

from . import _al_lowlevel
from . import imasdef

from ._version import version as __version__
from ._version import version_tuple

__all__ = ["_al_lowlevel", "imasdef", "__version__", "version_tuple"]

del os, pathlib
