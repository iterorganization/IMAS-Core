#include "al_lowlevel.h"

#include "dlfcn.h"
#include "access_layer_plugin.h"
#include "access_layer_plugin_manager.h"
#include <boost/filesystem.hpp>

#include <assert.h>
#include <string.h>
#include <complex.h>
#include <algorithm>
#include <regex>

#include <signal.h>

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
std::vector<std::string> LLplugin::readbackPlugins;
std::string LLplugin::getOperationPath;
std::vector<std::string> LLplugin::pluginsNames;
std::map<std::string, std::vector<std::string>> LLplugin::get_plugins;

void LLplugin::addPluginHandler(const char* name, void *plugin_handler) {
  llpluginsStore[std::string(name)].plugin_handler = plugin_handler;
}

void LLplugin::addDestroyPlugin(const char* name, void *destroy_plugin) {
  llpluginsStore[std::string(name)].destroy_plugin = destroy_plugin;
}

void LLplugin::addPlugin(const char* name, void *plugin) {
  if (!plugin) {
    char error_message[200];
    sprintf(error_message, "Plugin %s is NULL and can not be registered in the plugins store. Is the plugin implementing the create() function?\n", name);
    throw ALBackendException(error_message, LOG);
  }
  llpluginsStore[std::string(name)].al_plugin = plugin;
}

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

void LLplugin::getFullPath(int opctxID, const char* fieldPath,  std::string &full_path) {
  LLenv lle = Lowlevel::getLLenv(opctxID);
  assert(lle.context->getType() == CTX_OPERATION_TYPE);
  OperationContext *opctx = static_cast<OperationContext*> (lle.context);
  getFullPathFromOperationContext(opctx, fieldPath, full_path);
}

bool LLplugin::pluginsFrameworkEnabled(){
  char *pluginsFrameworkEnabled =  getenv("IMAS_AL_ENABLE_PLUGINS");
  if (!pluginsFrameworkEnabled) 
       return false;
  std::string b = std::string(pluginsFrameworkEnabled);
  return  (b == "TRUE");
} 

void LLplugin::checkIfPluginsFrameworkIsEnabled(){
  if(!pluginsFrameworkEnabled())
       throw ALLowlevelException("Plugins feature is disabled. Set the global variable 'IMAS_AL_ENABLE_PLUGINS' to 'TRUE' to enable this feature.");
} 

void LLplugin::getFullPathFromOperationContext(OperationContext *opctx, const char* fieldPath,  std::string &full_path) {
  std::string path = "";
  std::string dataObjectName = opctx->getDataobjectName();
  size_t found_sep = dataObjectName.find("/");
  if (found_sep == std::string::npos) 
      dataObjectName += ":0";
  else
      dataObjectName = std::regex_replace(dataObjectName, std::regex("/"), ":");
  path = dataObjectName;
  full_path = path + "/" + std::string(fieldPath);
}

bool LLplugin::getBoundPlugins(int ctxID, const char* fieldPath, std::vector<std::string> &pluginsNames) {
	if(!pluginsFrameworkEnabled()) return false;
  std::string fullPath;
  std::string dataObjectName;
  getFullPath(ctxID, fieldPath, fullPath, dataObjectName);
  //printf("--> getBoundPlugins::fullPath = %s\n ", fullPath.c_str());

  bool t = getBoundPlugins(fullPath, pluginsNames);
  if (t)
    return true;

  //we are searching now path like :"ids_name:occ/*" where occ is the occurrence and * is representing all nodes
  fullPath = dataObjectName + "/*";
  t = getBoundPlugins(fullPath, pluginsNames);
  if (t)
    return true;

  //we are now searching now path like :"ids_name:*/*" where latest * is representing all nodes
  size_t index = dataObjectName.find(":");
  fullPath = dataObjectName.substr(0, index - 1)  + ":*/*";
  if (t)
    return true;
  
  return false;
}

bool LLplugin::getBoundPlugins(const std::string &fullPath, std::vector<std::string> &pluginsNames) {
  if(!pluginsFrameworkEnabled()) return false;
  auto got = boundPlugins.find(fullPath);
  if (got != boundPlugins.end()) {
    pluginsNames = got->second;
    return true;
  }
  return false;
}

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

    //std::cout<<"fieldPath= "<< fieldPath <<std::endl;

    std::regex pattern("#?(([a-zA-Z0-9_\\-]*)(:([0-9]*))?/([:,()/a-zA-Z0-9_\\-]*))?"); //pattern for #idsname[:occurrence][/idspath]

    std::string full_path;

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
        sprintf(error_message, "bindPlugin: bad format: (%s) should follow the syntax of an URI fragment.\n", fieldPath);
        throw ALLowlevelException(error_message, LOG);
    }

    auto got = boundPlugins.find(idspath);
    if (got != boundPlugins.end()) {
		    auto &plugins = got->second;
        auto got2 = std::find(plugins.begin(), plugins.end(), pluginName);
        if ( got2 != plugins.end()) {
            char error_message[200];
            sprintf(error_message, "Plugin %s is already bound to path: %s.\n", pluginName, idspath.c_str());
            throw ALLowlevelException(error_message, LOG);
        }
        else
           plugins.push_back(pluginName);
    }
    else {
		    std::vector<std::string> plugins {pluginName};
        //printf("bindPlugin::binding plugin %s to path=%s\n", pluginName, full_path.c_str());
		    boundPlugins[full_path] = plugins;
    } 
}

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

void LLplugin::unbindPlugin(const char* fieldPath, const char* pluginName) {
    unbindPlugin(fieldPath, pluginName, boundPlugins);
}

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

bool LLplugin::isPluginRegistered(const char* name) {
   checkIfPluginsFrameworkIsEnabled();
   return llpluginsStore.find(std::string(name)) != llpluginsStore.end();
}

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
        printf("error:%s for plugin: %s\n", dlerror(), plugin_name);
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


void LLplugin::setvalueParameterPlugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* plugin_name) {
	LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->setParameter(parameter_name, datatype, dim, size, data);
}

void LLplugin::beginGlobalActionPlugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, const char* datapath, int mode, int opCtx) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->begin_global_action(pulseCtx, dataobjectname, datapath, mode, opCtx);
}

void LLplugin::beginSliceActionPlugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->begin_slice_action(pulseCtx, dataobjectname, mode, time, interp, opCtx);
}

void LLplugin::beginArraystructActionPlugin(const std::string &plugin_name, int ctxID, int *actxID, 
const char* fieldPath, const char* timeBasePath, int *arraySize) {
  AccessLayerPluginManager alplugin_manager;
  alplugin_manager.begin_arraystruct_action_handler(plugin_name, ctxID, actxID, fieldPath, timeBasePath, arraySize);
}

void LLplugin::endActionPlugin(int ctxID)
{
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.end_action_plugin_handler(ctxID);
}

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

void LLplugin::bindReadbackPlugins(int ctxID) { //function called before a get()
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.bind_readback_plugins(ctxID);
}

void LLplugin::unbindReadbackPlugins(int ctxID) { //function called after a get()
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.unbind_readback_plugins(ctxID);
}

void LLplugin::writePluginsMetadata(int ctxID) { //function called at the end of a put()
    if(!pluginsFrameworkEnabled()) return;
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.write_plugins_metadata(ctxID);
}

void LLplugin::writeDataPlugin(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void *data, int datatype, int dim, int *size)
{
    AccessLayerPluginManager alplugin_manager;
    alplugin_manager.write_data_plugin_handler(plugin_name, ctxID, field, timebase, 
              data, datatype, dim, size);
}

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

void Lowlevel::createAOS(int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *size){
        LLenv lle = Lowlevel::getLLenv(ctxID);
        ArraystructContext* actx = lle.create(fieldPath, timeBasePath);
        lle.backend->beginArraystructAction(actx, size); 
        *actxID = Lowlevel::addLLenv(lle.backend, actx);
  } 

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

int Lowlevel::beginUriAction(const std::string &uri)
{
  int ctxID=alerror::unknown_err;
  DataEntryContext *pctx=NULL;
  Backend *be=NULL;

  try {
    pctx = new DataEntryContext(uri);
  }
  catch (const ALContextException& e) {
    std::cerr << e.what() << "\n";
    ctxID = alerror::context_err;
    pctx = NULL;
  }


  if (pctx != NULL) 
    {
      be = Backend::initBackend(pctx->getBackendID());
      // store reference of this object 
      ctxID = Lowlevel::addLLenv(be, pctx);
    }

  return ctxID;
}

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
	   
	for (int i = 0; i < dim; i++) {
		if (size[i] == 0)
		   return false;
     }
     return true;
}


#endif


//////////////////// IMPLEMENTATION OF C WRAPPERS ////////////////////


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


al_status_t al_begin_dataentry_action(const char *uri, int mode, int *dectxID)
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
  }
  catch (const std::exception& e) {
    status.code = alerror::unknown_err;
    ALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


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

al_status_t al_build_uri_from_legacy_parameters(const int backendID, 
                         const int shot, 
                         const int run, 
                         const char *user, 
                         const char *tokamak, 
                         const char *version,
                         const char *options,
                         char** uri) {
    al_status_t status;
    status.code = 0;

    char opt[1024];
    if (options == nullptr)
       strcpy(opt, "");
    else
       strcpy(opt, options);

    try {
       DataEntryContext::build_uri_from_legacy_parameters(backendID, 
                         shot, 
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

al_status_t al_begin_global_action(int pctxID, const char* dataobjectname, const char* datapath, int rwmode,
                    int *octxID)
{
  al_status_t status;

  status.code = 0;

  try {
    status = al_plugin_begin_global_action(pctxID, dataobjectname, datapath, rwmode, octxID);
    if (status.code != 0)
        return status;
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataobjectname, pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
           LLplugin::beginGlobalActionPlugin(pluginName, pctxID, dataobjectname, datapath, rwmode, *octxID);
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

al_status_t al_begin_slice_action(int pctxID, const char* dataobjectname, int rwmode, 
                   double time, int interpmode, int *octxID)
{
  al_status_t status;

  status.code = 0;
  try {
    status = al_plugin_begin_slice_action(pctxID, dataobjectname, rwmode, time, interpmode, octxID);
    if (status.code != 0)
     return status;
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataobjectname, pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
		   LLplugin::beginSliceActionPlugin(pluginName, pctxID, dataobjectname, rwmode, time, interpmode, *octxID);
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
              std::string message = "Error calling al_begin_arraystruct_action(): only one plugin is allowed to create an AOS context at a given path.\n";
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

al_status_t al_get_occurrences(const char* uri, const char* ids_name, int** occurrences_list, int* size)
{
  al_status_t status;

  status.code = 0;
  try {
    int pctxID;
    al_begin_dataentry_action(uri, FORCE_OPEN_PULSE, &pctxID);
    LLenv lle = Lowlevel::getLLenv(pctxID);
    lle.backend->get_occurrences(ids_name, occurrences_list, size);
    al_close_pulse(pctxID, FORCE_OPEN_PULSE);
    al_end_action(pctxID);
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

