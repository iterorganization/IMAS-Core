import numpy as np
import pytest

import al_lowlevel


@pytest.mark.parametrize("attribute", ["al_defs", "exception", "imasdef"])
def test_modules(attribute):
    assert hasattr(al_lowlevel, attribute)


@pytest.mark.parametrize(
    "name, value", (["EMPTY_INT", -999999999], ["EMPTY_FLOAT", -9e40])
)
def test_constants(name, value):
    assert np.isclose(getattr(al_lowlevel.imasdef, name), value)


def test_DD_version():
    assert al_lowlevel.imasdef.DD_VERSION.decode() == "!!DEPRECATED!!"


def test_AL_version():
    AL_VERSION = al_lowlevel.imasdef.AL_VERSION.decode()
    assert (
        tuple(int(num) for num in AL_VERSION.split("."))
        == al_lowlevel.version_tuple[:3]
    )


if __name__ == "__main__":
    pytest.main([__file__])
