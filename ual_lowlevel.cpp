#include "ual_lowlevel.h"

#include <assert.h>
#include <string.h>
#include <complex.h>

#include <signal.h>


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

void Lowlevel::setScalarValue(void *data, int type, void **var)
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
      *((char *) mempcpy(*info, nullctx, nullctxsize)) = '\0';
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
	*((char *) mempcpy(*info, tmp.c_str(), size)) = '\0';
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
  catch (const UALLowlevelException e) {
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
  al_status_t status;

  status.code = 0;
  try {
    LLenv lle = Lowlevel::getLLenv(pctxID);
    PulseContext *pctx= dynamic_cast<PulseContext *>(lle.context); 
    if (pctx==NULL)
      throw UALLowlevelException("Wrong Context type stored",LOG);

    lle.backend->openPulse(pctx,
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
	    std::cerr << "Warning: " << lle.context->fullPath() << "/" << field
		      << " returned with type " 
		      << ualconst::data_type_str.at(retType-DATA_TYPE_0) 
		      << " while we expect type " 
		      << ualconst::data_type_str.at(datatype-DATA_TYPE_0) 
		      << "\n";
	  }
	else if (dim==0) 
	  {
	    Lowlevel::setScalarValue(retData, datatype, data);
	    free(retData);
	  }
	else
	  *data = retData;
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
