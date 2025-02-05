/*-*-c++-*-*/

/**
 * @file al_context.h
 * @brief Contains definition of Context classes and all its subclasses.
 */

#ifndef AL_CONTEXT_H
#define AL_CONTEXT_H 1

#include "al_exception.h"
#include "al_const.h"

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

#include "uri_parser.h"
#include <iostream>
#include <cstdlib>
#include <atomic>
#include <string>

#define CTX_TYPE 100
#define CTX_PULSE_TYPE 101
#define CTX_OPERATION_TYPE 102
#define CTX_ARRAYSTRUCT_TYPE 103


/**
 * @class Context
 * @brief Generic Context class. The simple generic Context is only associated to a given Back-end.
 */
class LIBRARY_API Context 
{
public:
   /**
    * @brief Context destructor.
    */
   virtual ~Context() {}

   /**
    * @brief Prints content of the context.
    * @return std::string Content of the context.
    */
   virtual std::string print() const; 

   /**
    * @brief Returns object unique id.
    * @return UID.
    */
   virtual unsigned long int getUid() const;

   /**
    * @brief Returns the type of context.
    * @return CTX_TYPE(int) Type of context.
    */
   virtual int getType() const = 0;

   /**
    * @brief Returns the URI.
    * @return uri::Uri URI string.
    */
   virtual uri::Uri getURI() const = 0;

   /**
    * @brief Returns the ID of associated backend.
    * @return backend_id(int) Backend ID.
    */
   virtual int getBackendID() const = 0;

   /**
    * @brief Returns the name of associated backend.
    * @return std::string Name of the backend.
    */
   virtual std::string getBackendName() const = 0;

protected:
   static std::atomic<unsigned long int> SID; /**< a global UID */
   unsigned long int uid;                     /**< a local ID to identify instances */
};


/**
 * @class DataEntryContext
 * @brief Context class for a given pulse file. The DataEntryContext is a Context associated to a given pulse file.
 */
class LIBRARY_API DataEntryContext : public Context 
{
public:
   /**
    * @brief Constructor
    * @param uri URI string
    */
   DataEntryContext(std::string uri);

   /**
    * @brief DataEntryContext destructor.
    */
   virtual ~DataEntryContext() {}

   /**
    * @brief Returns full description of the pulse context.
    * @return std::string Full description of the pulse context.
    */
   std::string print() const override; 

   /**
    * @brief Returns the type of context.
    * @return CTX_PULSE_TYPE(int) Type of context.
    */
   int getType() const override; 

   /**
    * @brief Returns the URI.
    * @return uri::Uri URI string.
    */
   uri::Uri getURI() const override;

   /**
    * @brief Returns the ID of associated backend.
    * @return int Backend ID.
    */
   int getBackendID() const override;

   /**
    * @brief Returns the name of associated backend.
    * @return std::string Name of the backend.
    */
   std::string getBackendName() const override;

   /**
    * @brief Builds the URI backend from the backend ID.
    * @param[in] backend_id ID of the backend
    * @return std::string Backend string from URI
    */
   static std::string getURIBackend(int backend_id);

   /**
    * @brief Builds an URI string using legacy parameters.
    * @param[in] backendID name/ID of the back-end
    * @param[in] pulse pulse number
    * @param[in] run run number
    * @param[in] user username
    * @param[in] tokamak tokamak name
    * @param[in] version data version
    * @param[in] options options
    * @param[out] uri URI string
    */
   static void build_uri_from_legacy_parameters(const int backendID, 
                                                                      const int pulse,
                                                                      const int run, 
                                                                      const std::string user, 
                                                                      const std::string tokamak, 
                                                                      const std::string version,
                                                                      const std::string options,
                                                                      std::string& uri);
                                     
   static void build_uri_from_legacy_parameters(const int backendID, 
                                                                      const int pulse,
                                                                      const int run, 
                                                                      const char *user, 
                                                                      const char *tokamak, 
                                                                      const char *version,
                                                                      const char *options,
                                                                      char** uri);

protected:
   uri::Uri uri;                         /**< URI */
   int backend_id;                       /**< a backend identifier */

private:
   uri::Uri checkUriHost(const uri::Uri& uri);
   void setBackendID(const std::string &path, const std::string &host);
   uri::Uri buildURIFromLegacy();
};


/**
 * @class OperationContext
 * @brief Context class for an operation on a DATAOBJECT. The OperationContext is a Context associated to a given DATAOBJECT for a given I/O operation on a given DataEntryContext.
 */
class LIBRARY_API OperationContext : public Context 
{
public:
   /**
    * @brief Operation context constructor.
    * Requires informations for all global put or get operations on a DATAOBJECT.
    * @param ctx data-entry context
    * @param dataobject name of the DATAOBJECT
    * @param datapath path to data node for partial get operation
    * @param access access type of the operation
    * - READ_OP: read operation
    * - WRITE_OP: write operation
    * - REPLACE_OP: replace operation [_for the moment only in sliced mode for "replace last slice"_]
    */
   OperationContext(DataEntryContext* ctx, std::string dataobject, std::string datapath, int access);

   /**
    * @brief Operation context constructor.
    * Requires informations for all possible put or get operations on a DATAOBJECT.
    * @param ctx data-entry context
    * @param dataobject name of the DATAOBJECT
    * @param access access type of the operation 
    * - READ_OP: read operation
    * - WRITE_OP: write operation
    * - REPLACE_OP: replace operation [_for the moment only in sliced mode for "replace last slice"_]
    * @param range range of the operation [_optional_]
    * - GLOBAL_OP: operation on all slices 
    * - SLICE_OP: operation on a single slice
    * @param t time [_optional_]
    * @param interp interpolation mode [_optional_]
    * - CLOSEST_INTERP: consider slice at closest time
    * - PREVIOUS_INTERP: consider slice at previous step
    * - LINEAR_INTERP: interpolating linearly values at previous and next slices
    * - UNDEFINED_INTERP: if not relevant [_e.g for write operations_]
    */
   OperationContext(DataEntryContext* ctx, std::string dataobject, int access, 
                            int range, double t, int interp);

   /**
    * @brief Operation context destructor.
    */
   virtual ~OperationContext() {}

   /**
    * @brief Returns full description of the operation context.
    * @return std::string Full description of the operation context.
    */
   std::string print() const override; 

   /**
    * @brief Returns the type of context.
    * @return int Type of context CTX_OPERATION_TYPE.
    */
   int getType() const override; 

   /**
    * @brief Returns the URI.
    * @return uri::Uri URI 
    */
   uri::Uri getURI() const override;

   /**
    * @brief Returns the ID of associated backend.
    * @return int Backend ID.
    */
   int getBackendID() const override;

   /**
    * @brief Returns the name of associated backend.
    * @return std::string Name of the backend.
    */
   std::string getBackendName() const override;

   /**
    * @brief Returns the name of the DATAOBJECT.
    * @return std::string Name of the DATAOBJECT.
    */
   std::string getDataobjectName() const;

   /**
    * @brief Returns the path of the requested data for a partial get operation. Will be empty for non-partial gets.
    * @return std::string Path of the requested data.
    */
   std::string getDatapath() const;

   /**
    * @brief Returns access type of the operation.
    * @return int Access type of the operation.
    * - READ_OP: read operation
    * - WRITE_OP: write operation
    * - REPLACE_OP: replace operation [_for the moment only in sliced mode for "replace last slice"_]
    */
   int getAccessmode() const;

   /**
    * @brief Returns range of the operation.
    * @return int Range of the operation.
    * - GLOBAL_OP: operation on all slices [_also in non time-dependent case_]
    * - SLICE_OP: operation on a single slice
    */
   int getRangemode() const;

   /**
    * @brief Returns time of the operation.
    * @return double Time of the operation.
    */
   double getTime() const;

   /**
    * @brief Returns type of interpolation selected for the operation.
    * @return int Interpolation mode.
    * - CLOSEST_INTERP: consider slice at closest time
    * - PREVIOUS_INTERP: consider slice at previous step
    * - LINEAR_INTERP: interpolating linearly values at previous and next slices
    * - UNDEFINED_INTERP: if not relevant [_e.g for write operations_]     
    */
   int getInterpmode() const;

   /**
    * @brief Returns the associated DataEntryContext.
    * @return DataEntryContext* Associated DataEntryContext.
    */
   DataEntryContext* getDataEntryContext() const;

protected:
   DataEntryContext* pctx;               /**< associated DataEntry context */
   std::string dataobjectname;           /**< DATAOBJECT name */
   int accessmode;                       /**< operation access type */
   int rangemode;                        /**< operation range */
   double time;                          /**< operation time */
   int interpmode;                       /**< operation interpolation type */
   std::string datapath;                 /**< path to data node for partial get operations */
};


/**
 * @class ArraystructContext
 * @brief Context class for an array of structures. The ArraystructContext is a Context associated to a given array of structure from a given OperationContext.
 */
class LIBRARY_API ArraystructContext : public Context 
{
public:
   /**
    * @brief Array of structure context constructor.
    * Requires informations for describing usage of stand-alone or top-most arrays of structure in a DATAOBJECT.
    * @param ctx operation context
    * @param p path of the array of structure field [_within the DATAOBJECT_]
    * @param tb path of the timebase associated with the array of structure
    */
   ArraystructContext(OperationContext* ctx, std::string p, std::string tb);

   /**
    * @brief Array of structure context constructor.
    * Requires informations for describing usage of nested arrays of structure in a DATAOBJECT.
    * @param parent context of the parent array of structure 
    * @param p path of the array of structure field [_within its parent container_]
    * @param tb path of the timebase associated with the array of structure
    */
   explicit ArraystructContext(ArraystructContext* parent, std::string p, std::string tb);

   /**
    * @brief Array of structure context constructor.
    * Requires informations for describing usage of nested arrays of structure in a DATAOBJECT.
    * @param parent context of the parent array of structure 
    * @param p path of the array of structure field [_within its parent container_]
    * @param tb path of the timebase associated with the array of structure
    * @param idx index of current element for this new ArraystructContext
    */
   explicit ArraystructContext(ArraystructContext* parent, std::string p, std::string tb, int idx);

   /**
    * @brief Array of structure context destructor.
    */
   virtual ~ArraystructContext() {}

   /**
    * @brief Returns full description of the array of structure context.
    * @return std::string Full description of the array of structure context.
    */
   std::string print() const override;

   /**
    * @brief Returns the type of context.
    * @return int Type of context, CTX_ARRAYSTRUCT_TYPE
    */
   int getType() const override;

   /**
    * @brief Returns the URI.
    * @return uri::Uri URI 
    */
   uri::Uri getURI() const override;

   /**
    * @brief Returns the ID of associated backend.
    * @return int Backend ID.
    */
   int getBackendID() const override;

   /**
    * @brief Returns the name of associated backend.
    * @return std::string Name of the backend.
    */
   std::string getBackendName() const override;

   /**
    * @brief Returns the path of the array of structure.
    * This path corresponds to the absolute path in the DATAOBJECT for the topmost array of structure or in.
    * @return std::string Path of the array of structure.
    */
   std::string getPath() const;

   /**
    * @brief Returns the timebase path of the array of structure.
    * This path corresponds to the absolute path in the DATAOBJECT for the topmost array of structure or in.
    * @return std::string Timebase path of the array of structure.
    */
   std::string getTimebasePath() const;

   /**
    * @brief Returns whether the array of structure is time-dependent or not.
    * @return bool True if time-dependent, false otherwise.
    */
   bool getTimed() const; 

   /**
    * @brief Returns the context of the container array of structure.
    * @return ArraystructContext* Context of the container array of structure.
    */
   ArraystructContext* getParent();

   /**
    * @brief Returns the position of the current element of interest within the array of structure.
    * @return int Position of the current element.
    */
   int getIndex() const;

   /**
    * @brief Updates the position of the current element of interest within the array of structure.
    * @param[in] step Step size for setting the next index.
    */
   void nextIndex(int step);

   /**
    * @brief Returns the associated OperationContext.
    * @return OperationContext* Associated OperationContext.
    */
   OperationContext* getOperationContext() const;

   /**
    * @brief Returns the associated DataEntryContext.
    * @return DataEntryContext* Associated DataEntryContext.
    */
   DataEntryContext* getDataEntryContext() const;

protected:
   std::string path;                     /**< path of the array of structure */
   std::string timebase;                 /**< path of the timebase associated with the array of structure */
   ArraystructContext* parent;           /**< container of the array of structure */
   int index = 0;                        /**< position of the current element of interest within the array of structure */
   OperationContext* opctx;              /**< associated operation context **/
};

LIBRARY_API std::ostream& operator<< (std::ostream& o, Context const& ctx);
LIBRARY_API std::ostream& operator<< (std::ostream& o, DataEntryContext const& ctx);
LIBRARY_API std::ostream& operator<< (std::ostream& o, OperationContext const& ctx);
LIBRARY_API std::ostream& operator<< (std::ostream& o, ArraystructContext const& ctx);

#endif

#endif // AL_CONTEXT_H
