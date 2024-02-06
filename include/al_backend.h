//-*-c++-*-

/**
   \file al_backend.h
   Contains definition of abstract Backend classe, defining the virtual API each 
   back-end implementation has to provide code for.
*/

#ifndef AL_BACKEND_H
#define AL_BACKEND_H 1

#include "al_context.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus

#include <vector>
#include <map>
#include <string>

/**
   Abstract Backend class.
   Defines the back-end API, as pure virtual member functions. 
*/
class LIBRARY_API Backend 
{
public:
  // virtual desctructor
  virtual ~Backend() {}

  /**
     Init a given back-end object.
     @param[in] id backend identifier
     @result pointer on backend object
  */
  static Backend* initBackend(int id);

  /**
     Returns version of the backend (pair <major,minor>), to be used for compatibility checks.
     Version number needs to be bumped when:
     - non-backward compatible changes are being introduced --> major
     - non-forward compatible changes are being introduced --> minor
     @param[in] ctx pointer on pulse context, if NULL then returns the version of the backend,
     otherwise returns the version stored in associated pulse file
     @result std::pair<int,int> for <major,minor> version numbers
  */
  virtual std::pair<int,int> getVersion(DataEntryContext *ctx) = 0;

  /**
     Opens a database entry.
     This function opens a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @throw BackendException
  */
  virtual void openPulse(DataEntryContext *ctx,
			 int mode) = 0;

  /**
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse
     @throw BackendException
  */
  virtual void closePulse(DataEntryContext *ctx,
			  int mode) = 0;

  /**
     Starts an I/O action on a DATAOBJECT.
     This function initiates a new operation on an entire DATAOBJECT.
     @param[in] ctx pointer on operation context
     @throw BackendException
  */
  virtual void beginAction(OperationContext *ctx) = 0;

  /**
     Stops an I/O action.
     This function finalizes an operation designed by the context passed as argument. 
     @param[in] ctx pointer on context (pulse, operation or arraystruct, as stated by getType())
     @throw BackendException
  */
  virtual void endAction(Context *ctx) = 0; 

  /**
     Writes data.
     This function writes a signal in the database given the passed operation context.
     @param[in] ctx pointer on Context (either Operation or Arraystruct)
     @param[in] fieldname field name 
     @param[in] timebasename time base field name
     @param[in] data pointer on the data to be written
     @param[in] datatype type of data to be written:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[in] dim dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[in] size array of the size of each dimension (NULL is dim=0)
     @throw BackendException
  */
  virtual void writeData(Context *ctx,
			 std::string fieldname,
			 std::string timebasename, 
			 void* data,
			 int datatype,
			 int dim,
			 int* size) = 0;

  /**
     Reads data.
     This function reads a signal in the database given the passed operation context.
     @param[in] ctx pointer on Context (either Operation or Arraystruct)
     @param[in] fieldname field name
     @param[in] timebasename timebase field name
     @param[out] data returned pointer on the read data 
     @param[inout] datatype type of data to be read:
     - CHAR_DATA strings
     - INTEGER_DATA integers
     - DOUBLE_DATA double precision floating points
     - COMPLEX_DATA complex numbers
     @param[inout] dim returned dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM)
     @param[out] size array returned with elements filled at the size of each dimension 
     @result returns 0 when there is no such data (or 1 on success)
     @throw BackendException
  */
  virtual int readData(Context *ctx,
		       std::string fieldname,
		       std::string timebasename, 
		       void** data,
		       int* datatype,
		       int* dim,
		       int* size) = 0;

  /**
    Deletes data.
    This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) in the database 
    given the passed context.
    @param[in] ctx pointer on operation context 
    @param[in] path path of the data structure element to delete (suppress the whole subtree)
    @throw BackendException
  **/
  virtual void deleteData(OperationContext *ctx,
			  std::string path) = 0;

  /**
     Starts an operation on a new array of structures.
     This function initiates the writing of a new top-level or nested array of structure.
     @param[in] ctx pointer on array of structure context
     @param[in,out] size specify the size of the array (number of elements)
     @throw BackendException
  */
  virtual void beginArraystructAction(ArraystructContext *ctx,
				      int *size) = 0;

   
   /**
    Return the list of non empty IDS occurrences.
    This function returns a list of IDS occurrences (integers) which are non empty in the database
    @param[out] list of non empty IDS occurrences (integers)
    @param[out] size of the list of non empty IDS occurrences (integers)
    @throw BackendException
  **/
  virtual void get_occurrences(const char* ids_name, int** occurrences_list, int* size) = 0;

};


#endif

#endif // AL_BACKEND_H
