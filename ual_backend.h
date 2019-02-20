//-*-c++-*-

/**
   \file ual_backend.h
   Contains definition of abstract Backend classe, defining the virtual API each 
   back-end implementation has to provide code for.
*/

#ifndef UAL_BACKEND_H
#define UAL_BACKEND_H 1

#include "ual_context.h"


/* c++ only part */
#if defined(__cplusplus)


/**
   Abstract Backend class.
   Defines the back-end API, as pure virtual member functions. 
   Contains the DataBrokerFactory, handle contexts, back-end IDs, error handling, mem-cache, etc...
*/
class Backend 
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
     Opens a database entry.
     This function opens a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @param[in] options additional options, ex: "name=treename refShot=1 refRun=2"
     (possibly backend specific)
     @throw BackendException
  */
  virtual void openPulse(PulseContext *ctx,
			 int mode,
			 std::string options) = 0;

  /**
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] ctx pointer on pulse context
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse 
     @param[in] options additional options (possibly backend specific)
     @throw BackendException
  */
  virtual void closePulse(PulseContext *ctx,
			  int mode,
			  std::string options) = 0;

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

  /*
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
     @result returns 0 when there is no such data in read mode (or 1 on success)
     @throw BackendException
  */
  virtual int beginArraystructAction(ArraystructContext *ctx,
				     int *size) = 0;

};

#endif 

#endif
