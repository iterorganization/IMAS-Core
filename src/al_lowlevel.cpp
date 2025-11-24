/**
 * @file al_lowlevel.cpp
 * @brief This file contains the implementation of low-level access layer functions and plugin management for the IMAS-Core project.
 * 
 * The file provides various functions to handle plugins, manage contexts, and perform data operations such as reading, writing, and deleting data.
 * It also includes functions to handle array structures and manage backend operations.
 * 
 * @author iterorganization
 * 
 * @details
 * The file includes the following functionalities:
 * - Plugin management: Registering, unregistering, binding, and unbinding plugins.
 * - Context management: Adding, retrieving, and deleting low-level environments (contexts).
 * - Data operations: Reading, writing, and deleting data using the backend.
 * - Array structure operations: Handling array structures and iterating over them.
 * - Backend operations: Initializing and managing backend operations.
 * 
 * The file also provides C wrappers for the implemented functions to facilitate their usage in C programs.
 * 
 * @note
 * The file uses various external libraries and dependencies such as Boost, dlfcn, and regex.
 * 
 * @warning
 * The file contains several exception handling mechanisms to ensure proper error handling and reporting.
 * 
 * @see
 * - al_lowlevel.h
 * - access_layer_plugin.h
 * - extended_access_layer_plugin.h
 * - access_layer_plugin_manager.h
 */


#include "al_lowlevel.h"

#include "dlfcn.h"
#include "access_layer_plugin.h"
#include "extended_access_layer_plugin.h"
#include "access_layer_plugin_manager.h"
#include <boost/filesystem.hpp>

#include <assert.h>
#include <string.h>
#include <complex.h>
#include <algorithm>
#include <regex>

#include <signal.h>

#define CREATION_DATE_PLUGIN "creation_date"
#define CREATION_DATE_NODE "ids_properties/creation_date"

#define PARTIAL_GET_PLUGIN "partial_get"

#if defined(_MSC_VER)
#  define mempcpy memcpy
#endif

#if defined(__APPLE__)
  #define mempcpy memcpy
#endif

// c++ only part
#if defined(__cplusplus)

#define STORE_CHUNCK 100

std::mutex Lowlevel::mutex;

int Lowlevel::curStoreElt = 1;
int Lowlevel::maxStoreElt = 1; //STORE_CHUNCK;
std::vector<LLenv> Lowlevel::llenvStore = { LLenv()}; //(STORE_CHUNCK);

const char Lowlevel::EMPTY_CHAR = '\0';
const int Lowlevel::EMPTY_INT   = -999999999;
const double Lowlevel::EMPTY_DOUBLE = -9.0E40;
const std::complex<double> Lowlevel::EMPTY_COMPLEX = std::complex<double>(-9.0E40,-9.0E40);

std::map<std::string, LLplugin> LLplugin::llpluginsStore;
std::map<std::string, std::vector<std::string>>  LLplugin::boundPlugins;
std::map<std::string, std::vector<std::string>>  LLplugin::boundReadbackPlugins;
std::map<std::string, std::vector<std::string>>  LLplugin::boundToFullPathPlugins;
std::vector<std::string> LLplugin::readbackPlugins;
std::string LLplugin::getOperationPath;
std::vector<std::string> LLplugin::pluginsNames;
std::map<std::string, std::vector<std::string>> LLplugin::get_plugins;

/**
 * @brief Adds a plugin handler to the llpluginsStore.
 *
 * This function associates a plugin handler with a given name and stores it
 * in the llpluginsStore map. The name is converted to a std::string and used
 * as the key, while the plugin_handler is stored as the value.
 *
 * @param name The name of the plugin handler to be added.
 * @param plugin_handler A pointer to the plugin handler to be stored.
 */
void LLplugin::addPluginHandler(const char* name, void *plugin_handler) {
  llpluginsStore[std::string(name)].plugin_handler = plugin_handler;
}

/**
 * @brief Adds a destroy plugin function to the plugin store.
 *
 * This function associates a destroy plugin function with a given plugin name
 * and stores it in the llpluginsStore map.
 *
 * @param name The name of the plugin.
 * @param destroy_plugin A pointer to the destroy plugin function.
 */
void LLplugin::addDestroyPlugin(const char* name, void *destroy_plugin) {
  llpluginsStore[std::string(name)].destroy_plugin = destroy_plugin;
}

/**
 * @brief Adds a plugin to the plugin store.
 *
 * This function registers a plugin in the plugin store with the given name.
 * If the provided plugin is NULL, an exception is thrown with an appropriate
 * error message indicating that the plugin cannot be registered.
 *
 * @param name The name of the plugin to be added.
 * @param plugin A pointer to the plugin to be added. Must not be NULL.
 *
 * @throws ALBackendException if the plugin is NULL.
 */
void LLplugin::addPlugin(const char* name, void *plugin) {
  if (!plugin) {
    char error_message[200];
    sprintf(error_message, "Plugin %s is NULL and can not be registered in the plugins store. Is the plugin implementing the create() function?\n", name);
    throw ALBackendException(error_message, LOG);
  }
  llpluginsStore[std::string(name)].al_plugin = plugin;
}

/**
 * @brief Constructs the full path and data object name for a given context and field path.
 *
 * This function constructs the full path and data object name based on the provided context ID and field path.
 * It handles both ArraystructContext and OperationContext types.
 *
 * @param ctxID The context ID used to retrieve the low-level environment.
 * @param fieldPath The field path to be appended to the constructed path.
 * @param full_path Reference to a string where the constructed full path will be stored.
 * @param fullDataObjectName Reference to a string where the constructed full data object name will be stored.
 */
void LLplugin::getFullPath(int ctxID, const char* fieldPath,  std::string &full_path, std::string &fullDataObjectName) {

  LLenv lle = Lowlevel::getLLenv(ctxID);
  std::string path = "";
  OperationContext *opctx = nullptr;
  if (lle.context->getType() == CTX_ARRAYSTRUCT_TYPE) {
    ArraystructContext *c =dynamic_cast<ArraystructContext*> (lle.context);
    opctx = c->getOperationContext();
    fullDataObjectName = opctx->getDataobjectName();
    size_t found_sep = fullDataObjectName.find("/");
    if (found_sep == std::string::npos) 
        fullDataObjectName += ":0";
    else
        fullDataObjectName = std::regex_replace(fullDataObjectName, std::regex("/"), ":");
    path = c->getPath();
    c = c->getParent();
    while (c != NULL && c->getType() == CTX_ARRAYSTRUCT_TYPE) {
      path = c->getPath() + "/" + path;
      c = c->getParent();
    } 
    path = fullDataObjectName + "/" + path;
  }
  else {
    opctx = dynamic_cast<OperationContext*> (lle.context);
    fullDataObjectName = opctx->getDataobjectName();
    size_t found_sep = fullDataObjectName.find("/");
    if (found_sep == std::string::npos) 
         fullDataObjectName += ":0";
    else
        fullDataObjectName = std::regex_replace(fullDataObjectName, std::regex("/"), ":");
    path = fullDataObjectName;
  }
  
  full_path = path + "/" + std::string(fieldPath);
}

/**
 * @brief Checks if the plugins framework is enabled.
 *
 * This function reads the environment variable "IMAS_AL_ENABLE_PLUGINS" to determine
 * if the plugins framework is enabled. If the environment variable is set to "TRUE",
 * the function returns true, indicating that the plugins framework is enabled.
 * Otherwise, it returns false.
 *
 * @return true if the plugins framework is enabled, false otherwise.
 */
bool LLplugin::pluginsFrameworkEnabled(){
  char *pluginsFrameworkEnabled =  getenv("IMAS_AL_ENABLE_PLUGINS");
  if (!pluginsFrameworkEnabled) 
       return false;
  std::string b = std::string(pluginsFrameworkEnabled);
  return  (b == "TRUE");
} 

/**
 * @brief Checks if the plugins framework is enabled.
 *
 * This function verifies whether the plugins framework is enabled by calling
 * the `pluginsFrameworkEnabled()` function. If the framework is not enabled,
 * it throws an `ALLowlevelException` with a message indicating that the plugins
 * feature is disabled and provides instructions on how to enable it.
 *
 * @throws ALLowlevelException if the plugins framework is not enabled.
 */
void LLplugin::checkIfPluginsFrameworkIsEnabled(){
  if(!pluginsFrameworkEnabled())
       throw ALLowlevelException("Plugins feature is disabled. Set the global variable 'IMAS_AL_ENABLE_PLUGINS' to 'TRUE' to enable this feature.");
}

/**
 * @brief Retrieves the names of bound plugins for a given context and field path.
 *
 * This function attempts to retrieve the names of plugins bound to a specified field path
 * within a given context. It first checks if the plugins framework is enabled and if the
 * field path is not a serialized buffer. It then constructs the full path and data object
 * name and attempts to get the bound plugins for the full path. If no plugins are found,
 * it searches for plugins using a wildcard path pattern and binds them to the current path.
 *
 * @param ctxID The context ID.
 * @param fieldPath The field path for which to retrieve bound plugins.
 * @param pluginsNames A reference to a vector that will be populated with the names of bound plugins.
 * @return true if bound plugins are found and retrieved, false otherwise.
 */
bool LLplugin::getBoundPlugins(int ctxID, const char* fieldPath, std::vector<std::string> &pluginsNames) {
	if(!pluginsFrameworkEnabled()) return false;
  if(std::string(fieldPath) == "<buffer>") return false;  // Skip plugins when storing serialized buffer in Serialization Backend
  std::string fullPath;
  std::string dataObjectName;
  getFullPath(ctxID, fieldPath, fullPath, dataObjectName);
  //printf("--> getBoundPlugins::fullPath = %s\n ", fullPath.c_str());

  bool t = getBoundPlugins(fullPath, pluginsNames);
  if (t)
    return true;

  //we are searching now path like :"ids_name:occ/*" where occ is the occurrence and * is representing all nodes
  std::string path = dataObjectName + "/*";
  t = getBoundPlugins(path, pluginsNames);

  //we bind these plugins to the current path=fieldPath
  if (t) {
    for (auto& pluginName:pluginsNames) {
      //printf("binding plugin=%s to path=%s\n", pluginName.c_str(), fullPath.c_str());
      bind_plugin(pluginName.c_str(), fullPath, boundPlugins);
    }
    return true;
  }

  //we are now searching now path like :"ids_name:*/*" where latest * is representing all nodes
  size_t index = dataObjectName.find(":");
  path = dataObjectName.substr(0, index - 1)  + ":*/*";
  t = getBoundPlugins(path, pluginsNames);

  if (t) {
    for (auto& pluginName:pluginsNames)
      bind_plugin(pluginName.c_str(), fullPath, boundPlugins);
    return true;
  }

  return false;
}

/**
 * @brief Retrieves the names of bound plugins for a given file path.
 *
 * This function checks if the plugins framework is enabled and then searches
 * for the specified file path in the bound plugins map. If found, it populates
 * the provided vector with the names of the bound plugins.
 *
 * @param fullPath The full path of the file for which bound plugins are to be retrieved.
 * @param pluginsNames A reference to a vector that will be populated with the names of the bound plugins.
 * @return true if the plugins framework is enabled and the file path is found in the bound plugins map, false otherwise.
 */
bool LLplugin::getBoundPlugins(const std::string &fullPath, std::vector<std::string> &pluginsNames) {
  if(!pluginsFrameworkEnabled()) return false;
  auto got = boundPlugins.find(fullPath);
  if (got != boundPlugins.end()) {
    pluginsNames = got->second;
    return true;
  }
  return false;
}

/**
 * @brief Retrieves the names of plugins bound to a specified data object.
 *
 * This function checks if the plugins framework is enabled and then searches for plugins
 * bound to the given data object name. If found, it populates the provided set with the
 * names of these plugins.
 *
 * @param dataobjectname The name of the data object to search for bound plugins.
 * @param pluginsNames A reference to a set of strings where the names of the bound plugins will be stored.
 * @return true if one or more plugins are found and added to the set, false otherwise.
 */
bool LLplugin::getBoundPlugins(const char* dataobjectname, std::set<std::string> &pluginsNames) {
  if(!pluginsFrameworkEnabled()) return false;
  std::string fullDataObjectName(dataobjectname);
  size_t found = fullDataObjectName.find("/");
  if (found == std::string::npos)
    fullDataObjectName += ":0";
  else
    fullDataObjectName = std::regex_replace(fullDataObjectName, std::regex("/"), ":");
    
  for(auto it = boundPlugins.begin(); it != boundPlugins.end(); ++it) {
      const std::string &key = it->first;
      if (key.rfind(fullDataObjectName, 0) != std::string::npos) {
          std::vector<std::string> &plugins = it->second; 
          for (auto &pluginName:plugins) 
            pluginsNames.insert(pluginName);
          return pluginsNames.size() > 0;
      }
  }
  return false;
}

/**
 * @brief Binds a plugin to a specified field path.
 *
 * This function binds a plugin to a given field path. It first checks if the plugin framework is enabled
 * and if the plugin is registered. If the plugin is not registered, it throws an exception. The function
 * supports binding to all nodes or specific nodes based on the provided field path.
 *
 * @param fieldPath The field path to which the plugin should be bound. The field path can follow different
 *                  patterns such as #idsname[:occurrence][/*] or #idsname[:occurrence][/idspath].
 * @param pluginName The name of the plugin to be bound.
 *
 * @throws ALLowlevelException if the plugin is not registered or if the field path format is incorrect.
 */
void LLplugin::bindPlugin(const char* fieldPath, const char* pluginName) {
    checkIfPluginsFrameworkIsEnabled();
    if (!isPluginRegistered(pluginName)) {
        char error_message[200];
        sprintf(error_message, "Plugin %s is not registered. Plugins need to be registered using al_register_plugin(name) before to be bound.\n", pluginName);
        throw ALLowlevelException(error_message, LOG);
    }
    std::string fieldPath_str(fieldPath);
    std::string idspath = "/";
    std::string idsname;
    std::smatch match;

    std::string full_path;

    std::regex pattern_all_nodes("#?([a-zA-Z0-9_\\-]*)(:([0-9]*))?/\\*"); //pattern for #idsname[:occurrence][/*]
    if (std::regex_match(fieldPath_str, match, pattern_all_nodes)) {
       idsname = match[1];

    //std::cout << "match1=" << match[1] << ";match2=" << match[2] << ";match3=" << match[3] << std::endl;
       //printf("idsname=%s\n",idsname.c_str());

       std::string occurrence = match[3];
       if (occurrence.empty()){
            idsname += ":0";
          } 
          else {
            idsname += ":" + occurrence;
          }
      idsname += "/*";
      printf("all IDS nodes are bound, path=%s, plugin=%s\n", idsname.c_str(), pluginName);
      bind_plugin(pluginName, idsname, boundPlugins);
      return;

    }

    std::regex pattern("#?(([a-zA-Z0-9_\\-]*)(:([0-9]*))?/([:,()/a-zA-Z0-9_\\-]*))?"); //pattern for #idsname[:occurrence][/idspath]

    if (std::regex_match(fieldPath_str, match, pattern)) {
        idsname = match[2];
        //std::cout<<"idsname value : "<<idsname<<std::endl;
        idspath += match[5];
        std::regex ids_path_pattern("^(/([a-zA-Z0-9_-]+))*(/([a-zA-Z0-9_-]+)(?:\\(([:,]+)\\))?(?=(?:$)|(?:\\/[a-zA-Z0-9_-]+)))*/?([a-zA-Z0-9_-]+)?");
        std::smatch idspath_match;

        if (std::regex_match(idspath, idspath_match, ids_path_pattern)) {
          //std::cout << idspath << std::endl;
          idspath = std::regex_replace(idspath, std::regex("\\([:,]+\\)"), std::string(""));
          //std::cout << idspath << ", after suppression" <<  std::endl;
          
          std::string occurrence = match[4];
          //std::cout<<"occurrence value : "<<occurrence<<std::endl;
          if (occurrence.empty()){
            idsname += ":0";
          } 
          else {
            idsname += ":" + occurrence;
          }
          full_path = idsname + idspath;

        } else {
          char error_message[200];
          sprintf(error_message, "bindPlugin: bad format: (%s) should follow the syntax of an IDS path.\n", idspath.c_str());
          throw ALLowlevelException(error_message, LOG);
      }
    } else {
        char error_message[200];
        sprintf(error_message, "bindPlugin!!!: bad format: (%s) should follow the syntax of an URI fragment.\n", fieldPath);
        throw ALLowlevelException(error_message, LOG);
    }

    bind_plugin(pluginName, full_path, boundPlugins);

}

/**
 * @brief Binds a plugin to a specified path.
 *
 * This function binds a plugin identified by `pluginName` to a given `path`.
 * If the plugin is already bound to the path, an exception is thrown.
 * Otherwise, the plugin is added to the list of plugins bound to the path.
 *
 * @param pluginName The name of the plugin to bind.
 * @param path The path to which the plugin should be bound.
 * @param bound_plugins A map that keeps track of paths and their associated plugins.
 *
 * @throws ALLowlevelException if the plugin is already bound to the specified path.
 */
void LLplugin::bind_plugin(const char* pluginName, const std::string &path, std::map<std::string, std::vector<std::string>>& bound_plugins) {
    auto got = bound_plugins.find(path);
    if (got != boundPlugins.end()) {
		    auto &plugins = got->second;
        auto got2 = std::find(plugins.begin(), plugins.end(), pluginName);
        if ( got2 != plugins.end()) {
            char error_message[200];
            sprintf(error_message, "Plugin %s is already bound to path: %s.\n", pluginName, path.c_str());
            throw ALLowlevelException(error_message, LOG);
        }
        else
           plugins.push_back(pluginName);
    }
    else {
		    std::vector<std::string> plugins {pluginName};
        //printf("bindPlugin::binding plugin %s to path=%s\n", pluginName, full_path.c_str());
		    boundPlugins[path] = plugins;
    } 
}

/**
 * @brief Checks if a plugin is bound to a specified path.
 *
 * This function verifies whether a plugin with the given name is bound to the specified path.
 * It first checks if the plugins framework is enabled. If not, it returns false.
 * Then, it searches for the path in the bound plugins map. If the path is found,
 * it checks if the plugin name exists in the list of plugins bound to that path.
 *
 * @param path The file path to check for the bound plugin.
 * @param pluginName The name of the plugin to check.
 * @return true if the plugin is bound to the specified path, false otherwise.
 */
bool LLplugin::isPluginBound(const char* path, const char* pluginName){
    if(!pluginsFrameworkEnabled())
      return false;
    auto got = boundPlugins.find(std::string(path));
    if (got != boundPlugins.end()) {
		    auto &plugins = got->second;
        auto got2 = std::find(plugins.begin(), plugins.end(), pluginName);
        return got2 != plugins.end();
    }
    return false;
} 

/**
 * @brief Unbinds a plugin from a specified field path.
 *
 * This function unbinds a plugin identified by its name from a given field path.
 *
 * @param fieldPath The path of the field from which the plugin should be unbound.
 * @param pluginName The name of the plugin to be unbound.
 */
void LLplugin::unbindPlugin(const char* fieldPath, const char* pluginName) {
    unbindPlugin(fieldPath, pluginName, boundPlugins);
}

/**
 * @brief Unbinds a plugin from a specified field path.
 *
 * This function removes the binding of a plugin identified by `pluginName` from the field path
 * specified by `fieldPath`. If the plugin is successfully unbound and no other plugins are bound
 * to the same field path, the field path entry is removed from the `boundPlugins_` map.
 *
 * @param fieldPath The field path from which the plugin should be unbound.
 * @param pluginName The name of the plugin to unbind.
 * @param boundPlugins_ A reference to a map containing field paths and their associated plugins.
 */
void LLplugin::unbindPlugin(const char* fieldPath, const char* pluginName, std::map<std::string, std::vector<std::string>> &boundPlugins_) {
    auto got = boundPlugins_.find(std::string(fieldPath));
    if (got == boundPlugins_.end()) {
        printf("No plugin bound to field path: %s\n", fieldPath);
    }
    else {
		std::vector<std::string> &plugins = got->second;
		auto itr = std::find(plugins.begin(), plugins.end(), std::string(pluginName));
        if (itr != plugins.end()) {
            plugins.erase(itr);
            if (plugins.size() == 0) {
                  boundPlugins_.erase(got);
            }  
		      }
    }
}

/**
 * @brief Checks if a plugin is registered.
 *
 * This function checks if a plugin with the given name is registered in the 
 * plugin store. It first ensures that the plugins framework is enabled before 
 * performing the check.
 *
 * @param name The name of the plugin to check for registration.
 * @return true if the plugin is registered, false otherwise.
 */
bool LLplugin::isPluginRegistered(const char* name) {
   checkIfPluginsFrameworkIsEnabled();
   return llpluginsStore.find(std::string(name)) != llpluginsStore.end();
}

/**
 * @brief Registers a plugin with the given name.
 *
 * This function registers a plugin by loading its shared library and storing
 * its handler and symbols in the plugin store. It performs several checks to
 * ensure the plugin framework is enabled, the plugin is not already registered,
 * and the shared library exists.
 *
 * @param plugin_name The name of the plugin to register.
 * @return true if the plugin is successfully registered.
 * @throws ALLowlevelException if the IMAS_AL_PLUGINS environment variable is not defined,
 *         if the plugin is already registered, if the plugin shared library is not found,
 *         or if the required symbols cannot be loaded.
 */
bool LLplugin::registerPlugin(const char* plugin_name) {
    checkIfPluginsFrameworkIsEnabled();
    const char* IMAS_AL_PLUGINS = std::getenv("IMAS_AL_PLUGINS");
    if (IMAS_AL_PLUGINS == NULL)
        throw ALLowlevelException("IMAS_AL_PLUGINS environment variable not defined",LOG);

    if (isPluginRegistered(plugin_name)) {
        char error_message[200];
        sprintf(error_message, "Plugin %s already registered in the plugins store.\n", plugin_name);
        throw ALLowlevelException(error_message, LOG);
    }
  
    std::string ids_plugin = std::string(IMAS_AL_PLUGINS) + "/" + plugin_name + "_plugin.so";
    if (!boost::filesystem::exists(ids_plugin.c_str())) { 
        char error_message[200];
        sprintf(error_message, "Plugin shared library %s not found", ids_plugin.c_str());
        throw ALLowlevelException(error_message, LOG); 
    }

    llpluginsStore[std::string(plugin_name)] = LLplugin();
    LLplugin &lle = llpluginsStore[std::string(plugin_name)];
    void* plugin_handler = lle.plugin_handler;

    create_t* create_plugin = NULL;
    destroy_t* destroy_plugin = NULL;

    //printf("-->ids_plugins:%s\n", ids_plugin.c_str());
    plugin_handler =  dlopen(ids_plugin.c_str(), RTLD_LAZY);
    if (plugin_handler == nullptr) {
        //char error_message[200];
        //sprintf(error_message, "%s for plugin: %s.\n", dlerror(), plugin_name);
        printf("An error has occurred:%s for plugin: %s\n", dlerror(), plugin_name);
        //throw ALLowlevelException(error_message, LOG);
    }
    assert(plugin_handler != nullptr);
    addPluginHandler(plugin_name, plugin_handler);
    //load the symbols
    create_plugin = (create_t*) dlsym(plugin_handler, "create");
    if (!create_plugin) {
        char error_message[200];
        sprintf(error_message, "Cannot load symbol create:%s for plugin:%s.\n", dlerror(), plugin_name);
        throw ALLowlevelException(error_message, LOG);
    }
    destroy_plugin = (destroy_t*) dlsym(plugin_handler, "destroy");
    if (!destroy_plugin) {
        char error_message[200];
        sprintf(error_message, "Cannot load symbol destroy:%s for plugin:%s.\n", dlerror(), plugin_name);
        throw ALLowlevelException(error_message, LOG);
    }
    
    dlerror();
    access_layer_base_plugin* al_plugin = (access_layer_base_plugin*) lle.al_plugin;
    al_plugin = create_plugin();
    addPlugin(plugin_name, al_plugin);
    addDestroyPlugin(plugin_name, (void*) destroy_plugin);
    return true;
}


/**
 * @brief Unregisters a plugin from the plugins framework.
 *
 * This function removes the specified plugin from the plugins store and 
 * erases all paths bound to this plugin from the boundPlugins map. If the 
 * plugin is not registered, an exception is thrown.
 *
 * @param plugin_name The name of the plugin to unregister.
 *
 * @throws ALLowlevelException if the plugin is not registered in the plugins store.
 */
void LLplugin::unregisterPlugin(const char *plugin_name)
{
    checkIfPluginsFrameworkIsEnabled();
    if (!isPluginRegistered(plugin_name))
    {
        char error_message[200];
        sprintf(error_message, "Plugin %s not registered in the plugins store.\n", plugin_name);
        throw ALLowlevelException(error_message, LOG);
    }
    // Erasing all paths bound to this plugin from the boundPlugins map
    auto it = boundPlugins.begin();
    std::set<std::string> paths;
    while (it != boundPlugins.end())
    {
        std::vector<std::string> &plugins = it->second;
        auto itr = std::find(plugins.begin(), plugins.end(), plugin_name);
        if (itr != plugins.end())
        {
            const std::string &fieldPath = it->first;
            paths.insert(fieldPath);
            auto got = llpluginsStore.find(plugin_name);
            if (got != llpluginsStore.end())
            {
                  LLplugin &llp = got->second;
                  access_layer_plugin *al_plugin_ptr = (access_layer_plugin *)llp.al_plugin;
                  if (al_plugin_ptr != NULL)
                  {
                    // Deleting plugin instance
                    destroy_t *destroy = (destroy_t *)llp.destroy_plugin;
                    if (destroy != NULL)
                    {
                      destroy(al_plugin_ptr);
                      llpluginsStore.erase(got);
                    }
                  }
            }
        }
        it++;
    }
    for(auto it = paths.begin(); it != paths.end(); ++it) {
      auto got = boundPlugins.find(*it);
      if (got == boundPlugins.end()) continue;
      auto &v = got->second; //vector of plugins names for field path=*it
      auto p = std::find(v.begin(), v.end(), plugin_name);
      if (p != v.end())
          v.erase(p);
      if (v.size() == 0)
          boundPlugins.erase(*it);
    } 
}

void LLplugin::register_core_plugins(int pctxID, const char* dataobjectname, int rwmode, int *octxID)
{
  /*if (rwmode == WRITE_OP) {
    if (!LLplugin::isPluginRegistered(CREATION_DATE_PLUGIN))
        LLplugin::registerPlugin(CREATION_DATE_PLUGIN);

    std::string fullDataObjectName = dataobjectname;
    size_t found_sep = fullDataObjectName.find("/");
    if (found_sep == std::string::npos)
        fullDataObjectName += ":0";
    else
        fullDataObjectName = std::regex_replace(fullDataObjectName, std::regex("/"), ":");
    LLplugin::bindPlugin((fullDataObjectName + "/" + std::string(CREATION_DATE_NODE)).c_str(), CREATION_DATE_PLUGIN);
  }
  else if (rwmode == READ_OP) {
    if (!LLplugin::isPluginRegistered(PARTIAL_GET_PLUGIN))
        LLplugin::registerPlugin(PARTIAL_GET_PLUGIN);
  }*/
}

/**
 * @brief Sets the value of a parameter in the specified plugin.
 *
 * This function sets the value of a parameter in the plugin identified by the given plugin name.
 *
 * @param parameter_name The name of the parameter to set.
 * @param datatype The data type of the parameter.
 * @param dim The dimension of the parameter.
 * @param size An array representing the size of each dimension.
 * @param data A pointer to the data to be set for the parameter.
 * @param plugin_name The name of the plugin where the parameter is to be set.
 */
void LLplugin::setvalueParameterPlugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* plugin_name) {
	LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->setParameter(parameter_name, datatype, dim, size, data);
}

/**
 * @brief Begins a global action for a specified plugin.
 *
 * This function initiates a global action for the plugin identified by the given name.
 *
 * @param plugin_name The name of the plugin for which the global action is to be started.
 * @param pulseCtx The pulse context identifier.
 * @param dataobjectname The name of the data object involved in the action.
 * @param datapath The path to the data object.
 * @param mode The mode in which the action is to be performed.
 * @param opCtx The operation context identifier.
 */
void LLplugin::beginGlobalActionPlugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->begin_global_action(pulseCtx, dataobjectname, datapath, mode, opCtx);
}

/**
 * @brief Begins a slice action for a specified plugin.
 *
 * This function initiates a slice action for the given plugin by calling the
 * corresponding method in the access layer plugin.
 *
 * @param plugin_name The name of the plugin to begin the slice action for.
 * @param pulseCtx The pulse context identifier.
 * @param dataobjectname The name of the data object.
 * @param mode The mode of the slice action.
 * @param time The time at which the slice action is to be performed.
 * @param interp The interpolation method to be used.
 * @param opCtx The operation context identifier.
 */
void LLplugin::beginSliceActionPlugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->begin_slice_action(pulseCtx, dataobjectname, mode, time, interp, opCtx);
}

/**
 * @brief Begins a time range action for a specified plugin.
 *
 * This function initiates a time range action for a plugin identified by its name.
 * It checks if the plugin implements the extended access layer plugin interface and
 * calls the appropriate method if available.
 *
 * @param plugin_name The name of the plugin.
 * @param pulseCtx The pulse context identifier.
 * @param dataobjectname The name of the data object.
 * @param mode The mode of operation.
 * @param tmin The minimum time value.
 * @param tmax The maximum time value.
 * @param dtime A vector of time values.
 * @param interp The interpolation method.
 * @param opCtx The operation context identifier.
 *
 * @throws ALLowlevelException if the plugin does not implement the extended access layer plugin interface.
 */
void LLplugin::beginTimeRangeActionPlugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, double tmin, double tmax, std::vector<double> dtime, int interp, int opCtx) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* base_plugin = static_cast<access_layer_plugin*>(llp.al_plugin);
  extended_access_layer_plugin* al_plugin = dynamic_cast<extended_access_layer_plugin*> (base_plugin);
  if (al_plugin != nullptr) {
      al_plugin->begin_timerange_action(pulseCtx, dataobjectname, mode, tmin, tmax, dtime, interp, opCtx);
  } else {
      // Apparently this plugin does not implement the begin_timerange_action method from the extended_access_layer_plugin interface
      //char message[200]; 
      //sprintf(message, "Plugin %s does not implement the new interface for time range operations.", plugin_name.c_str());
      //throw ALLowlevelException(message,LOG);
      printf("Plugin %s does not implement the interface including time range operations.", plugin_name.c_str());
  }
  
}

/**
 * @brief Begins an array structure action for a plugin.
 *
 * This function initializes an array structure action for a specified plugin
 * by invoking the corresponding handler in the AccessLayerPluginManager.
 *
 * @param plugin_name The name of the plugin.
 * @param ctxID The context ID.
 * @param actxID Pointer to the action context ID.
 * @param fieldPath The path to the field.
 * @param timeBasePath The path to the time base.
 * @param arraySize Pointer to the size of the array.
 */
void LLplugin::beginArraystructActionPlugin(const std::string &plugin_name, int ctxID, int *actxID, 
const char* fieldPath, const char* timeBasePath, int *arraySize) {
  AccessLayerPluginManager alplugin_manager;
  alplugin_manager.begin_arraystruct_action_handler(plugin_name, ctxID, actxID, fieldPath, timeBasePath, arraySize);
}

/**
 * @brief Ends the action plugin for the given context ID.
 *
 * This function checks if the plugins framework is enabled. If it is not enabled,
 * the function returns immediately. Otherwise, it creates an instance of 
 * AccessLayerPluginManager and calls the end_action_plugin_handler method with 
 * the provided context ID.
 *
 * @param ctxID The context ID for which the action plugin should be ended.
 */
void LLplugin::endActionPlugin(int ctxID)
{
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.end_action_plugin_handler(ctxID);
}

/**
 * @brief Reads data from a plugin and handles the data accordingly.
 *
 * This function interfaces with the AccessLayerPluginManager to read data from a specified plugin.
 * If the data read is unsuccessful (indicated by a return value of 0), it sets the data to a default value.
 *
 * @param plugin_name The name of the plugin from which to read data.
 * @param ctxID The context ID for the plugin operation.
 * @param field The specific field within the plugin to read data from.
 * @param timebase The timebase associated with the data.
 * @param data A pointer to the location where the read data will be stored.
 * @param datatype The type of the data being read.
 * @param dim The dimension of the data.
 * @param size An array representing the size of each dimension of the data.
 */
void LLplugin::readDataPlugin(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void **data, int datatype, int dim, int *size)
{
    AccessLayerPluginManager alplugin_manager;
    int ret = alplugin_manager.read_data_plugin_handler(plugin_name, ctxID, field, timebase, data, datatype, dim, size);
    if (ret == 0) {
        // no data
        Lowlevel::setDefaultValue(datatype, dim, data, size);
    }
}

/**
 * @brief Binds readback plugins for the given context ID if the plugins framework is enabled.
 * 
 * This function is called before a get() operation. It checks if the plugins framework is enabled,
 * and if so, it creates an instance of AccessLayerPluginManager and binds the readback plugins
 * for the specified context ID.
 * 
 * @param ctxID The context ID for which the readback plugins should be bound.
 */
void LLplugin::bindReadbackPlugins(int ctxID) { //function called before a get()
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.bind_readback_plugins(ctxID);
}

/**
 * @brief Unbinds readback plugins for a given context ID.
 *
 * This function is called after a get() operation to unbind any readback plugins
 * associated with the specified context ID. It first checks if the plugins framework
 * is enabled. If not, the function returns immediately. Otherwise, it creates an
 * instance of AccessLayerPluginManager and calls its unbind_readback_plugins method
 * with the provided context ID.
 *
 * @param ctxID The context ID for which the readback plugins should be unbound.
 */
void LLplugin::unbindReadbackPlugins(int ctxID) { //function called after a get()
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.unbind_readback_plugins(ctxID);
}

/**
 * @brief Writes metadata for plugins associated with the given context ID.
 *
 * This function is called at the end of a put() operation. It checks if the
 * plugins framework is enabled, and if so, it creates an instance of 
 * AccessLayerPluginManager and calls its write_plugins_metadata method with 
 * the provided context ID.
 *
 * @param ctxID The context ID for which the plugins metadata should be written.
 */
void LLplugin::writePluginsMetadata(int ctxID) { //function called at the end of a put()
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.write_plugins_metadata(ctxID);
}

/**
 * @brief Writes data to a plugin.
 *
 * This function interfaces with the AccessLayerPluginManager to write data to a specified plugin.
 *
 * @param plugin_name The name of the plugin to which data will be written.
 * @param ctxID The context ID associated with the data.
 * @param field The field within the plugin where the data will be written.
 * @param timebase The timebase associated with the data.
 * @param data A pointer to the data to be written.
 * @param datatype The type of the data being written.
 * @param dim The number of dimensions of the data.
 * @param size An array representing the size of each dimension of the data.
 */
void LLplugin::writeDataPlugin(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void *data, int datatype, int dim, int *size)
{
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.write_data_plugin_handler(plugin_name, ctxID, field, timebase, 
              data, datatype, dim, size);
}

/**
 * @brief Adds a new LLenv object to the llenvStore.
 *
 * This function creates a new LLenv object using the provided Backend and Context pointers.
 * It then adds this object to the llenvStore in an atomic operation protected by a mutex.
 * If the current store element index is equal to the maximum store element index, the new
 * LLenv object is pushed to the back of the llenvStore and the maximum store element index
 * is incremented. Otherwise, the existing LLenv object at the current store element index
 * is updated with the new Backend and Context pointers.
 *
 * @param be Pointer to the Backend object.
 * @param ctx Pointer to the Context object.
 * @return The index of the current store element before incrementing.
 */
int Lowlevel::addLLenv(Backend *be, Context *ctx)
{
  LLenv lle = LLenv(be,ctx);

  // atomic operation
  std::lock_guard<std::mutex> guard(Lowlevel::mutex);

  if (Lowlevel::curStoreElt == Lowlevel::maxStoreElt)
    {
      llenvStore.push_back(lle);
      Lowlevel::maxStoreElt++;
    }
  else
    {
      Lowlevel::llenvStore[Lowlevel::curStoreElt].backend = be;
      Lowlevel::llenvStore[Lowlevel::curStoreElt].context = ctx;
    }

  return Lowlevel::curStoreElt++;
}

/**
 * @brief Retrieves the low-level environment (LLenv) from the store based on the given index.
 *
 * This function attempts to fetch the LLenv object from the llenvStore using the provided index.
 * If the context within the retrieved LLenv object is NULL, or if any exception occurs during
 * retrieval, an ALLowlevelException is thrown.
 *
 * @param idx The index of the LLenv object to retrieve from the llenvStore.
 * @return The LLenv object corresponding to the given index.
 * @throws ALLowlevelException If the context is NULL or if the index is out of range.
 */
LLenv Lowlevel::getLLenv(int idx)
{
  LLenv lle;
  try {
    lle = llenvStore.at(idx);
    if (lle.context == NULL)
      throw ALLowlevelException("Cannot find context "+std::to_string(idx)+
				 " in store",LOG);
  }
  catch (const std::exception& e) {
    throw ALLowlevelException("Cannot find context "+std::to_string(idx)+
			       " in store",LOG);
  }
  return lle;
}

/**
 * @brief Deletes a low-level environment (LLenv) from the store at the specified index.
 *
 * This function performs an atomic operation to safely remove an LLenv from the store.
 * It locks the mutex to ensure thread safety, retrieves the LLenv at the given index,
 * sets the backend and context of the LLenv at that index to NULL, and decrements the
 * current store element counter if the index is the last element in the store.
 *
 * @param idx The index of the LLenv to be deleted.
 * @return The LLenv that was deleted from the store.
 */
LLenv Lowlevel::delLLenv(int idx)
{
  // atomic operation
  std::lock_guard<std::mutex> guard(Lowlevel::mutex);

  LLenv lle = llenvStore.at(idx);

  llenvStore[idx].backend = NULL;
  llenvStore[idx].context = NULL;
  if (idx == Lowlevel::curStoreElt-1)
    Lowlevel::curStoreElt--;

  return lle;
}

/**
 * @brief Creates a new ArraystructContext object.
 *
 * This function creates a new ArraystructContext object based on the provided path and timebase.
 * It checks the type of the current context and creates the ArraystructContext object accordingly.
 *
 * @param path The path to be used for the ArraystructContext.
 * @param timebase The timebase to be used for the ArraystructContext.
 * @return A pointer to the newly created ArraystructContext object.
 */
ArraystructContext* LLenv::create(const char* path, const char* timebase) {
    ArraystructContext* actx;
    ArraystructContext* parent = NULL;
    if (context->getType() == CTX_ARRAYSTRUCT_TYPE)
        parent = dynamic_cast<ArraystructContext*>(context);
    
    if (parent!=NULL)
      {
	actx = new ArraystructContext(parent,
				      std::string(path),
				      std::string(timebase));
      }
    else
      {
	OperationContext* octx = dynamic_cast<OperationContext*>(context);
	actx = new ArraystructContext(octx,
				      std::string(path),
				      std::string(timebase));
      }
    return actx;
}

/**
 * @brief Creates an Array of Structures (AOS) in the low-level environment.
 *
 * This function initializes an array structure context and begins an array structure action
 * using the provided field path and time base path. It also assigns a unique context ID to the
 * created array structure context.
 *
 * @param ctxID The context ID of the low-level environment.
 * @param actxID Pointer to an integer where the function will store the unique context ID of the created array structure context.
 * @param fieldPath The path to the field in the data structure.
 * @param timeBasePath The path to the time base in the data structure.
 * @param size Pointer to an integer representing the size of the array structure.
 */
void Lowlevel::createAOS(int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *size){
        LLenv lle = Lowlevel::getLLenv(ctxID);
        ArraystructContext* actx = lle.create(fieldPath, timeBasePath);
        lle.backend->beginArraystructAction(actx, size); 
        *actxID = Lowlevel::addLLenv(lle.backend, actx);
  } 

/**
 * @brief Sets the value of a variable based on the provided data, type, and dimension.
 *
 * This function assigns the value pointed to by `data` to the variable pointed to by `var`.
 * The assignment is done based on the specified `type` and `dim`.
 *
 * @param data Pointer to the data to be assigned.
 * @param type Integer representing the type of the data. Possible values are:
 *             - alconst::char_data: Character data
 *             - alconst::integer_data: Integer data
 *             - alconst::double_data: Double precision floating point data
 *             - alconst::complex_data: Complex number data (std::complex<double>)
 * @param dim Integer representing the dimension of the data. If `dim` is 0, the function
 *            assigns the value directly. Otherwise, it assigns the pointer to `data` to `var`.
 * @param var Pointer to the variable where the data should be assigned.
 *
 * @throws ALLowlevelException if the provided `type` is unknown.
 */
void Lowlevel::setValue(void *data, int type, int dim, void **var)
{
  if (dim==0) 
    {
      switch(type)
	{
	case alconst::char_data:
	  **(char **)var = *(char*)data;
	  break;
	case alconst::integer_data:
	  **(int**)var = *(int*)data;
	  break;
	case alconst::double_data:
	  **(double**)var = *(double*)data;
	  break;
	case alconst::complex_data:
	  **(std::complex<double>**)var = *(std::complex<double>*)data;
	  break;
	default:
	  throw ALLowlevelException("Unknown data type="+std::to_string(type),LOG);
	}
      free(data);
    }
  else
    *var = data;
}

/**
 * @brief Sets the default value for a given variable based on its type and dimension.
 *
 * This function initializes the variable pointed to by `var` with a default value
 * depending on the specified `type` and `dim`. If `dim` is 0, it sets the variable
 * to a predefined empty value based on the type. If `dim` is greater than 0, it 
 * sets the variable to NULL and initializes the size array to zeros.
 *
 * @param type The data type of the variable. It can be one of the following:
 *             - alconst::char_data: Character data type.
 *             - alconst::integer_data: Integer data type.
 *             - alconst::double_data: Double precision floating point data type.
 *             - alconst::complex_data: Complex number data type.
 * @param dim The dimension of the variable. If 0, the variable is a scalar. If greater than 0, the variable is an array.
 * @param var A pointer to the variable to be initialized.
 * @param size An array representing the size of each dimension of the variable. It is used only if `dim` is greater than 0.
 *
 * @throws ALLowlevelException if the data type is unknown.
 */
void Lowlevel::setDefaultValue(int type, int dim, void **var, int *size)
{
  int i;
  if (dim==0)
    {
      switch(type)
	{
	case alconst::char_data:
	  **(char**)var = Lowlevel::EMPTY_CHAR;
	  break;
	case alconst::integer_data:
	  **(int**)var = Lowlevel::EMPTY_INT;
	  break;
	case alconst::double_data:
	  **(double**)var = Lowlevel::EMPTY_DOUBLE;
	  break;
	case alconst::complex_data:
	  **(std::complex<double>**)var = Lowlevel::EMPTY_COMPLEX;
	  break;
	default:
	  throw ALLowlevelException("Unknown data type="+std::to_string(type),LOG);
	}
    }
  else
    {
      *var = NULL;
      for (i=0; i<dim; i++)
	size[i] = 0;
    }
}

/**
 * @brief Converts data from one type to another specified by desttype.
 *
 * This function takes a pointer to data of type From, the size of the data, 
 * and an integer representing the destination type. It allocates memory 
 * for the converted data and copies the original data into the newly 
 * allocated memory.
 *
 * @tparam From The type of the input data.
 * @param data Pointer to the input data.
 * @param size The number of elements in the input data.
 * @param desttype The type to which the data should be converted. 
 *                 It should be one of the constants defined in alconst.
 * @return A void pointer to the newly allocated and converted data.
 * @throws ALLowlevelException If the desttype is unknown.
 */
template <typename From>
void* Lowlevel::convertData(From* data, size_t size, int desttype)
{
  switch (desttype)
    {
    case alconst::char_data:
      {
	char* convdata = (char*)malloc(size*sizeof(char));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    case alconst::integer_data:
      {
	int* convdata = (int*)malloc(size*sizeof(int));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    case alconst::double_data:
      {
	double* convdata = (double*)malloc(size*sizeof(double));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    case alconst::complex_data:
      {
	std::complex<double>* convdata = (std::complex<double>*)malloc(size*sizeof(std::complex<double>));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    default:
      throw ALLowlevelException("Unknown data type="+std::to_string(desttype),LOG);
    }
}

/**
 * @brief Converts data from one type to another and sets the converted value.
 *
 * This function takes data of a specified source type, converts it to a destination type,
 * and sets the converted value. If the source type is complex, it sets a default value instead.
 *
 * @param data Pointer to the data to be converted.
 * @param srctype The type of the source data. Possible values are defined in alconst.
 * @param dim The number of dimensions of the data.
 * @param size An array containing the size of each dimension.
 * @param desttype The type to which the data should be converted. Possible values are defined in alconst.
 * @param var Pointer to the variable where the converted data will be stored.
 */
void Lowlevel::setConvertedValue(void *data, int srctype, int dim, int *size, int desttype, void** var)
{
  void* convdata;
  size_t totsize = 1;

  for (int i=0; i<dim; i++)
    totsize*=size[i];
  
  switch (srctype) {
  case alconst::char_data:
    convdata = Lowlevel::convertData((char*)data,totsize,desttype);
    Lowlevel::setValue(convdata,desttype,dim,var);
    break;
      
  case alconst::integer_data:
    convdata = Lowlevel::convertData((int*)data,totsize,desttype);
    Lowlevel::setValue(convdata,desttype,dim,var);
    break;

  case alconst::double_data:
    convdata = Lowlevel::convertData((double*)data,totsize,desttype);
    Lowlevel::setValue(convdata,desttype,dim,var);
    break;
    
  case alconst::complex_data:
    // can't convert, set default
    Lowlevel::setDefaultValue(desttype, dim, var, size);
    break;
  }

  free(data);
}

/**
 * @brief Begins a URI action by initializing the backend and creating a data entry context.
 *
 * This function creates a new DataEntryContext object using the provided URI. It then initializes
 * the backend using the backend ID obtained from the DataEntryContext. Finally, it stores a reference
 * to the backend and the data entry context and returns a context ID.
 *
 * @param uri The URI string used to create the DataEntryContext.
 * @return An integer representing the context ID. If an error occurs, it returns alerror::unknown_err.
 */
int Lowlevel::beginUriAction(const std::string &uri)
{
  int ctxID=alerror::unknown_err;
  DataEntryContext *pctx=NULL;
  Backend *be=NULL;

  pctx = new DataEntryContext(uri);
  if (pctx != NULL) {
    be = Backend::initBackend(pctx);
    // store reference of this object 
    ctxID = Lowlevel::addLLenv(be, pctx);
  }

  return ctxID;
}

/**
 * @brief Checks if the given data has a non-zero shape.
 *
 * This function verifies if the provided data has a non-zero shape based on its datatype and dimensions.
 * It handles different datatypes including integers, doubles, and complex numbers.
 *
 * @param datatype The type of the data (e.g., INTEGER_DATA, DOUBLE_DATA, COMPLEX_DATA, CHAR_DATA).
 * @param data Pointer to the data to be checked.
 * @param dim The number of dimensions of the data.
 * @param size Array containing the size of each dimension.
 * @return true if the data has a non-zero shape, false otherwise.
 */
bool Lowlevel::data_has_non_zero_shape(int datatype, void *data, int dim, int *size) {

	if (dim == 0) {
		if (data == NULL) return false;
		if (datatype == INTEGER_DATA) {
			int *p = (int*) data;
			if (*p == Lowlevel::EMPTY_INT)
			   return false;
		}
		else if (datatype == DOUBLE_DATA) {
			double *p = (double*) data;
			if (*p == Lowlevel::EMPTY_DOUBLE)
			   return false;
		}
		else if (datatype == COMPLEX_DATA) {
			std::complex<double> *p = (std::complex<double>*) data;
			if (*p == Lowlevel::EMPTY_COMPLEX)
			   return false;
		}
	}
		
	if (dim != 0 && (data == NULL || size == NULL))
	   return false;

    //don't check dimension sizes if data is list of strings (IMAS-4948)
    if(!(datatype == CHAR_DATA && dim > 1)){
        for (int i = 0; i < dim; i++){
            if (size[i] == 0){
                return false;
            }
         }
    }
     return true;
}


#endif


//////////////////// IMPLEMENTATION OF C WRAPPERS ////////////////////


/**
 * @brief Retrieves information about a specific context.
 *
 * This function provides detailed information about a context identified by the given context ID.
 * The information is returned as a dynamically allocated string, which must be freed by the caller.
 *
 * @param ctxID The ID of the context to retrieve information for.
 * @param info A pointer to a char pointer where the context information string will be stored.
 *             The caller is responsible for freeing this memory.
 * @return An al_status_t structure indicating the success or failure of the operation.
 *         - status.code = 0: Success
 *         - status.code = alerror::lowlevel_err: Failure due to a low-level error
 */
al_status_t al_context_info(int ctxID, char **info)
{
  al_status_t status;
  std::string str;

  status.code = 0;
  if (ctxID==0)
    {
      str = "NULL context";
    }
  else
    {
      std::stringstream desc;
      try {
	LLenv lle = Lowlevel::getLLenv(ctxID);
	desc << "Context type = " 
	     << lle.context->getType() << "\n";
	desc << "Backend @ = " << lle.backend << "\n";
	desc << lle.context->print();
	str = desc.str();
      }
      catch (const ALLowlevelException& e) {
	status.code = alerror::lowlevel_err;
	ALException::registerStatus(status.message, __func__, e);
      }
    }

  const char* cstr = str.c_str();
  size_t size = strlen(cstr);
  *info = (char *)malloc(size+1);
  mempcpy(*info, cstr, size);
  (*info)[size] = '\0';

  return status;
}


/**
 * @brief Retrieves the backend ID for a given context ID.
 *
 * This function attempts to retrieve the backend ID associated with the provided
 * context ID. If successful, the backend ID is stored in the location pointed to
 * by the `beid` parameter. If an exception occurs during the process, the function
 * catches the exception, sets the status code to indicate a low-level error, and
 * registers the exception status.
 *
 * @param ctxID The context ID for which the backend ID is to be retrieved.
 * @param beid Pointer to an integer where the retrieved backend ID will be stored.
 * @return al_status_t Status code indicating success or failure of the operation.
 */
al_status_t al_get_backendID(int ctxID, int *beid)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(ctxID);
    DataEntryContext *pctx = static_cast<DataEntryContext *>(lle.context);
    *beid = pctx->getBackendID();
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}

/**
 * @brief Begins a data entry action for a given URI.
 *
 * This function initializes a data entry context and opens a pulse based on the specified mode.
 * It ensures compatibility between the opened file version and the backend version.
 *
 * @param uri The URI for which the data entry action is to be started.
 * @param mode The mode in which the pulse should be opened. Possible values are:
 *             - alconst::open_pulse
 *             - alconst::force_open_pulse
 * @param dectxID Pointer to an integer where the data entry context ID will be stored.
 * @return al_status_t Status of the operation. The status code will be set to one of the following:
 *         - 0: Success
 *         - alerror::backend_err: Backend error
 *         - alerror::lowlevel_err: Low-level error
 *         - alerror::unknown_err: Unknown error
 * 
 * @throws ALLowlevelException If the context type stored is incorrect or if there is a compatibility issue.
 * @throws ALBackendException If there is an error with the backend.
 * @throws std::exception If any other exception occurs.
 */
al_status_t al_begin_dataentry_action(const std::string uri, int mode, int *dectxID)
{
  al_status_t status = { 0 };

  status.code = 0;
  try {
    *dectxID = Lowlevel::beginUriAction(uri);
    LLenv lle = Lowlevel::getLLenv(*dectxID);
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
    if (pctx==NULL)
      throw ALLowlevelException("Wrong Context type stored",LOG);

    lle.backend->openPulse(pctx,
			   mode);

    switch (mode) {
    case alconst::open_pulse:
    case alconst::force_open_pulse:
      std::pair<int,int> ver = lle.backend->getVersion(NULL);
      std::pair<int,int> sver = lle.backend->getVersion(pctx);
      if ((ver.first!=sver.first)||(ver.second<sver.second))
	throw ALLowlevelException("Compatibility between opened file version "+
				   std::to_string(sver.first)+"."+std::to_string(sver.second)+
				   " and backend "+pctx->getBackendName()+
				   " version "+std::to_string(ver.first)+"."+std::to_string(ver.second)+
				   " can't be ensured. ABORT.\n",LOG);
      break;
    }
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}

/**
 * @brief Begins a data entry action.
 *
 * This function initializes a data entry action based on the provided URI and mode.
 * It returns a status indicating the success or failure of the operation.
 *
 * @param uri The URI as a C-string that specifies the data entry location.
 * @param mode An integer representing the mode of the data entry action.
 * @param dectxID A pointer to an integer where the data entry context ID will be stored.
 * @return al_status_t Status of the data entry action.
 */
al_status_t al_begin_dataentry_action(const char *uri, int mode, int *dectxID)
{
  std::string uri_str(uri);
  return al_begin_dataentry_action(uri_str, mode, dectxID);
}


/**
 * @brief Closes a pulse in the low-level environment.
 *
 * This function attempts to close a pulse identified by the given context ID (pctxID)
 * and mode. It handles various exceptions that might occur during the process and 
 * sets the appropriate status code and message.
 *
 * @param pctxID The context ID of the pulse to be closed.
 * @param mode The mode in which the pulse should be closed.
 * @return al_status_t The status of the operation, including a code and message.
 *
 * @exception ALBackendException If there is an error in the backend.
 * @exception ALLowlevelException If there is an error in the low-level environment.
 * @exception std::exception If an unknown error occurs.
 */
al_status_t al_close_pulse(int pctxID, int mode)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
    if (pctx==NULL)
      throw ALLowlevelException("Wrong Context type stored",LOG);

    lle.backend->closePulse(pctx, mode);
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}

/**
 * @brief Begins a global action for a plugin.
 *
 * This function initializes an operation context and begins an action in the backend.
 * It ensures compatibility between the opened file version and the backend version when writing.
 *
 * @param pctxID The ID of the parent context.
 * @param dataobjectname The name of the data object.
 * @param datapath The path to the data.
 * @param rwmode The read/write mode (e.g., alconst::write_op).
 * @param octxID A pointer to an integer where the operation context ID will be stored.
 * @return An al_status_t structure containing the status code and message.
 *
 * @throws ALLowlevelException if the context type is wrong or if there is a version compatibility issue.
 * @throws ALContextException if there is an error related to the context.
 * @throws ALBackendException if there is an error related to the backend.
 * @throws std::exception for any other unknown errors.
 */
al_status_t al_plugin_begin_global_action(int pctxID, const char* dataobjectname, const char* datapath, int rwmode,
                                            int *octxID)
{
  al_status_t status;
  OperationContext *octx=NULL;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID); 
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
    if (pctx==NULL)
      throw ALLowlevelException("Wrong Context type stored",LOG);

    octx = new OperationContext(pctx,
				std::string(dataobjectname),
                std::string(datapath),
				rwmode);
    lle.backend->beginAction(octx);

    switch (rwmode) {
    case alconst::write_op:
      std::pair<int,int> ver = lle.backend->getVersion(NULL);
      std::pair<int,int> sver = lle.backend->getVersion(pctx);
      if (ver.second!=sver.second)
	throw ALLowlevelException("Compatibility between opened file version "+
				   std::to_string(sver.first)+"."+std::to_string(sver.second)+
				   " and backend "+pctx->getBackendName()+
				   " version "+std::to_string(ver.first)+"."+std::to_string(ver.second)+
				   " can't be ensured (minor versions should match when writing). ABORT.\n",LOG);
      break;
    }
    *octxID = Lowlevel::addLLenv(lle.backend, octx); 
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


/**
 * @brief Begins a slice action for a given data object.
 *
 * This function initializes and begins a slice action for a specified data object
 * within a given context. It handles various exceptions that may occur during the
 * process and returns the status of the operation.
 *
 * @param pctxID The ID of the parent context.
 * @param dataobjectname The name of the data object to perform the action on.
 * @param rwmode The read/write mode for the action.
 * @param time The time parameter for the action.
 * @param interpmode The interpolation mode for the action.
 * @param octxID A pointer to an integer where the ID of the operation context will be stored.
 * @return al_status_t The status of the operation, including any error codes and messages.
 *
 * @exception ALContextException Thrown if there is an error with the context.
 * @exception ALBackendException Thrown if there is an error with the backend.
 * @exception ALLowlevelException Thrown if there is a low-level error.
 * @exception std::exception Thrown if an unknown error occurs.
 */
al_status_t al_plugin_begin_slice_action(int pctxID, const char* dataobjectname, int rwmode, 
				   double time, int interpmode, int *octxID)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
    if (pctx==NULL)
      throw ALLowlevelException("Wrong Context type stored",LOG);

    OperationContext *octx= new OperationContext(pctx, 
						 std::string(dataobjectname),
						 rwmode, 
						 alconst::slice_op, 
						 time, 
						 interpmode);
    lle.backend->beginAction(octx);

    switch (rwmode) {
    case alconst::write_op:
      std::pair<int,int> ver = lle.backend->getVersion(NULL);
      std::pair<int,int> sver = lle.backend->getVersion(pctx);
      if (ver.second!=sver.second)
	throw ALLowlevelException("Compatibility between opened file version "+
				   std::to_string(sver.first)+"."+std::to_string(sver.second)+
				   " and backend "+pctx->getBackendName()+
				   " version "+std::to_string(ver.first)+"."+std::to_string(ver.second)+
				   " can't be ensured (minor versions should match when writing). ABORT.\n",LOG);
      break;
    }
    *octxID = Lowlevel::addLLenv(lle.backend, octx); 
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}

/**
 * @brief Begins a time range action on a data object.
 *
 * This function initializes and begins a time range action on a specified data object
 * within a given time range. It supports various read/write modes and interpolation modes.
 *
 * @param pctxID The ID of the parent context.
 * @param dataobjectname The name of the data object to perform the action on.
 * @param rwmode The read/write mode (e.g., read, write).
 * @param tmin The minimum time value for the time range.
 * @param tmax The maximum time value for the time range.
 * @param dtime_buffer A buffer containing time values for the operation.
 * @param dtime_shape The shape of the time buffer.
 * @param interpmode The interpolation mode to use.
 * @param octxID A pointer to store the ID of the operation context.
 * @return al_status_t The status of the operation.
 *
 * @throws ALLowlevelException If the backend does not support time range operations or if the context type is incorrect.
 * @throws ALContextException If there is an error related to the context.
 * @throws ALBackendException If there is an error related to the backend.
 * @throws std::exception If an unknown error occurs.
 */
al_status_t al_plugin_begin_timerange_action(int pctxID, const char* dataobjectname, int rwmode, 
				   double tmin, double tmax, const double* dtime_buffer, const int* dtime_shape, int interpmode, int *octxID)
{
  al_status_t status;
  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);

    if (!lle.backend->supportsTimeRangeOperation()) {
      std::string message = "Selected backend does not support time range operations.";
      throw ALLowlevelException(message.c_str());
    }
    
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
    if (pctx==NULL)
      throw ALLowlevelException("Wrong Context type stored",LOG);

    std::vector<double> dtime;
    if (*dtime_shape > 0)
      dtime = std::vector<double> (dtime_buffer, dtime_buffer + *dtime_shape);

    OperationContext *octx= new OperationContext(pctx, 
						 std::string(dataobjectname),
						 rwmode, 
						 alconst::timerange_op, 
						 tmin, 
             tmax,
             dtime,
						 interpmode);
    lle.backend->beginAction(octx);
    switch (rwmode) {
    case alconst::write_op:
      std::pair<int,int> ver = lle.backend->getVersion(NULL);
      std::pair<int,int> sver = lle.backend->getVersion(pctx);
      if (ver.second!=sver.second)
	throw ALLowlevelException("Compatibility between opened file version "+
				   std::to_string(sver.first)+"."+std::to_string(sver.second)+
				   " and backend "+pctx->getBackendName()+
				   " version "+std::to_string(ver.first)+"."+std::to_string(ver.second)+
				   " can't be ensured (minor versions should match when writing). ABORT.\n",LOG);
      break;
    }
    *octxID = Lowlevel::addLLenv(lle.backend, octx); 
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


/**
 * @brief Ends the action associated with the given context ID.
 *
 * This function terminates the action for the specified context ID by deleting
 * the associated low-level environment and context. It handles exceptions that
 * may occur during this process and sets the appropriate status code.
 *
 * @param ctxID The context ID for which the action is to be ended.
 * @return al_status_t The status of the operation, including any error codes.
 *
 * @note If ctxID is 0, the function does nothing and returns a status code of 0.
 * @note The function handles three types of exceptions:
 *       - ALBackendException: Sets the status code to alerror::backend_err.
 *       - ALLowlevelException: Sets the status code to alerror::lowlevel_err.
 *       - std::exception: Sets the status code to alerror::unknown_err.
 */
al_status_t al_plugin_end_action(int ctxID)
{
  al_status_t status;

  status.code = 0;
  if (ctxID!=0)
    {
      try {
	LLenv lle = Lowlevel::delLLenv(ctxID);
	lle.backend->endAction(lle.context);

	if (lle.context->getType() == CTX_PULSE_TYPE) 
	  delete(lle.backend);
    
	delete(lle.context);
      }
      catch (const ALBackendException& e) {
	status.code = alerror::backend_err;
	ALException::registerStatus(status.message, __func__, e);
      }
      catch (const ALLowlevelException& e) {
	status.code = alerror::lowlevel_err;
	ALException::registerStatus(status.message, __func__, e);
      }
      catch (const std::exception& e) {
	status.code = alerror::unknown_err;
	ALException::registerStatus(status.message, __func__, e);
      }
    }
  
  return status;
}


/**
 * @brief Writes data to a specified field in the backend.
 *
 * This function writes data to a specified field in the backend using the provided context ID.
 * It handles different types of exceptions and sets the status code accordingly.
 *
 * @param ctxID The context ID used to retrieve the low-level environment.
 * @param field The field to which the data will be written.
 * @param timebase The timebase associated with the data.
 * @param data A pointer to the data to be written.
 * @param datatype The type of the data being written.
 * @param dim The dimension of the data.
 * @param size An array representing the size of each dimension.
 * @return al_status_t The status of the write operation, including any error codes.
 */
al_status_t al_plugin_write_data(int ctxID, const char *field, const char *timebase,  
			 void *data, int datatype, int dim, int *size)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(ctxID);
    lle.backend->writeData(lle.context,
			   std::string(field),
			   std::string(timebase),
			   data,
			   datatype,
			   dim,
			   size);
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  
  return status;
}


/**
 * @brief Reads data from a plugin and handles various data types and dimensions.
 *
 * @param ctxID The context ID for the low-level environment.
 * @param field The field name to read data from.
 * @param timebase The timebase associated with the data.
 * @param data Pointer to the location where the read data will be stored.
 * @param datatype The expected data type of the read data.
 * @param dim The expected dimension of the read data.
 * @param size Pointer to an integer where the size of the read data will be stored.
 * @return al_status_t Status code indicating success or type of error encountered.
 *
 * @throws ALLowlevelException If the dimension or type of the data returned by the backend is incorrect.
 * @throws ALBackendException If there is an error with the backend.
 * @throws std::exception For any other unknown errors.
 */
al_status_t al_plugin_read_data(int ctxID, const char *field, const char *timebase, 
			  void **data, int datatype, int dim, int *size)
{
  al_status_t status;
  void *retData=NULL;
  int retType=datatype;
  int retDim=dim;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(ctxID);

    if (lle.backend->readData(lle.context, 
			      std::string(field),
			      std::string(timebase),
			      &retData,
			      &retType,
			      &retDim,
			      size) == 0)
      {
	// no data
	Lowlevel::setDefaultValue(datatype, dim, data, size);
      }
    else
      {
	if (retDim!=dim)
	  {
	    throw ALLowlevelException("Wrong dimension of Data returned by backend: expected "+
				       std::string(const2str(datatype))+" in "+
				       std::to_string(dim)+"D but got "+
				       std::string(const2str(retType))+" in "+
				       std::to_string(retDim)+"D",LOG);
	  }
	else if (retType!=datatype)
	  {
	    Lowlevel::setConvertedValue(retData, retType, retDim, size, datatype, data);
	    ALException::registerStatus(status.message, __func__,
					 ALLowlevelException("Warning: " + lle.context->getURI().to_string() +
							      "/" + field + " returned with type " +
							      std::string(const2str(retType)) +
							      " while we expect type " +
							      std::string(const2str(datatype)) + "\n"));
	  }
	else 
	  Lowlevel::setValue(retData, datatype, dim, data);
      }
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
    printf("-->status.message = %s\n", status.message);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


/**
 * @brief Deletes data associated with a given field in the specified operation context.
 *
 * This function attempts to delete data identified by the provided field name
 * within the operation context identified by octxID. It handles various exceptions
 * that may arise during the deletion process and sets the appropriate status code
 * and message.
 *
 * @param octxID The ID of the operation context.
 * @param field The name of the field whose data is to be deleted.
 * @return al_status_t The status of the delete operation, including a status code
 *         and a message if an error occurred.
 *
 * @exception ALBackendException If there is an error in the backend during the deletion process.
 * @exception ALLowlevelException If there is an error related to the low-level context.
 * @exception std::exception If an unknown error occurs.
 */
al_status_t al_delete_data(int octxID, const char *field)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(octxID);
    OperationContext *octx= dynamic_cast<OperationContext *>(lle.context); 
    if (octx==NULL)
      throw ALLowlevelException("Wrong Context type stored",LOG);

    lle.backend->deleteData(octx, std::string(field));
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


/**
 * @brief Begins an array structure action in the AL plugin.
 *
 * This function initializes an array structure action context and begins the action
 * using the provided context ID, path, and timebase. It also sets the size of the 
 * array structure and returns the action context ID.
 *
 * @param ctxID The context ID.
 * @param path The path to the array structure.
 * @param timebase The timebase for the array structure.
 * @param size Pointer to an integer where the size of the array structure will be stored.
 * @param actxID Pointer to an integer where the action context ID will be stored.
 * @return al_status_t The status of the operation.
 *
 * @throws ALLowlevelException If the returned size for the array of structure is negative.
 * @throws ALContextException If there is an error with the context.
 * @throws ALBackendException If there is an error with the backend.
 * @throws std::exception If there is an unknown error.
 */
al_status_t al_plugin_begin_arraystruct_action(int ctxID, const char *path, 
					 const char *timebase, int *size,
					 int *actxID)
{
  al_status_t status;
  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(ctxID);
    ArraystructContext* actx = lle.create(path, timebase);
    lle.backend->beginArraystructAction(actx, size);
    *actxID = Lowlevel::addLLenv(lle.backend, actx); 
    if (*size == 0)
      {
	// no data
	lle.backend->endAction(actx);
	delete(actx);
	*actxID = 0; 
      }
    else
      {
	
	if (*size < 0)
	  {
	    throw ALLowlevelException("Returned size for array of structure is negative! ("+
				       std::to_string(*size)+")",LOG);
	  }
      }
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


/**
 * @brief Iterates over an Arraystruct context by a given step.
 *
 * This function retrieves the low-level environment using the provided context ID,
 * casts the context to an ArraystructContext, and advances the index by the specified step.
 * It handles exceptions that may occur during this process and sets the appropriate status code.
 *
 * @param aosctxID The ID of the Arraystruct context.
 * @param step The step by which to advance the index.
 * @return al_status_t The status of the operation, including any error codes and messages.
 */
al_status_t al_iterate_over_arraystruct(int aosctxID, 
					 int step)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(aosctxID);
    ArraystructContext *actx = static_cast<ArraystructContext *>(lle.context);
    
    actx->nextIndex(step);
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}

/**
 * @brief Builds a URI from legacy parameters.
 *
 * This function constructs a URI based on the provided legacy parameters such as 
 * backend ID, pulse, run, user, tokamak, version, and options. The resulting URI 
 * is stored in the provided reference parameter.
 *
 * @param backendID The ID of the backend.
 * @param pulse The pulse number.
 * @param run The run number.
 * @param user The user name.
 * @param tokamak The tokamak name.
 * @param version The version string.
 * @param options Additional options as a string.
 * @param uri Reference to a string where the constructed URI will be stored.
 * @return al_status_t Status code indicating success or failure of the operation.
 */
al_status_t al_build_uri_from_legacy_parameters(const int backendID, 
                         const int pulse,
                         const int run, 
                         const std::string user, 
                         const std::string tokamak, 
                         const std::string version,
                         const std::string options,
                         std::string& uri) {

    al_status_t status;
    status.code = 0;

    try {
       DataEntryContext::build_uri_from_legacy_parameters(backendID, 
                         pulse,
                         run, 
                         user, 
                         tokamak, 
                         version,
                         options,
                         uri);
    }
    catch (const ALContextException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

//-----------------------------------------------------------


/**
 * @brief Builds a URI from legacy parameters.
 *
 * This function constructs a URI based on the provided legacy parameters.
 * It uses the DataEntryContext to build the URI and handles any exceptions
 * that may occur during the process.
 *
 * @param backendID The backend identifier.
 * @param pulse The pulse number.
 * @param run The run number.
 * @param user The user name.
 * @param tokamak The tokamak name.
 * @param version The version string.
 * @param options Additional options for URI construction.
 * @param uri A pointer to the resulting URI string.
 * @return An al_status_t structure containing the status code and message.
 */
al_status_t al_build_uri_from_legacy_parameters(const int backendID, 
                         const int pulse,
                         const int run, 
                         const char *user, 
                         const char *tokamak, 
                         const char *version,
                         const char *options,
                         char** uri) {
    al_status_t status;
    status.code = 0;

    char opt[1024];
    if (options == nullptr) {
      strcpy(opt, "");
    }
    else {
      strcpy(opt, options);
    }
    
    try {
       DataEntryContext::build_uri_from_legacy_parameters(backendID, 
                         pulse,
                         run, 
                         user, 
                         tokamak, 
                         version,
                         opt,
                         uri);
    }
    catch (const ALContextException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

//HLI Wrappers for calling LL functions - Call plugins if required

/**
 * @brief Begins a global action for a specified data object.
 *
 * This function initiates a global action for a given data object name and path, 
 * with the specified read/write mode. It also registers core plugins and begins 
 * global action for bound plugins if any.
 *
 * @param pctxID The context ID of the parent context.
 * @param dataobjectname The name of the data object.
 * @param datapath The path to the data object.
 * @param rwmode The read/write mode.
 * @param octxID Pointer to the output context ID.
 * @return al_status_t The status of the operation.
 *
 * @exception ALContextException Thrown when there is a context-related error.
 * @exception ALLowlevelException Thrown when there is a low-level error.
 * @exception ALPluginException Thrown when there is a plugin-related error.
 * @exception std::exception Thrown when there is an unknown error.
 */
al_status_t al_begin_global_action(int pctxID, const char* dataobjectname, const char* datapath, int rwmode,
                    int *octxID)
{
  al_status_t status;

  status.code = 0;

  std::string dataobjectnameStr(dataobjectname);
  if(dataobjectnameStr.size() >= 2 && dataobjectnameStr.substr(dataobjectnameStr.size() - 2) == "/0") {
    dataobjectnameStr.erase(dataobjectnameStr.size() - 2);
  }

  try {
    status = al_plugin_begin_global_action(pctxID, dataobjectnameStr.c_str(), datapath, rwmode, octxID);
    if (status.code != 0)
        return status;
    LLplugin::register_core_plugins(pctxID, dataobjectname, rwmode, octxID);
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataobjectnameStr.c_str(), pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
           LLplugin::beginGlobalActionPlugin(pluginName, pctxID, dataobjectnameStr.c_str(), datapath, rwmode, *octxID);
    }
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALPluginException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Begins a slice action for a given data object.
 *
 * This function initiates a slice action for the specified data object name with the given parameters.
 * It handles various exceptions that might occur during the process and registers the status accordingly.
 *
 * @param pctxID The context ID of the parent context.
 * @param dataobjectname The name of the data object for which the slice action is to be initiated.
 * @param rwmode The read/write mode for the slice action.
 * @param time The time parameter for the slice action.
 * @param interpmode The interpolation mode for the slice action.
 * @param octxID Pointer to the output context ID.
 * @return al_status_t The status of the slice action initiation.
 *
 * @exception ALContextException Thrown when there is a context-related error.
 * @exception ALLowlevelException Thrown when there is a low-level error.
 * @exception ALPluginException Thrown when there is a plugin-related error.
 * @exception std::exception Thrown when there is an unknown error.
 */
al_status_t al_begin_slice_action(int pctxID, const char* dataobjectname, int rwmode, 
                   double time, int interpmode, int *octxID)
{
  al_status_t status;

  status.code = 0;

  std::string dataobjectnameStr(dataobjectname);
  if(dataobjectnameStr.size() >= 2 && dataobjectnameStr.substr(dataobjectnameStr.size() - 2) == "/0") {
    dataobjectnameStr.erase(dataobjectnameStr.size() - 2);
  }

  try {
    status = al_plugin_begin_slice_action(pctxID, dataobjectnameStr.c_str(), rwmode, time, interpmode, octxID);
    if (status.code != 0)
     return status;
    LLplugin::register_core_plugins(pctxID, dataobjectname, rwmode, octxID);
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataobjectnameStr.c_str(), pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
		   LLplugin::beginSliceActionPlugin(pluginName, pctxID, dataobjectnameStr.c_str(), rwmode, time, interpmode, *octxID);
	}
   }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALPluginException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Begins a time range action for a specified data object.
 *
 * This function initializes and begins a time range action for a given data object
 * within a specified context. It handles the setup of the time range, interpolation mode,
 * and manages the interaction with plugins.
 *
 * @param pctxID The context ID of the parent context.
 * @param dataobjectname The name of the data object to perform the action on.
 * @param rwmode The read/write mode for the action.
 * @param tmin The minimum time value for the time range.
 * @param tmax The maximum time value for the time range.
 * @param dtime_buffer A pointer to the buffer containing time data.
 * @param dtime_shape A pointer to the shape of the time data buffer.
 * @param interpmode The interpolation mode to use.
 * @param octxID A pointer to the output context ID.
 * @return An al_status_t structure containing the status code and message.
 *
 * @throws ALContextException If there is an error related to the context.
 * @throws ALLowlevelException If there is a low-level error.
 * @throws ALPluginException If there is a plugin-related error.
 * @throws std::exception If an unknown error occurs.
 */
al_status_t al_begin_timerange_action(int pctxID, const char* dataobjectname, int rwmode, 
                   double tmin, double tmax, const double* dtime_buffer, const int* dtime_shape, int interpmode, int *octxID)
{
  al_status_t status;

  status.code = 0;
  std::vector<double> dtime;
    if (*dtime_shape > 0)
      dtime = std::vector<double> (dtime_buffer, dtime_buffer + *dtime_shape);

  std::string dataobjectnameStr(dataobjectname);
  if(dataobjectnameStr.size() >= 2 && dataobjectnameStr.substr(dataobjectnameStr.size() - 2) == "/0") {
    dataobjectnameStr.erase(dataobjectnameStr.size() - 2);
  }

  try {
    status = al_plugin_begin_timerange_action(pctxID, dataobjectnameStr.c_str(), rwmode, tmin, tmax, dtime_buffer, dtime_shape, interpmode, octxID);
    if (status.code != 0)
     return status;

    LLenv lle = Lowlevel::getLLenv(*octxID);
  
    LLplugin::register_core_plugins(pctxID, dataobjectname, rwmode, octxID);
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataobjectnameStr.c_str(), pluginsNames);
    if (isPluginBound) {
    std::vector<double> dtime(dtime_buffer, dtime_buffer + *dtime_shape);
		for (const auto& pluginName : pluginsNames)
		   LLplugin::beginTimeRangeActionPlugin(pluginName, pctxID, dataobjectnameStr.c_str(), rwmode, tmin, tmax, dtime, interpmode, *octxID);
	}
   }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALPluginException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Begins an array structure action in the Access Layer.
 *
 * This function initializes an array structure action, potentially involving multiple plugins.
 * It manages the creation of an Array of Structures (AOS) context and handles various error conditions.
 *
 * @param ctxID The context ID.
 * @param path The path to the array structure.
 * @param timebase The timebase for the array structure.
 * @param size Pointer to an integer where the size of the array structure will be stored.
 * @param actxID Pointer to an integer where the AOS context ID will be stored.
 * @return al_status_t The status of the operation.
 *
 * @throws ALLowlevelException If multiple plugins attempt to create an AOS context or if no AOS context is created.
 * @throws ALContextException If there is a context-related error.
 * @throws ALPluginException If there is a plugin-related error.
 * @throws std::exception If an unknown error occurs.
 */
al_status_t al_begin_arraystruct_action(int ctxID, const char *path, 
                     const char *timebase, int *size,
                     int *actxID)
{
  al_status_t status;
  status.code = 0;
  try {
    *actxID = 0; //no default AOS context, plugin has to manage the creation of this object
    AccessLayerPluginManager al_plugin_manager;
    bool skipAOSWriteAccess = false;
    if (al_plugin_manager.skipWriteAccess(ctxID, path)){ 
      *size = 0;
      skipAOSWriteAccess = true;
    } 
    std::vector<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(ctxID, path, pluginsNames);
    //printf("al_begin_arraystruct_action::isPluginBound=%d for path=%s\n", isPluginBound, path);
    if (!skipAOSWriteAccess && isPluginBound) {
        int actxID_default = *actxID;
        int actxID_user = 0;
        std::vector<std::string> plugins;
	      for (const auto& pluginName : pluginsNames) {
              LLplugin::beginArraystructActionPlugin(pluginName, ctxID, actxID, path, timebase, size);
           if ( (actxID_user == 0) && (actxID_default != *actxID) ) {//plugin has created another AOS context
               actxID_user = *actxID;
               plugins.push_back(pluginName);
           }
           else if (actxID_user != 0 && *actxID != 0) { //at least 2 plugins have created an AOS context, it's an error
              plugins.push_back(pluginName);
              std::string message = "An error has occurred calling al_begin_arraystruct_action(): only one plugin is allowed to create an AOS context at a given path.\n";
              message += "AOS context path: " + std::string(path) + "\n";
              for (size_t i = 0; i < plugins.size(); i++)
                  message += "--> Plugin: " + plugins[i] + "\n";
              throw ALLowlevelException(message.c_str());
           }
        }
        if (*actxID == 0) { //no AOS context, unexpected error
            char message[200];
            sprintf(message, "No AOS context has been created, or one plugin has removed it at path:%s.\n", path);
            throw ALLowlevelException(message);
        }
    }
    else {
        LLenv lle = Lowlevel::getLLenv(ctxID);
        ArraystructContext* actx = lle.create(path, timebase);
        lle.backend->beginArraystructAction(actx, size); //TO DISCUSS
        *actxID = Lowlevel::addLLenv(lle.backend, actx);
    }

    if (*size == 0) {
        if (!skipAOSWriteAccess && isPluginBound)
           LLplugin::endActionPlugin(*actxID);
        assert(actxID != 0);
        LLenv lle_aos = Lowlevel::getLLenv(*actxID);
        assert(lle_aos.context != NULL);
        ArraystructContext* actx = dynamic_cast<ArraystructContext *>(lle_aos.context);
        LLenv lle = Lowlevel::getLLenv(ctxID);
        lle.backend->endAction(actx);
        delete(actx);
        *actxID = 0;
    }
    
    if (*size < 0)
          {
            throw ALLowlevelException("Returned size for array of structure is negative! ("+
                           std::to_string(*size)+")",LOG);
          }
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALPluginException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  
  return status;
}

/**
 * @brief Ends an action associated with a given context ID.
 *
 * This function terminates an action by invoking the appropriate low-level
 * plugin and environment cleanup routines. It handles various exceptions
 * that may occur during the process and sets the status code accordingly.
 *
 * @param ctxID The context ID for which the action is to be ended. If ctxID is 0,
 *              the function does nothing and returns a status code of 0.
 * @return al_status_t The status of the operation, including any error codes
 *                     and messages if exceptions were caught.
 *
 * @exception ALContextException Thrown if there is an error related to the context.
 * @exception ALLowlevelException Thrown if there is a low-level error.
 * @exception ALPluginException Thrown if there is a plugin-related error.
 * @exception std::exception Thrown if an unknown error occurs.
 */
al_status_t al_end_action(int ctxID)
{
  al_status_t status;
  status.code = 0;
  if (ctxID!=0)
    {
      try {
        LLplugin::endActionPlugin(ctxID);
        LLenv lle = Lowlevel::delLLenv(ctxID);
        lle.backend->endAction(lle.context);

        if (lle.context->getType() == CTX_PULSE_TYPE) 
          delete(lle.backend);
        
        delete(lle.context);
        
      }
      catch (const ALContextException& e) {
        status.code = alerror::context_err;
        ALException::registerStatus(status.message, __func__, e);
      }
      catch (const ALLowlevelException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
      }
      catch (const ALPluginException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
      }
      catch (const std::exception& e) {
        status.code = alerror::unknown_err;
        ALException::registerStatus(status.message, __func__, e);
      }
  }
  return status;
}

/**
 * @brief Writes data to the specified field and timebase in the context identified by ctxID.
 *
 * This function attempts to write data using bound plugins if available. If no plugins are bound,
 * it writes the data directly using the low-level API. It handles various exceptions and sets the
 * status code accordingly.
 *
 * @param ctxID The context identifier.
 * @param field The field to which data is to be written.
 * @param timebase The timebase associated with the data.
 * @param data Pointer to the data to be written.
 * @param datatype The type of the data.
 * @param dim The dimension of the data.
 * @param size Pointer to an array containing the size of each dimension.
 * @return al_status_t The status of the write operation.
 */
al_status_t al_write_data(int ctxID, const char *field, const char *timebase,  
			 void *data, int datatype, int dim, int *size)
{
  al_status_t status;

  status.code = 0;
  try {
    std::vector<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(ctxID, field, pluginsNames);
    if (isPluginBound) {
		  for (const auto& pluginName : pluginsNames)
        LLplugin::writeDataPlugin(pluginName, ctxID, field, timebase, data, datatype, dim, size);
    }
    else {
      if (Lowlevel::data_has_non_zero_shape(datatype, data, dim, size))
        status = al_plugin_write_data(ctxID, field, timebase, data, datatype, dim, size);
    }
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALPluginException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Reads data from a specified field and timebase.
 *
 * This function attempts to read data using bound plugins if available. If no plugins are bound,
 * it falls back to the default plugin read function. It handles various exceptions and returns
 * an appropriate status code.
 *
 * @param ctxID The context ID.
 * @param field The field from which to read data.
 * @param timebase The timebase for the data.
 * @param data A pointer to the data buffer.
 * @param datatype The type of the data.
 * @param dim The dimension of the data.
 * @param size A pointer to the size of the data.
 * @return al_status_t The status of the read operation.
 */
al_status_t al_read_data(int ctxID, const char *field, const char *timebase, 
              void **data, int datatype, int dim, int *size)
{
  al_status_t status;
  status.code = 0;
  try {
    std::vector<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(ctxID, field, pluginsNames);
    //printf("al_read_data::isPluginBound=%d for field = %s\n ", isPluginBound, field);
    if (isPluginBound) {
	   for (const auto& pluginName : pluginsNames)
                LLplugin::readDataPlugin(pluginName, ctxID, field, timebase, data, datatype, dim, size);
    }
    else {
        status = al_plugin_read_data(ctxID, field, timebase, data, datatype, dim, size);
        if (status.code != 0)
            return status;
    }
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALPluginException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
   
}

/**
 * @brief Retrieves the occurrences of a given IDS name.
 *
 * This function fetches the occurrences of the specified IDS name and populates
 * the occurrences list and its size. It handles exceptions that may arise from
 * backend or low-level operations and sets the appropriate status code.
 *
 * @param pctxID The context ID used to get the low-level environment.
 * @param ids_name The name of the IDS for which occurrences are to be retrieved.
 * @param occurrences_list Pointer to an array that will be populated with the occurrences.
 * @param size Pointer to an integer that will be set to the size of the occurrences list.
 * @return al_status_t The status of the operation, including error codes if any exceptions are caught.
 */
al_status_t al_get_occurrences(int pctxID, const char* ids_name, int** occurrences_list, int* size)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);
    lle.backend->get_occurrences(lle.context, ids_name, occurrences_list, size);
  }
  catch (const ALBackendException& e) {
    status.code = alerror::backend_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


/**
 * @brief Sets the value of a parameter in a plugin.
 *
 * This function sets the value of a specified parameter in a plugin by calling the 
 * LLplugin::setvalueParameterPlugin function. It handles various exceptions that might 
 * be thrown during the process and sets the appropriate status code and message.
 *
 * @param parameter_name The name of the parameter to set.
 * @param datatype The data type of the parameter.
 * @param dim The dimension of the parameter.
 * @param size An array representing the size of each dimension.
 * @param data A pointer to the data to be set.
 * @param pluginName The name of the plugin.
 * @return al_status_t The status of the operation, including a code and a message.
 */
al_status_t al_setvalue_parameter_plugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
        LLplugin::setvalueParameterPlugin(parameter_name, datatype, dim, size, data, pluginName);
    }
    catch (const ALContextException& e) {
        status.code = alerror::context_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALLowlevelException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALPluginException& e) {
	status.code = alerror::lowlevel_err;
	ALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = alerror::unknown_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

/**
 * @brief Sets the value of an integer scalar parameter for a specified plugin.
 *
 * This function sets the value of an integer scalar parameter identified by its name
 * for a given plugin. It handles various exceptions that might occur during the process
 * and updates the status accordingly.
 *
 * @param parameter_name The name of the parameter to be set.
 * @param parameter_value The integer value to set for the parameter.
 * @param pluginName The name of the plugin for which the parameter is being set.
 * @return al_status_t The status of the operation, including any error codes and messages.
 */
al_status_t al_setvalue_int_scalar_parameter_plugin(const char* parameter_name, int parameter_value, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
		int dim = 0;
		int datatype = INTEGER_DATA;
		int *data = &parameter_value;
        LLplugin::setvalueParameterPlugin(parameter_name, datatype, dim, NULL, (void*) data, pluginName);
    }
    catch (const ALContextException& e) {
        status.code = alerror::context_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALLowlevelException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALPluginException& e) {
	status.code = alerror::lowlevel_err;
	ALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = alerror::unknown_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

/**
 * @brief Sets the value of a double scalar parameter for a specified plugin.
 *
 * This function sets the value of a double scalar parameter identified by its name
 * for a given plugin. It handles various exceptions that might occur during the process
 * and updates the status accordingly.
 *
 * @param parameter_name The name of the parameter to set.
 * @param parameter_value The double value to set for the parameter.
 * @param pluginName The name of the plugin for which the parameter value is to be set.
 * @return al_status_t The status of the operation, including error codes if any exceptions are caught.
 *
 * @exception ALContextException Thrown when there is a context-related error.
 * @exception ALLowlevelException Thrown when there is a low-level error.
 * @exception ALPluginException Thrown when there is a plugin-related error.
 * @exception std::exception Thrown for any other standard exceptions.
 */
al_status_t al_setvalue_double_scalar_parameter_plugin(const char* parameter_name, double parameter_value, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
	int dim = 0;
	int datatype = DOUBLE_DATA;
	double *data = &parameter_value;
        LLplugin::setvalueParameterPlugin(parameter_name, datatype, dim, NULL, (void*) data, pluginName);
    }
    catch (const ALContextException& e) {
        status.code = alerror::context_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALLowlevelException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALPluginException& e) {
	status.code = alerror::lowlevel_err;
	ALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = alerror::unknown_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

//HLI wrappers for plugins API
/**
 * @brief Registers a plugin with the given name.
 *
 * This function attempts to register a plugin by its name. It handles
 * exceptions that may occur during the registration process and sets
 * the appropriate status code and message.
 *
 * @param plugin_name The name of the plugin to register.
 * @return al_status_t The status of the registration process. The status
 *         code will be set to 0 if the registration is successful, or an
 *         appropriate error code if an exception is caught.
 */
al_status_t al_register_plugin(const char *plugin_name)
{
  al_status_t status;
  status.code = 0;
  try {
    LLplugin::registerPlugin(plugin_name);
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Unregisters a plugin by its name.
 *
 * This function attempts to unregister a plugin identified by the given
 * plugin name. It handles various exceptions that might be thrown during
 * the unregistration process and sets the appropriate status code and
 * message.
 *
 * @param plugin_name The name of the plugin to unregister.
 * @return al_status_t The status of the unregistration operation.
 *         - status.code = 0: Success.
 *         - status.code = alerror::context_err: Context error occurred.
 *         - status.code = alerror::lowlevel_err: Low-level error occurred.
 *         - status.code = alerror::unknown_err: An unknown error occurred.
 */
al_status_t al_unregister_plugin(const char *plugin_name)
{
  al_status_t status;
  status.code = 0;
  try {
    LLplugin::unregisterPlugin(plugin_name);
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Checks if a plugin is registered.
 *
 * This function checks whether a plugin with the given name is registered.
 *
 * @param pluginName The name of the plugin to check.
 * @param is_registered A pointer to a boolean that will be set to true if the plugin is registered, false otherwise.
 * @return An al_status_t structure containing the status code and message.
 *
 * @exception ALContextException If there is a context-related error.
 * @exception ALLowlevelException If there is a low-level error.
 * @exception std::exception If there is an unknown error.
 */
al_status_t al_is_plugin_registered(const char* pluginName, bool *is_registered) {
    al_status_t status;
    status.code = 0;
    try {
        *is_registered = LLplugin::isPluginRegistered(pluginName);
    }
    catch (const ALContextException& e) {
        status.code = alerror::context_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALLowlevelException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = alerror::unknown_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

/**
 * @brief Binds a plugin to a specified field path.
 *
 * This function attempts to bind a plugin identified by `pluginName` to the
 * field specified by `fieldPath`. It handles various exceptions that might
 * occur during the binding process and sets the appropriate status code and
 * message.
 *
 * @param fieldPath The path to the field where the plugin should be bound.
 * @param pluginName The name of the plugin to bind.
 * @return al_status_t The status of the binding operation, including a status
 * code and message.
 *
 * @exception ALContextException If there is a context-related error during
 * binding, the status code is set to `alerror::context_err`.
 * @exception ALLowlevelException If there is a low-level error during binding,
 * the status code is set to `alerror::lowlevel_err`.
 * @exception std::exception For any other exceptions, the status code is set
 * to `alerror::unknown_err`.
 */
al_status_t al_bind_plugin(const char* fieldPath, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
        LLplugin::bindPlugin(fieldPath, pluginName);
    }
    catch (const ALContextException& e) {
        status.code = alerror::context_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALLowlevelException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = alerror::unknown_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

/**
 * @brief Unbinds a plugin from a specified field path.
 *
 * This function attempts to unbind a plugin identified by `pluginName` from the 
 * field specified by `fieldPath`. It handles various exceptions that might occur 
 * during the unbinding process and sets the appropriate status code and message.
 *
 * @param fieldPath The path to the field from which the plugin should be unbound.
 * @param pluginName The name of the plugin to unbind.
 * @return al_status_t A status object containing the result of the unbinding operation.
 *         - status.code: 0 if successful, or an error code if an exception occurred.
 *         - status.message: A message describing the error if an exception occurred.
 */
al_status_t al_unbind_plugin(const char* fieldPath, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
        LLplugin::unbindPlugin(fieldPath, pluginName);
    }
    catch (const ALContextException& e) {
        status.code = alerror::context_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const ALLowlevelException& e) {
        status.code = alerror::lowlevel_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = alerror::unknown_err;
        ALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

/**
 * @brief Writes plugin metadata for a given context ID.
 *
 * This function attempts to write plugin metadata (name, description, commit, version, 
 * repository, parameters) using the provided context ID.
 * It handles various exceptions that may occur during the process and sets the
 * appropriate status code and message.
 *
 * @param ctxid The context ID for which the plugin metadata is to be written.
 * @return al_status_t The status of the operation, including a status code and message.
 *
 * @exception ALContextException Thrown if there is an error related to the context.
 * @exception ALLowlevelException Thrown if there is a low-level error.
 * @exception std::exception Thrown if an unknown error occurs.
 */
al_status_t al_write_plugins_metadata(int ctxid)
{
  al_status_t status;
  status.code = 0;
  try {
    LLplugin::writePluginsMetadata(ctxid);
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Binds readback plugins for a given context ID.
 *
 * This function attempts to bind readback plugins for the specified context ID.
 * It handles exceptions that may occur during the binding process and sets the
 * appropriate status code and message based on the type of exception caught.
 *
 * @param ctxid The context ID for which to bind readback plugins.
 * @return al_status_t The status of the binding operation, including a status code
 *         and message if an error occurred.
 *
 * Possible status codes:
 * - 0: Success
 * - alerror::context_err: An ALContextException was caught.
 * - alerror::lowlevel_err: An ALLowlevelException was caught.
 * - alerror::unknown_err: A standard exception was caught.
 */
al_status_t al_bind_readback_plugins(int ctxid)
{
  al_status_t status;
  status.code = 0;
  try {
    LLplugin::bindReadbackPlugins(ctxid);
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

/**
 * @brief Unbinds readback plugins for a given context ID.
 *
 * This function attempts to unbind readback plugins associated with the specified
 * context ID meaning it unregister plugins. It handles exceptions that may occur 
 * during the unbinding process and sets the appropriate status code and message.
 *
 * @param ctxid The context ID for which to unbind readback plugins.
 * @return al_status_t The status of the unbinding operation, including a status code
 *         and message if an error occurs.
 *
 * @exception ALContextException If there is an error related to the context.
 * @exception ALLowlevelException If there is a low-level error.
 * @exception std::exception If an unknown error occurs.
 */
al_status_t al_unbind_readback_plugins(int ctxid)
{
  al_status_t status;
  status.code = 0;
  try {
    LLplugin::unbindReadbackPlugins(ctxid);
  }
  catch (const ALContextException& e) {
    status.code = alerror::context_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const ALLowlevelException& e) {
    status.code = alerror::lowlevel_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }
  return status;
}








