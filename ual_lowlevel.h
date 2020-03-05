/*-*-c++-*-*/

/**
   \file ual_lowlevel.h
   Contains definition of Lowlevel static class and all C wrappers around back-end API.
*/

#ifndef UAL_LOWLEVEL_H
#define UAL_LOWLEVEL_H 1

#include <stdbool.h>
#include <stdio.h>

/*#include "ual_const.h"*/
#include "ual_backend.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus

#include <vector>
#include <mutex>
#include <complex>

/**
   Lowlevel environment structure.
   Associates a Context with the corresponding Backend.
*/
class LIBRARY_API LLenv
{ 
public:
  Backend* backend;                               /**< pointer on Backend object */
  Context* context;                               /**< pointer on Context object */

  LLenv()
  {
    backend = NULL;
    context = NULL;
  }
  LLenv(Backend *be, Context *ctx)
  {
    backend = be;
    context = ctx;
  }
};


/**
   Generic Low-level static implementation.
*/
class LIBRARY_API Lowlevel 
{
public:
  static const char EMPTY_CHAR;     //                    = '\0';
  static const int EMPTY_INT;       //                      = -999999999;
  static const double EMPTY_DOUBLE; //                = -9.0E40;
  static const std::complex<double> EMPTY_COMPLEX;// = std::complex<double>(-9.0E40,-9.0E40);

  /**
     Adds a new lowlevel environment.
     Associates and stores a Backend,Context pair of objects.
     @param[in] be pointer on backend object
     @param[in] ctx pointer on context object
     @result storage element identifier
  */
  static int addLLenv(Backend *be, Context *ctx);

  /**
     Retrieves a stored lowlevel environment.
     @param[in] idx storage element identifier
     @result LLenv structure containing a pair of pointers on backend and context objects
  */
  static LLenv getLLenv(int idx);

  /**
     Returns and removes a lowlevel environment from storage.
     @param[in] idx storage element identifier
     @result LLenv structure (not stored anymore)
  */
  static LLenv delLLenv(int idx);
  
  /**
     Sets a scalar data to the allocated variable.
     @param[in] data data value passed as an opaque void*
     @param[in] type data type
     @param[in,out] var variable which should store the data, passed as void**
  */
  static void setScalarValue(void *data, int type, void **var);

  /**
     Sets a variable to its default value.
     @param[in] type data type
     @param[in] dim data dimension
     @param[in,out] var variable which should store the default value, passed as void**
     @param[in,out] size passed array for storing the size of each dimension (size[i] undefined if i>=dim)
  */
  static void setDefaultValue(int type, int dim, void **var, int *size);

  /**
     Starts an action on a pulse in the database.
     This function associates a specified back-end with a specific entry in the database.
     @param[in] backendID name/ID of the back-end
     @param[in] shot shot number
     @param[in] run run number
     @param[in] usr username [_optional, empty string for default_]
     @param[in] tok tokamak name [_optional, empty string for default_]
     @param[in] ver data version [_optional, empty string for default_]
     @return pulse context id [_error status < 0 or null context if = 0_]
  */
  static int beginPulseAction(int backendID,
			      int shot,
			      int run,
			      std::string usr,
			      std::string tok,
			      std::string ver);

private:
  static std::mutex mutex;                        /**< mutex for thread safe Factory accesses */

  static std::vector<LLenv> llenvStore;           /**< objects (Backend, Context) storage */
  static int curStoreElt;                         /**< position of next free slot in Store */
  static int maxStoreElt;                         /**< size of allocated Store */
};



extern "C"
{
#endif

  /******************** DEFINITION OF THE C API ********************/

  /**
     Print all the Context information corresponding to the passed Context identifier.
     @param[in] ctx Context ID (either PulseContext, OperationContext or ArraystructContext)
     @result error status [_success if = 0 or failure if < 0_]
   */
  LIBRARY_API int ual_print_context(int ctx);


  /**
     Get backendID from the passed Context identifier.
     @param[in] ctx Context ID (either PulseContext, OperationContext or ArraystructContext)
     @result backendID [_or error status < 0_]
   */
  LIBRARY_API int ual_get_backendID(int ctx);



  
  /**
     Starts an action on a pulse in the database.
     This function associates a specified back-end with a specific entry in the database.
     @param[in] backendID name/ID of the back-end
     @param[in] shot shot number
     @param[in] run run number
     @param[in] user username [_optional, "" for default_]
     @param[in] tokamak tokamak name [_optional, "" for default_]
     @param[in] version data version [_optional, "" for default_]
     @return pulse context id [_error status if < 0 or null context if = 0_]
  */
  LIBRARY_API int ual_begin_pulse_action(const int backendID, 
			     const int shot, 
			     const int run, 
			     const char *user, 
			     const char *tokamak, 
			     const char *version);

  /**
     Opens a database entry.
     This function opens a database entry described by the passed pulse context.
     @param[in] pulseCtx pulse context id (from ual_begin_pulse_action())
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @param[in] options additional options, ex: "name=treename refShot=1 refRun=2"
     (possibly backend specific)
     @result error status [_success if = 0 or failure if < 0_]
  */
  LIBRARY_API int ual_open_pulse(int pulseCtx, 
		     int mode, 
		     const char *options);

  /**
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] pulseCtx pulse context id (from ual_begin_pulse_action())
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse 
     @param[in] options additional options (possibly backend specific)
     @result error status [_success if = 0 or failure if < 0_]
  */
  LIBRARY_API int ual_close_pulse(int pulseCtx, 
		      int mode,
		      const char *options);

  /**
     Starts an I/O action on a DATAOBJECT.
     This function gives a new operation context for the duration of an action on a DATAOBJECT.
     @param[in] ctx pulse context id (from ual_begin_pulse_action())
     @param[in] dataobjectname name of the DATAOBJECT
     @param[in] rwmode mode for this operation:
     - READ_OP = read operation
     - WRITE_OP = write operation
     @result operation context id [_error status if < 0 or null context if = 0_]
  */
  LIBRARY_API int ual_begin_global_action(int ctx,
			      const char *dataobjectname,
			      int rwmode);

  /**
     Starts an I/O action on a DATAOBJECT slice.
     This function gives a new operation context for the duration of an action on a slice.  
     @param[in] ctx pulse context (from ual_begin_pulse_action())
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
     @result operation context id [_error status if < 0 or null context if = 0_]
  */
  LIBRARY_API int ual_begin_slice_action(int ctx,
			     const char *dataobjectname,
			     int rwmode,
			     double time,
			     int interpmode);

  /**
     Stops an I/O action.
     This function stop the current action designed by the context passed as argument. This context is then 
     not valide anymore.
     @param[in] ctx a pulse (ual_begin_pulse_action()), an operation (ual_begin_global_action() or ual_begin_slice_action()) or an array of structure context id (ual_begin_array_struct_action())
     @result error status [_success if = 0 or failure if < 0_]
  */
  LIBRARY_API int ual_end_action(int ctx); 

  /**
     Writes data.
     This function writes a signal in the database given the passed context.
     @param[in] ctx operation context id (from ual_begin_global_action() or ual_begin_slice_action()) or
     array of structure context id (from ual_begin_arraystruct_action())
     @param[in] fieldpath field path for the data (paths are always relative to current Context, dataobject absolute path can be specified with a prepended '/')
     @param[in] timebasepath field path for the timebase (paths are always relative to current Context, dataobject absolute path can be specified with a prepended '/')
     @param[in] data pointer on the data to be written
     @param[in] datatype type of data to be written:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in] size array of the size of each dimension (can be NULL if dim=0)
     @result error status [_success if = 0 or failure if < 0_]
  */
  LIBRARY_API int ual_write_data(int ctx,
		     const char *fieldpath,
		     const char *timebasepath,
		     void *data,
		     int datatype,
		     int dim,
		     int *size);

  /**
     Reads data.
     This function reads a signal in the database given the passed context.
     @param[in] ctx operation context id (from ual_begin_global_action() or ual_begin_slice_action()) or
     array of structure context id (from ual_begin_arraystruct_action())
     @param[in] fieldpath field path for the data (paths are always relative to current Context, dataobject absolute path can be specified with a prepended '/')
     @param[in] timebasepath field path for the timebase (paths are always relative to current Context, dataobject absolute path can be specified with a prepended '/')
     @param[out] data returned pointer on the read data 
     @param[in] datatype type of data to be read:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in,out] size passed array for storing the size of each dimension (size[i] undefined if i>=dim)
     @result error status [_success if = 0 or failure if < 0_]
  */
  LIBRARY_API int ual_read_data(int ctx,
		    const char *fieldpath,
		    const char *timebasepath,
		    void **data,
		    int datatype,
		    int dim,
		    int *size);

  /**
    Deletes data.
    This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) in the database 
    given the passed context.
    @param[in] ctx operation context id (from ual_begin_global_action() or ual_begin_slice_action())
    @param[in] path path of the data structure element to delete (suppress the whole subtree)
    @result error status [_success if = 0 or failure if < 0_]
  **/
  LIBRARY_API int ual_delete_data(int ctx,
		      const char *path);



  /**
     Starts operations on a new array of structure.
     This function gives a new array of structure context id for the duration of an action 
     on an array of structures, either from an operation context id (single or top-level array) 
     or from an array of structure context id (nested array).  
     @param[in] ctx operation context id (single case) or array of structure context id 
     (nested case) 
     @param[in] path path of array of structure (relative to ctx, or absolute if starts with "/")
     @param[in] timebase path of timebase associated with the array of structure 
     @param[in,out] size specify the size of the struct_array (number of elements)
     @result array of structure context [_error status if < 0 or null context if = 0_]
  */
  LIBRARY_API int ual_begin_arraystruct_action(int ctx,
				   const char *path,
				   const char *timebase,
				   int *size);



  /**
     Change current index of interest in an array of structure.
     This function updates the index pointing at the current element of interest within an array of structure.
     @param[in] aosctx array of structure Context
     @param[in] step iteration step size (typically=1)
     @result error status [_success if = 0 or failure if < 0_]
   */
  LIBRARY_API int ual_iterate_over_arraystruct(int aosctx, 
				   int step);


#if defined(__cplusplus)
}
#endif


#endif // UAL_LOWLEVEL_H
