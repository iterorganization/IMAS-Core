#include "ual_context.h"

#include <unordered_map>
#include "uri_parser.h"

std::atomic<unsigned long int> Context::SID(0);


std::ostream& operator<< (std::ostream& o, Context const& ctx)
{
  return o << ctx.print();
}

std::ostream& operator<<(std::ostream& o, DataEntryContext const& ctx)
{
  return o << ctx.print();
}

std::ostream& operator<< (std::ostream& o, OperationContext const& ctx)
{
  return o << ctx.print();
}

std::ostream& operator<< (std::ostream& o, ArraystructContext const& ctx)
{
  return o << ctx.print();
}



/// Context ///


std::string Context::print() const 
{
  std::string s = "context_uid \t\t = " + 
    std::to_string(this->uid) + "\n";
  return s;
}

std::string Context::fullPath() const
{
  std::string s = "";
  return s;
}


unsigned long int Context::getUid() const 
{
  return uid;
}

/// DataEntryContext ///

DataEntryContext::DataEntryContext(std::string uri_) : uri(uri_)
{

  auto uri_object = uri::parse_uri(uri_);

  if (uri_object.error != uri::Error::None)
        throw UALContextException("Unable to parse the URI",LOG);

  setBackendID(uri_object.path, uri_object.authority.host);

  std::string userFromURI;
  if(uri::queryParameter("user", userFromURI, uri_object))
    user = userFromURI;
  else {
    char *usr = std::getenv("USER"); 
    if (usr!=NULL)
        user = usr;
  }

  std::string databaseFromURI;
  if(uri::queryParameter("database", databaseFromURI, uri_object))
    tokamak = databaseFromURI;
  else {
    char *tok = std::getenv("TOKAMAKNAME");
    if (tok!=NULL)
        tokamak = tok;
  }

  std::string versionFromURI;
  if(uri::queryParameter("version", versionFromURI, uri_object))
      version = versionFromURI;
  else {
    char *ver = std::getenv("DATAVERSION");
    if (ver!=NULL)
        version = ver;
  }

  if (!userFromURI.empty()) {
     if (tokamak.empty() || version.empty()) {
         throw UALContextException("'user' is specified in URI however 'database' or 'version' parameters are not both defined (nor in URI, nor in global variables)",LOG);
     }
  }

  if (!tokamak.empty()) {
     if (user.empty() || version.empty()) {
         throw UALContextException("'database' is specified (in URI or from global variable 'TOKAMAKNAME'), however 'user' or 'version' parameters are not both defined (nor in URI, nor in global variables)",LOG);
     }
  }

  if (!version.empty()) {
     if (user.empty() || tokamak.empty()) {
         throw UALContextException("'version' is specified (in URI or from global variable 'DATAVERSION'), however 'user' or 'database' parameters are not defined (nor in URI, nor in global variables)",LOG);
     }
  }

  std::string pathFromURI;
  if(uri::queryParameter("path", pathFromURI, uri_object))
      path = pathFromURI;
      
  if (!pathFromURI.empty() && (!userFromURI.empty() && !databaseFromURI.empty() && !versionFromURI.empty())) {
      throw UALContextException("path should not be specified in the URI since user/database/version parameters are specified",LOG);
  }
  
  std::string pulseFromURI;
   if(uri::queryParameter("pulse", pulseFromURI, uri_object))
      pulse = pulseFromURI;

  std::string shotFromURI;
  if(uri::queryParameter("shot", shotFromURI, uri_object)) {
	  try {
          shot = std::stoi(shotFromURI);
       }
       catch(std::invalid_argument &exc) {
		  throw UALContextException(exc.what(),LOG);
       }
  }

  std::string runFromURI;
  if(uri::queryParameter("run", runFromURI, uri_object)) {
      try {
          run = std::stoi(runFromURI);
       }
       catch(std::invalid_argument &exc) {
		  throw UALContextException(exc.what(),LOG);
       }
  }
      
  if ( (!shotFromURI.empty() && !pulseFromURI.empty()) || (!runFromURI.empty() && !pulseFromURI.empty()) ) {
	  throw UALContextException("ambiguous URI, both shot/run and pulse parameters are specified in the URI",LOG);
  }  
  
  if (!pulseFromURI.empty()) {
	  char* s = strdup( pulseFromURI.c_str());
	  char* token = strtok(s, "/");
	  char* token2 = strtok(NULL, "/");
	  if (token != NULL && token2 != NULL) { // ID = S/R
		  //printf("find ID=S/R\n");
		  try {
			  shotFromURI = std::string(token);
			  shot = std::stoi(shotFromURI);
			  runFromURI = std::string(token2);
			  run = std::stoi(runFromURI);
		  }
	      catch(std::invalid_argument &exc) {
			  throw UALContextException(exc.what(),LOG);
		  }
	   }
	   else {  // ID = S or keyword ?
		    //printf("find ID=S or keyword\n");
		    try {
			  shotFromURI = std::string(pulseFromURI);
			  shot = std::stoi(shotFromURI);
		  }
	      catch(std::invalid_argument &exc) {
			  //probably a keyword
		  }
	   }
	   free(s);
  }

  std::string optionsFromURI;
  if(uri::queryParameter("options", optionsFromURI, uri_object))
      options = optionsFromURI;

  this->uid = ++SID;
}

std::string DataEntryContext::print() const 
{
  std::string s = this->print() +
   "uri \t\t\t = " + uri + "\n";
  return s;
}

std::string DataEntryContext::fullPath() const
{
  std::string s = "";
  return s;
}

int DataEntryContext::getType() const 
{
  return CTX_PULSE_TYPE;
}

int DataEntryContext::getBackendID() const
{ 
  return backend_id; 
}

std::string DataEntryContext::getBackendName() const 
{ 
  return ualconst::backend_id_str.at(backend_id-BACKEND_ID_0); 
}

int DataEntryContext::getShot() const
{ 
  return shot; 
}

int DataEntryContext::getRun() const 
{ 
  return run; 
}

std::string DataEntryContext::getUser() const
{ 
  return user; 
}

std::string DataEntryContext::getTokamak() const
{ 
  return tokamak; 
}

std::string DataEntryContext::getVersion() const
{ 
  return version; 
}

std::string DataEntryContext::getOptions() const
{ 
  return options; 
}

std::string DataEntryContext::getURI() const
{
  return uri;
}

std::string DataEntryContext::getPath() {

    std::string filePath;
    
    if (path.empty()) {
		
		if (!strcmp(user.c_str(), "public")) {
			char *home = getenv("IMAS_HOME");
			if (home == NULL)
				throw UALBackendException("when user is 'public', IMAS_HOME environment variable should be set.", LOG);
			filePath += home;
			filePath += "/shared/imasdb/";
			filePath += tokamak;
			filePath += "/";
			filePath += version;
		} else if (user.rfind("/", 0) == 0) {
			filePath += user;
			filePath += "/";
			filePath += tokamak;
			filePath += "/";
			filePath += version;
		} else {
#ifdef WIN32
			char szHomeDir[256];
			if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szHomeDir))) {
				filePath += szHomeDir;
#else 
			struct passwd *pw = getpwnam( user.c_str() );
			if( pw != NULL ) {
				filePath += pw->pw_dir;
#endif
			}
			else {
				throw  UALBackendException("Can't find or access "+std::string(user)+" user's data",LOG);
			}
			filePath += "/public/imasdb/";
			filePath += tokamak;
			filePath += "/";
			filePath += version;
    }
    
		if (!pulse.empty()) {
			filePath += "/";
			filePath += pulse;
		}
		else {
			filePath += "/";
			filePath += std::to_string(shot);
			filePath += "/";
			filePath += std::to_string(run);
		}
		
    }
    else {
		filePath = path;
	}

    return filePath;
}

void DataEntryContext::setBackendID(const std::string &path, const std::string &host) {

    if (path.compare("mdsplus") == 0)
        backend_id = MDSPLUS_BACKEND;
    else if (path.compare("hdf5") == 0)
        backend_id = HDF5_BACKEND;
    else if (path.compare("ascii") == 0)
        backend_id = ASCII_BACKEND;
    else if (path.compare("memory") == 0)
        backend_id = MEMORY_BACKEND;
    else if (!host.empty()){
           backend_id = UDA_BACKEND;
    }
    else {
        throw UALContextException("Unable to identify a backend from the URI",LOG);
    }
}


void DataEntryContext::addOptionToURIQuery(const std::string &option_name, const std::string &option_value) {
    uri += ";" + option_name + "=" + option_value;
}


void DataEntryContext::build_uri_from_legacy_parameters(const int backendID, 
                         const int shot, 
                         const int run, 
                         const char *user, 
                         const char *tokamak, 
                         const char *version,
                         const char *options,
                         char** uri) {

    std::stringstream desc;
    std::string backend = getURIBackend(backendID);
    desc << "imas:" << backend.c_str() << "?user=" << user << ";shot=" << shot << ";run=" << run << ";database=" << tokamak << ";version=" << version[0] << ";options=" << options << "\n";
    const std::string& tmp = desc.str();
    int size = tmp.length()+1;
    *uri = (char *)malloc(size);
    memcpy(*uri, tmp.c_str(), tmp.length());
    (*uri)[tmp.length()] = '\0';
}

std::string DataEntryContext::getURIBackend(int backend_id)
{
    std::string backend;

    if (backend_id == MDSPLUS_BACKEND) {
        backend = "mdsplus";
    }
    else if (backend_id == HDF5_BACKEND) {
        backend = "hdf5";
    }
    else if (backend_id == ASCII_BACKEND) {
        backend = "ascii";
    }
    else if (backend_id == MEMORY_BACKEND) {
        backend = "memory";
    }
    else if (backend_id == UDA_BACKEND) {
        throw UALContextException("getURIBackend, converting backend ID to backend URI string not yet implemented",LOG);
    }
    return backend;
}


/// OperationContext ///

OperationContext::OperationContext(DataEntryContext* ctx, std::string dataobject, int access)
  : pctx(ctx), dataobjectname(dataobject)
{
  rangemode = ualconst::global_op;
  time = ualconst::undefined_time;
  interpmode = ualconst::undefined_interp;

  try {
    ualconst::op_access_str.at(access-OP_ACCESS_0);
  } 
  catch (const std::out_of_range& e) {
    throw UALContextException("Wrong access mode "+std::to_string(access),LOG);
  }
  accessmode = access;
  pctx = ctx;
  this->uid = ++SID;
}

OperationContext::OperationContext(DataEntryContext* ctx, std::string dataobject, int access, 
				   int range, double t, int interp)
  : DataEntryContext(ctx->getURI()), pctx(ctx), dataobjectname(dataobject), time(t)
{
  try {
    ualconst::op_range_str.at(range-OP_RANGE_0);
  } 
  catch (const std::out_of_range& e) {
    throw UALContextException("Wrong range mode "+std::to_string(range),LOG);
  }
  rangemode = range;

  try {
    ualconst::op_access_str.at(access-OP_ACCESS_0);
  } 
  catch (const std::out_of_range& e) {
    throw UALContextException("Wrong access mode "+std::to_string(access),LOG);
  }
  accessmode = access;

  try {
    ualconst::op_interp_str.at(interp-OP_INTERP_0);
  } 
  catch (const std::out_of_range& e) {
    throw UALContextException("Wrong interp mode "+std::to_string(interp),LOG);
  }
  interpmode = interp;

  // test consistency [missing or wrong expected args, not all possible missmatches!]
  if (rangemode==ualconst::slice_op)
    {
      if (accessmode==ualconst::read_op && interpmode==ualconst::undefined_interp)
	throw UALContextException("Missing interpmode",LOG);
    }
  this->uid = ++SID;
}

std::string OperationContext::print() const 
{
  std::string s = this->pctx->print() +
    "dataobjectname \t\t = " + this->dataobjectname + "\n" +
    "accessmode \t\t = " + std::to_string(this->accessmode) + 
    " (" + ualconst::op_access_str.at(this->accessmode-OP_ACCESS_0) + ")\n" +
    "rangemode \t\t = " + std::to_string(this->rangemode) +
    " (" + ualconst::op_range_str.at(this->rangemode-OP_RANGE_0) + ")\n" +
    "time \t\t\t = " + std::to_string(this->time) + "\n" +
    "interpmode \t\t = " + std::to_string(this->interpmode) +
    " (" + ualconst::op_interp_str.at(this->interpmode-OP_INTERP_0) + ")\n";
  return s;
}

std::string OperationContext::fullPath() const
{
  std::string s = this->pctx->fullPath() + this->dataobjectname;
  return s;
}

int OperationContext::getType() const 
{
  return CTX_OPERATION_TYPE;
}

int OperationContext::getBackendID() const
{ 
  return getPulseContext()->getBackendID(); 
}

std::string OperationContext::getBackendName() const 
{ 
  return ualconst::backend_id_str.at(getPulseContext()->getBackendID()-BACKEND_ID_0); 
}

std::string OperationContext::getDataobjectName() const
{ 
  return dataobjectname; 
}

int OperationContext::getAccessmode() const
{ 
  return accessmode; 
}

int OperationContext::getRangemode() const 
{ 
  return rangemode; 
}

double OperationContext::getTime() const
{ 
  return time; 
}

int OperationContext::getInterpmode() const
{ 
  return interpmode; 
}

DataEntryContext* OperationContext::getPulseContext() const
{
  return pctx;
}




/// ArraystructContext ///

ArraystructContext::ArraystructContext(OperationContext* ctx, std::string p, std::string tb)
  : path(p), timebase(tb), opctx(ctx)
{
  parent = NULL;
  this->uid = ++SID;
}

ArraystructContext::ArraystructContext(ArraystructContext* cont, std::string p, std::string tb)
  : path(p), timebase(tb), parent(cont), opctx(cont->opctx)
{
  if (cont != NULL)
    opctx = cont->opctx;
  this->uid = ++SID;
}

ArraystructContext::ArraystructContext(ArraystructContext* cont, std::string p, std::string tb, int idx)
  : path(p), timebase(tb), parent(cont), index(idx), opctx(cont->opctx)
{
  if (cont != NULL)
    opctx = cont->opctx;
  this->uid = ++SID;
}

std::string ArraystructContext::print() const
{
  std::string s = this->opctx->print() +
    "path \t\t\t = \"" + this->path + "\"\n" +
    "timebase \t\t = \"" + this->timebase + "\"\n" +
    "timed \t\t\t = " + 
    (timebase.empty()?"no":"yes") + "\n" +
    "parent \t\t\t = " +
    ((this->parent==NULL)?"NULL":this->parent->path) + "\n" +
    "index \t\t\t = " + std::to_string(this->index) + "\n";
  return s;
}

std::string ArraystructContext::fullPath() const
{
  std::string ppath = "";
  ArraystructContext *tmp = this->parent;
  while (tmp != NULL)
    {
      ppath = tmp->path + "/" + ppath;
      tmp = tmp->parent;
    }
  std::string s = this->opctx->fullPath() +
    "/" + ppath + this->path;
  return s;
}

int ArraystructContext::getType() const
{
  return CTX_ARRAYSTRUCT_TYPE;
}

int ArraystructContext::getBackendID() const
{ 
  return getOperationContext()->getBackendID(); 
}

std::string ArraystructContext::getBackendName() const 
{ 
  return ualconst::backend_id_str.at(getOperationContext()->getBackendID()-BACKEND_ID_0); 
}

std::string ArraystructContext::getPath() const
{ 
  return path; 
}

std::string ArraystructContext::getTimebasePath() const
{ 
  return timebase; 
}

bool ArraystructContext::getTimed() const 
{ 
  return !timebase.empty(); 
}

ArraystructContext* ArraystructContext::getParent() 
{ 
  return parent; 
}

int ArraystructContext::getIndex() const
{ 
  return index; 
}

OperationContext* ArraystructContext::getOperationContext() const
{ 
  return opctx; 
}

void ArraystructContext::nextIndex(int step) 
{ 
  this->index += step; 
}
