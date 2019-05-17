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


/* allow future extension where another slice than the last one is replaced */




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



