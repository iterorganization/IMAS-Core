"""Exception classes for the AL Core."""

raise_error_flag = False

class ALException(Exception):
    """Exception thrown by the Access Layer."""

    def __init__(self, message, errorStatus=None):
        self.errorStatus = errorStatus
        self.message = message
        if errorStatus:
            super().__init__(f"{message}\nError status={errorStatus}")
        else:
            super().__init__(message)

class ALCoreUnknownException(ALException):
    """Exception thrown by the Access Layer lowlevel when errorStatus == al_defs.UNKNOWN_ERR"""
    def __init__(self, message, errorStatus=None):
        super().__init__(message, errorStatus)

class ALCoreContextException(ALException):
    """Exception thrown by the Access Layer lowlevel when errorStatus == al_defs.CONTEXT_ERR"""
    def __init__(self, message, errorStatus=None):
        super().__init__(message, errorStatus)

class ALCoreBackendException(ALException):
    """Exception thrown by the Access Layer lowlevel when errorStatus == al_defs.BACKEND_ERR"""
    def __init__(self, message, errorStatus=None):
        super().__init__(message, errorStatus)

class ALCoreLowLevelException(ALException):
    """Exception thrown by the Access Layer lowlevel when errorStatus == al_defs.LOWLEVEL_ERR"""
    def __init__(self, message, errorStatus=None):
        super().__init__(message, errorStatus)
