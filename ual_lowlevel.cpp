#include "ual_lowlevel.h"

#include "dlfcn.h"
#include "access_layer_plugin.h"

#include <assert.h>
#include <string.h>
#include <complex.h>
#include <algorithm>

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
    throw UALBackendException(error_message, LOG);
  }
  llpluginsStore[std::string(name)].al_plugin = plugin;
}

bool LLplugin::getBoundPlugins(int ctxID, const char* fieldPath, std::vector<std::string> &pluginsNames) {
  LLenv lle = Lowlevel::getLLenv(ctxID);
  std::string fullPath = lle.context->fullPath() + "/" + std::string(fieldPath);
  auto got = boundPlugins.find(fullPath);
  if (got != boundPlugins.end()) {
    pluginsNames = got->second;
    return true;
  }
  else {
	OperationContext *opctx = nullptr;
    if (lle.context->getType() == CTX_ARRAYSTRUCT_TYPE) {
        opctx = (static_cast<ArraystructContext*> (lle.context))->getOperationContext();
    }
    else {
        opctx = static_cast<OperationContext*> (lle.context);
    }
    const std::string &dataObjectName = opctx->getDataobjectName();
    //printf("ids name = %s\n", dataObjectName.c_str());
    fullPath = dataObjectName + "/*";
    //printf("fullPath = %s\n", fullPath.c_str());
    auto got2 = boundPlugins.find(fullPath);
    if (got2 != boundPlugins.end()) {
		//printf("found fullPath = %s\n", fullPath.c_str());
		pluginsNames = got2->second;
		return true;
    }
  }
  return false;
}

bool LLplugin::getBoundPlugins(const char* dataobjectname, std::set<std::string> &pluginsNames) {
  std::string ids_name = std::string(dataobjectname);
  char *copy = strdup(dataobjectname);
  char* ids_name_cstr = strtok(copy, "/");
  if (ids_name_cstr != NULL) {
     ids_name = std::string(ids_name_cstr);
     free(ids_name_cstr);
  }
  for(auto it = boundPlugins.begin(); it != boundPlugins.end(); ++it) {
	  char* key = strdup(it->first.c_str());
	  char *token = strtok(key, "/");
	  if (token != NULL) {
		  std::string path = std::string(token);
		  if (path.compare(ids_name) == 0) {
			  std::vector<std::string> &plugins = it->second; 
			  for (auto &pluginName:plugins) 
			     pluginsNames.insert(pluginName);
		  }
	  }
	  free(key);
  }
  if (pluginsNames.size() > 0)
     return true;
  return false;
}

void LLplugin::bindPlugin(const char* fieldPath, const char* pluginName) {
    if (!isPluginRegistered(pluginName)) {
        char error_message[200];
        sprintf(error_message, "Plugin %s is not registered. Plugins need to be registered using ual_register_plugin(name) before to be bound.\n", pluginName);
        throw UALLowlevelException(error_message, LOG);
    }
    auto got = boundPlugins.find(std::string(fieldPath));
    if (got != boundPlugins.end()) {
		auto &plugins = got->second; 
        plugins.push_back(pluginName);
    }
    else {
		std::vector<std::string> plugins {pluginName};
		boundPlugins[fieldPath] = plugins;
	}
}

void LLplugin::unbindPlugin(const char* fieldPath, const char* pluginName) {
    auto got = boundPlugins.find(std::string(fieldPath));
    if (got == boundPlugins.end()) {
        printf("No plugin bound to field path:%s\n", fieldPath);
    }
    else {
		std::vector<std::string> &plugins = got->second;
		auto itr = std::find(plugins.begin(), plugins.end(), std::string(pluginName));
        if (itr != plugins.end()) {
            plugins.erase(itr);
            if (plugins.size() == 0)
			   boundPlugins.erase(got);
		}
    }
}

bool LLplugin::isPluginRegistered(const char* name) {
   return llpluginsStore.find(std::string(name)) != llpluginsStore.end();
}

void LLplugin::register_plugin(const char* plugin_name) {
    const char* AL_PLUGINS = std::getenv("AL_PLUGINS");
    if (AL_PLUGINS == NULL)
        throw UALLowlevelException("AL_PLUGINS environment variable not defined",LOG);

    if (isPluginRegistered(plugin_name)) {
        char error_message[200];
        sprintf(error_message, "Plugin %s already registered in the plugins store.\n", plugin_name);
        throw UALLowlevelException(error_message, LOG);
    }
    llpluginsStore[std::string(plugin_name)] = LLplugin();
    LLplugin &lle = llpluginsStore[std::string(plugin_name)];
    void* plugin_handler = lle.plugin_handler;

    create_t* create_plugin = NULL;
    destroy_t* destroy_plugin = NULL;

    std::string ids_plugin = std::string(AL_PLUGINS) + "/" + plugin_name + "_plugin.so";
    //printf("-->ids_plugins:%s\n", ids_plugin.c_str());
    plugin_handler =  dlopen(ids_plugin.c_str(), RTLD_LAZY);
    if (plugin_handler == nullptr) {
        //char error_message[200];
        //sprintf(error_message, "%s for plugin: %s.\n", dlerror(), plugin_name);
        printf("error:%s for plugin: %s\n", dlerror(), plugin_name);
        //throw UALLowlevelException(error_message, LOG);
    }
    assert(plugin_handler != nullptr);
    addPluginHandler(plugin_name, plugin_handler);
    //load the symbols
    create_plugin = (create_t*) dlsym(plugin_handler, "create");
    if (!create_plugin) {
        char error_message[200];
        sprintf(error_message, "Cannot load symbol create:%s for plugin:%s.\n", dlerror(), plugin_name);
        throw UALLowlevelException(error_message, LOG);
    }
    destroy_plugin = (destroy_t*) dlsym(plugin_handler, "destroy");
    if (!destroy_plugin) {
        char error_message[200];
        sprintf(error_message, "Cannot load symbol destroy:%s for plugin:%s.\n", dlerror(), plugin_name);
        throw UALLowlevelException(error_message, LOG);
    }
    //reset errors
    dlerror();
    access_layer_plugin* al_plugin = (access_layer_plugin*) lle.al_plugin;
    al_plugin = create_plugin();
    addPlugin(plugin_name, al_plugin);
    addDestroyPlugin(plugin_name, (void*) destroy_plugin);
}

void LLplugin::unregister_plugin(const char* plugin_name) {
    if (!isPluginRegistered(plugin_name)) {
        char error_message[200];
        sprintf(error_message, "Plugin %s not registered in the plugins store.\n", plugin_name);
        throw UALLowlevelException(error_message, LOG);
    }
    //Erasing all paths bound to this plugin from the boundPlugins map
    auto it = boundPlugins.begin();
    while(it != boundPlugins.end())
    {
		std::vector<std::string> &plugins = it->second;
		auto itr = std::find(plugins.begin(), plugins.end(), plugin_name);
        if (itr != plugins.end()) {
			const std::string &fieldPath = it->first; 
			unbindPlugin(fieldPath.c_str(), plugin_name);
			auto got = llpluginsStore.find(plugin_name);
			if (got != llpluginsStore.end()) {
				LLplugin &llp = got->second;
				access_layer_plugin* al_plugin_ptr = (access_layer_plugin*) llp.al_plugin;
				if (al_plugin_ptr != NULL) {
					//Deleting plugin instance
					destroy_t* destroy = (destroy_t*) llp.destroy_plugin;
					if (destroy != NULL) {
					   destroy(al_plugin_ptr);
					   llpluginsStore.erase(got);
					}
				}
		    }
		}
        it++;
    }
}

void LLplugin::setvalueParameterPlugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* plugin_name) {
	LLplugin &llp = llpluginsStore[plugin_name];
    access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
    al_plugin->setParameter(parameter_name, datatype, dim, size, data);
}

void LLplugin::begin_global_action_plugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, int opCtx) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->begin_global_action(pulseCtx, dataobjectname, mode, opCtx);
}

void LLplugin::begin_slice_action_plugin(const std::string &plugin_name, int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->begin_slice_action(pulseCtx, dataobjectname, mode, time, interp, opCtx);
}

void LLplugin::begin_arraystruct_action_plugin(const std::string &plugin_name, int ctxID, int *actxID, const char* fieldPath, const char* timeBasePath, int *arraySize) {
  LLplugin &llp = llpluginsStore[plugin_name];
  access_layer_plugin* al_plugin = (access_layer_plugin*) llp.al_plugin;
  al_plugin->begin_arraystruct_action(ctxID, actxID, fieldPath, timeBasePath, arraySize);
}

void LLplugin::read_data_plugin(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void **data, int datatype, int dim, int *size)
{
    LLenv lle = Lowlevel::getLLenv(ctxID);
    Context *c= dynamic_cast<Context *>(lle.context);
    std::string fullPath = c->fullPath() + "/" + std::string(field);
    access_layer_plugin* al_plugin = NULL;
    LLplugin &llp = llpluginsStore[plugin_name];
    al_plugin = (access_layer_plugin*) llp.al_plugin;
    int ret = al_plugin->read_data(ctxID, field, timebase, data, datatype, dim, size);
    if (ret == 0) {
        // no data
        Lowlevel::setDefaultValue(datatype, dim, data, size);
    }
}

void LLplugin::write_data_plugin(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void *data, int datatype, int dim, int *size)
{
    LLenv lle = Lowlevel::getLLenv(ctxID);
    Context *c= dynamic_cast<Context *>(lle.context);
    std::string fullPath = c->fullPath() + "/" + std::string(field);
    access_layer_plugin* al_plugin = NULL;
    LLplugin &llp = llpluginsStore[plugin_name];
    al_plugin = (access_layer_plugin*) llp.al_plugin;
    al_plugin->write_data(ctxID, field, timebase, data, datatype, dim, size);
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
      throw UALLowlevelException("Cannot find context "+std::to_string(idx)+
				 " in store",LOG);
  }
  catch (const std::exception& e) {
    throw UALLowlevelException("Cannot find context "+std::to_string(idx)+
			       " in store",LOG);
  }
  return lle;
}

LLenv Lowlevel::delLLenv(int idx)
{
  // atomic operation
  std::lock_guard<std::mutex> guard(Lowlevel::mutex);

  //if (idx)
  LLenv lle = llenvStore.at(idx);

  llenvStore[idx].backend = NULL;
  llenvStore[idx].context = NULL;
  if (idx == Lowlevel::curStoreElt-1)
    Lowlevel::curStoreElt--;

  return lle;
}

void Lowlevel::setValue(void *data, int type, int dim, void **var)
{
  if (dim==0) 
    {
      switch(type)
	{
	case ualconst::char_data:
	  **(char **)var = *(char*)data;
	  break;
	case ualconst::integer_data:
	  **(int**)var = *(int*)data;
	  break;
	case ualconst::double_data:
	  **(double**)var = *(double*)data;
	  break;
	case ualconst::complex_data:
	  **(std::complex<double>**)var = *(std::complex<double>*)data;
	  break;
	default:
	  throw UALLowlevelException("Unknown data type="+std::to_string(type),LOG);
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
	case ualconst::char_data:
	  **(char**)var = Lowlevel::EMPTY_CHAR;
	  break;
	case ualconst::integer_data:
	  **(int**)var = Lowlevel::EMPTY_INT;
	  break;
	case ualconst::double_data:
	  **(double**)var = Lowlevel::EMPTY_DOUBLE;
	  break;
	case ualconst::complex_data:
	  **(std::complex<double>**)var = Lowlevel::EMPTY_COMPLEX;
	  break;
	default:
	  throw UALLowlevelException("Unknown data type="+std::to_string(type),LOG);
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
    case ualconst::char_data:
      {
	char* convdata = (char*)malloc(size*sizeof(char));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    case ualconst::integer_data:
      {
	int* convdata = (int*)malloc(size*sizeof(int));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    case ualconst::double_data:
      {
	double* convdata = (double*)malloc(size*sizeof(double));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    case ualconst::complex_data:
      {
	std::complex<double>* convdata = (std::complex<double>*)malloc(size*sizeof(std::complex<double>));
	std::copy_n(data, size, convdata);
	return (void*)convdata;
      }
    default:
      throw UALLowlevelException("Unknown data type="+std::to_string(desttype),LOG);
    }
}

void Lowlevel::setConvertedValue(void *data, int srctype, int dim, int *size, int desttype, void** var)
{
  void* convdata;
  size_t totsize = 1;

  for (int i=0; i<dim; i++)
    totsize*=size[i];
  
  switch (srctype) {
  case ualconst::char_data:
    convdata = Lowlevel::convertData((char*)data,totsize,desttype);
    Lowlevel::setValue(convdata,desttype,dim,var);
    break;
      
  case ualconst::integer_data:
    convdata = Lowlevel::convertData((int*)data,totsize,desttype);
    Lowlevel::setValue(convdata,desttype,dim,var);
    break;

  case ualconst::double_data:
    convdata = Lowlevel::convertData((double*)data,totsize,desttype);
    Lowlevel::setValue(convdata,desttype,dim,var);
    break;
    
  case ualconst::complex_data:
    // can't convert, set default
    Lowlevel::setDefaultValue(desttype, dim, var, size);
    break;
  }

  free(data);
}





int Lowlevel::beginPulseAction(int backendID, int shot, int run, 
			       std::string usr, std::string tok, std::string ver)
{
  int ctxID=ualerror::unknown_err;
  PulseContext *pctx=NULL;
  Backend *be=NULL;

  try {
    pctx = new PulseContext(backendID, 
			    shot, 
			    run, 
			    usr, 
			    tok, 
			    ver);
  }
  catch (const UALContextException& e) {
    std::cerr << e.what() << "\n";
    ctxID = ualerror::context_err;
    pctx = NULL;
  }


  if (pctx != NULL) 
    {
      be = Backend::initBackend(backendID);
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


al_status_t ual_context_info(int ctxID, char **info)
{
  al_status_t status;
  std::stringstream desc;

  status.code = 0;
  if (ctxID==0)
    {
      const char *nullctx = "NULL context";
      int nullctxsize = strlen(nullctx)+1;
      *info = (char *)malloc(nullctxsize);
      mempcpy(*info, nullctx, strlen(nullctx));
      (*info)[strlen(nullctx)] = '\0';
    }
  else
    {
      try {
	LLenv lle = Lowlevel::getLLenv(ctxID);
	desc << "Context type = " 
	     << lle.context->getType() << "\n";
	desc << "Backend @ = " << lle.backend << "\n";
	desc << lle.context->print();
	const std::string& tmp = desc.str();
	int size = tmp.length()+1;
	*info = (char *)malloc(size);
	mempcpy(*info, tmp.c_str(), tmp.length());
    (*info)[tmp.length()] = '\0';
      }
      catch (const UALLowlevelException& e) {
	status.code = ualerror::lowlevel_err;
	UALException::registerStatus(status.message, __func__, e);
      }
    }

  return status;
}


al_status_t ual_get_backendID(int ctxID, int *beid)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(ctxID);

    switch (lle.context->getType())
      {
      case CTX_PULSE_TYPE:
	{
	  PulseContext* pctx = dynamic_cast<PulseContext *>(lle.context); 
	  if (pctx==NULL)
	    throw UALLowlevelException("Wrong Context type stored",LOG);
	  *beid = pctx->getBackendID();
	}
	break;
      case CTX_OPERATION_TYPE:
	{
	  OperationContext* octx = dynamic_cast<OperationContext *>(lle.context);
	  if (octx==NULL)
	    throw UALLowlevelException("Wrong Context type stored",LOG);
	  *beid = octx->getPulseContext()->getBackendID();
	}
	break;
      case CTX_ARRAYSTRUCT_TYPE:
	{
	  ArraystructContext* actx = dynamic_cast<ArraystructContext *>(lle.context);
	  if (actx==NULL)
	    throw UALLowlevelException("Wrong Context type stored",LOG);
	  *beid = actx->getOperationContext()->getPulseContext()->getBackendID();
	}
	break;
      default:
	throw UALLowlevelException("Unknown Context type stored",LOG);
      }
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_begin_pulse_action(const int backendID, const int shot, const int run, 
				   const char *usr, const char *tok, const char *ver,
				   int *pctxID)
{
  al_status_t status;

  status.code = 0;
  try {
    *pctxID = Lowlevel::beginPulseAction(backendID, 
					 shot, 
					 run, 
					 usr, 
					 tok, 
					 ver);
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_open_pulse(int pctxID, int mode, const char *options)
{
  al_status_t status = { 0 };

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);
    PulseContext *pctx= dynamic_cast<PulseContext *>(lle.context); 
    if (pctx==NULL)
      throw UALLowlevelException("Wrong Context type stored",LOG);

    std::string strOptions;
    if (options)
    {
      strOptions.assign(options);
    }
    lle.backend->openPulse(pctx,
			   mode,
			   strOptions);

    switch (mode) {
    case ualconst::open_pulse:
    case ualconst::force_open_pulse:
      std::pair<int,int> ver = lle.backend->getVersion(NULL);
      std::pair<int,int> sver = lle.backend->getVersion(pctx);
      if ((ver.first!=sver.first)||(ver.second<sver.second))
	throw UALLowlevelException("Compatibility between opened file version "+
				   std::to_string(sver.first)+"."+std::to_string(sver.second)+
				   " and backend "+pctx->getBackendName()+
				   " version "+std::to_string(ver.first)+"."+std::to_string(ver.second)+
				   " can't be ensured. ABORT.\n",LOG);
      break;
    }
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_close_pulse(int pctxID, int mode, const char *options)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);
    PulseContext *pctx= dynamic_cast<PulseContext *>(lle.context); 
    if (pctx==NULL)
      throw UALLowlevelException("Wrong Context type stored",LOG);

    lle.backend->closePulse(pctx,
			    mode,
			    options);
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_begin_global_action(int pctxID, const char* dataobjectname, int rwmode,
				    int *octxID)
{
  al_status_t status;
  OperationContext *octx=NULL;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID); 
    PulseContext *pctx= dynamic_cast<PulseContext *>(lle.context); 
    if (pctx==NULL) 
      throw UALLowlevelException("Wrong Context type stored",LOG);
  
    octx = new OperationContext(pctx, 
				std::string(dataobjectname),
				rwmode);
    lle.backend->beginAction(octx);

    switch (rwmode) {
    case ualconst::write_op:
    case ualconst::replace_op:
      std::pair<int,int> ver = lle.backend->getVersion(NULL);
      std::pair<int,int> sver = lle.backend->getVersion(pctx);
      if (ver.second!=sver.second)
	throw UALLowlevelException("Compatibility between opened file version "+
				   std::to_string(sver.first)+"."+std::to_string(sver.second)+
				   " and backend "+pctx->getBackendName()+
				   " version "+std::to_string(ver.first)+"."+std::to_string(ver.second)+
				   " can't be ensured (minor versions should match when writing). ABORT.\n",LOG);
      break;
    }
    *octxID = Lowlevel::addLLenv(lle.backend, octx); 
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_begin_slice_action(int pctxID, const char* dataobjectname, int rwmode, 
				   double time, int interpmode, int *octxID)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);
    PulseContext *pctx= dynamic_cast<PulseContext *>(lle.context); 
    if (pctx==NULL)
      throw UALLowlevelException("Wrong Context type stored",LOG);

    OperationContext *octx= new OperationContext(pctx, 
						 std::string(dataobjectname),
						 rwmode, 
						 ualconst::slice_op, 
						 time, 
						 interpmode);
    lle.backend->beginAction(octx);

    switch (rwmode) {
    case ualconst::write_op:
    case ualconst::replace_op:
      std::pair<int,int> ver = lle.backend->getVersion(NULL);
      std::pair<int,int> sver = lle.backend->getVersion(pctx);
      if (ver.second!=sver.second)
	throw UALLowlevelException("Compatibility between opened file version "+
				   std::to_string(sver.first)+"."+std::to_string(sver.second)+
				   " and backend "+pctx->getBackendName()+
				   " version "+std::to_string(ver.first)+"."+std::to_string(ver.second)+
				   " can't be ensured (minor versions should match when writing). ABORT.\n",LOG);
      break;
    }
    *octxID = Lowlevel::addLLenv(lle.backend, octx); 
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_end_action(int ctxID)
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
      catch (const UALBackendException& e) {
	status.code = ualerror::backend_err;
	UALException::registerStatus(status.message, __func__, e);
      }
      catch (const UALLowlevelException& e) {
	status.code = ualerror::lowlevel_err;
	UALException::registerStatus(status.message, __func__, e);
      }
      catch (const std::exception& e) {
	status.code = ualerror::unknown_err;
	UALException::registerStatus(status.message, __func__, e);
      }
    }
  
  return status;
}


al_status_t ual_write_data(int ctxID, const char *field, const char *timebase,  
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
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  
  return status;
}


al_status_t ual_read_data(int ctxID, const char *field, const char *timebase, 
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
	    throw UALLowlevelException("Wrong dimension of Data returned by backend: expected "+
				       ualconst::data_type_str.at(datatype-DATA_TYPE_0)+" in "+
				       std::to_string(dim)+"D but got "+
				       ualconst::data_type_str.at(retType-DATA_TYPE_0)+" in "+
				       std::to_string(retDim)+"D",LOG);
	  }
	else if (retType!=datatype)
	  {
	    Lowlevel::setConvertedValue(retData, retType, retDim, size, datatype, data);
	    UALException::registerStatus(status.message, __func__,
					 UALLowlevelException("Warning: "+lle.context->fullPath()+
							      "/"+field+" returned with type "+
							      ualconst::data_type_str.at(retType-DATA_TYPE_0)+
							      " while we expect type "+
							      ualconst::data_type_str.at(datatype-DATA_TYPE_0)+"\n"));
	  }
	else 
	  Lowlevel::setValue(retData, datatype, dim, data);
      }
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_delete_data(int octxID, const char *field)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(octxID);
    OperationContext *octx= dynamic_cast<OperationContext *>(lle.context); 
    if (octx==NULL)
      throw UALLowlevelException("Wrong Context type stored",LOG);

    lle.backend->deleteData(octx, std::string(field));
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_begin_arraystruct_action(int ctxID, const char *path, 
					 const char *timebase, int *size,
					 int *actxID)
{
  al_status_t status;
  ArraystructContext* actx=NULL;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(ctxID);

    ArraystructContext* parent = dynamic_cast<ArraystructContext*>(lle.context);
    if (parent!=NULL)
      {
	actx = new ArraystructContext(parent,
				      std::string(path),
				      std::string(timebase));
      }
    else
      {
	OperationContext* octx = dynamic_cast<OperationContext*>(lle.context);
	actx = new ArraystructContext(octx,
				      std::string(path),
				      std::string(timebase));
      }
    lle.backend->beginArraystructAction(actx, size);

    if (*size == 0)
      {
	// no data
	delete(actx);
	*actxID = 0; 
      }
    else
      {
	*actxID = Lowlevel::addLLenv(lle.backend, actx); 
	if (*size < 0)
	  {
	    throw UALLowlevelException("Returned size for array of structure is negative! ("+
				       std::to_string(*size)+")",LOG);
	  }
      }
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALBackendException& e) {
    status.code = ualerror::backend_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_iterate_over_arraystruct(int aosctxID, 
					 int step)
{
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(aosctxID);
    ArraystructContext *actx = static_cast<ArraystructContext *>(lle.context);
    
    actx->nextIndex(step);
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}

//HLI Wrappers for calling LL functions - Call plugins if required

al_status_t hli_begin_global_action(int pctxID, const char* dataobjectname, int rwmode,
                    int *octxID)
{
  al_status_t status;

  status.code = 0;

  try {
    status = ual_begin_global_action(pctxID, dataobjectname, rwmode, octxID);
    if (status.code != 0)
        return status;
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataobjectname, pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
           LLplugin::begin_global_action_plugin(pluginName, pctxID, dataobjectname, rwmode, *octxID);
    }
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALPluginException& e) {
	printf("An AL plugin exception has occurred:%s\n", e.what());
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

al_status_t hli_begin_slice_action(int pctxID, const char* dataobjectname, int rwmode, 
                   double time, int interpmode, int *octxID)
{
  al_status_t status;

  status.code = 0;
  try {
    status = ual_begin_slice_action(pctxID, dataobjectname, rwmode, time, interpmode, octxID);
    if (status.code != 0)
     return status;
    std::set<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(dataobjectname, pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
		   LLplugin::begin_slice_action_plugin(pluginName, pctxID, dataobjectname, rwmode, time, interpmode, *octxID);
	}
   }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALPluginException& e) {
	printf("An AL plugin exception has occurred:%s\n", e.what());
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

al_status_t hli_begin_arraystruct_action(int ctxID, const char *path, 
                     const char *timebase, int *size,
                     int *actxID)
{
  al_status_t status;
  status.code = 0;
  try {
    status = ual_begin_arraystruct_action(ctxID, path, timebase, size, actxID);
      if (status.code != 0)
         return status;
    std::vector<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(ctxID, path, pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
           LLplugin::begin_arraystruct_action_plugin(pluginName, ctxID, actxID, path, timebase, size);
    }
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALPluginException& e) {
	printf("An AL plugin exception has occurred:%s\n", e.what());
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  
  return status;
}

al_status_t hli_write_data(int ctxID, const char *field, const char *timebase,  
			 void *data, int datatype, int dim, int *size)
{
  al_status_t status;

  status.code = 0;
  try {
    std::vector<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(ctxID, field, pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
           LLplugin::write_data_plugin(pluginName, ctxID, field, timebase, data, datatype, dim, size);
    }
    else {
		if (Lowlevel::data_has_non_zero_shape(datatype, data, dim, size))
			status = ual_write_data(ctxID, field, timebase, data, datatype, dim, size);
        return status;
    }
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALPluginException& e) {
	printf("An AL plugin exception has occurred:%s\n", e.what());
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

al_status_t hli_read_data(int ctxID, const char *field, const char *timebase, 
              void **data, int datatype, int dim, int *size)
{
  al_status_t status;

  status.code = 0;
  try {
  
    std::vector<std::string> pluginsNames;
    bool isPluginBound = LLplugin::getBoundPlugins(ctxID, field, pluginsNames);
    if (isPluginBound) {
		for (const auto& pluginName : pluginsNames)
           LLplugin::read_data_plugin(pluginName, ctxID, field, timebase, data, datatype, dim, size);
    }
    else {
        status = ual_read_data(ctxID, field, timebase, data, datatype, dim, size);
        if (status.code != 0)
            return status;
    }
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALPluginException& e) {
	printf("An AL plugin exception has occurred:%s\n", e.what());
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  return status;
   
}

al_status_t hli_setvalue_parameter_plugin(const char* parameter_name, int datatype, int dim, int *size, void *data, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
        LLplugin::setvalueParameterPlugin(parameter_name, datatype, dim, size, data, pluginName);
    }
    catch (const UALContextException& e) {
        status.code = ualerror::context_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALLowlevelException& e) {
        status.code = ualerror::lowlevel_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALPluginException& e) {
		printf("An AL plugin exception has occurred:%s\n", e.what());
		status.code = ualerror::lowlevel_err;
		UALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = ualerror::unknown_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

al_status_t hli_setvalue_int_scalar_parameter_plugin(const char* parameter_name, int parameter_value, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
		int dim = 0;
		int datatype = INTEGER_DATA;
		int *data = &parameter_value;
        LLplugin::setvalueParameterPlugin(parameter_name, datatype, dim, NULL, (void*) data, pluginName);
    }
    catch (const UALContextException& e) {
        status.code = ualerror::context_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALLowlevelException& e) {
        status.code = ualerror::lowlevel_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALPluginException& e) {
		printf("An AL plugin exception has occurred:%s\n", e.what());
		status.code = ualerror::lowlevel_err;
		UALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = ualerror::unknown_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

al_status_t hli_setvalue_double_scalar_parameter_plugin(const char* parameter_name, double parameter_value, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
		int dim = 0;
		int datatype = DOUBLE_DATA;
		double *data = &parameter_value;
        LLplugin::setvalueParameterPlugin(parameter_name, datatype, dim, NULL, (void*) data, pluginName);
    }
    catch (const UALContextException& e) {
        status.code = ualerror::context_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALLowlevelException& e) {
        status.code = ualerror::lowlevel_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALPluginException& e) {
		printf("An AL plugin exception has occurred:%s\n", e.what());
		status.code = ualerror::lowlevel_err;
		UALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = ualerror::unknown_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

//HLI wrappers for plugins API
al_status_t hli_register_plugin(const char *plugin_name)
{
  al_status_t status;
  status.code = 0;
  try {
    LLplugin::register_plugin(plugin_name);
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

al_status_t hli_unregister_plugin(const char *plugin_name)
{
  al_status_t status;
  status.code = 0;
  try {
    LLplugin::unregister_plugin(plugin_name);
  }
  catch (const UALContextException& e) {
    status.code = ualerror::context_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  catch (const std::exception& e) {
    status.code = ualerror::unknown_err;
    UALException::registerStatus(status.message, __func__, e);
  }
  return status;
}

al_status_t hli_bind_plugin(const char* fieldPath, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
        LLplugin::bindPlugin(fieldPath, pluginName);
    }
    catch (const UALContextException& e) {
        status.code = ualerror::context_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALLowlevelException& e) {
        status.code = ualerror::lowlevel_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = ualerror::unknown_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    return status;
}

al_status_t hli_unbind_plugin(const char* fieldPath, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    try {
        LLplugin::unbindPlugin(fieldPath, pluginName);
    }
    catch (const UALContextException& e) {
        status.code = ualerror::context_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const UALLowlevelException& e) {
        status.code = ualerror::lowlevel_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    catch (const std::exception& e) {
        status.code = ualerror::unknown_err;
        UALException::registerStatus(status.message, __func__, e);
    }
    return status;
}
