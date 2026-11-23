import pytest


def pytest_configure(config):
    # Fail on features removed in pytest 10 (e.g. iterator argvalues).
    # pytest < 9 has no such class and would abort.
    if hasattr(pytest, "PytestRemovedIn10Warning"):
        config.addinivalue_line("filterwarnings", "error::pytest.PytestRemovedIn10Warning")
