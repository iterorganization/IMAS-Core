import numpy as np
import pytest

import imas_core


def test_constants():
    assert imas_core.imasdef.EMPTY_INT == -999_999_999
    assert imas_core.imasdef.EMPTY_FLOAT == -9e40


def test_DD_version():
    assert imas_core.imasdef.DD_VERSION.decode() == "!!DEPRECATED!!"


def test_AL_version():
    AL_VERSION = imas_core.imasdef.AL_VERSION.decode()
    assert (
        tuple(int(num) for num in AL_VERSION.split(".")) == imas_core.version_tuple[:3]
    )


if __name__ == "__main__":
    pytest.main([__file__])
