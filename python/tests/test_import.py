import numpy as np
import pytest

import imas_core


@pytest.mark.parametrize("attribute", ["al_defs", "exception", "imasdef"])
def test_modules(attribute):
    assert hasattr(imas_core, attribute)


@pytest.mark.parametrize(
    "name, value", (["EMPTY_INT", -999999999], ["EMPTY_FLOAT", -9e40])
)
def test_constants(name, value):
    assert np.isclose(getattr(imas_core.imasdef, name), value)


def test_DD_version():
    assert imas_core.imasdef.DD_VERSION.decode() == "!!DEPRECATED!!"


def test_AL_version():
    AL_VERSION = imas_core.imasdef.AL_VERSION.decode()
    assert (
        tuple(int(num) for num in AL_VERSION.split("."))
        == imas_core.version_tuple[:3]
    )


if __name__ == "__main__":
    pytest.main([__file__])
