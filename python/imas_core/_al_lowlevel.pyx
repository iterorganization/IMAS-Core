import logging
import sys

from cython.view cimport array as cvarray
from cython cimport view
from cython cimport *
from cpython cimport *
from libc.stdlib cimport *
from libc.string cimport *
import numpy as np
cimport numpy as np

from . cimport al_lowlevel_interface as ll
from . import exception
from .exception import ALException, ImasCoreContextException, ImasCoreBackendException, ImasCoreException, raise_error_flag

cdef extern from "Python.h":
    const char* PyUnicode_AsUTF8(object unicode) except NULL

# directly import a few parameters
cdef extern from "al_defs.h":
    enum:
        CHAR_DATA
        INTEGER_DATA
        DOUBLE_DATA
        COMPLEX_DATA
        # alerror:: error codes
        UNKNOWN_ERR
        CONTEXT_ERR
        BACKEND_ERR
        LOWLEVEL_ERR

###########################################################################################

np.import_array()
# np.import_umath()


def __cinit__(self):
    pass

##########################################################################################

cdef void * convertStringArrayToCBuffer(stringList, maxStringLen):
    cdef char* retBuffer
    cdef int offset = 0
    cdef int iBufferSize = 0

    iBufferSize = len(stringList) * maxStringLen * sizeof(char)
    retBuffer = <char*> malloc(iBufferSize)
    memset(retBuffer, 0, iBufferSize)

    for string in stringList:
        memcpy(retBuffer + offset, PyUnicode_AsUTF8(string), len(string) * sizeof(char))
        offset = offset + maxStringLen * sizeof(char)

    return retBuffer


cdef convertCBufferToStringArray(char * cBuffer, np.ndarray arrSizes):

    pyString = b''
    cdef int offset = 0
    cdef int listSize = arrSizes[0]
    cdef int maxStringLen = arrSizes[1]

    retArray = []

    for _ in range(listSize):
        pyString = cBuffer[offset: (
            offset + maxStringLen)].decode('UTF-8', errors='replace').rstrip('\x00')
        retArray.append(pyString)
        offset = offset + maxStringLen

    return retArray

# PyObject* PyBuffer_FromMemory (void *ptr, int size)

# int PyCObject_Check(     PyObject *p)
# PyObject* PyCObject_FromVoidPtr(     void* cobj, void (*destr)(void *))
# void* PyCObject_AsVoidPtr(     PyObject* self)

# PyCObjects are deprecated from Python 2.7+
# See https://docs.python.org/3/howto/cporting.html
"""
cdef void* convertObjectToVoidPtr(object inObj):

     cdef void* retPtr = NULL

     if inObj is None:
       return NULL

     retPtr = PyCapsule_GetPointer(inObj, "ull")

     return retPtr


cdef object  convertVoidPtrToObject(void* voidPtr):

     cdef object retObj = None

     if voidPtr is NULL:
       return None

     #retObj = <object> PyCObject_FromVoidPtr( voidPtr, NULL)
     retObj = PyCapsule_New(voidPtr, "ull", NULL)


     return retObj

"""


def _getDataType(data):
    dataType = type(data)

    if dataType == np.ndarray:
        # ARRAY data
        npDataType = data.dtype

        if np.issubdtype(npDataType, np.signedinteger):
            retType = INTEGER_DATA
        elif np.issubdtype(npDataType, np.floating):
            retType = DOUBLE_DATA
        elif np.issubdtype(npDataType, np.complexfloating):
            retType = COMPLEX_DATA
        elif np.issubdtype(npDataType, str) or np.issubdtype(npDataType, unicode):
            retType = CHAR_DATA
        else:
            raise ALException('UNKNOWN DATA TYPE :' + str(npDataType))
    elif dataType == list:
        if all([(isinstance(i, str) or
                 isinstance(i, bytes) or
                 isinstance(i, unicode)) for i in data]):
            retType = CHAR_DATA
        else:
            raise ALException('UNKNOWN DATA TYPE :' + str(dataType))
    else:
        # SCALAR data
        if issubclass(dataType, int) or issubclass(dataType, np.integer):
            retType = INTEGER_DATA
        elif issubclass(dataType, float):
            retType = DOUBLE_DATA
        elif issubclass(dataType, complex):
            retType = COMPLEX_DATA
        elif issubclass(dataType, str) or issubclass(dataType, unicode):
            retType = CHAR_DATA
        else:
            raise ALException('UNKNOWN DATA TYPE :' + str(dataType))

    return retType


###########################################################################################
#                 NEW LL
###########################################################################################

def imas_core_config_enable_exceptions():
    '''Function used to enable Access Layer Core raising exceptions on error, instead of returning status value'''
    exception.raise_error_flag = True

def imas_core_config_disable_exceptions():
    '''Function used to disable Access Layer Core raising exceptions on error. Functions will return status value on error'''
    exception.raise_error_flag = False


def get_proper_exception_class(exception_message, exception_code):
    '''
    Helper function used by _al_lowlevel functions.

    Parameters:
        exception_message (str): Exception message to be passed to exception __init__
        exception_code (int): code used to determine type of exception

    Returns:
        exception: Exception instance initialized with exception message and exception code. Ready to be raised.
    '''
    exception_classes = {UNKNOWN_ERR : ALException,
                        CONTEXT_ERR : ImasCoreContextException,
                        BACKEND_ERR : ImasCoreBackendException,
                        LOWLEVEL_ERR : ImasCoreException}

    exception_to_be_raised = exception_classes.get(exception_code,ALException)

    return exception_to_be_raised(exception_message, exception_code)


"""
     Return all the Context information corresponding to the passed Context identifier.
     @param[in] ctx Context ID (either DataEntryContext, OperationContext or ArraystructContext)
     @param[out] info context info as a string
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
     al_status_t al_context_info(int ctx, char **info)
"""


def al_context_info(ctx):

    cdef ll.al_status_t al_status
    cdef char * cContextInfo = ""

    al_status = ll.al_context_info(ctx, & cContextInfo)

    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
            return al_status.code, ""

    contextInfo = cContextInfo.decode('UTF-8', errors='replace')
    return contextInfo


"""
     Get backendID from the passed Context identifier.
     @param[in] ctx Context ID (either PulseContext, OperationContext or ArraystructContext)
     @param[out] beid backendID
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
"""

def al_get_backendID(ctx):

    cdef ll.al_status_t al_status
    cdef int cBEID

    al_status = ll.al_get_backendID(ctx, & cBEID)

    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
            return al_status.code, ""

    return cBEID




"""
     Starts an action on a pulse in the database.
     This function associates a specified back-end with a specific entry in the database.
     @param[in] backendID name/ID of the back-end
     @param[in] pulse pulse number
     @param[in] run run number
     @param[in] user username
     @param[in] tokamak tokamak name
     @param[in] version data version
     @param[in] options [_optional, "" for default_]
     @result error status [_success if al_status_t.code = 0 or failure if < 0_], URI
  al_status_t al_build_uri_from_legacy_parameters(const int backendID,
                   const int pulse,
                   const int run,
                   const char *user,
                   const char *tokamak,
                   const char *version,
                   const char * options,
                   char** uri);
"""


def al_build_uri_from_legacy_parameters(backendID, pulse, run, user, tokamak, version, options):

    cdef int pulseCtx = -1
    cdef ll.al_status_t al_status
    cdef char * cStringUri = NULL
    al_status = ll.al_build_uri_from_legacy_parameters(backendID,
                                           pulse,
                                           run,
                                           user.encode('UTF-8'),
                                           tokamak.encode('UTF-8'),
                                           version.encode('UTF-8'),
                                           options.encode('UTF-8'),
                                           &cStringUri)

    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
            return al_status.code, -1

    py_uri = cStringUri.decode('UTF-8', errors='replace')

    free(cStringUri)

    return al_status.code, py_uri

###########################################################################################


"""
     Starts an action for a IMAS data entry.
     This function opens a data entry
     @param[in] URI of this data entry
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @param[out] IMAS data entry context
     @result error status

  int al_begin_dataentry_action(const char* uri,
               int mode,
               int *dectxID)
"""


def al_begin_dataentry_action(uri, mode):

    cdef int deCtx = -1
    cdef ll.al_status_t al_status
    al_status = ll.al_begin_dataentry_action(uri.encode('UTF-8'), mode,  &deCtx)
    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
    return al_status.code, deCtx


###########################################################################################
"""
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] pulseCtx pulse context id (from al_begin_pulse_action())
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse
     @result error status

  int al_close_pulse(int pulseCtx,
                int mode)
"""


def al_close_pulse(pulseCtx, mode):

    al_status = ll.al_close_pulse(pulseCtx, mode)
    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
    return al_status.code


###########################################################################################
"""
     Starts an I/O action on a DATAOBJECT.
     This function gives a new operation context for the duration of an action on a DATAOBJECT.
     @param[in] ctx pulse context id (from al_begin_pulse_action())
     @param[in] dataobjectname name of the DATAOBJECT
     @param[in] rwmode mode for this operation:
     - READ_OP = read operation
     - WRITE_OP = write operation
     @param[in] datapath optional path to data node for partial_get operations
     @param[out] opctx operation context id [_null context if = 0_]
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  al_status_t al_begin_global_action(int ctx,
                    const char *dataobjectname,
                    int rwmode,
                    int *opctx);

"""


def al_begin_global_action(pulseCtx, dataobjectname, rwmode, datapath=""):

    cdef int opCtx = -1

    al_status = ll.al_begin_global_action(pulseCtx,
                                            dataobjectname.encode('UTF-8'),
                                            datapath.encode('UTF-8'),
                                            rwmode,
                                            & opCtx)

    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
            return al_status.code, -1

    return al_status.code, opCtx


###########################################################################################
"""
     Starts an I/O action on a DATAOBJECT slice.
     This function gives a new operation context for the duration of an action on a slice.
     @param[in] ctx pulse context (from al_begin_pulse_action())
     @param[in] dataobjectname name of the DATAOBJECT
     @param[in] rwmode mode for this operation:
     - READ_OP: read operation
     - WRITE_OP: write operation
     - REPLACE_OP: replace operation
     @param[in] time time of the slice
     - UNDEFINED_TIME if not relevant (e.g to append a slice or replace the last slice)
     @param[in] interpmode mode for interpolation:
     - CLOSEST_INTERP take the slice at the closest time
     - PREVIOUS_INTERP take the slice at the previous time
     - LINEAR_INTERP interpolate the slice between the values of the previous and next slice
     - UNDEFINED_INTERP if not relevant (for write operations)
     @param[out] opctx operation context id [_null context if = 0_]
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]

  al_status_t al_begin_slice_action(int ctx,
                   const char *dataobjectname,
                   int rwmode,
                   double time,
                   int interpmode,
                   int *opctx);
"""


def al_begin_slice_action(pulseCtx, dataobjectname, rwmode, time, interpmode):

    cdef int sliceOpCtx = -1

    al_status = ll.al_begin_slice_action(pulseCtx,
                                           dataobjectname.encode('UTF-8'),
                                           rwmode,
                                           time,
                                           interpmode,
                                           & sliceOpCtx)

    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
            return al_status.code, -1

    return al_status.code, sliceOpCtx


def al_begin_timerange_action(pulseCtx, dataobjectname, rwmode, tmin, tmax, dtime=None, interpmode=None):
    """Begin a time range action.

    Args:
        pulseCtx: data entry context returned by ``al_begin_dataentry_action``
        dataobjectname: name of the data object (``IDS[/occurrence]``)
        rwmode: read/write mode, currently only supports ``READ_OP``
        tmin: lower bound of the requested time range interval
        tmax: upper bound of the requested time range interval
        dtime: time increment of the time range interval. If dtime != None,
            data are resampled on a new homogeneous time vector
        interpmode: mode for interpolation

    Returns:
        Created operation context ID
    """
    if tmin > tmax:
        raise ValueError(f"Invalid time range: {tmin} > {tmax}")
    if dtime is not None and interpmode is None:
        raise ValueError("Interpolation requested (dtime != None) but no interpolation mode provided.")

    cdef int opctx = -1
    al_status = ll.al_begin_timerange_action(
        pulseCtx,
        dataobjectname.encode('UTF-8'),
        rwmode,
        tmin,
        tmax,
        -1 if dtime is None else dtime,
        interpmode or 0,
        &opctx
    )

    if al_status.code < 0:
        raise ALException(al_status.message, al_status.code)

    return opctx


###########################################################################################
"""
     Stops an I/O action.
     This function stop the current action designed by the context passed as argument.
     This context is then not valid anymore.
     @param[in] ctx a pulse (al_begin_pulse_action()),
                an operation (al_begin_global_action() or al_begin_slice_action())
                or an array of structure context id (al_begin_array_struct_action())
     @result error status

     @test Low-level API, implementation of endDataObjectGetSlice()
     @snippet al_low_level.c ex_al_end_action

  int al_end_action(int ctx)
"""


def al_end_action(ctx):

    al_status = ll.al_end_action(ctx)
    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
            return al_status.code, -1

    return al_status.code


###########################################################################################
"""
     Writes data.
     This function writes a signal in the database given the passed context.
     @param[in] ctx operation context id
                (from al_begin_global_action() or al_begin_slice_action()) or
                array of structure context id (from al_begin_arraystruct_action())
     @param[in] fieldname field name
     @param[in] timebasename field name for the timebase
     @param[in] data pointer on the data to be written
     @param[in] datatype type of data to be written:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in] size array of the size of each dimension (can be NULL if dim=0)
     @result error status

     @test Low-level API, implementation of putVect2DIntSlice()
     @snippet al_low_level.c ex_al_write_data

  al_status_t al_write_data(int ctx,
               const char *fieldname,
               const char *timebasename,
               void *data,
               int datatype,
               int dim,
               int *size)
"""


def _al_write_data_scalar(ctx, fieldname, pyTimebasePath,  inputData, dataType):
    cdef int cDataType = dataType
    cdef int cIntData
    cdef double cDoubleData
    cdef double complex cComplexData
    cdef char * cStringData
    cdef void * cData

    # Set data type
    dataType = type(inputData)

    if issubclass(dataType, int) or issubclass(dataType, np.integer):
        cIntData = inputData
        cData = <void*> & cIntData

    elif issubclass(dataType, float):
        cDoubleData = inputData
        cData = <void*> & cDoubleData

    elif issubclass(dataType, complex):
        cComplexData = inputData
        cData = <void*> & cComplexData

    elif issubclass(dataType, str) or dataType == unicode:
        cStringData = inputData
        cData = <void*>cStringData
    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(dataType))

    if len(pyTimebasePath) == 0:
        cTimebasePath = ""
    else:
        cTimebasePath = pyTimebasePath

    al_status = ll.al_write_data(ctx, fieldname.encode(
        'UTF-8'), cTimebasePath.encode('UTF-8'), cData, cDataType, 0, NULL)

    if al_status.code < 0:
        raise get_proper_exception_class('Error while writing data: ' + al_status.message, al_status.code)

    return al_status.code


def _al_write_data_array(ctx, fieldname, pyTimebasePath,  np.ndarray inputData, dataType):
    cdef int cDim = inputData.ndim
    cdef int cDataType = dataType
    cdef np.ndarray normalizedArray
    cdef np.ndarray npSizeArray
    cdef char * cTimebasePath
    cdef char * cstring

    # is the <object> necessary?
    npSizeArray = np.asarray((<object> inputData).shape, dtype=np.int32)  # noqa: E225

    if np.amin(npSizeArray) == 0:
        return 0

    # checking if an array is valid
    if np.amin(npSizeArray) < 0:
        raise ALException('Array dimension size cannot be < 0')

    npDataType = inputData.dtype
    if np.issubdtype(npDataType, np.signedinteger):
        normalizedArray = inputData.astype(np.int32)
    elif np.issubdtype(npDataType, np.floating):
        normalizedArray = inputData.astype(np.float64)
    elif np.issubdtype(npDataType, np.complexfloating):
        normalizedArray = inputData.astype(np.complex128)
    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(npDataType))

    #print (normalizedArray.flags)
    #cdef void * cData = convertArrayToCBuffer(normalizedArray)

    #normalizedArray = normalizedArray.ravel(order='F')
    if not normalizedArray.flags['F_CONTIGUOUS']:
        normalizedArray = np.asfortranarray(normalizedArray)

    cdef void * cData = <void*> normalizedArray.data

    cdef int * cSizeArray = <int*> npSizeArray.data

    if len(pyTimebasePath) == 0:
        cTimebasePath = ""
    else:
        if not PyBytes_Check(pyTimebasePath):
            py_byte_string = pyTimebasePath.encode('UTF-8')
            cstring = py_byte_string
            cTimebasePath = cstring
        else:
            cTimebasePath = pyTimebasePath

    if not PyBytes_Check(fieldname):
        fieldname = fieldname.encode('UTF-8')

    if not PyBytes_Check(cTimebasePath):
        py_byte_string = cTimebasePath.encode('UTF-8')
        cstring = py_byte_string
        cTimebasePath = cstring

    al_status = ll.al_write_data(
        ctx, fieldname, cTimebasePath, cData, cDataType, cDim, cSizeArray)
    if al_status.code < 0:
        raise get_proper_exception_class('Error while writing data: ' + al_status.message, al_status.code)
    return al_status.code


def al_write_slice_data(ctx, fieldname, pyTimebasePath, inputData):

    alDataType = _getDataType(inputData)
    pyDataType = type(inputData)

    if pyDataType == np.ndarray and alDataType != CHAR_DATA:
        if inputData.ndim == 1:
            inputData = inputData[:1]
        elif inputData.ndim == 2:
            inputData = inputData[:, :1]
        elif inputData.ndim == 3:
            inputData = inputData[:, :, :1]
        elif inputData.ndim == 4:
            inputData = inputData[:, :, :, :1]
        elif inputData.ndim == 5:
            inputData = inputData[:, :, :, :, :1]
        elif inputData.ndim == 6:
            inputData = inputData[:, :, :, :, :, :1]

    elif alDataType == CHAR_DATA:  # Only STR_1D are supported
        inputData = inputData[:1]
    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(pyDataType))

    al_status = al_write_data(ctx, fieldname.encode(
        'UTF-8'), pyTimebasePath.encode('UTF-8'), inputData)
    if al_status.code < 0:
        raise get_proper_exception_class('Error while writing data: ' + al_status.message, al_status.code)

    return al_status.code


def _al_write_data_string_array(ctx, fieldname, pyTimebasePath, list stringList, dataType):

    cdef np.ndarray npSizeArray
    cdef int listSize = len(stringList)
    cdef int strLen = -1
    cdef int strMaxLen = -1

    npSizeArray = np.zeros(2, dtype=np.int32)

    # determining the longest string
    for i in range(listSize):
        strLen = len(stringList[i])
        if strLen > strMaxLen:
            strMaxLen = strLen

    npSizeArray[0] = listSize
    npSizeArray[1] = strMaxLen

    cdef void * cData = convertStringArrayToCBuffer(stringList, strMaxLen)
    cdef int * cSizeArray = <int*> npSizeArray.data

    if not PyBytes_Check(fieldname):
        fieldname = fieldname.encode('UTF-8')
    if not PyBytes_Check(pyTimebasePath):
        pyTimebasePath = pyTimebasePath.encode('UTF-8')

    al_status = ll.al_write_data(
        ctx, fieldname, pyTimebasePath, cData, CHAR_DATA, 2, cSizeArray)
    if al_status.code < 0:
        raise get_proper_exception_class('Error while writing data: ' + al_status.message, al_status.code)


    free(cData)
    return al_status.code


def _al_write_data_string(ctx, fieldname, pyTimebasePath, inputString, dataType):
    cdef char * cStringData
    cdef int cStringSize[1]
    py_byte_string = inputString.encode('UTF-8')
    cStringData = py_byte_string
    cStringSize[0] = len(cStringData)

    al_status = ll.al_write_data(ctx,
                                   fieldname.encode('UTF-8'),
                                   pyTimebasePath.encode('UTF-8'),
                                   <void*>cStringData,  # noqa: E225
                                   CHAR_DATA,
                                   1,
                                   & (cStringSize[0]))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while writing data: ' + al_status.message, al_status.code)

    return al_status.code


def al_write_data(ctx,
                   pyFieldPath,
                   pyTimebasePath,
                   inputData):

    cdef ll.al_status_t al_status
    if inputData is None:
        al_status = ll.al_write_data(ctx, pyFieldPath.encode(
        'UTF-8'), pyTimebasePath.encode('UTF-8'), NULL, -1, 0, NULL)
        return al_status.code

    alDataType = _getDataType(inputData)
    pyDataType = type(inputData)

    if pyDataType == np.ndarray and alDataType != CHAR_DATA:
        status = _al_write_data_array(
            ctx, pyFieldPath, pyTimebasePath, inputData, alDataType)
    elif pyDataType == list and alDataType == CHAR_DATA:
        status = _al_write_data_string_array(
            ctx, pyFieldPath, pyTimebasePath, inputData, alDataType)

    elif pyDataType == str or pyDataType == unicode:
        status = _al_write_data_string(
            ctx, pyFieldPath, pyTimebasePath, inputData, alDataType)

    elif (issubclass(pyDataType, int)
          or issubclass(pyDataType, np.integer)
          or issubclass(pyDataType, float)
          or issubclass(pyDataType, complex)
          or pyDataType == str
          or pyDataType == unicode):
        status = _al_write_data_scalar(
            ctx, pyFieldPath, pyTimebasePath, inputData, alDataType)

    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(pyDataType))

    return status


###########################################################################################
"""
     Reads data.
     This function reads a signal in the database given the passed context.
     @param[in] ctx operation context id
                (from al_begin_global_action() or al_begin_slice_action()) or
                array of structure context id (from al_begin_arraystruct_action())
     @param[in] fieldname field name
     @param[in] timebasename field name for the timebase
     @param[out] data returned pointer on the read data
     @param[in] datatype type of data to be read:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in,out] size passed array for storing the size of each dimension
                    (size[i] undefined if i>=dim)
     @result error status

     @test Low-level API, implementation of getVect3DDouble()
     @snippet al_low_level.c ex_al_read_data

  int al_read_data(int ctx,
              const char *fieldname,
              const char *timebasename,
              void **data,
              int datatype,
              int dim,
              int *size)
"""


def al_read_data_scalar(ctx, fieldPath, pyTimebasePath, dataType):
    cdef int cIntData
    cdef double cDoubleData
    cdef double complex cComplexData
    cdef char cCharData
    cdef void * cData
    cdef int cSize

    if dataType == INTEGER_DATA:
        cData = <void*> & cIntData
    elif dataType == DOUBLE_DATA:
        cData = <void*> & cDoubleData
    elif dataType == COMPLEX_DATA:
        cData = <void*> & cComplexData
    elif dataType == CHAR_DATA:
        cData = <void*> & cCharData
    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(dataType))

    if not PyBytes_Check(fieldPath):
        fieldPath = fieldPath.encode('UTF-8')
    if not PyBytes_Check(pyTimebasePath):
        pyTimebasePath = pyTimebasePath.encode('UTF-8')

    al_status = ll.al_read_data(ctx, fieldPath, pyTimebasePath, & cData, dataType, 0, & cSize)
    if al_status.code < 0:
        raise get_proper_exception_class('Error while reading data: ' + al_status.message, al_status.code)

    if dataType == INTEGER_DATA:
        return al_status.code, cIntData

    elif dataType == DOUBLE_DATA:
        return al_status.code, cDoubleData

    elif dataType == COMPLEX_DATA:
        return al_status.code, cComplexData

    elif dataType == CHAR_DATA:
        return al_status.code, cCharData

    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(dataType))


def al_read_data_array(ctx, fieldPath, pTimebasePath, dataType, dim):
    cdef int cDim = dim
    # cdef int size = MAXDIM

    cdef void * cData
    cdef int[:] cSizeArray = cvarray(shape=(dim,), itemsize=sizeof(int), format="i")

    cdef view.array arrayMemView

    if not PyBytes_Check(fieldPath):
        fieldPath = fieldPath.encode('UTF-8')
    if not PyBytes_Check(pTimebasePath):
        pTimebasePath = pTimebasePath.encode('UTF-8')

    al_status = ll.al_read_data(ctx,
                                  fieldPath,
                                  pTimebasePath,
                                  & cData,
                                  dataType,
                                  cDim,
                                  & (cSizeArray[0]))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while reading data: ' + al_status.message, al_status.code)

    if cData is NULL:
        return al_status.code, None
    for dimsize in cSizeArray:
        if dimsize == <int>0:
            return al_status.code, None

    if dataType == INTEGER_DATA:
        item_size = sizeof(int)
        item_type = "i"
    elif dataType == DOUBLE_DATA:
        item_size = sizeof(double)
        item_type = "d"
    elif dataType == COMPLEX_DATA:
        item_size = sizeof(double complex)
        item_type = "Zd"
    elif dataType == CHAR_DATA:
        item_size = sizeof(char)
        item_type = "b"
    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(dataType))

    arrayMemView  = view.array(shape = tuple(cSizeArray), itemsize = item_size, format = item_type,  mode="fortran", allocate_buffer=False)
    arrayMemView.data = <char*  > cData
    arrayMemView.callback_free_data = free

    return al_status.code, np.asarray(arrayMemView)



def al_read_data_array_string(ctx, fieldPath, pTimebasePath, dataType, dim):
    # cdef int size = MAXDIM

    cdef char * cStringData
    # cdef int[2] cSizeArray= cvarray(shape=(dim,), itemsize=sizeof(int), format="i")
    cdef int[2] cSizeArray
    cdef np.ndarray npSizeArray

    if not PyBytes_Check(fieldPath):
        fieldPath = fieldPath.encode('UTF-8')
    if not PyBytes_Check(pTimebasePath):
        pTimebasePath = pTimebasePath.encode('UTF-8')

    al_status = ll.al_read_data(ctx,
                                  fieldPath,
                                  pTimebasePath,
                                  < void**> & cStringData,
                                  CHAR_DATA,
                                  dim,
                                  & (cSizeArray[0]))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while reading data: ' + al_status.message, al_status.code)

    npSizeArray = np.reshape(cSizeArray, (dim))

    # checking if an array is empty
    if (npSizeArray == 0).all():
        return al_status.code, None

    retArray = convertCBufferToStringArray(cStringData, npSizeArray)

    free(cStringData)

    return al_status.code, retArray


def al_read_data_string(ctx, fieldPath, pTimebasePath, dataType, dim):

    cdef char * cStringData
    cdef char * cTimebasePath
    cdef int cSize

    if not PyBytes_Check(fieldPath):
        fieldPath = fieldPath.encode('UTF-8')
    if not PyBytes_Check(pTimebasePath):
        pTimebasePath = pTimebasePath.encode('UTF-8')

    al_status = ll.al_read_data(ctx,
                                  fieldPath,
                                  pTimebasePath,
                                  < void**> & cStringData,
                                  CHAR_DATA,
                                  1,
                                  & (cSize))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while reading data: ' + al_status.message, al_status.code)

    if cSize == 0:
        return al_status.code, ''
    retString = cStringData[0:cSize].decode('UTF-8', errors='replace')

    free(cStringData)

    return al_status.code, retString


def al_read_data(ctx, fieldPath, pyTimebasePath, alDataType, dim):

    if dim > 0 and alDataType != CHAR_DATA:
        status, retData = al_read_data_array(
            ctx, fieldPath, pyTimebasePath, alDataType, dim)
    elif dim == 1 and alDataType == CHAR_DATA:
        status, retData = al_read_data_string(
            ctx, fieldPath, pyTimebasePath, alDataType, dim)
    elif dim > 1 and alDataType == CHAR_DATA:
        status, retData = al_read_data_array_string(
            ctx, fieldPath, pyTimebasePath, alDataType, dim)
    else:
        status, retData = al_read_data_scalar(
            ctx, fieldPath, pyTimebasePath, alDataType)

    return status, retData


###########################################################################################
"""
    Deletes data.
    This function deletes some data (can be a signal, a structure, the whole DATAOBJECT)
    in the database given the passed context.
    @param[in] ctx operation context id
               (from al_begin_global_action() or al_begin_slice_action())
    @param[in] path path of the data structure element to delete (suppress the whole subtree)
    @result error status
  int al_delete_data(int ctx,
                const char *path)
"""


def al_delete_data(ctx, path):

    al_status = ll.al_delete_data(ctx, path.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while reading data: ' + al_status.message, al_status.code)

    return al_status.code


###########################################################################################
"""
     Starts operations on a new array of structure.
     This function gives a new array of structure context id for the duration of an action
     on an array of structures, either from an operation context id (single or top-level array)
     or from an array of structure context id (nested array).
     @param[in] ctx operation context id (single case) or array of structure context id
     (nested case)
     @param[in] path path of array of structure (relative to ctx, or absolute if starts with "/")
     @param[in] timebase path of timebase associated with the array of structure
     @param[in,out] size specify the size of the struct_array (number of elements)
     @param[out] aosctx array of structure context id [_null context if = 0_]
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  al_status_t al_begin_arraystruct_action(int ctx,
                     const char *path,
                     const char *timebase,
                     int *size,
                     int *aosctx);
"""


def al_begin_arraystruct_action(ctx, path, pyTimebase, size):

    cdef int cSize = size
    cdef int aosOpCtx = -1

    al_status = ll.al_begin_arraystruct_action(ctx,
                                                 path.encode('UTF-8'),
                                                 pyTimebase.encode('UTF-8'),
                                                 & cSize,
                                                 & aosOpCtx)

    if al_status.code < 0:
        if exception.raise_error_flag:
            raise get_proper_exception_class(al_status.message, al_status.code)
        else:
            logging.error(al_status.message)
            return al_status.code, -1, 0

    return al_status.code, aosOpCtx, cSize

###########################################################################################


"""
     Change current index of interest in an array of structure.
     This function updates the index pointing at the current element
     of interest within an array of structure.
     @param[in] aosctx array of structure Context
     @param[in] step iteration step size (typically=1)
     @result error status

  int al_iterate_over_arraystruct(int aosctx,
                       int step)
"""


def al_iterate_over_arraystruct(aosctx, step):

    al_status = ll.al_iterate_over_arraystruct(aosctx, step)
    if al_status.code < 0:
        raise get_proper_exception_class('Error while reading data: ' + al_status.message, al_status.code)

    return al_status.code

###########################################################################################

def al_register_plugin(name):
    al_status = ll.al_register_plugin(name.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while registering plugin: ' + al_status.message, al_status.code)
    return al_status.code

def al_unregister_plugin(name):
    al_status = ll.al_unregister_plugin(name.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while unregistering plugin: ' + al_status.message, al_status.code)
    return al_status.code

def al_bind_plugin(path, name):
    al_status = ll.al_bind_plugin(path.encode('UTF-8'), name.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while binding plugin: ' + al_status.message, al_status.code)
    return al_status.code

def al_unbind_plugin(path, name):
    al_status = ll.al_unbind_plugin(path.encode('UTF-8'), name.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while unbinding plugin: ' + al_status.message, al_status.code)
    return al_status.code

def al_bind_readback_plugins(ctx):
    al_status = ll.al_bind_readback_plugins(ctx)
    if al_status.code < 0:
       raise get_proper_exception_class('Error while binding readback plugins: ' + al_status.message, al_status.code)
    return al_status.code

def al_unbind_readback_plugins(ctx):
    al_status = ll.al_unbind_readback_plugins(ctx)
    if al_status.code < 0:
        raise get_proper_exception_class('Error while unbinding readback plugins: ' + al_status.message, al_status.code)
    return al_status.code

def al_write_plugins_metadata(ctx):
    al_status = ll.al_write_plugins_metadata(ctx)
    if al_status.code < 0:
        raise get_proper_exception_class('Error while storing plugins metadata: ' + al_status.message, al_status.code)
    return al_status.code

def al_setvalue_parameter_plugin(parameter_name, inputData, pluginName):
    cdef int cDataType
    cdef int cDim
    cdef int cIntData
    cdef int cSize
    cdef double cDoubleData
    cdef double complex cComplexData
    cdef char * cStringData
    cdef np.ndarray npSizeArray
    cdef np.ndarray normalizedArray

    # Set data type
    cDataType = _getDataType(inputData)

    if cDataType == CHAR_DATA:
        py_byte_string = inputData.encode('UTF-8')
        cStringData = py_byte_string
        ll.al_setvalue_parameter_plugin(parameter_name.encode('UTF-8'), cDataType, 1, &cSize, <void*> cStringData, pluginName.encode('UTF-8'))
        return

    if np.isscalar(inputData):
        return al_setvalue_int_scalar_parameter_plugin(parameter_name, inputData, pluginName)

    npSizeArray = np.asarray((<object> inputData).shape, dtype=np.int32)  # noqa: E225
    # checking if an array is valid
    if np.amin(npSizeArray) < 0:
        raise ALException('Array dimension size cannot be < 0')

    cDim = npSizeArray.ndim

    cdef int * cSizeArray = <int*> npSizeArray.data
    #if np.amin(npSizeArray) == 0:
    #    return 0

    npDataType = inputData.dtype
    if np.issubdtype(npDataType, np.signedinteger):
        normalizedArray = inputData.astype(np.int32)
    elif np.issubdtype(npDataType, np.floating):
        normalizedArray = inputData.astype(np.float64)
    elif np.issubdtype(npDataType, np.complexfloating):
        normalizedArray = inputData.astype(np.complex128)
    else:
        raise ALException('UNKNOWN DATA TYPE :' + str(npDataType))

    if not normalizedArray.flags['F_CONTIGUOUS']:
        normalizedArray = np.asfortranarray(normalizedArray)

    cdef void * cData = <void*> normalizedArray.data

    al_status = ll.al_setvalue_parameter_plugin(parameter_name.encode('UTF-8'), cDataType, cDim, &(cSizeArray[0]), cData, pluginName.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while setting parameter value for plugin: ' + al_status.message, al_status.code)
    return al_status.code


def al_setvalue_int_scalar_parameter_plugin(parameter_name, parameter_value, pluginName):
    #cdef int[:] cSizeArray = cvarray(shape=(dim,), itemsize=sizeof(int), format="i")
    cdef int p = parameter_value
    al_status = ll.al_setvalue_int_scalar_parameter_plugin(parameter_name.encode('UTF-8'), p, pluginName.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while setting parameter value for plugin: ' + al_status.message, al_status.code)
    return al_status.code


def al_setvalue_double_scalar_parameter_plugin(parameter_name, parameter_value, pluginName):
    cdef double p = parameter_value
    al_status = ll.al_setvalue_double_scalar_parameter_plugin(parameter_name.encode('UTF-8'), p, pluginName.encode('UTF-8'))
    if al_status.code < 0:
        raise get_proper_exception_class('Error while setting parameter value for plugin: ' + al_status.message, al_status.code)
    return al_status.code

def al_get_occurrences(ctx, ids_name):

    cdef int * occurrences_list
    cdef int cSize
    cdef view.array arrayMemView

    al_status = ll.al_get_occurrences(ctx, ids_name.encode('UTF-8'), &occurrences_list, &cSize)

    if al_status.code < 0:
        raise get_proper_exception_class('Error while calling al_get_occurrences: ' + al_status.message, al_status.code)

    if cSize != 0:
        arrayMemView  = view.array(shape=(cSize,), itemsize = sizeof(int), format = "i",  mode="fortran", allocate_buffer=False)
        arrayMemView.data = <char*  > occurrences_list
        arrayMemView.callback_free_data = free
        return al_status.code, np.asarray(arrayMemView)
    else:
        return al_status.code, None


def get_al_version():
    version = ll.getALVersion()
    return version.decode('UTF-8')
