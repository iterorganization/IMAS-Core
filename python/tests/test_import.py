import pytest

import imas_core


@pytest.mark.parametrize("attribute", ["al_defs", "exception", "imasdef"])
def test_modules(attribute):
    assert hasattr(imas_core, attribute)


if __name__ == "__main__":
    pytest.main([__file__])
