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
#include <set>
#include <map>
#include <mutex>
#include <complex>


/**
   Holds a plugin.
*/
class LIBRARY_API LLplugin
{ 

private:

  static void addPluginHandler(const char* name, void *plugin_handler);
  static void addDestroyPlugin(const char* name, void *destroy_plugin);
  static void addPlugin(const char* name, void *plugin);

public:

  void* plugin_handler;
  void* al_plugin;
  void* destroy_plugin;
  char* name;
  static std::map<std::string, LLplugin>  llpluginsStore;                            /**< plugins */
  static std::map<std::string, std::vector<std::string>>  boundPlugins;           /** key = field path, value=plugins names*/
  static std::map<std::string, std::vector<std::string>>  boundReadbackPlugins;
  static std::vector<std::string> readbackPlugins;
  static std::string get_operation_path;
  static std::vector<std::string> pluginsNames;
  static std::map<std::string, std::vector<std::string>> get_plugins;

  static void getFullPath(int ctxID, const char* fieldPath,  std::string &full_path, std::string &fullDataObjectName);
  static void getFullPath(int opctxID, const char* fieldPath,  std::string &full_path);
  static void getFullPathFromOperationContext(OperationContext *opctx, const char* fieldPath,  std::string &full_path);
  static bool pluginsFrameworkEnabled();
  static void checkIfPluginsFrameworkIsEnabled();

  LLplugin()
  {
    plugin_handler = NULL;
    al_plugin = NULL;
    destroy_plugin = NULL;
    name = NULL;
  }

  static void begin_global_action_plugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx);
  static void begin_slice_action_plugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx);
  static void begin_arraystruct_action_plugin(const std::string &plugin_name, int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *arraySize);
  static void end_action_plugin(int ctxID);
  static void read_data_plugin(const std::string &plugin_name, int ctx, const char* fieldPath, const char* timeBasePath, void **data, int datatype, int dim, int *size);
  static void write_data_plugin(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, void *data, int datatype, int dim, int *size);
  static void write_plugins_metadata(int ctxID);
  static void bind_readback_plugins(int ctxID); 
  static void unbind_readback_plugins(int ctxID); 
  //static al_status_t ual_close_pulse_plugins(int pulseCtx, int mode);
  //static al_status_t ual_open_pulse_plugins(int pulseCtx, int mode);
  static void register_plugin(const char* plugin_name);
  static void unregister_plugin(const char* plugin_name);
  static bool isPluginRegistered(const char* name);
  static void bindPlugin(const char* fieldPath, const char* name);
  static void unbindPlugin(const char* fieldPath, const char* name);
  static void unbindPlugin(const char* fieldPath, const char* pluginName, std::map<std::string, std::vector<std::string>> &boundPlugins_);
  static bool getBoundPlugins(const std::string &fullPath, std::vector<std::string> &pluginsNames);
  static bool getBoundPlugins(int ctxID, const char* fieldPath, std::vector<std::string> &pluginsNames);
  static bool getBoundPlugins(const char* dataobjectname, std::set<std::string> &pluginsNames);
  static bool isPluginBound(const char* path, const char* pluginName);
  static void setvalueParameterPlugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* pluginName);

};

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
  
  ArraystructContext* create(const char* path, const char* timebase); 
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
     Create an array of structure for the context ctxID.
     @param[in] ctxID current context
     @param[in] fieldPath path of the current node
     @param[in] timeBasePath time base path of the current node
     @param[out] actxID pointer to the new AOS context
     @param[out] size pointer to the size of the new AOS
  */
  static void createAOS(int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *size);
  
  /**
     Sets a variable to its value.
     @param[in] data data value passed as an opaque void*
     @param[in] type data type
     @param[in] dim data dimension
     @param[in,out] var variable which should store the data, passed as void**
  */
  static void setValue(void *data, int type, int dim, void **var);

  /**
     Sets a variable to its default value.
     @param[in] type data type
     @param[in] dim data dimension
     @param[in,out] var variable which should store the default value, passed as void**
     @param[in,out] size passed array for storing the size of each dimension (size[i] undefined if i>=dim)
  */
  static void setDefaultValue(int type, int dim, void **var, int *size);

  /**
     Converts data to the specified new type.
     @tparam From type of the data
     @param[in] data source data value
     @param[in] size total size of the data
     @param[in] desttype type ID for the converted data
     @result converted data (returned as void*)
   */
  template <typename From>
    static void* convertData(From* data, size_t size, int desttype);

  /**
     Sets a variable to a converted value.
     @param[in] data source data passed as an opaque void*
     @param[in] srctype type ID of the source data
     @param[in] dim dimension of the source/converted data
     @param[in,out] size array storing size of each dimension (size[i] undefined if i>=dim, modified when conversion is not possible) 
     @param[in] desttype desired type ID for the converted data
     @param[in,out] var variable which will store the converted value, passed as void**
   */
  static void setConvertedValue(void *data, int srctype, int dim, int *size, int desttype, void** var);

  /**
     Starts an action on a pulse in the database using an URI.
     @param[in] uri URI
     @return pulse context id [_error status < 0 or null context if = 0_]
  */
  static int beginUriAction(const std::string &uri);

  static bool data_has_non_zero_shape(int datatype, void *data, int dim , int *size);
  

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

  typedef struct 
  {
    int code;
    char message[MAX_ERR_MSG_LEN];
  } al_status_t;


  /**
     Return all the Context information corresponding to the passed Context identifier.
     @param[in] ctx Context ID (either DataEntryContext, OperationContext or ArraystructContext)
     @param[out] info context info as a string -> NEED TO BE FREEED!!
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
   */
  LIBRARY_API al_status_t ual_context_info(int ctx, char **info);


  /**
     Get backendID from the passed Context identifier.
     @param[in] ctx Context ID (either DataEntryContext, OperationContext or ArraystructContext)
     @param[out] beid backendID 
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
   */
  LIBRARY_API al_status_t ual_get_backendID(int ctx, int *beid);


  /**
     Opens a database entry.
     This function opens an IMAS data entry.
     @param[in] uri URI of the IMAS data entry
     @param[in] mode opening option:
     - OPEN_PULSE = open an existing pulse (only if exist)
     - FORCE_OPEN_PULSE = open a pulse (create it if not exist)
     - CREATE_PULSE = create a new pulse (do not overwrite if already exist)
     - FORCE_CREATE_PULSE = create a new pulse (erase old one if already exist)
     @param[out] dectxID data entry context id [_null context if = 0_] 
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_begin_dataentry_action(const char *uri, int mode, int *dectxID);

  /**
     Closes a database entry.
     This function closes a database entry described by the passed pulse context.
     @param[in] pulseCtx pulse context id (from ual_begin_dataentry_action())
     @param[in] mode closing option:
     - CLOSE_PULSE = close the pulse
     - ERASE_PULSE = close and remove the pulse 
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_close_pulse(int pulseCtx, 
					  int mode);

  /**
     Starts an I/O action on a DATAOBJECT.
     This function gives a new operation context for the duration of an action on a DATAOBJECT.
     @param[in] ctx pulse context id (from ual_begin_dataentry_action())
     @param[in] dataobjectname name of the DATAOBJECT
     @param[in] datapath path to data node for partial get operation
     @param[in] rwmode mode for this operation:
     - READ_OP = read operation
     - WRITE_OP = write operation
     @param[out] opctx operation context id [_null context if = 0_]
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_begin_global_action(int ctx,
                          const char *dataobjectname,
                          const char* datapath,
                          int rwmode,
                          int *opctx);

  /**
     Starts an I/O action on a DATAOBJECT slice.
     This function gives a new operation context for the duration of an action on a slice.  
     @param[in] ctx pulse context (from ual_begin_dataentry_action())
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
     @param[out] opctx operation context id [_null context if = 0_]
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_begin_slice_action(int ctx,
						 const char *dataobjectname,
						 int rwmode,
						 double time,
						 int interpmode,
						 int *opctx);

  /**
     Stops an I/O action.
     This function stop the current action designed by the context passed as argument. This context is then 
     not valide anymore.
     @param[in] ctx a pulse (ual_begin_dataentry_action()), an operation (ual_begin_global_action() or ual_begin_slice_action()) or an array of structure context id (ual_begin_array_struct_action())
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_end_action(int ctx); 

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
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_write_data(int ctx,
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
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_read_data(int ctx,
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
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_delete_data(int ctx,
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
     @param[out] aosctx array of structure context id [_null context if = 0_]
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
  */
  LIBRARY_API al_status_t ual_begin_arraystruct_action(int ctx,
						       const char *path,
						       const char *timebase,
						       int *size,
						       int *aosctx);


  /**
     Change current index of interest in an array of structure.
     This function updates the index pointing at the current element of interest within an array of structure.
     @param[in] aosctx array of structure Context
     @param[in] step iteration step size (typically=1)
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
   */
  LIBRARY_API al_status_t ual_iterate_over_arraystruct(int aosctx, 
						       int step);

  /**
     Builds an URI string using legacy parameters.
     @param[in] backendID name/ID of the back-end
     @param[in] shot shot number
     @param[in] run run number
     @param[in] user username 
     @param[in] tokamak tokamak name 
     @param[in] version data version 
     @param[in] options additional options (possibly backend specific)
     @param[out] uri string
     @result error status [_success if al_status_t.code = 0 or failure if < 0_]
   */
  LIBRARY_API al_status_t ual_build_uri_from_legacy_parameters(const int backendID, 
                         const int shot, 
                         const int run, 
                         const char *user, 
                         const char *tokamak, 
                         const char *version,
                         const char *options,
                         char** uri);

  LIBRARY_API al_status_t hli_begin_global_action(int pctxID, const char* dataobjectname, const char* datapath, int rwmode, int *octxID);

  LIBRARY_API al_status_t hli_begin_slice_action(int pctxID, const char* dataobjectname, int rwmode, double time, int interpmode, int *octxID);
  
  LIBRARY_API al_status_t hli_end_action(int ctxID);

  LIBRARY_API al_status_t hli_begin_arraystruct_action(int ctxID, const char *path, const char *timebase, int *size, int *actxID);

  LIBRARY_API al_status_t hli_read_data(int ctxID, const char *field, const char *timebase, void **data, int datatype, int dim, int *size);
  
  LIBRARY_API al_status_t hli_write_data(int ctxID, const char *field, const char *timebase, void *data, int datatype, int dim, int *size);

  //LIBRARY_API al_status_t hli_close_pulse(int pctxID, int mode, const char *options);
  
  //HLI wrappers for plugins API

  LIBRARY_API al_status_t hli_is_plugin_registered(const char* pluginName, bool *is_registered);

  LIBRARY_API al_status_t hli_register_plugin(const char *plugin_name);

  LIBRARY_API al_status_t hli_unregister_plugin(const char *plugin_name);

  LIBRARY_API al_status_t hli_bind_plugin(const char* fieldPath, const char* pluginName);

  LIBRARY_API al_status_t hli_unbind_plugin(const char* fieldPath, const char* pluginName);
  
  LIBRARY_API al_status_t hli_setvalue_parameter_plugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* pluginName);
  
  LIBRARY_API al_status_t hli_setvalue_int_scalar_parameter_plugin(const char* parameter_name, int parameter_value, const char* pluginName);
  
  LIBRARY_API al_status_t hli_setvalue_double_scalar_parameter_plugin(const char* parameter_name, double parameter_value, const char* pluginName);

  LIBRARY_API al_status_t hli_write_plugins_metadata(int ctxid);

  LIBRARY_API al_status_t hli_bind_readback_plugins(int ctxid);

  LIBRARY_API al_status_t hli_unbind_readback_plugins(int ctxID);

#if defined(__cplusplus)
}
#endif


#endif // UAL_LOWLEVEL_H
