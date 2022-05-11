/**
   \file matlab_adpater.cpp
   R2 low-level API implementation with R3 low-level API.
   Implies near-direct compatibility with R2 high-level.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "matlab_adapter.h"


#define TRUE 1
#define FALSE 0

int defaultBackend() 
{
   int backend = MDSPLUS_BACKEND;
   char* backend_value;
   backend_value = getenv("IMAS_AL_DEFAULT_BACKEND");
   if (backend_value != NULL)
	   backend = atoi(backend_value);
   return backend;
}

int fallbackBackend() 
{
   int backend = NO_BACKEND;
   char* backend_value;
   backend_value = getenv("IMAS_AL_FALLBACK_BACKEND");
   if (backend_value != NULL)
	   backend = atoi(backend_value);
   return backend;
}

int mtl_ual_iterate_over_arraystruct(int aosctx, int step)
{
	al_status_t status = ual_iterate_over_arraystruct(aosctx, step);
	return status.code;
}

/**
   Reads an array of structure from a DATAOBJECT.
   This function reads a signal made of an array of structures from a DATAOBJECT.
   @param[in] opCtx operation context
   @param[in] fieldPath name of the field
   @param[in] timebasePath name of the timebase
   @param[out] size returned size of the array of structure
   @result array of structure context ID [__error if < 0__]

   @todo Low-level API modification:
   - returns array of structure context ID as result
   - does not return void pointer on object
   - returns size of the array of structure
   - does not
 */
int mtl_ual_begin_arraystruct_action(int opCtx, const char *fieldPath, const char *timebasePath, int *size)
{
  int actxID;
  al_status_t status = ual_begin_arraystruct_action(opCtx, fieldPath, timebasePath, size, &actxID);
  if (status.code==0)
    return actxID;
  else
    return status.code;
}

double *mtl_getVectNDDouble(int ctx, const char *fieldPath,
		const char *timebasePath, int *shapes, int dim, int *status)
{
	double *array=(double *)malloc(sizeof(double*));
	int retSize[MAXDIM];
	al_status_t status_t = ual_read_data(ctx, fieldPath, timebasePath, (void **)&array,
				DOUBLE_DATA, dim, &retSize[0]);
	*status = status_t.code;
	if (*status==0)
		{   int i;
			for (i = 0; i < dim; i++)
			shapes[i] = retSize[i];
		}
	return array;
}

int *mtl_getVectNDInt(int ctx, const char *fieldPath,
		const char *timebasePath, int *shapes, int dim, int *status)
{
	int *array=(int *)malloc(sizeof(int*));
	int retSize[MAXDIM];
	al_status_t status_t = ual_read_data(ctx, fieldPath, timebasePath, (void **)&array,
				INTEGER_DATA, dim, &retSize[0]);
	*status = status_t.code;
	if (*status==0)
		{   int i;
			for (i = 0; i < dim; i++)
			shapes[i] = retSize[i];
		}
	return array;
}

/**
   Reads a ND double complex array.
   This function reads a ND vector signal made of complex double from a DATAOBJECT.
   @param[in] ctx: context ID
   @param[in] fieldPath: field name
   @param[in] timebasePath: timebase path
   @param[in] ndim: number of array dimensions
   @param[out] status: low level error status
   @param[out] c_complex_struct: struct defined in the header containing real and imaginary data arrays
   */

void mtl_getCPX_ND(int ctx, const char *fieldPath, const char *timebasePath,
		int ndim, int *status, struct c_complex_struct **st)
{
	struct c_complex_struct* local=(struct c_complex_struct*) calloc(sizeof(struct c_complex_struct),1);
	*st = local;

	void* ptrData = NULL;
	int* shapes = (int *)malloc(MAXDIM*sizeof(int));
	al_status_t status_t = ual_read_data(ctx, fieldPath, timebasePath, &ptrData,
			COMPLEX_DATA, ndim, shapes);
	*status = status_t.code;
	std::complex<double> *array = (std::complex<double>*) ptrData;
	int i;
	int n = 1;
	for (i = 0; i < ndim; i++)
		n = shapes[i]*n;

	if (n == 0) {
		for (i = 0; i < ndim; i++)
			shapes[i]=0;
		local->shapes = shapes;
		return;
	}

	double *array_real_local = (double *)malloc(n*sizeof(double));
	double *array_imag_local = (double *)malloc(n*sizeof(double));

	local->array_real_part = array_real_local;
	local->array_imag_part = array_imag_local;

	for (i = 0; i < n; i++) {
		array_real_local[i] = std::real(array[i]);
		array_imag_local[i] = std::imag(array[i]);
	}
	local->shapes = shapes;
}

/**
   Reads a complex scalar.
   This function reads a complex scalar.
   @param[in] ctx: context ID
   @param[in] fieldPath: field name
   @param[in] timebasePath: timebase path
   @param[in] ndim: number of array dimensions
   @param[out] cpx_real: complex real part
   @param[out] cpx_imag: complex imaginary part
   @param[out] status: low level error status
   */

int mtl_getCPX_0D(int ctx, const char *fieldPath, const char *timebasePath, double *cpx_real, double *cpx_imag)
{
	int status;
	int retSize[MAXDIM];
	std::complex<double> *data = (std::complex<double> *)malloc(sizeof(std::complex<double>*));
	al_status_t status_t = ual_read_data(ctx, fieldPath, timebasePath, (void **)&data, COMPLEX_DATA, 0, &retSize[0]);
	status = status_t.code;
	*cpx_real = std::real(data[0]);
	*cpx_imag = std::imag(data[0]);
	return status;
}

/**
   Writes a complex double in an array of structure.
   This function writes a double field into an element of an array
   of structure.
   @param[in] fieldPath: field path
   @param[in] timebasePath: timebase path
   @param[in] data: array data
   @result error status
 */
int mtl_putCPX_0D(int ctx, const char *fieldPath, const char *timebasePath,
		double data_real, double data_imag)
{
	std::complex<double> data(data_real, data_imag);
	int size;
	al_status_t status_t = ual_write_data(ctx, fieldPath, timebasePath, (void *)(&data), COMPLEX_DATA, 0, &size);
	return status_t.code;
}

/**
   Writes a CPX_ND complex double array (N>0).
   This function writes a CPX_ND matrix signal made of complex double
   @param[in] ctx: context ID
   @param[in] fieldPath: field path
   @param[in] timebasePath: timebase path
   @param[in] data: array data
   @param[in] ndim: number of array dimensions
   @param[in] data: data contained in the array
   @param[in] shapes: size of each dimension (array)
   @result error status
 */
int mtl_putCPX_ND(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data_real, double *data_imag, int ndim, int *shapes)
{
	int i;
	int n = 1;
	for (i = 0; i < ndim; i++) {
		n = shapes[i]*n;
	}
	std::complex<double> *data = (std::complex<double> *)malloc(n*sizeof(std::complex<double>*));
	for (i = 0; i < n; i++) {
		data[i] = std::complex<double>(data_real[i], data_imag[i]);
	}
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			COMPLEX_DATA, ndim, shapes);
	return status_t.code;
}


/*Low level function prtotypes*/

/**
   Closes an entry in the MDSPlus database.
   This function closes an opened pulse file in the MDSPlus database.
   @param[in] pulseCtx pulse context ID
   @result error status
 */
int mtl_ual_close(int pulseCtx)
{
	al_status_t status_t = ual_close_pulse(pulseCtx, CLOSE_PULSE, "");
	return status_t.code;
}


/**
   Creates an entry in the MDSPlus database.
   This function creates (overwrite existing entry) and opens a new pulse 
   file in the MDSPlus database identified by the given arguments.
   @param[in] name identifier for the database [__deprecated???__]
   @param[in] shot shot number (entry in the database)
   @param[in] run run number (entry in the database)
   @param[in] refShot reference shot number [__deprecated???__]
   @param[in] refRun reference run number [__deprecated???__]
   @param[out] pulseCtx returned pulse context ID
   @param[in] user database username 
   @param[in] tokamak database tokamak name 
   @param[in] version database data version    
   @result error status
 */
int mtl_ual_create_env(const char *name, int shot, int run, int refShot, 
		int refRun, int *pulseCtx, char *user, char *tokamak,
		char *version)
{
	al_status_t status_t = ual_begin_pulse_action(defaultBackend(), shot, run,
			user, tokamak, version, pulseCtx);

	if (*pulseCtx < 0)
		return *pulseCtx;
	else {
		status_t = ual_open_pulse(*pulseCtx, FORCE_CREATE_PULSE, "");
	    return status_t.code;
	}

}

/**
   Creates an entry in the UDA backend.
   @param[in] shot shot number (entry in the database)
   @param[in] run run number (entry in the database)
   @param[in] refShot reference shot number [__deprecated???__]
   @param[in] refRun reference run number [__deprecated???__]
   @param[out] pulseCtx returned pulse context ID
   @param[in] user database username 
   @param[in] tokamak database tokamak name 
   @param[in] version database data version    
   @result error status
 */
int mtl_ual_create_public(int shot, int run, int *pulseCtx, char *user, char *tokamak, 
		char *version)
{
	al_status_t status_t = ual_begin_pulse_action(UDA_BACKEND, shot, run,
			user, tokamak, version, pulseCtx);

	if (*pulseCtx < 0)
		return *pulseCtx;
	else {
		status_t = ual_open_pulse(*pulseCtx, FORCE_CREATE_PULSE, "");
		return status_t.code;
	}
}

/**
   Opens an entry in the MDSPlus database.
   This function opens an existing pulse file (fail if does not exist)
   in the MDSPlus database identified by the given arguments.
   @param[in] name identifier for the database [__deprecated???__]
   @param[in] shot shot number (entry in the database)
   @param[in] run run number (entry in the database)
   @param[in] user database username 
   @param[in] tokamak database tokamak name 
   @param[in] version database data version 
   @result error status
 */
int mtl_ual_open_env(const char *name, int shot, int run, int *pulseCtx,
		char *user, char *tokamak, char *version)
{
        int default_backend = defaultBackend();
	al_status_t status_t = ual_begin_pulse_action(default_backend, shot, run,
			user, tokamak, version, pulseCtx);
	if (*pulseCtx < 0) 
		return *pulseCtx;
	else {
		status_t = ual_open_pulse(*pulseCtx, OPEN_PULSE, "");

		if (status_t.code != 0) {
		  int fallback_backend = fallbackBackend();
		  if (fallback_backend != NO_BACKEND) {
		    printf("WARNING: the pulse file is not available with the backend %d, now attempting to access it with the fallback backend %d\n",default_backend,fallback_backend);
		    status_t = ual_begin_pulse_action(fallback_backend, shot, run,
						      user, tokamak, version, pulseCtx);
		    if (*pulseCtx < 0) 
		      return *pulseCtx;
		    else {
		      status_t = ual_open_pulse(*pulseCtx, OPEN_PULSE, "");
		      return status_t.code;
		    }
		  }
		}
		return status_t.code;
	}
}

/**
   Opens an entry in the UDA backend.
   @param[in] shot shot number (entry in the database)
   @param[in] run run number (entry in the database)
   @param[out] pulseCtx returned pulse context ID
   @param[in] user database username 
   @param[in] tokamak database tokamak name 
   @param[in] version database data version 
   @result error status
 */
int mtl_ual_open_public(int shot, int run, int *pulseCtx, 
		char *user, char *tokamak, char *version)
{
	al_status_t status_t = ual_begin_pulse_action(UDA_BACKEND, shot, run,
			user, tokamak, version, pulseCtx);

	if (*pulseCtx < 0)
		return *pulseCtx;
	else {
			status_t = ual_open_pulse(*pulseCtx, OPEN_PULSE, "");
			return status_t.code;
		}
}

/**
   Starts a read action on a DATAOBJECT.
   This function marks the beginning of a serie of read operations on all 
   fields of a DATAOBJECT.
   @param[in] pulseCtx pulse context ID
   @param[in] path name of the DATAOBJECT
   @result operation context ID [_errror if < 0_]
 */
int mtl_ual_begin_global_action(int pulseCtx, const char *path) 
{
	int octxID;
	al_status_t status = ual_begin_global_action(pulseCtx, path, READ_OP, &octxID);
	if (status.code==0)
	    return octxID;
	 else
	    return status.code;

}

/**
   Terminates a read action on a DATAOBJECT.
   This function marks the end of a get operation initiated by a call to 
   mtl_ual_begin_global_action().
   @param[in] opCtx operation context ID (from mtl_ual_begin_global_action())
   @result error status
 */
int mtl_ual_end_action(int opCtx)
{
	al_status_t status_t = ual_end_action(opCtx);
	return status_t.code;
}

/**
   Starts a read action on a DATAOBJECT slice.
   This function marks the beginning of a serie of read operations on all fields
   (both timed and non-timed) of a DATAOBJECT.
   @param[in] pulseCtx pulse context ID
   @param[in] path name of the DATAOBJECT
   @param[in] time time of the slice
   @param[in] interpMode mode for interpolation:
   - CLOSEST_INTERP take the slice at the closest time
   - PREVIOUS_INTERP take the slice at the previous time
   - LINEAR_INTERP interpolate the slice between the values of the previous 
   and next slice
   @result operation context ID [_error if < 0_]

   @note shouldn't we return value of returned time here instead of subsequent 
   getFieldSlice calls? Is this information of any use at all? the time field
   will contain this info anyway no?

   @todo Low-level API modification:
   - additional parameter interpMode 

   [ex_ual_begin_slice_action]*/
int mtl_ual_begin_slice_action(int pulseCtx, const char *path, double time, 
		int interpMode)
{
	int octxID;
	al_status_t status = ual_begin_slice_action(pulseCtx, path, READ_OP, time,
			interpMode, &octxID);
	if (status.code==0)
		 return octxID;
	else
		 return status.code;
	return octxID;
}
/*[ex_ual_begin_slice_action]*/

/*
  OH: F90 interface, non timed DATAOBJECT are using beginDataobjectPutNonTimed, C++ is using 
  beginDataobjectPut => need to clarify use-cases in order to avoid possible bugs and 
  different behavior with different interfaces
 */

/**
   Starts a write action on the non-timed part of a DATAOBJECT.
   This function marks the beginning of a serie of write operations on all 
   non-timed fields of a DATAOBJECT.
   @param[in] pulseCtx pulse context ID
   @param[in] path name of the DATAOBJECT
   @result operation context [_error if < 0_]
 */
int mtl_ual_begin_global_action_write(int pulseCtx, const char *path)
{
	int octxID;
	al_status_t status = ual_begin_global_action(pulseCtx, path, WRITE_OP, &octxID);
	if (status.code==0)
		return octxID;
	else
		return status.code;
	return octxID;
}

/**
   Starts a write action on a DATAOBJECT slice.
   This function marks the beginning of a serie write operations on all timed 
   fields of a DATAOBJECT slice.
   @param[in] pulseCtx pulse context ID
   @param[in] path name of the DATAOBJECT
   @param[in] time time of the slice to write
   @result operation context ID [_error if < 0_]
 */
int mtl_ual_begin_put_slice_action(int pulseCtx, const char *path, double time)
{
	int octxID;
	al_status_t status = ual_begin_slice_action(pulseCtx, path, WRITE_OP, UNDEFINED_TIME,
			UNDEFINED_INTERP, &octxID);
	if (status.code==0)
		return octxID;
	else
		return status.code;
	return octxID;
}


/* management 
 ********************************************************************************/

/**
   Deletes data.
   This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) 
   in the database given the passed context.
   @param[in] ctx operation context ID 
   @param[in] fieldPath path of the data structure element to delete (suppress the 
   whole subtree)
   @result error status
 */
int mtl_deleteData(int ctx, const char *fieldPath)
{
	al_status_t status_t = ual_delete_data(ctx, fieldPath);
	return status_t.code;
}


/* readers
 ********************************************************************************/

/**
   Reads a 1D character array.
   This function reads a 1D vector signal made of characters from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int mtl_getVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, 
		char **data, int *dim)
{
	int status;
	int retSize[MAXDIM];
	char* szTemp = NULL;
	//  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data,
	al_status_t status_t = ual_read_data(opCtx, fieldPath, timebasePath, (void **)&szTemp,
			CHAR_DATA, 1, &retSize[0]);
	status = status_t.code;
	if (status==0)
	{
		*dim = retSize[0];
		*data = (char *)malloc(*dim + 1);
		memset(*data, 0, *dim + 1);
		strncpy(*data, szTemp, *dim);
		free (szTemp);
	}
	return status;
}


/**
   Reads a 2D character array.
   This function reads a 2D vector signal made of characters from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int mtl_getVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, 
		char **data, int *dim1, int *dim2)
{
	int status;
	int retSize[MAXDIM];
	char* szTemp = NULL;
	al_status_t status_t = ual_read_data(opCtx, fieldPath, timebasePath, (void **)&szTemp, CHAR_DATA, 2, &retSize[0]);
	status = status_t.code;
	if (status==0)
	{
		*dim1 = retSize[0];
		*dim2 = retSize[1];
		*data = (char *)malloc((*dim1)*(*dim2) + 1);
		memset(*data, 0, (*dim1)*(*dim2) + 1);
		strncpy(*data, szTemp, (*dim1)*(*dim2));
		free (szTemp);
	}
	return status;
}

/* readers
 ********************************************************************************/

/**
   Reads an integer.
   This function reads a scalar signal made of an integer from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value
   @result error status
 */
int mtl_getInt(int opCtx, const char *fieldPath, const char *timebasePath, int *data)
{
	int retSize[MAXDIM];
	al_status_t status_t = ual_read_data(opCtx, fieldPath, timebasePath, (void **)&data,
			INTEGER_DATA, 0, &retSize[0]);
	return status_t.code;
}

/**
   Reads a double.
   This function reads a scalar signal made of a double from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value
   @result error status
 */
int mtl_getDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data)
{
	int retSize[MAXDIM];
	al_status_t status_t = ual_read_data(opCtx, fieldPath, timebasePath, (void**)&data,
			DOUBLE_DATA, 0, &retSize[0]);
	return status_t.code;
}


/* writers 
 ********************************************************************************/

/**
   Writes an integer.
   This function writes a scalar signal made of integer into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
 */
int mtl_putInt(int opCtx, const char *fieldPath, const char *timebasePath, int data)
{
	int size[1] = {1};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)(&data),
			INTEGER_DATA, 0, size);
	return status_t.code;
}

/**
   Writes a double.
   This function writes a scalar signal made of double into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
 */
int mtl_putDouble(int opCtx, const char *fieldPath, const char *timebasePath, double data)
{
	int size[1] = {1};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)(&data),
			DOUBLE_DATA, 0, size);
	return status_t.code;
}


/**
   Writes a 1D char array.
   This function writes a 1D vector signal made of characters into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath,
		char *data, int dim)
{
	int size[1] = {dim};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			CHAR_DATA, 1, size);
	return status_t.code;
}

/**
   Writes a 1D int array.
   This function writes a 1D vector signal made of integers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim)
{
	int size[1] = {dim};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			INTEGER_DATA, 1, size);
	return status_t.code;
}

/**
   Writes a 1D double array.
   This function writes a 1D vector signal made of doubles into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data, int dim)
{
	int size[1] = {dim};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			DOUBLE_DATA, 1, size);
	return status_t.code;
}

/**
   Writes a 2D char array.
   This function writes a 2D vector signal made of characters into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath,
		char *data, int dim1, int dim2)
{
	int size[2] = {dim1, dim2};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			CHAR_DATA, 2, size);
	return status_t.code;
}

/**
   Writes a 2D int array.
   This function writes a 2D vector signal made of integers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim1, int dim2)
{
	int size[2] = {dim1, dim2};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			INTEGER_DATA, 2, size);
	return status_t.code;
}

/**
   Writes a 2D double array.
   This function writes a 2D vector signal made of doubles into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data, int dim1, int dim2)
{
	int size[2] = {dim1, dim2};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			DOUBLE_DATA, 2, size);
	return status_t.code;
}

/**
   Writes a 3D integer array.
   This function writes a 3D vector signal made of integers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim1, int dim2, int dim3)
{
	int size[3] = {dim1, dim2, dim3};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			INTEGER_DATA, 3, size);
	return status_t.code;
}

/**
   Writes a 3D double array.
   This function writes a 3D vector signal made of doubles into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data, int dim1, int dim2, int dim3)
{
	int size[3] = {dim1, dim2, dim3};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			DOUBLE_DATA, 3, size);
	return status_t.code;
}

/**
   Writes a 4D integer array.
   This function writes a 4D vector signal made of integers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim1, int dim2, int dim3, int dim4)
{
	int size[4] = {dim1, dim2, dim3, dim4};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			INTEGER_DATA, 4, size);
	return status_t.code;
}

/**
   Writes a 4D double array.
   This function writes a 4D vector signal made of doubles into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data, int dim1, int dim2, int dim3, int dim4)
{
	int size[4] = {dim1, dim2, dim3, dim4};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			DOUBLE_DATA, 4, size);
	return status_t.code;
}

/**
   Writes a 5D integer array.
   This function writes a 5D vector signal made of integers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
	int size[5] = {dim1, dim2, dim3, dim4, dim5};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			INTEGER_DATA, 5, size);
	return status_t.code;
}

/**
   Writes a 5D double array.
   This function writes a 5D vector signal made of doubles into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data, int dim1, int dim2, int dim3, int dim4,
		int dim5)
{
	int size[5] = {dim1, dim2, dim3, dim4, dim5};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			DOUBLE_DATA, 5, size);
	return status_t.code;
}

/**
   Writes a 6D integer array.
   This function writes a 6D vector signal made of integers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @param[in] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim1, int dim2, int dim3, int dim4, int dim5,
		int dim6)
{
	int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			INTEGER_DATA, 6, size);
	return status_t.code;
}

/**
   Writes a 6D double array.
   This function writes a 6D vector signal made of doubles into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @param[in] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data, int dim1, int dim2, int dim3, int dim4,
		int dim5, int dim6)
{
	int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			DOUBLE_DATA, 6, size);
	return status_t.code;
}

/**
   Writes a 7D integer array.
   This function writes a 7D vector signal made of integers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @param[in] dim6 size of sixth dimension
   @param[in] dim7 size of seventh dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim1, int dim2, int dim3, int dim4, int dim5,
		int dim6, int dim7)
{
	int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			INTEGER_DATA, 7, size);
	return status_t.code;
}

/**
   Writes a 7D double array.
   This function writes a 7D vector signal made of doubles into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @param[in] dim6 size of sixth dimension
   @param[in] dim7 size of seventh dimension
   @result error status

   @todo Low-level API modification:
   - additional timebasePath argument
   - remove isTimed argument
 */
int mtl_putVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		double *data, int dim1, int dim2, int dim3, int dim4,
		int dim5, int dim6, int dim7)
{
	int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
	al_status_t status_t = ual_write_data(opCtx, fieldPath, timebasePath, (void *)data,
			DOUBLE_DATA, 7, size);
	return status_t.code;
}

/* array of structure element writers
 *******************************************************************************/


/**
   Writes an integer in an array of structure.
   This function writes an integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */

int mtl_putIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, 
		int data)
{  
	int size[1] = {1};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,(void *)(&data), INTEGER_DATA, 0, size);
	return status_t.code;
}

/**
   Writes a double in an array of structure.
   This function writes a double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath,
		double data)
{
	int size[1] = {1};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath, (void *)(&data), DOUBLE_DATA, 0, size);
	return status_t.code;
}


/**
   Writes a 1D character array in an array of structure.
   This function writes a 1D character field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int mtl_putVect1DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath,
		char *data, int dim)
{
	int size[1] = {dim};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath, (void *)data, CHAR_DATA, 1, size);
	return status_t.code;
}

/**
   Writes a 1D integer array in an array of structure.
   This function writes a 1D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect1DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath,
		int *data, int dim)
{
	int size[1] = {dim};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, INTEGER_DATA, 1, size);
	return status_t.code;
}

/**
   Writes a 1D double array in an array of structure.
   This function writes a 1D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect1DDoubleInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		double *data, int dim)
{
	int size[1] = {dim};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, DOUBLE_DATA, 1, size);
	return status_t.code;
}


/**
   Writes a 2D character array in an array of structure.
   This function writes a 2D character field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int mtl_putVect2DCharInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		char *data, int dim1, int dim2)
{
	int size[2] = {dim1, dim2};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, CHAR_DATA, 2, size);
	return status_t.code;
}

/**
   Writes a 2D integer array in an array of structure.
   This function writes a 2D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect2DIntInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		int *data, int dim1, int dim2)
{
	int size[2] = {dim1, dim2};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, INTEGER_DATA, 2, size);
	return status_t.code;
}

/**
   Writes a 2D double array in an array of structure.
   This function writes a 2D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)

   [ex_ual_mtl_put_in_arraystruct]*/
int mtl_putVect2DDoubleInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		double *data, int dim1, int dim2)
{
	int size[2] = {dim1, dim2};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, DOUBLE_DATA, 2, size);
	return status_t.code;
}

/**
   Writes a 3D integer array in an array of structure.
   This function writes a 3D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect3DIntInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		int *data, int dim1, int dim2, int dim3)
{
	int size[3] = {dim1, dim2, dim3};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, INTEGER_DATA, 3, size);
	return status_t.code;
}

/**
   Writes a 3D double array in an array of structure.
   This function writes a 3D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect3DDoubleInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		double *data, int dim1, int dim2, int dim3)
{
	int size[3] = {dim1, dim2, dim3};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, DOUBLE_DATA, 3, size);
	return status_t.code;
}

/**
   Writes a 4D integer array in an array of structure.
   This function writes a 4D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect4DIntInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		int *data, int dim1, int dim2, int dim3, int dim4)
{
	int size[4] = {dim1, dim2, dim3, dim4};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, INTEGER_DATA, 4, size);
	return status_t.code;
}

/**
   Writes a 4D double array in an array of structure.
   This function writes a 4D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect4DDoubleInObject(int aosCtx, const char *fieldPath, 
		const char* timebasePath,
		double *data, int dim1, int dim2, int dim3, int dim4)
{
	int size[4] = {dim1, dim2, dim3, dim4};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, DOUBLE_DATA, 4, size);
	return status_t.code;
}


/**
   Writes a 5D integer array in an array of structure.
   This function writes a 5D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect5DIntInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		int *data, int dim1, int dim2, int dim3, int dim4,
		int dim5)
{
	int size[5] = {dim1, dim2, dim3, dim4, dim5};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, INTEGER_DATA, 5, size);
	return status_t.code;
}

/**
   Writes a 5D double array in an array of structure.
   This function writes a 5D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect5DDoubleInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		double *data, int dim1, int dim2, int dim3,
		int dim4, int dim5)
{
	int size[5] = {dim1, dim2, dim3, dim4, dim5};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, DOUBLE_DATA, 5, size);
	return status_t.code;
}

/**
   Writes a 6D integer array in an array of structure.
   This function writes a 6D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @param[in] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect6DIntInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		int *data, int dim1, int dim2, int dim3, int dim4,
		int dim5, int dim6)
{
	int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
	al_status_t status_t = ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, INTEGER_DATA, 6, size);
	return status_t.code;
}

/**
   Writes a 6D double array in an array of structure.
   This function writes a 6D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @param[in] dim4 size of fourth dimension
   @param[in] dim5 size of fifth dimension
   @param[in] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int mtl_putVect6DDoubleInObject(int aosCtx, const char *fieldPath, 
		const char *timebasePath,
		double *data, int dim1, int dim2, int dim3,
		int dim4, int dim5, int dim6)
{
	int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
	al_status_t status_t =  ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, DOUBLE_DATA, 6, size);
	return status_t.code;
}
