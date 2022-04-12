#include "ual_lowlevel.h"

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

int Lowlevel::beginUriAction(const std::string &uri)
{
  int ctxID=ualerror::unknown_err;
  DataEntryContext *pctx=NULL;
  Backend *be=NULL;

  try {
    pctx = new DataEntryContext(uri);
  }
  catch (const UALContextException& e) {
    std::cerr << e.what() << "\n";
    ctxID = ualerror::context_err;
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
    DataEntryContext *pctx = static_cast<DataEntryContext *>(lle.context);
    *beid = pctx->getBackendID();
  }
  catch (const UALLowlevelException& e) {
    status.code = ualerror::lowlevel_err;
    UALException::registerStatus(status.message, __func__, e);
  }

  return status;
}


al_status_t ual_begin_dataentry_action(const char *uri, int mode, int *dectxID)
{
  al_status_t status = { 0 };

  status.code = 0;
  try {
    *dectxID = Lowlevel::beginUriAction(uri);
    LLenv lle = Lowlevel::getLLenv(*dectxID);
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
    if (pctx==NULL)
      throw UALLowlevelException("Wrong Context type stored",LOG);

    lle.backend->openPulse(pctx,
			   mode,
			   pctx->getOptions());

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
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
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
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
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
    DataEntryContext *pctx= dynamic_cast<DataEntryContext *>(lle.context); 
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
	lle.backend->endAction(actx);
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

al_status_t ual_build_uri_from_legacy_parameters(const int backendID, 
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
    catch (const UALContextException& e) {
        status.code = ualerror::lowlevel_err;
        UALException::registerStatus(status.message, __func__, e);
    }

    return status;
}
