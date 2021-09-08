//-*-c++-*-

/**
   \file ual_backend.h
   Contains definition of abstract Backend classe, defining the virtual API each 
   back-end implementation has to provide code for.
*/

#ifndef UAL_BACKEND_H
#define UAL_BACKEND_H 1

#include "ual_context.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus

#include <vector>
#include <map>
#include <string>

// Check if regex is available (fully implemented since GCC 4.9)
// see https://stackoverflow.com/questions/12530406/is-gcc-4-8-or-earlier-buggy-about-regular-expressions
#include <regex>

#if __cplusplus >= 201103L &&								\
	(!defined(__GLIBCXX__) || (__cplusplus >= 201402L)	||	\
		(defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT)	||	\
		defined(_GLIBCXX_REGEX_STATE_LIMIT)				||	\
			(defined(_GLIBCXX_RELEASE)					&&	\
			_GLIBCXX_RELEASE > 4)))
#  define HAVE_WORKING_REGEX 1
#else
#  define HAVE_WORKING_REGEX 0
#endif


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
     @param[in] options additional options, ex: "name=treename refShot=1 refRun=2"
     (possibly backend specific)
     @throw BackendException
  */
  virtual void openPulse(DataEntryContext *ctx,
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
  virtual void closePulse(DataEntryContext *ctx,
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

};


extern "C" {

/**
  Function uses to extract options and store them in map.
  @param[in] string containing options separated by spaces (ex: "-verbose -silent -readonly -ids=myids")
  @param[out] map containing extracted options and optionnal values
  @result number of options in map
*/
LIBRARY_API int extractOptions(const std::string& strOptions, std::map<std::string, std::string>& mapOptions);

/**
  Function uses to check in option is in map.
  @param[in] string containing option name
  @param[in] map containing options and optionnal values
  @param[out] optionnal value of the found option
  @result true if option found in map otherwise false
*/
LIBRARY_API bool isOptionExist(const std::string& strOption, const std::map<std::string, std::string>& mapOptions, std::string& strValue);

/**
  Function uses to compare UAL or DD version.
  The version format can be "x.y", "x.y.z", "x.y.z-www", "x.y.z_www", "x-y", "x-y-z" etc...
  @param[in] string containing version V1
  @param[in] string containing version V2
  @result 0 if versions are equal, -1 if V1 > V2 and 1 if V1 < V2
*/
LIBRARY_API int compareVersion(const std::string& strV1, const std::string& strV2);

}

#endif

#endif // UAL_BACKEND_H
