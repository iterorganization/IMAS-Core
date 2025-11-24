//-*-c++-*-

/**
 * @file al_backend.h
 * @brief Contains definition of abstract Backend class, defining the virtual API each 
 * back-end implementation has to provide code for.
 */

#ifndef AL_BACKEND_H
#define AL_BACKEND_H 1

#include "al_context.h"

#if defined(_WIN32)
#  define IMAS_CORE_LIBRARY_API __declspec(dllexport)
#else
#  define IMAS_CORE_LIBRARY_API
#endif

#ifdef __cplusplus

#include <vector>
#include <map>
#include <string>
#include "data_interpolation.h"

/**
 * @class Backend
 * @brief Abstract Backend class. Defines the back-end API as pure virtual member functions.
 */
class IMAS_CORE_LIBRARY_API Backend
{
public:
  /**
   * @brief Virtual destructor.
   */
   virtual ~Backend() {}

  /**
   * @brief Initializes a given back-end object.
   * @param[in] ctx Data entry context.
   * @return Pointer to the backend object.
   */
   static Backend* initBackend(DataEntryContext *ctx);

  /**
   * @brief Returns the version of the backend (pair <major,minor>), to be used for compatibility checks.
   *  Version number needs to be bumped when:
   *  - non-backward compatible changes are being introduced --> major
   *  - non-forward compatible changes are being introduced --> minor
   * @param[in] ctx Pointer to pulse context. If NULL, returns the version of the backend,
   * otherwise returns the version stored in the associated pulse file.
   * @return std::pair<int,int> for <major,minor> version numbers.
   */
   virtual std::pair<int,int> getVersion(DataEntryContext *ctx) = 0;

  /**
   * @brief Opens a database entry.
   * This function opens a database entry described by the passed pulse context.
   * @param[in] ctx Pointer to pulse context.
   * @param[in] mode Opening option:
   * - OPEN_PULSE: Open an existing pulse (only if it exists).
   * - FORCE_OPEN_PULSE: Open a pulse (create it if it does not exist).
   * - CREATE_PULSE: Create a new pulse (do not overwrite if it already exists).
   * - FORCE_CREATE_PULSE: Create a new pulse (erase old one if it already exists).
   * @throw BackendException
   */
   virtual void openPulse(DataEntryContext *ctx, int mode) = 0;

  /**
   * @brief Closes a database entry.
   * This function closes a database entry described by the passed pulse context.
   * @param[in] ctx Pointer to pulse context.
   * @param[in] mode Closing option:
   * - CLOSE_PULSE: Close the pulse.
   * - ERASE_PULSE: Close and remove the pulse.
   * @throw BackendException
   */
   virtual void closePulse(DataEntryContext *ctx, int mode) = 0;

  /**
   * @brief Starts an I/O action on a DATAOBJECT.
   * This function initiates a new operation on an entire DATAOBJECT.
   * @param[in] ctx Pointer to operation context.
   * @throw BackendException
   */
   virtual void beginAction(OperationContext *ctx) = 0;

  /**
   * @brief Stops an I/O action.
   * This function finalizes an operation designed by the context passed as argument. 
   * @param[in] ctx Pointer to context (pulse, operation, or arraystruct, as stated by getType()).
   * @throw BackendException
   */
   virtual void endAction(Context *ctx) = 0; 

 /**
  * @brief Writes data. This function writes a signal in the database given the passed operation context.
  * @param[in] ctx Pointer to Context (either Operation or Arraystruct).
  * @param[in] fieldname Field name.
  * @param[in] timebasename Time base field name.
  * @param[in] data Pointer to the data to be written.
  * @param[in] datatype Type of data to be written:
  * - CHAR_DATA: Strings.
  * - INTEGER_DATA: Integers.
  * - DOUBLE_DATA: Double precision floating points.
  * - COMPLEX_DATA: Complex numbers.
  * @param[in] dim Dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM).
  * @param[in] size Array of the size of each dimension (NULL if dim=0).
  * @throw BackendException
  */
  virtual void writeData(Context *ctx,
			 std::string fieldname,
			 std::string timebasename, 
			 void* data,
			 int datatype,
			 int dim,
			 int* size) = 0;

 /**
  * @brief Reads data. This function reads a signal in the database given the passed operation context.
  * @param[in] ctx Pointer to Context (either Operation or Arraystruct).
  * @param[in] fieldname Field name.
  * @param[in] timebasename Time base field name.
  * @param[out] data Returned pointer to the read data.
  * @param[inout] datatype Type of data to be read:
  * - CHAR_DATA: Strings.
  * - INTEGER_DATA: Integers.
  * - DOUBLE_DATA: Double precision floating points.
  * - COMPLEX_DATA: Complex numbers.
  * @param[inout] dim Returned dimension of the data (0=scalar, 1=1D vector, etc... up to MAXDIM).
  * @param[out] size Array returned with elements filled with the size of each dimension.
  * @return Returns 0 when there is no such data (or 1 on success).
  * @throw BackendException
  */
  virtual int readData(Context *ctx,
		       std::string fieldname,
		       std::string timebasename, 
		       void** data,
		       int* datatype,
		       int* dim,
		       int* size) = 0;

 /**
  * @brief Deletes data. This function deletes some data (can be a signal, a structure, the whole DATAOBJECT) in the database 
  * given the passed context.
  * @param[in] ctx Pointer to operation context.
  * @param[in] path Path of the data structure element to delete (suppress the whole subtree).
  * @throw BackendException
  */
  virtual void deleteData(OperationContext *ctx,
			  std::string path) = 0;

 /**
  * @brief Starts an operation on a new array of structures. This function initiates the writing of a new top-level or nested array of structure.
  * @param[in] ctx Pointer to array of structure context.
  * @param[in,out] size Specifies the size of the array (number of elements).
  * @throw BackendException
  */
  virtual void beginArraystructAction(ArraystructContext *ctx,
				      int *size) = 0;

 /**
  * @brief Returns the list of non-empty IDS occurrences.
  * This function returns a list of IDS occurrences (integers) which are non empty in the database
  * @param[in] ctx data entry context
  * @param[in] ids_name name of the ids
  * @param[in] occurrences_list List of non-empty IDS occurrences (integers).
  * @param[in] size Size of the list of non-empty IDS occurrences (integers).
  * @throw BackendException
  */
  virtual void get_occurrences(Context* ctx, const char* ids_name, int** occurrences_list, int* size) = 0;

 /**
  * @brief Returns true if the backend performs time data interpolation (e.g., time slices operations or IMAS-3885 with data resampling), false otherwise.
  * @return True if the backend supports time data interpolation, false otherwise.
  */
  virtual bool supportsTimeDataInterpolation() = 0;

 /**
  * @brief Sets the data interpolation component if the backend supports time data interpolation or time range features.
  * This function is used by the LL framework during backend instanciation. 
  * Throws a backend exception if the backend does not perform time data interpolation.
  * This function is used by the LL framework during backend instantiation.
  * @throw BackendException
  */
  virtual void initDataInterpolationComponent() = 0;

 /**
  * @brief Returns true if the backend supports time range operation (IMAS-3885), false otherwise.
  * @return True if the backend supports time range operation, false otherwise.
  */
  virtual bool supportsTimeRangeOperation() = 0;

};

#endif

#endif // AL_BACKEND_H
