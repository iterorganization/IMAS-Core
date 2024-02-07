"""This module contains all constants defined by IMAS.

.. data:: CLOSEST_INTERP

    Interpolation method that returns the `closest` time slice in the original IDS
    (can break causality as it can return data ahead of requested time).

    .. seealso::
        :meth:`imas.DBEntry.get_slice` and :meth:`imas.ids_base.IDSBase.getSlice`.

.. data:: PREVIOUS_INTERP

    Interpolation method that returns the `previous` time slice if the requested time
    does not exactly exist in the original IDS.

    .. seealso::
        :meth:`imas.DBEntry.get_slice` and :meth:`imas.ids_base.IDSBase.getSlice`.

.. data:: LINEAR_INTERP

    Interpolation method that returns a linear interpolation between the existing slices
    before and after the requested time.

    .. seealso::
        :meth:`imas.DBEntry.get_slice` and :meth:`imas.ids_base.IDSBase.getSlice`.

.. data:: ASCII_SERIALIZER_PROTOCOL

    Identifier for the ASCII serialization protocol.

    .. seealso::
        :meth:`imas.ids_base.IDSBase.serialize` and
        :meth:`imas.ids_base.IDSBase.deserialize`

.. data:: DEFAULT_SERIALIZER_PROTOCOL

    Identifier for the default serialization protocol.

    .. seealso::
        :meth:`imas.ids_base.IDSBase.serialize` and
        :meth:`imas.ids_base.IDSBase.deserialize`

.. data:: OPEN_PULSE

    Opens the access to the data only if the Data Entry exists, returns error
    otherwise.

.. data:: FORCE_OPEN_PULSE

    Opens access to the data, creates the Data Entry if it does not exists yet.

.. data:: CREATE_PULSE

    Creates a new empty Data Entry (returns error if Data Entry already exists)
    and opens it at the same time.

.. data:: FORCE_CREATE_PULSE

    Creates an empty Data Entry (overwrites if Data Entry already exists) and
    opens it at the same time.
"""

from typing import Any

import numpy as np
from .al_defs import *

# This is Python 3 --> the default string type is str (always unicode)
# The bytes type is equivalent to str in Python 2
string_types = (str, bytes)

# Not all of the constants can be gotten from the Access Layer yet.
# redefine the missing ones here

# IDS NODE TYPES
NODE_TYPE_BUILT_IN = 1
NODE_TYPE_ARRAY = 2

NODE_TYPE_IDS = 3
NODE_TYPE_STRUCTURE = 4
NODE_TYPE_AOS = 5

INTERPOLATION = 3
CLOSEST_SAMPLE = 1
PREVIOUS_SAMPLE = 2
EMPTY_INT = -999999999
"""Value representing an unset integer in an IDS.
"""
EMPTY_FLOAT = -9.0e40
EMPTY_DOUBLE = -9.0e40
"""Value representing an unset floating point number in an IDS.
"""
EMPTY_COMPLEX = complex(EMPTY_DOUBLE, EMPTY_DOUBLE)
"""Value representing an unset complex number in an IDS.
"""
# new defines for struct_array management
NON_TIMED = 0
TIMED = 1
TIMED_CLEAR = 2
# printing level defines, can be changed at runtime
PRINT_DEBUG = 0
VERBOSE_DEBUG = 0
DEVEL_DEBUG = 0


IDS_TIME_MODE_UNKNOWN = -999999999
IDS_TIME_MODE_HETEROGENEOUS = 0
"""Time mode indicating that dynamic nodes may be asynchronous.

Timebases of quantities are as indicated in the "Coordinates" column of the Data
Dictionary documentation.
"""
IDS_TIME_MODE_HOMOGENEOUS = 1
"""Time mode indicating that dynamic nodes are synchronous.

Timebases of quantities are the "time" node that is the child of the nearest parent IDS.
"""
IDS_TIME_MODE_INDEPENDENT = 2
"""Time mode indicating that no dynamic nodes are filled in the IDS.
"""
IDS_TIME_MODES = [0, 1, 2]


def check_status(status):
    if PRINT_DEBUG:
        if status:
            pass
            # print(ull.imas_last_errmsg()) # this function does not exist anymore.


def verb():
    return VERBOSE_DEBUG


def dev():
    return DEVEL_DEBUG


def isFieldValid(field: Any) -> bool:
    """Test if a field in an IDS is valid (not empty or unset).

    Args:
        field: A field in an IDS.

    Returns:
        True iff the field is not empty or unset.

    Example:
        >>> import imas
        >>> ids = imas.core_profiles()
        >>> imas.imasdef.isFieldValid(ids.ids_properties.homogeneous_time)
        False
    """
    if isinstance(field, np.ndarray):
        return field.size > 0
    if isinstance(field, string_types):
        return len(field) > 0
    if isinstance(field, list):
        return len(field) > 0
    if isinstance(field, int):
        return field != EMPTY_INT
    if isinstance(field, float):
        return field != EMPTY_FLOAT
    if isinstance(field, complex):
        return field != EMPTY_COMPLEX
    return False
