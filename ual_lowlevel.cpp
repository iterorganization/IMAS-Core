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
std::map<std::string, std::string>  LLplugin::attachedPlugins;

void LLplugin::addPluginHandler(const char* name, void *plugin_handler) {
  llpluginsStore[std::string(name)].plugin_handler = plugin_handler;
}

void LLplugin::addDestroyPlugin(const char* name, void *destroy_plugin) {
  llpluginsStore[std::string(name)].destroy_plugin = destroy_plugin;
}

void LLplugin::addPlugin(const char* name, void *plugin) {
  llpluginsStore[std::string(name)].al_plugin = plugin;
}

bool LLplugin::attachedPlugin(int ctxID, const char* fieldPath, std::string &pluginName) {
  LLenv lle = Lowlevel::getLLenv(ctxID);
  Context *c= dynamic_cast<Context *>(lle.context);
  std::string fullPath = c->fullPath() + "/" + std::string(fieldPath);
  auto got = attachedPlugins.find(fullPath);
  if (got != attachedPlugins.end()) {
    pluginName = got->second;
    return true;
  }
  return false;
}

void LLplugin::attachPlugin(const char* fieldPath, const char* pluginName) {
    if (!isPluginRegistered(pluginName)) {
        char error_message[200];
        sprintf(error_message, "Plugin %s is not registered. Plugins need to be registered using ual_register_plugin(name) before to be attached.\n", pluginName);
        throw UALBackendException(error_message, LOG);
    }
    auto got = attachedPlugins.find(std::string(fieldPath));
    if (got != attachedPlugins.end()) {
        char error_message[200];
        sprintf(error_message, "Field path: %s has already an attached plugin (plugin name=%s).\n", fieldPath, (got->second).c_str());
        throw UALBackendException(error_message, LOG);
    }
    attachedPlugins[fieldPath] = pluginName;
}

void LLplugin::detachPlugin(const char* fieldPath, const char* pluginName) {
    auto got = attachedPlugins.find(std::string(fieldPath));
    if (got == attachedPlugins.end()) {
        printf("No plugin attached to field path:%s\n", fieldPath);
    }
    else {
        attachedPlugins.erase(got);
    }
}

bool LLplugin::isPluginRegistered(const char* name) {
   return llpluginsStore.find(std::string(name)) != llpluginsStore.end();
}

void LLplugin::register_plugin(const char* plugin_name) {
    const char* AL_PLUGINS = std::getenv("UAL_PLUGINS");
    if (AL_PLUGINS == NULL)
        throw UALLowlevelException("AL_PLUGINS environment variable not defined",LOG);

    if (isPluginRegistered(plugin_name)) {
        char error_message[200];
        sprintf(error_message, "Plugin %s already registered in the plugins store.\n", plugin_name);
        throw UALBackendException(error_message, LOG);
    }
    llpluginsStore[std::string(plugin_name)] = LLplugin();
    LLplugin &lle = llpluginsStore[std::string(plugin_name)];
    void* plugin_handler = lle.plugin_handler;

    create_t* create_plugin = NULL;
    destroy_t* destroy_plugin = NULL;

    std::string ids_plugin = std::string(AL_PLUGINS) + "/" + plugin_name + "_plugin.so";
    //printf("ids_plugins:%s\n", ids_plugin.c_str());
    plugin_handler =  dlopen(ids_plugin.c_str(), RTLD_LAZY);
    addPluginHandler(plugin_name, plugin_handler);
    //load the symbols
    create_plugin = (create_t*) dlsym(plugin_handler, "create");
    const char* dlsym_error = dlerror();
    if (dlsym_error) {
        char error_message[200];
        sprintf(error_message, "Cannot load symbol create:%s for plugin:%s.\n", dlerror(), plugin_name);
        throw UALBackendException(error_message, LOG);
    }
    destroy_plugin = (destroy_t*) dlsym(plugin_handler, "destroy");
    dlsym_error = dlerror();
    if (dlsym_error) {
        char error_message[200];
        sprintf(error_message, "Cannot load symbol destroy:%s for plugin:%s.\n", dlerror(), plugin_name);
        throw UALBackendException(error_message, LOG);
    }
    //reset errors
    dlerror();
    access_layer_plugin* al_plugin = (access_layer_plugin*) lle.al_plugin;
    al_plugin = create_plugin();
    addPlugin(plugin_name, al_plugin);
    addDestroyPlugin(plugin_name, (void*) destroy_plugin);
}

al_status_t LLplugin::begin_global_action_plugins(int pulseCtx, const char* dataobjectname, int mode, int opCtx) {
  al_status_t al_status;
  access_layer_plugin* al_plugin = NULL;
  auto it = llpluginsStore.begin();
  while (it != llpluginsStore.end()) {
      //const std::string &plugin_name = it->first;
      LLplugin &lle = it->second;
      al_plugin = (access_layer_plugin*) lle.al_plugin;
      al_status = al_plugin->begin_global_action(pulseCtx, dataobjectname, mode, opCtx);
      if (al_status.code != 0)
         return al_status;
      ++it;
  }
  al_status.code = 0;
  return al_status;
}

al_status_t LLplugin::begin_slice_action_plugins(int pulseCtx, const char* dataobjectname, int mode, double time, int interp, int opCtx) {
  al_status_t al_status;
  access_layer_plugin* al_plugin = NULL;
  auto it = llpluginsStore.begin();
  while (it != llpluginsStore.end()) {
      LLplugin &lle = it->second;
      al_plugin = (access_layer_plugin*) lle.al_plugin;
      al_status = al_plugin->begin_slice_action(pulseCtx, dataobjectname, mode, time, interp, opCtx);
      if (al_status.code != 0)
         return al_status;
      ++it;
  }
  al_status.code = 0;
  return al_status;
}

al_status_t LLplugin::begin_arraystruct_action_plugins(int ctx, const char* fieldPath, const char* timeBasePath, int arraySize) {
  al_status_t al_status;
  access_layer_plugin* al_plugin = NULL;
  auto it = llpluginsStore.begin();
  while (it != llpluginsStore.end()) {
      //const std::string &plugin_name = it->first;
      LLplugin &lle = it->second;
      al_plugin = (access_layer_plugin*) lle.al_plugin;
      al_status = al_plugin->begin_arraystruct_action(ctx, fieldPath, timeBasePath, arraySize);
      if (al_status.code != 0)
         return al_status;
      ++it;
  }
  al_status.code = 0;
  return al_status;
}

al_status_t LLplugin::read_data_plugin(const std::string &plugin_name, int ctxID, const char *field, const char *timebase, 
              void **data, int datatype, int dim, int *size)
{

al_status_t status;
void *retData=NULL;
LLenv lle = Lowlevel::getLLenv(ctxID);
Context *c= dynamic_cast<Context *>(lle.context);
std::string fullPath = c->fullPath() + "/" + std::string(field);

status.code = 0;
try {
    access_layer_plugin* al_plugin = NULL;
    LLplugin &llp = llpluginsStore[plugin_name];
    al_plugin = (access_layer_plugin*) llp.al_plugin;
    int ret = al_plugin->read_data(ctxID, field, timebase, &retData, datatype, dim, size);

    if (ret == 0) {
        // no data
        Lowlevel::setDefaultValue(datatype, dim, data, size);
    }
    else {
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

/*al_status_t LLplugin::ual_write_aos_content_plugins(int ctx, const char* fieldPath, const char* timeBasePath) {
  al_status_t al_status;
  access_layer_plugin* al_plugin = NULL;
  LLplugin llp = LLplugin::getPluginsStore();
  al_plugin = (access_layer_plugin*) llp.al_plugin;
  if (al_plugin != NULL) {
    return al_plugin->write_aos_content(ctx, fieldPath, timeBasePath);
  }
  al_status.code = 0;
  return al_status;
}*/

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
    *beid = lle.context->getBackendID();
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
  
    octx = new OperationContext(*pctx, 
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

    OperationContext *octx= new OperationContext(*pctx, 
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
  ArraystructContext *actx=NULL;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(ctxID);

    actx = new ArraystructContext(*(static_cast<OperationContext *>(lle.context)),
					std::string(path),
					std::string(timebase),
				  dynamic_cast<ArraystructContext *>(lle.context));

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
  al_status_t status = ual_begin_global_action(pctxID, dataobjectname, rwmode, octxID);
  if (status.code != 0)
     return status;
  return LLplugin::begin_global_action_plugins(pctxID, dataobjectname, rwmode, *octxID);
}

al_status_t hli_begin_slice_action(int pctxID, const char* dataobjectname, int rwmode, 
                   double time, int interpmode, int *octxID)
{
  al_status_t status = ual_begin_slice_action(pctxID, dataobjectname, rwmode, time, interpmode, octxID);
  if (status.code != 0)
     return status;
  return LLplugin::begin_slice_action_plugins(pctxID, dataobjectname, rwmode, time, interpmode, *octxID);
}

al_status_t hli_begin_arraystruct_action(int ctxID, const char *path, 
                     const char *timebase, int *size,
                     int *actxID)
{
  al_status_t status = ual_begin_arraystruct_action(ctxID, path, timebase, size, actxID);
  if (status.code != 0)
     return status;
  return LLplugin::begin_arraystruct_action_plugins(*actxID, path, timebase, *size);
}

al_status_t hli_read_data(int ctxID, const char *field, const char *timebase, 
              void **data, int datatype, int dim, int *size)
{
  std::string pluginName;
  bool isPluginAttached = LLplugin::attachedPlugin(ctxID, field, pluginName);
  if (isPluginAttached) {
     return LLplugin::read_data_plugin(pluginName, ctxID, field, timebase, data, datatype, dim, size);
  }
  else {
    return ual_read_data(ctxID, field, timebase, data, datatype, dim, size);
  }
}

//HLI wrappers for plugins API
al_status_t hli_register_plugin(const char *plugin_name)
{
  al_status_t status;
  status.code = 0;
  LLplugin::register_plugin(plugin_name);
  return status;
}

al_status_t hli_attach_plugin(const char* fieldPath, const char* pluginName) {
    al_status_t status;
    status.code = 0;
    LLplugin::attachPlugin(fieldPath, pluginName);
    return status;
}