"""Exception classes for the AL Core."""


class ALException(Exception):
    """Exception thrown by the Access Layer."""

    def __init__(self, message, errorStatus=None):
        self.errorStatus = errorStatus
        self.message = message
        if errorStatus:
            super().__init__(f"{message}\nError status={errorStatus}")
        else:
            super().__init__(message)
