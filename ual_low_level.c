/**
   \file ual_low_level.c
   R2 low-level API implementation with R3 low-level API.
   Implies near-direct compatibility with R2 high-level.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "ual_low_level.h"
#include "ual_lowlevel.h"

#define TRUE 1
#define FALSE 0


//const Complex EMPTY_COMPLEX = EMPTY_DOUBLE + EMPTY_DOUBLE*I;


void check_status(int status, const char *file, const int line)
{
  if (status < 0)
    { 
      fprintf(stderr,"Failed status in %s:%d = %s\n",file, line, err2str(status));
      exit(EXIT_FAILURE);
    }
}

/*Low level function prototypes*/

/**
   Creates an entry in the MDSPlus database.
   This function creates (overwrite existing entry) and opens a new pulse 
   file in the MDSPlus database identified by environment variables.
   @param[in] name identifier for the database [__deprecated???__]
   @param[in] shot shot number (entry in the database)
   @param[in] run run number (entry in the database)
   @param[in] refShot reference shot number [__deprecated???__]
   @param[in] refRun reference run number [__deprecated???__]
   @param[out] pulseCtx returned pulse context ID
   @result error status

   @note what about name? is it really mandatory? same for refShot, refRun? 
   - we can either pass a string filled with additional parameters for the 
   backend
   - and/or define additional function in lowlevel C++ API
 */
int ual_create(const char *name, int shot, int run, int refShot, int refRun, 
	       int *pulseCtx)
{
  /* name, refShot, refRun not considered */
  *pulseCtx = ual_begin_pulse_action(MDSPLUS_BACKEND, shot, run, 
				      "", "", ""); 

  if (*pulseCtx < 0)
    return *pulseCtx;
  else
    return ual_open_pulse(*pulseCtx, FORCE_CREATE_PULSE, "");
}

/**
   Opens an entry in the MDSPlus database.
   This function opens an existing pulse file (fail if does not exist)
   in the MDSPlus database identified by environment variables.
   @param[in] name identifier for the database [__deprecated???__]
   @param[in] shot shot number (entry in the database)
   @param[in] run run number (entry in the database)
   @param[out] pulseCtx returned pulse context ID
   @result error status
 */
int ual_open(const char *name, int shot, int run, int *pulseCtx)
{
  /* name not considered */
  *pulseCtx = ual_begin_pulse_action(MDSPLUS_BACKEND, shot, run, 
				      "", "", ""); 

  if (*pulseCtx < 0)
    return *pulseCtx;
  else
    return ual_open_pulse(*pulseCtx, OPEN_PULSE, "");
}

/**
   Closes an entry in the MDSPlus database.
   This function closes an opened pulse file in the MDSPlus database.
   @param[in] pulseCtx pulse context ID
   @result error status
 */
int ual_close(int pulseCtx)
{
  int status = ual_close_pulse(pulseCtx, CLOSE_PULSE, "");
  if (status < 0)
    return status;
  else
    return ual_end_action(pulseCtx);
}


int ual_create_hdf5(const char *name, int shot, int run, int refShot, int refRun, int *retIdx);
int ual_open_hdf5(const char *name, int shot, int run, int *retIdx);

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
int ual_create_env(const char *name, int shot, int run, int refShot, 
		   int refRun, int *pulseCtx, char *user, char *tokamak, 
		   char *version)
{
  *pulseCtx = ual_begin_pulse_action(MDSPLUS_BACKEND, shot, run, 
				     user, tokamak, version); 

  if (*pulseCtx < 0)
    return *pulseCtx;
  else
    return ual_open_pulse(*pulseCtx, FORCE_CREATE_PULSE, "");
}

/**
   Opens an entry in the MDSPlus database.
   This function opens an existing pulse file (fail if does not exist)
   in the MDSPlus database identified by the given arguments.
   @param[in] name identifier for the database [__deprecated???__]
   @param[in] shot shot number (entry in the database)
   @param[in] run run number (entry in the database)
   @param[out] pulseCtx returned pulse context ID
   @param[in] user database username 
   @param[in] tokamak database tokamak name 
   @param[in] version database data version 
   @result error status
 */
int ual_open_env(const char *name, int shot, int run, int *pulseCtx, 
		 char *user, char *tokamak, char *version)
{
  *pulseCtx = ual_begin_pulse_action(MDSPLUS_BACKEND, shot, run, 
				     user, tokamak, version); 

  if (*pulseCtx < 0)
    return *pulseCtx;
  else
    return ual_open_pulse(*pulseCtx, OPEN_PULSE, "");
}

/**
   Starts a read action on a DATAOBJECT.
   This function marks the beginning of a serie of read operations on all 
   fields of a DATAOBJECT.
   @param[in] pulseCtx pulse context ID
   @param[in] path name of the DATAOBJECT
   @result operation context ID [_errror if < 0_]
 */
int beginDataobjectGet(int pulseCtx, const char *path) 
{
  return ual_begin_global_action(pulseCtx, path, READ_OP);
}

/**
   Terminates a read action on a DATAOBJECT.
   This function marks the end of a get operation initiated by a call to 
   beginDataobjectGet().
   @param[in] opCtx operation context ID (from beginDataobjectGet())
   @result error status
 */
int endDataobjectGet(int opCtx)
{
  return ual_end_action(opCtx);
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
int beginDataobjectGetSlice(int pulseCtx, const char *path, double time, 
			    int interpMode) 
{
  return ual_begin_slice_action(pulseCtx, path, READ_OP, time, 
				interpMode); 
}
/*[ex_ual_begin_slice_action]*/

/**
   Terminates a read action on a DATAOBJECT slice.
   This function marks the end of a get operation initiated by a call to 
   beginDataobjectGetSlice().
   @param[in] opCtx operation context ID (from beginDataobjectGetSlice())
   @result error status
   
   [ex_ual_end_action]*/
int endDataobjectGetSlice(int opCtx)
{
  return ual_end_action(opCtx);
}
/*[ex_ual_end_action]*/

/*
  OH: F90 interface, non timed DATAOBJECT are using beginDataobjectPutNonTimed, C++ is using 
  beginDataobjectPut => need to clarify use-cases in order to avoid possible bugs and 
  different behavior with different interfaces
*/

/**
   Starts a write action on the timed part of a DATAOBJECT.
   This function marks the beginning of a serie of write operations on all 
   timed fields of a DATAOBJECT.
   @param[in] pulseCtx pulse context ID
   @param[in] path name of the DATAOBJECT
   @result operation context [_error if < 0_]

   [ex_ual_begin_global_action]*/
int beginDataobjectPutTimed(int pulseCtx, const char *path)
{
  return ual_begin_global_action(pulseCtx, path, WRITE_OP);
}
/*[ex_ual_begin_global_action]*/

/**
   Terminates a write action on the timed part of a DATAOBJECT.
   This function marks the end of a put operation initiated by a call to 
   beginDataobjectPutTimed().
   @param[in] opCtx operation context ID (from beginDataobjectPutTimed())
   @result error status
 */
int endDataobjectPutTimed(int opCtx)
{
  return ual_end_action(opCtx);
}

/**
   Starts a write action on the non-timed part of a DATAOBJECT.
   This function marks the beginning of a serie of write operations on all 
   non-timed fields of a DATAOBJECT.
   @param[in] pulseCtx pulse context ID
   @param[in] path name of the DATAOBJECT
   @result operation context [_error if < 0_]
 */
int beginDataobjectPutNonTimed(int pulseCtx, const char *path)
{
  return ual_begin_global_action(pulseCtx, path, WRITE_OP);
}

/**
   Terminates a write action on the non-timed part of a DATAOBJECT.
   This function marks the end of a put operation initiated by a call to 
   beginDataobjectPutNonTimed().
   @param[in] opCtx operation context ID (from beginDataobjectPutNonTimed())
   @result error status
 */
int endDataobjectPutNonTimed(int opCtx)
{
  return ual_end_action(opCtx);
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
int beginDataobjectPutSlice(int pulseCtx, const char *path, double time)
{
  return ual_begin_slice_action(pulseCtx, path, WRITE_OP, time, 
				UNDEFINED_INTERP);
}

/**
   Terminates a write action on a DATAOBJECT slice.
   This function marks the end of a put operation initiated by a call to 
   beginDataobjectPutSlice().
   @param[in] opCtx operation context ID (from beginDataobjectPutSlice())
   @result error status
 */
int endDataobjectPutSlice(int opCtx)
{
  return ual_end_action(opCtx);
}

/* allow future extension where another slice than the last one is replaced */



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
int deleteData(int ctx, const char *fieldPath)
{
  return ual_delete_data(ctx, fieldPath);
}


/**
   Flush cached data.
   @todo TDB
 */
int ual_flush_dataobject_mem_cache(int ctx, const char *path) 
{
  return 0;
}

/**
   Discard cached data.
   @todo TDB
 */
int ual_discard_dataobject_mem_cache(int ctx, const char *path) 
{
  return 0;
}



/* writers 
********************************************************************************/

/**
   Writes a character.
   This function writes a scalar signal made of character into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
*/
int putChar(int opCtx, const char *fieldPath, const char *timebasePath, char data)
{
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)(&data), 
			CHAR_DATA, 0, NULL);
}

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
int putInt(int opCtx, const char *fieldPath, const char *timebasePath, int data)
{
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)(&data), 
			INTEGER_DATA, 0, NULL);
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
int putDouble(int opCtx, const char *fieldPath, const char *timebasePath, double data)
{
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)(&data), 
			DOUBLE_DATA, 0, NULL);
}

/**
   Writes a complex number.
   This function writes a scalar signal made of complex number into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @result error status

   @todo Low-level API modification: 
   - additional timebasePath argument
*/
int putComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
	       Complex data)
{
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)(&data), 
			COMPLEX_DATA, 0, NULL);
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
int putVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath,
		  char *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			CHAR_DATA, 1, size);
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
int putVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		 int *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			INTEGER_DATA, 1, size);
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
int putVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		    double *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			DOUBLE_DATA, 1, size);
}

/**
   Writes a 1D complex number array.
   This function writes a 1D vector signal made of complex numbers into a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath field name for the timebase
   @param[in] data field value 
   @param[in] dim size of first dimension
   @result error status
   @note draft 

   @todo Low-level API modification: 
   - additional timebasePath argument
   - remove isTimed argument
*/
int putVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath,
		     Complex *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			COMPLEX_DATA, 1, size);
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
int putVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath,
		  char *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			CHAR_DATA, 2, size);
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
int putVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		 int *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			INTEGER_DATA, 2, size);
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
int putVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		    double *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			DOUBLE_DATA, 2, size);
}

/**
   Writes a 2D complex number array.
   This function writes a 2D vector signal made of complex numbers into a DATAOBJECT.
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
int putVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath,
		     Complex *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			COMPLEX_DATA, 2, size);
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
int putVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		 int *data, int dim1, int dim2, int dim3)
{
  int size[3] = {dim1, dim2, dim3};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			INTEGER_DATA, 3, size);
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
int putVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		    double *data, int dim1, int dim2, int dim3)
{
  int size[3] = {dim1, dim2, dim3};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			DOUBLE_DATA, 3, size);
}

/**
   Writes a 3D complex number array.
   This function writes a 3D vector signal made of complex numbers into a DATAOBJECT.
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
int putVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath,
		     Complex *data, int dim1, int dim2, int dim3)
{
  int size[3] = {dim1, dim2, dim3};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			COMPLEX_DATA, 3, size);
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
int putVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		 int *data, int dim1, int dim2, int dim3, int dim4)
{
  int size[4] = {dim1, dim2, dim3, dim4};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			INTEGER_DATA, 4, size);
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
int putVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		    double *data, int dim1, int dim2, int dim3, int dim4)
{
  int size[4] = {dim1, dim2, dim3, dim4};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			DOUBLE_DATA, 4, size);
}

/**
   Writes a 4D complex number array.
   This function writes a 4D vector signal made of complex numbers into a DATAOBJECT.
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
int putVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath,
		     Complex *data, int dim1, int dim2, int dim3, int dim4)
{
  int size[4] = {dim1, dim2, dim3, dim4};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			COMPLEX_DATA, 4, size);
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
int putVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		 int *data, int dim1, int dim2, int dim3, int dim4, int dim5)
{
  int size[5] = {dim1, dim2, dim3, dim4, dim5};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			INTEGER_DATA, 5, size);
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
int putVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		    double *data, int dim1, int dim2, int dim3, int dim4, 
		    int dim5)
{
  int size[5] = {dim1, dim2, dim3, dim4, dim5};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			DOUBLE_DATA, 5, size);
}

/**
   Writes a 5D complex number array.
   This function writes a 5D vector signal made of complex numbers into a DATAOBJECT.
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
int putVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath,
		     Complex *data, int dim1, int dim2, int dim3, int dim4, 
		     int dim5)
{
  int size[5] = {dim1, dim2, dim3, dim4, dim5};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			COMPLEX_DATA, 5, size);
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
int putVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		 int *data, int dim1, int dim2, int dim3, int dim4, int dim5, 
		 int dim6)
{
  int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			INTEGER_DATA, 6, size);
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
int putVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		    double *data, int dim1, int dim2, int dim3, int dim4, 
		    int dim5, int dim6)
{
  int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			DOUBLE_DATA, 6, size);
}

/**
   Writes a 6D complex number array.
   This function writes a 6D vector signal made of complex numbers into a DATAOBJECT.
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
int putVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath,
		     Complex *data, int dim1, int dim2, int dim3, int dim4, 
		     int dim5, int dim6)
{
  int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			COMPLEX_DATA, 6, size);
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
int putVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath,
		 int *data, int dim1, int dim2, int dim3, int dim4, int dim5, 
		 int dim6, int dim7)
{
  int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			INTEGER_DATA, 7, size);
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
int putVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath,
		    double *data, int dim1, int dim2, int dim3, int dim4, 
		    int dim5, int dim6, int dim7)
{
  int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			DOUBLE_DATA, 7, size);
}

/**
   Writes a 7D complex number array.
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
int putVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath,
		     Complex *data, int dim1, int dim2, int dim3, int dim4, 
		     int dim5, int dim6, int dim7)
{
  int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
  return ual_write_data(opCtx, fieldPath, timebasePath, (void *)data, 
			COMPLEX_DATA, 7, size);
}



/* readers
********************************************************************************/


/**
   Reads a character.
   This function reads a scalar signal made of a character from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @result error status

   @todo Low-level API modification:
   - new function
 */
int getChar(int opCtx, const char *fieldPath, const char *timebasePath,
	    char *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void**)&data, 
			 CHAR_DATA, 0, &retSize[0]);
  return status;
}

/**
   Reads an integer.
   This function reads a scalar signal made of an integer from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @result error status
 */
int getInt(int opCtx, const char *fieldPath, const char *timebasePath,
	   int *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void**)&data, 
			 INTEGER_DATA, 0, &retSize[0]);
  return status;
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
int getDouble(int opCtx, const char *fieldPath, const char *timebasePath,
	      double *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void**)&data, 
			 DOUBLE_DATA, 0, &retSize[0]);
  return status;
}

/**
   Reads a complex number.
   This function reads a scalar signal made of a complex number from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @result error status
 */
int getComplex(int opCtx, const char *fieldPath, const char *timebasePath,
	       Complex *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void**)&data, 
			 COMPLEX_DATA, 0, &retSize[0]);
  return status;
}

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
int getVect1DChar(int opCtx, const char *fieldPath, const char *timebasePath, 
		  char **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 CHAR_DATA, 1, &retSize[0]);
  if (status==0)
    {
      *dim = retSize[0];
    }
  return status;
}

/**
   Reads a 1D integer array.
   This function reads a 1D vector signal made of integers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect1DInt(int opCtx, const char *fieldPath, const char *timebasePath, 
		 int **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 1, &retSize[0]);
  if (status==0)
    {
      *dim = retSize[0];
    }
  return status;
}

/**
   Reads a 1D double array.
   This function reads a 1D vector signal made of doubles from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect1DDouble(int opCtx, const char *fieldPath, const char *timebasePath, 
		    double **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 1, &retSize[0]);
  if (status==0)
    {
      *dim = retSize[0];
    }
  return status;
}

/**
   Reads a 1D complex number array.
   This function reads a 1D vector signal made of complex numbers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect1DComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
		     Complex **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 1, &retSize[0]);
  if (status==0)
    {
      *dim = retSize[0];
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
int getVect2DChar(int opCtx, const char *fieldPath, const char *timebasePath, 
		  char **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 CHAR_DATA, 2, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;
}

/**
   Reads a 2D integer array.
   This function reads a 2D vector signal made of integers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect2DInt(int opCtx, const char *fieldPath, const char *timebasePath, 
		 int **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 2, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;
}

/**
   Reads a 2D double array.
   This function reads a 2D vector signal made of doubles from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect2DDouble(int opCtx, const char *fieldPath, const char *timebasePath, 
		    double **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 2, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;  
}

/**
   Reads a 2D complex number array.
   This function reads a 2D vector signal made of complex numbers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect2DComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
		     Complex **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 2, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;  
}

/**
   Reads a 3D integer array.
   This function reads a 3D vector signal made of integers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect3DInt(int opCtx, const char *fieldPath, const char *timebasePath, 
		 int **data, int *dim1, int *dim2, int *dim3)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 3, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
    }
  return status;
}

/**
   Reads a 3D double array.
   This function reads a 3D vector signal made of doubles from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed

   [ex_ual_read_data]*/
int getVect3DDouble(int opCtx, const char *fieldPath, const char *timebasePath, 
		    double **data, int *dim1, int *dim2, int *dim3)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 3, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
    }
  return status;
}
/*[ex_ual_read_data]*/

/**
   Reads a 3D complex number array.
   This function reads a 3D vector signal made of complex numbers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect3DComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
		     Complex **data, int *dim1, int *dim2, 
		     int *dim3)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 3, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
    }
  return status;
}

/**
   Reads a 4D integer array.
   This function reads a 4D vector signal made of integers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect4DInt(int opCtx, const char *fieldPath, const char *timebasePath, 
		 int **data, int *dim1, int *dim2, int *dim3, 
		 int *dim4)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 4, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
    }
  return status;
}

/**
   Reads a 4D double array.
   This function reads a 4D vector signal made of doubles from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect4DDouble(int opCtx, const char *fieldPath, const char *timebasePath, 
		    double **data, int *dim1, int *dim2, int *dim3,
		    int *dim4)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 4, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
    }
  return status;
}

/**
   Reads a 4D complex number array.
   This function reads a 4D vector signal made of complex numbers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect4DComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
		     Complex **data, int *dim1, int *dim2, 
		     int *dim3, int *dim4)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 4, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
    }
  return status;
}

/**
   Reads a 5D integer array.
   This function reads a 5D vector signal made of integers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect5DInt(int opCtx, const char *fieldPath, const char *timebasePath, 
		 int **data, int *dim1, int *dim2, int *dim3, 
		 int *dim4, int *dim5)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 5, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
    }
  return status;
}

/**
   Reads a 5D double array.
   This function reads a 5D vector signal made of doubles from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect5DDouble(int opCtx, const char *fieldPath, const char *timebasePath, 
		    double **data, int *dim1, int *dim2, int *dim3,
		    int *dim4, int *dim5)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 5, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
    }
  return status;
}

/**
   Reads a 5D complex number array.
   This function reads a 5D vector signal made of complex numbers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect5DComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
		     Complex **data, int *dim1, int *dim2, 
		     int *dim3, int *dim4, int *dim5)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 5, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
    }
  return status;
}

/**
   Reads a 6D integer array.
   This function reads a 6D vector signal made of integers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect6DInt(int opCtx, const char *fieldPath, const char *timebasePath, 
		 int **data, int *dim1, int *dim2, int *dim3,
		 int *dim4, int *dim5, int *dim6)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 6, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
    }
  return status;
}

/**
   Reads a 6D double array.
   This function reads a 6D vector signal made of doubles from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect6DDouble(int opCtx, const char *fieldPath, const char *timebasePath, 
		    double **data, int *dim1, int *dim2, int *dim3,
		    int *dim4, int *dim5, int *dim6)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 6, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
    }
  return status;
}

/**
   Reads a 6D complex number array.
   This function reads a 6D vector signal made of complex numbers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect6DComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
		     Complex **data, int *dim1, int *dim2, 
		     int *dim3, int *dim4, int *dim5, int *dim6)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 6, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
    }
  return status;
}

/**
   Reads a 7D integer array.
   This function reads a 7D vector signal made of integers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @param[out] dim7 size of seventh dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect7DInt(int opCtx, const char *fieldPath, const char *timebasePath, 
		 int **data, int *dim1, int *dim2, int *dim3, 
		 int *dim4, int *dim5, int *dim6, int *dim7)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 7, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
      *dim7 = retSize[6];
    }
  return status;
}

/**
   Reads a 7D double array.
   This function reads a 7D vector signal made of doubles from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @param[out] dim7 size of seventh dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect7DDouble(int opCtx, const char *fieldPath, const char *timebasePath, 
		    double **data, int *dim1, int *dim2, int *dim3,
		    int *dim4, int *dim5, int *dim6, int *dim7)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 7, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
      *dim7 = retSize[6];
    }
  return status;
}

/**
   Reads a 7D complex number array.
   This function reads a 7D vector signal made of complex numbers from a DATAOBJECT.
   @param[in] opCtx operation context ID
   @param[in] fieldPath field name
   @param[in] timebasePath timebase name
   @param[out] data field value 
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @param[out] dim7 size of seventh dimension
   @result error status

   @todo Low-level API modification:
   - additional parameter isTimed
 */
int getVect7DComplex(int opCtx, const char *fieldPath, const char *timebasePath, 
		     Complex **data, int *dim1, int *dim2, 
		     int *dim3, int *dim4, int *dim5, int *dim6, int *dim7)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(opCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 7, &retSize[0]);
  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
      *dim7 = retSize[6];
    }
  return status;
}





/* objects management
*******************************************************************************/


/* OH: present in F90 in release_object F77 implementation, but not in C++???
   there is a releaseObject at lowlevel and a releaseObject at high level, at 
   high level it is defined as getObjectFromList + removeObjectFromList + 
   releaseObject...
   => in C++ HL interface, only releaseObject (lowlevel function) is called!!! 
   Also present in F90 impl of put_object_in_object (recent fix june 2013)
   => in this case, both C++ and Java diverge !!! 

   Are these 'List' functions related to memcache management? In that case they
   should not appear at this level but be kept within the virtual back-end / 
   ual_lowlevel.cpp code.
*/
void removeObjectFromList(int idx);
int addObjectToList(void *obj);
void replaceObjectInList(int idx, void *obj);
void *getObjectFromList(int idx);
/*********************************************/


/**
   Initializes an array of structure.
   This function initialize room for a generic array of structures.
   @param[in] ctx context ID (operation or array of structure)
   @param[in] index position in the container
   @param[in] fieldPath path of the field
   - simple array of structure or container: from the DATAOBJECT (included) to the 
   array of structure name
   - embedded array of structure: from the container (included) to the 
   array of structure name
   @param[in] timebasePath path of the timebase for this array of structure
   @param[in] size number of elements in the new array of structure 
   @result array of structure context ID [_errror if < 0_]
   @todo Low-level API modification: 
   - no void* argument (passed context is either operation or array or struct)
   - returned object is integer (int) instead of pointer (void *)
   - added argument for specifying size of the object
   - added argument for specifying timebase
   - no isTimed argument (redundant with timebasePath)

   [ex_ual_begin_write_arraystruct]*/
int beginObject(int ctx, int index, const char *fieldPath, const char *timebasePath,
		int size)
{
  return ual_begin_arraystruct_action(ctx, fieldPath, timebasePath, &size);
}
/*[ex_ual_begin_write_arraystruct]*/


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
int getObject(int opCtx, const char *fieldPath, const char *timebasePath, int *size)
{
  return ual_begin_arraystruct_action(opCtx, fieldPath, timebasePath, size);
}






/* array of structure element writers
*******************************************************************************/

/**
   Writes a character in an array of structure.
   This function writes a character field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @result error status

   @todo Low-level API modification:
   - new function
 */
int putCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, char data)
{  
  return ual_write_data(aosCtx, fieldPath, timebasePath, (void *)(&data), 
			CHAR_DATA, 0, NULL);
}

/**
   Writes an integer in an array of structure.
   This function writes an integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath, 
		   int idx, int data)
{  
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)(&data), INTEGER_DATA, 0, NULL);
}

/**
   Writes a double in an array of structure.
   This function writes a double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putDoubleInObject(int aosCtx, const char *fieldPath, const char *timebasePath,
		      int idx, double data)
{
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)(&data), DOUBLE_DATA, 0, NULL);
}

/**
   Writes a complex number in an array of structure.
   This function writes a complex number field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putComplexInObject(int aosCtx, const char *fieldPath, const char *timebasePath,
		       int idx, Complex data)
{
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)(&data), COMPLEX_DATA, 0, NULL);
}

/**
   Writes a 1D character array in an array of structure.
   This function writes a 1D character field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int putVect1DCharInObject(int aosCtx, const char *fieldPath, const char *timebasePath,
			  int idx, char *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, CHAR_DATA, 1, size);
}

/**
   Writes a 1D integer array in an array of structure.
   This function writes a 1D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect1DIntInObject(int aosCtx, const char *fieldPath, const char *timebasePath,
			 int idx, int *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, INTEGER_DATA, 1, size);
}

/**
   Writes a 1D double array in an array of structure.
   This function writes a 1D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect1DDoubleInObject(int aosCtx, const char *fieldPath, 
			    const char *timebasePath, int idx, 
			    double *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, DOUBLE_DATA, 1, size);
}

/**
   Writes a 1D complex number array in an array of structure.
   This function writes a 1D complex number field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect1DComplexInObject(int aosCtx, const char *fieldPath, 
			     const char *timebasePath, int idx, 
			     Complex *data, int dim)
{
  int size[1] = {dim};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, COMPLEX_DATA, 1, size);
}

/**
   Writes a 2D character array in an array of structure.
   This function writes a 2D character field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int putVect2DCharInObject(int aosCtx, const char *fieldPath, 
			  const char *timebasePath, int idx, 
			  char *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, CHAR_DATA, 2, size);
}

/**
   Writes a 2D integer array in an array of structure.
   This function writes a 2D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect2DIntInObject(int aosCtx, const char *fieldPath, 
			 const char *timebasePath, int idx, 
			 int *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, INTEGER_DATA, 2, size);
}

/**
   Writes a 2D double array in an array of structure.
   This function writes a 2D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)

   [ex_ual_put_in_arraystruct]*/
int putVect2DDoubleInObject(int aosCtx, const char *fieldPath, 
			    const char *timebasePath, int idx, 
			    double *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, DOUBLE_DATA, 2, size);
}
/*[ex_ual_put_in_arraystruct]*/

/**
   Writes a 2D complex number array in an array of structure.
   This function writes a 2D complex number field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect2DComplexInObject(int aosCtx, const char *fieldPath, 
			     const char *timebasePath, int idx, 
			     Complex *data, int dim1, int dim2)
{
  int size[2] = {dim1, dim2};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, COMPLEX_DATA, 2, size);
}

/**
   Writes a 3D integer array in an array of structure.
   This function writes a 3D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect3DIntInObject(int aosCtx, const char *fieldPath, 
			 const char *timebasePath, int idx, 
			 int *data, int dim1, int dim2, int dim3)
{
  int size[3] = {dim1, dim2, dim3};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, INTEGER_DATA, 3, size);
}

/**
   Writes a 3D double array in an array of structure.
   This function writes a 3D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect3DDoubleInObject(int aosCtx, const char *fieldPath, 
			    const char *timebasePath, int idx, 
			    double *data, int dim1, int dim2, int dim3)
{
  int size[3] = {dim1, dim2, dim3};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, DOUBLE_DATA, 3, size);
}

/**
   Writes a 3D complex number array in an array of structure.
   This function writes a 3D complex number field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[in] data field value
   @param[in] dim1 size of first dimension
   @param[in] dim2 size of second dimension
   @param[in] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
   - result = error status (int) instead of opaque pointer (void *)
 */
int putVect3DComplexInObject(int aosCtx, const char *fieldPath, 
			     const char *timebasePath, int idx, 
			     Complex *data, int dim1, int dim2, int dim3)
{
  int size[3] = {dim1, dim2, dim3};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, COMPLEX_DATA, 3, size);
}

/**
   Writes a 4D integer array in an array of structure.
   This function writes a 4D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect4DIntInObject(int aosCtx, const char *fieldPath, 
			 const char *timebasePath, int idx, 
			 int *data, int dim1, int dim2, int dim3, int dim4)
{
  int size[4] = {dim1, dim2, dim3, dim4};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, INTEGER_DATA, 4, size);
}

/**
   Writes a 4D double array in an array of structure.
   This function writes a 4D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect4DDoubleInObject(int aosCtx, const char *fieldPath, 
			    const char* timebasePath, int idx, 
			    double *data, int dim1, int dim2, int dim3, int dim4)
{
  int size[4] = {dim1, dim2, dim3, dim4};
  return ual_write_data(aosCtx, fieldPath, timebasePath,
			(void *)data, DOUBLE_DATA, 4, size);
}

/**
   Writes a 4D complex number array in an array of structure.
   This function writes a 4D complex number field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect4DComplexInObject(int aosCtx, const char *fieldPath, 
			     const char *timebasePath, int idx, 
			     Complex *data, int dim1, int dim2, int dim3, 
			     int dim4)
{
  int size[4] = {dim1, dim2, dim3, dim4};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, COMPLEX_DATA, 4, size);
}

/**
   Writes a 5D integer array in an array of structure.
   This function writes a 5D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect5DIntInObject(int aosCtx, const char *fieldPath, 
			 const char *timebasePath, int idx, 
			 int *data, int dim1, int dim2, int dim3, int dim4, 
			 int dim5)
{
  int size[5] = {dim1, dim2, dim3, dim4, dim5};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, INTEGER_DATA, 5, size);
}

/**
   Writes a 5D double array in an array of structure.
   This function writes a 5D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect5DDoubleInObject(int aosCtx, const char *fieldPath, 
			    const char *timebasePath, int idx, 
			    double *data, int dim1, int dim2, int dim3, 
			    int dim4, int dim5)
{
  int size[5] = {dim1, dim2, dim3, dim4, dim5};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, DOUBLE_DATA, 5, size);
}

/**
   Writes a 5D complex number array in an array of structure.
   This function writes a 5D complex number field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect5DComplexInObject(int aosCtx, const char *fieldPath, 
			     const char *timebasePath, int idx, 
			     Complex *data, int dim1, int dim2, int dim3, 
			     int dim4, int dim5)
{
  int size[5] = {dim1, dim2, dim3, dim4, dim5};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, COMPLEX_DATA, 5, size);
}

/**
   Writes a 6D integer array in an array of structure.
   This function writes a 6D integer field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect6DIntInObject(int aosCtx, const char *fieldPath, 
			 const char *timebasePath, int idx, 
			 int *data, int dim1, int dim2, int dim3, int dim4, 
			 int dim5, int dim6)
{
  int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, INTEGER_DATA, 6, size);
}

/**
   Writes a 6D double array in an array of structure.
   This function writes a 6D double field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect6DDoubleInObject(int aosCtx, const char *fieldPath, 
			    const char *timebasePath, int idx, 
			    double *data, int dim1, int dim2, int dim3, 
			    int dim4, int dim5, int dim6)
{
  int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, DOUBLE_DATA, 6, size);
}

/**
   Writes a 6D complex number array in an array of structure.
   This function writes a 6D complex number field into an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath name of the field
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
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
int putVect6DComplexInObject(int aosCtx, const char *fieldPath, 
			     const char *timebasePath, int idx, 
			     Complex *data, int dim1, int dim2, int dim3, 
			     int dim4, int dim5, int dim6)
{
  int size[6] = {dim1, dim2, dim3, dim4, dim5, dim6};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, COMPLEX_DATA, 6, size);
}

/**
   @deprecated no 7D field in an array of structure (7D = 6D + time)
 */
int putVect7DIntInObject(int aosCtx, const char *fieldPath, 
			 const char *timebasePath, int idx, 
			 int *data, int dim1, int dim2, int dim3, int dim4, 
			 int dim5, int dim6, int dim7)
{
  int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, INTEGER_DATA, 7, size);
}

/**
   @deprecated no 7D field in an array of structure (7D = 6D + time)
 */
int putVect7DDoubleInObject(int aosCtx, const char *fieldPath, 
			    const char *timebasePath, int idx, 
			    double *data, int dim1, int dim2, int dim3, 
			    int dim4, int dim5, int dim6, int dim7)
{
  int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, DOUBLE_DATA, 7, size);
}

/**
   @deprecated no 7D field in an array of structure (7D = 6D + time)
 */
int putVect7DComplexInObject(int aosCtx, const char *fieldPath, 
			     const char *timebasePath, int idx, 
			     Complex *data, int dim1, int dim2, int dim3, 
			     int dim4, int dim5, int dim6, int dim7)
{
  int size[7] = {dim1, dim2, dim3, dim4, dim5, dim6, dim7};
  return ual_write_data(aosCtx, fieldPath, timebasePath, 
			(void *)data, COMPLEX_DATA, 7, size);
}





/* array of structure element readers
*******************************************************************************/

/**
   Reads a character from an array of structure.
   This function reads a character field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @result error status

   @todo Low-level API modification:
   - new function
 */
int getCharFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, char *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)&data, 
			 CHAR_DATA, 0, &retSize[0]);
  return status;
}

/**
   Reads an integer from an array of structure.
   This function reads an integer field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, int *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)&data, 
			 INTEGER_DATA, 0, &retSize[0]);
  return status;
}

/**
   Reads a double from an array of structure.
   This function reads a double field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)

   [ex_ual_get_from_arraystruct]*/
int getDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			double *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)&data, 
			 DOUBLE_DATA, 0, &retSize[0]);
  return status;
}
/*[ex_ual_get_from_arraystruct]*/

/**
   Reads a complex number from an array of structure.
   This function reads a complex number field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getComplexFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			 Complex *data)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)&data, 
			 COMPLEX_DATA, 0, &retSize[0]);
  return status;
}

/**
   Reads a 1D character array from an array of structure.
   This function reads a 1D character vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int getVect1DCharFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			    char **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 CHAR_DATA, 1, &retSize[0]);

  if (status==0)
    {
      *dim = retSize[0];
    }
  return status;
}

/**
   Reads a 1D integer array from an array of structure.
   This function reads a 1D integer vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect1DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 1, &retSize[0]);

  if (status==0)
    {
      *dim = retSize[0];
    }
  return status;
}

/**
   Reads a 1D double array from an array of structure.
   This function reads a 1D double vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect1DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      double **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 1, &retSize[0]);

  if (status==0)
    {
      *dim = retSize[0];
    }
  return status;
}

/**
   Reads a 1D complex number array from an array of structure.
   This function reads a 1D complex number vector field from an element of 
   an array of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim size of first dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect1DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			       Complex **data, int *dim)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 1, &retSize[0]);

  if (status==0)
    {
      *dim = retSize[0];
    }
  return status;
}

/**
   Reads a 2D character array from an array of structure.
   This function reads a 2D character vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - new function
 */
int getVect2DCharFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			    char **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 CHAR_DATA, 2, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;
}

/**
   Reads a 2D integer array from an array of structure.
   This function reads a 2D integer vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect2DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 2, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;
}

/**
   Reads a 2D double array from an array of structure.
   This function reads a 2D double vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect2DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      double **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 2, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;
}

/**
   Reads a 2D complex number array from an array of structure.
   This function reads a 2D complex number vector field from an element of 
   an array of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect2DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			       Complex **data, int *dim1, int *dim2)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 2, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
    }
  return status;
}

/**
   Reads a 3D integer array from an array of structure.
   This function reads a 3D integer vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect3DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int **data, int *dim1, int *dim2, int *dim3)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 3, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
    }
  return status;
}

/**
   Reads a 3D double array from an array of structure.
   This function reads a 3D double vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect3DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      double **data, int *dim1, int *dim2, int *dim3)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 3, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
    }
  return status;
}

/**
   Reads a 3D complex number array from an array of structure.
   This function reads a 3D complex number vector field from an element of 
   an array of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect3DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			       Complex **data, int *dim1, int *dim2, 
			       int *dim3)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 3, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
    }
  return status;
}

/**
   Reads a 4D integer array from an array of structure.
   This function reads a 4D integer vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect4DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int **data, int *dim1, int *dim2, int *dim3, 
			   int *dim4)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 4, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
    }
  return status;
}

/**
   Reads a 4D double array from an array of structure.
   This function reads a 4D double vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect4DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      double **data, int *dim1, int *dim2, int *dim3, 
			      int *dim4)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 4, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
    }
  return status;
}

/**
   Reads a 4D complex number array from an array of structure.
   This function reads a 4D complex number vector field from an element of 
   an array of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect4DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			       Complex **data, int *dim1, int *dim2, 
			       int *dim3, int *dim4)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 4, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
    }
  return status;
}

/**
   Reads a 5D integer array from an array of structure.
   This function reads a 5D integer vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect5DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int **data, int *dim1, int *dim2, int *dim3, 
			   int *dim4, int *dim5)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 5, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
    }
  return status;
}

/**
   Reads a 5D double array from an array of structure.
   This function reads a 5D double vector field from an element of an array of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect5DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      double **data, int *dim1, int *dim2, int *dim3, 
			      int *dim4, int *dim5)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 5, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
    }
  return status;
}

/**
   Reads a 5D complex number array from an array of structure.
   This function reads a 5D complex number vector field from an element of 
   an array of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect5DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			       Complex **data, int *dim1, int *dim2, 
			       int *dim3, int *dim4, int *dim5)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 5, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
    }
  return status;
}

/**
   Reads a 6D integer array from an array of structure.
   This function reads a 6D integer vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect6DIntFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			   int **data, int *dim1, int *dim2, int *dim3, 
			   int *dim4, int *dim5, int *dim6)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 INTEGER_DATA, 6, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
    }
  return status;
}

/**
   Reads a 6D double array from an array of structure.
   This function reads a 6D double vector field from an element of an array 
   of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect6DDoubleFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			      double **data, int *dim1, int *dim2, int *dim3, 
			      int *dim4, int *dim5, int *dim6)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 DOUBLE_DATA, 6, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
    }
  return status;
}

/**
   Reads a 6D complex number array from an array of structure.
   This function reads a 6D complex number vector field from an element of 
   an array of structure.
   @param[in] aosCtx array of structure context ID
   @param[in] fieldPath field name
   @param[in] timebasePath path to the field containing the timebase
   @param[in] idx index of the array element
   @param[out] data field value
   @param[out] dim1 size of first dimension
   @param[out] dim2 size of second dimension
   @param[out] dim3 size of third dimension
   @param[out] dim4 size of fourth dimension
   @param[out] dim5 size of fifth dimension
   @param[out] dim6 size of sixth dimension
   @result error status

   @todo Low-level API modification:
   - no passed pointer on array of structure (void *obj)
 */
int getVect6DComplexFromObject(int aosCtx, const char *fieldPath, const char *timebasePath, int idx, 
			       Complex **data, int *dim1, int *dim2, 
			       int *dim3, int *dim4, int *dim5, int *dim6)
{
  int status;
  int retSize[MAXDIM];
  status = ual_read_data(aosCtx, fieldPath, timebasePath, (void **)data, 
			 COMPLEX_DATA, 6, &retSize[0]);

  if (status==0)
    {
      *dim1 = retSize[0];
      *dim2 = retSize[1];
      *dim3 = retSize[2];
      *dim4 = retSize[3];
      *dim5 = retSize[4];
      *dim6 = retSize[5];
    }
  return status;
}

