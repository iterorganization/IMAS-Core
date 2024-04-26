#cython: language_level=3

# This file makes the al definitions available to our cython code.
# We need this to ensure the (numeric) constants used are compatible
# with the back-end version used.

cdef extern from "al_defs.h":
    # We rename the al_defs.h values, so we can export them under their proper
    # names in al_defs.pyx
    enum:
        AL_MAXDIM "MAXDIM"

        AL_OP_INTERP_0 "OP_INTERP_0"
        AL_BACKEND_ID_0 "BACKEND_ID_0"
        AL_OP_RANGE_0 "OP_RANGE_0"
        AL_OP_ACCESS_0 "OP_ACCESS_0"
        AL_ACCESS_PULSE_0 "ACCESS_PULSE_0"
        AL_DATA_TYPE_0 "DATA_TYPE_0"
        AL_SERIALIZER_PROTOCOL_0 "SERIALIZER_PROTOCOL_0"
        AL_ERR_0 "ERR_0"

        AL_GLOBAL_OP "GLOBAL_OP"
        AL_SLICE_OP "SLICE_OP"

        AL_READ_OP "READ_OP"
        AL_WRITE_OP "WRITE_OP"
        AL_REPLACE_OP "REPLACE_OP"

        AL_UNDEFINED_INTERP "UNDEFINED_INTERP"
        AL_CLOSEST_INTERP "CLOSEST_INTERP"
        AL_PREVIOUS_INTERP "PREVIOUS_INTERP"
        AL_LINEAR_INTERP "LINEAR_INTERP"

        AL_OPEN_PULSE "OPEN_PULSE"
        AL_FORCE_OPEN_PULSE "FORCE_OPEN_PULSE"
        AL_CREATE_PULSE "CREATE_PULSE"
        AL_FORCE_CREATE_PULSE "FORCE_CREATE_PULSE"
        AL_CLOSE_PULSE "CLOSE_PULSE"
        AL_ERASE_PULSE "ERASE_PULSE"

        AL_CHAR_DATA "CHAR_DATA"
        AL_INTEGER_DATA "INTEGER_DATA"
        AL_DOUBLE_DATA "DOUBLE_DATA"
        AL_COMPLEX_DATA "COMPLEX_DATA"

        AL_ASCII_SERIALIZER_PROTOCOL "ASCII_SERIALIZER_PROTOCOL"
        AL_DEFAULT_SERIALIZER_PROTOCOL "DEFAULT_SERIALIZER_PROTOCOL"

        AL_UNKNOWN_ERR "UNKNOWN_ERR"
        AL_CONTEXT_ERR "CONTEXT_ERR"
        AL_BACKEND_ERR "BACKEND_ERR"
        AL_LOWLEVEL_ERR "LOWLEVEL_ERR"

        AL_MAX_ERR_MSG_LEN "MAX_ERR_MSG_LEN"

    float AL_UNDEFINED_TIME "UNDEFINED_TIME"
    char[] AL_AL_VERSION "AL_VERSION"
    char[] AL_DD_VERSION "DD_VERSION"


cdef extern from "al_const.h":
    enum:
        AL_NO_BACKEND "NO_BACKEND"
        AL_ASCII_BACKEND "ASCII_BACKEND"
        AL_MDSPLUS_BACKEND "MDSPLUS_BACKEND"
        AL_HDF5_BACKEND "HDF5_BACKEND"
        AL_MEMORY_BACKEND "MEMORY_BACKEND"
        AL_UDA_BACKEND "UDA_BACKEND"
