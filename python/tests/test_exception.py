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


@pytest.mark.parametrize("exception_type", ["", "Backend", "Context"])
def test_ImasCoreException(exception_type):
    _Exception = getattr(imas_core.exception, f"ImasCore{exception_type}Exception")
    if not exception_type:
        exception_type = "LowLevel"
    status = getattr(imas_core.al_defs, f"{exception_type.upper()}_ERR")
    message = "from user"
    match = f"{message}\nError status={status}"
    with pytest.raises(_Exception, match=match):
        raise _Exception(message)


if __name__ == "__main__":
    pytest.main([__file__])
