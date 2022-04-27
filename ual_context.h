/*-*-c++-*-*/

/**
   \file ual_context.h
   Contains definition of Context classes and all its subclasses.
*/

#ifndef UAL_CONTEXT_H
#define UAL_CONTEXT_H 1

#include "ual_utilities.h"
#include "ual_exception.h"
#include "ual_const.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#   define NOMINMAX
#include <windows.h>
#include <Shlobj.h>
#else
#  define LIBRARY_API
#include <pwd.h>
#endif

#ifdef __cplusplus

#include <iostream>
#include <cstdlib>
#include <atomic>

#define CTX_TYPE 100
#define CTX_PULSE_TYPE 101
#define CTX_OPERATION_TYPE 102
#define CTX_ARRAYSTRUCT_TYPE 103


/**
   Generic Context class.
   The simple generic Context is only associated to a given Back-end.
*/
class LIBRARY_API Context 
{
public:
  /**
    Context destructor.
  */
  virtual ~Context() {}

  /**
     Prints content of the context.
  */
  virtual std::string print() const; 

  /**
     Returns simplified pseudo path for the context.
  */
  virtual std::string fullPath() const = 0; 

  /**
     Returns object unique id.
     @result uid
  */
  virtual unsigned long int getUid() const;

  /**
     Returns the type of context.
     @result CTX_TYPE
  */
  virtual int getType() const = 0;

  /**
     Returns the ID of associated backend.
     @result backend_id
  */
  virtual int getBackendID() const = 0;

  /**
     Returns the name of associated backend.
     @result name of the backend
  */
  virtual std::string getBackendName() const = 0;


  
protected:
  static std::atomic<unsigned long int> SID; /**< a global UID */
  unsigned long int uid;                     /**< a local ID to identify instances */
};



/**
   Context class for a given pulse file.
   The DataEntryContext is a Context associated to a given pulse file.
*/
class LIBRARY_API DataEntryContext : public Context 
{
public:

  /**
     Constructor
     @param uri URI
  */
  DataEntryContext(std::string uri);


  /**
     DataEntryContext destructor.
  */
  virtual ~DataEntryContext() {}

  /**
     Returns full description of the pulse context.
  */
  virtual std::string print() const; 

  /**
     Returns simplified pseudo path for the context.
  */
  virtual std::string fullPath() const; 

  /**
     Returns the type of context.
     @result CTX_PULSE_TYPE
  */
  virtual int getType() const; 

  /**
     Returns the URI.
     @result URI 
  */
  std::string getURI() const;

  /**
     Returns the shot number.
     @result shot 
  */
  int getShot() const;

  /**
     Returns the run number.
     @result run
  */
  int getRun() const;

  /**
     Returns the user name.
     @result user 
  */
  std::string getUser() const;

  /**
     Returns the tokamak name.
     @result tokamak 
  */
  std::string getTokamak() const;

  /**
     Returns the version.
     @result version 
  */
  std::string getVersion() const;

  /**
     Returns the ID of associated backend.
     @result backend_id
  */
  virtual int getBackendID() const;

  /**
     Returns the name of associated backend.
     @result name of the backend
  */
  virtual std::string getBackendName() const;

  /**
    Returns options string.
    @result options 
  */
  std::string getOptions() const;

  /**
     Add an option to the URI query.
     @param[in] option_name name of the option
     @param[in] option_value value of the option
   */
  void addOptionToURIQuery(const std::string &option_name, const std::string &option_value);

  /**
     Returns the URI path.
     @result path
  */
  std::string getPath();

  /**
     Builds the URI backend from the backend ID.
     @param[in] backend ID
     @return URI backend string
  */
  static std::string getURIBackend(int backend_id);

  /**
     Builds an URI string using legacy parameters.
     @param[in] backendID name/ID of the back-end
     @param[in] shot shot number
     @param[in] run run number
     @param[in] user username
     @param[in] tokamak tokamak name
     @param[in] version data version
     @param[in] options options
     @result uri string
   */
  static void build_uri_from_legacy_parameters(const int backendID, 
                         const int shot, 
                         const int run, 
                         const char *user, 
                         const char *tokamak, 
                         const char *version,
                         const char *options,
                         char** uri);

 protected:
  std::string uri;                      /**< URI */
  int shot;                             /**< shot number */
  int run;                              /**< run number */
  std::string pulse;                    /**< can be shot, shot/run, keyword */
  std::string user;                     /**< user name */
  std::string tokamak;                  /**< tokamak name */
  std::string version;                  /**< data version */
  std::string path;                     /**< data path */
  std::string options;                  /**< options */
  int backend_id;                       /**< a backend identifier */

 private:
  void setBackendID(const std::string &path, const std::string &host);

};



/**
   Context class for an operation on a DATAOBJECT.
   The OperationContext is a Context associated to a given DATAOBJECT for a given I/O operation on a given DataEntryContext .
*/
class LIBRARY_API OperationContext : public Context 
{
public:
  /**
     Operation context constructor.
     Requires informations for all global put or get operations on a DATAOBJECT.
     @param ctx data-entry context
     @param dataobject name of the DATAOBJECT
     @param access access type of the operation 
     - READ_OP: read operation
     - WRITE_OP: write operation
     - REPLACE_OP: replace operation [_for the moment only in sliced mode for 
     "replace last slice"_]
  */
  OperationContext(DataEntryContext* ctx, std::string dataobject, int access);

  /**
     Operation context constructor.
     Requires informations for all possible put or get operations on a DATAOBJECT.
     @param ctx data-entry context
     @param dataobject name of the DATAOBJECT
     @param access access type of the operation 
     - READ_OP: read operation
     - WRITE_OP: write operation
     - REPLACE_OP: replace operation [_for the moment only in sliced mode for 
     "replace last slice"_]
     @param range range of the operation [_optional_]
     - GLOBAL_OP: operation on all slices 
     - SLICE_OP: operation on a single slice
     @param t time [_optional_]
     @param interp interpolation mode [_optional_]
     - CLOSEST_INTERP: consider slice at closest time
     - PREVIOUS_INTERP: consider slice at previous step
     - LINEAR_INTERP: interpolating linearly values at previous and next slices
     - UNDEFINED_INTERP: if not relevant [_e.g for write operations_]
  */
  OperationContext(DataEntryContext* ctx, std::string dataobject, int access, 
		   int range, double t, int interp);

  /**
     Operation context destructor.
  */
  virtual ~OperationContext() {}

  /**
     Returns full description of the operation context.
  */
  virtual std::string print() const; 

  /**
     Returns simplified pseudo path for this context.
  */
  virtual std::string fullPath() const; 

  /**
     Returns the type of context.
     @result CTX_OPERATION_TYPE
  */
  virtual int getType() const; 

  /**
     Returns the ID of associated backend.
     @result backend_id
  */
  virtual int getBackendID() const;

  /**
     Returns the name of associated backend.
     @result name of the backend
  */
  virtual std::string getBackendName() const;

  /**
     Returns the name of the DATAOBJECT.
     @result dataobjectname
  */
  std::string getDataobjectName() const;

  /**
     Returns access type of the operation.
     @result accessmode
     - READ_OP: read operation
     - WRITE_OP: write operation
     - REPLACE_OP: replace operation [_for the moment only in sliced mode for 
     "replace last slice"_]
  */
  int getAccessmode() const;

  /**
     Returns range of the operation.
     @result rangemode
     - GLOBAL_OP: operation on all slices [_also in non time-dependent case_]
     - SLICE_OP: operation on a single slice
  */
  int getRangemode() const;

  /**
     Returns time of the operation.
     @result time [_=UNDEFINED_TIME when time is not relevant_]
  */
  double getTime() const;

  /**
     Returns type of interpolation selected for the operation.
     @result interpmode
     - CLOSEST_INTERP: consider slice at closest time
     - PREVIOUS_INTERP: consider slice at previous step
     - LINEAR_INTERP: interpolating linearly values at previous and next slices
     - UNDEFINED_INTERP: if not relevant [_e.g for write operations_]     
  */
  int getInterpmode() const;

  /**
     Returns the associated DataEntryContext.
     @result ctx
   */
  DataEntryContext* getDataEntryContext() const;
  
protected:
  DataEntryContext* pctx;               /**< associated DataEntry context */
  std::string dataobjectname;           /**< DATAOBJECT name */
  int accessmode;                       /**< operation access type */
  int rangemode;                        /**< operation range */
  double time;                          /**< operation time */
  int interpmode;                       /**< operation interpolation type */
};



/**
   Context class for an array of structures.
   The ArraystructContext is a Context associated to a given array of structure from a given OperationContext.
*/
class LIBRARY_API ArraystructContext : public Context 
{
 public:
  /**
     Array of structure context constructor.
     Requires informations for describing usage of stand-alone or top-most arrays of structure in a DATAOBJECT.
     @param ctx operation context
     @param p path of the array of structure field [_within the DATAOBJECT_]
     @param tb path of the timebase associated with the array of structure
  */
  ArraystructContext(OperationContext* ctx, std::string p, std::string tb);

  /**
     Array of structure context constructor.
     Requires informations for describing usage of nested arrays of structure in a DATAOBJECT.
     @param parent context of the parent array of structure 
     @param p path of the array of structure field [_within its parent container_]
     @param tb path of the timebase associated with the array of structure
     @param timed time dependency of the DATAOBJECT
  */
  explicit ArraystructContext(ArraystructContext* parent, std::string p, std::string tb);

  /**
     Array of structure context constructor.
     Requires informations for describing usage of nested arrays of structure in a DATAOBJECT.
     @param parent context of the parent array of structure 
     @param p path of the array of structure field [_within its parent container_]
     @param tb path of the timebase associated with the array of structure
     @param idx index of the array of structure within its container [_by default first element_]
     @param timed time dependency of the DATAOBJECT
  */
  explicit ArraystructContext(ArraystructContext* parent, std::string p, std::string tb, int idx);

  /**
     Array of structure context destructor.
  */
  virtual ~ArraystructContext() {}

  /**
     Returns full description of the array of structure context.
  */
  virtual std::string print() const;

  /**
     Returns simplified pseudo path for this context.
  */
  virtual std::string fullPath() const; 

  /**
     Returns the type of context.
     @result CTX_ARRAYSTRUCT_TYPE
  */
  virtual int getType() const;

  /**
     Returns the ID of associated backend.
     @result backend_id
  */
  virtual int getBackendID() const;

  /**
     Returns the name of associated backend.
     @result name of the backend
  */
  virtual std::string getBackendName() const;

  /**
     Returns the path of the array of structure.
     This path corresponds to the absolute path in the DATAOBJECT for the topmost array of structure or in.
     @result path
  */
  std::string getPath() const;

 /**
     Returns the timebase path of the array of structure.
     This path corresponds to the absolute path in the DATAOBJECT for the topmost array of structure or in.
     @result path
  */
  std::string getTimebasePath() const;

  /**
     Returns whether the array of structure is time-dependent or not.
     @result timed
  */
  bool getTimed() const; 

  /**
     Returns the context of the container array of structure
     @result parent
  */
  ArraystructContext* getParent();

  /**
     Returns the position of the current element of interest within the array of structure.
     @result index
  */
  int getIndex() const;

  /**
     Updates the position of the current element of interest within the array of structure.
     @param[in] step step size for setting the next index
  */
  void nextIndex(int step);

  /**
     Returns the associated OperationContext.
     @result opctx
   */
  OperationContext* getOperationContext() const;

  
protected:
  std::string path;                     /**< path of the array of structure */
  std::string timebase;			/**< path of the timebase associated with the array of structure */
  ArraystructContext* parent;           /**< container of the array of structure */
  int index = 0;                        /**< position of the current element of interest within the array of structure */
  OperationContext* opctx;              /**< associated operation context **/

};

LIBRARY_API std::ostream& operator<< (std::ostream& o, Context const& ctx);
LIBRARY_API std::ostream& operator<< (std::ostream& o, DataEntryContext const& ctx);
LIBRARY_API std::ostream& operator<< (std::ostream& o, OperationContext const& ctx);
LIBRARY_API std::ostream& operator<< (std::ostream& o, ArraystructContext const& ctx);

#endif

#endif // UAL_CONTEXT_H
