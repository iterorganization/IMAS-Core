import itertools

import pytest

import imas_core


@pytest.mark.parametrize(
    "message, status",
    itertools.product(["an exception", "another exception"], [None, -3, 44]),
)
def test_ALException(message, status):
    match = message
    if status is not None:
        match += f"\nError status={status}"
    with pytest.raises(imas_core.exception.ALException, match=match):
        raise imas_core.exception.ALException(message, status)


if __name__ == "__main__":
    pytest.main([__file__])
