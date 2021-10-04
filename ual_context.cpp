#include "ual_context.h"

#include <unordered_map>
#include "ual_utilities.h"
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

int Context::getType() const 
{
  return CTX_TYPE;
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

  if (!userFromURI.empty() && !databaseFromURI.empty() && !versionFromURI.empty()) {
      std::string pathFromURI;
      if(uri::queryParameter("path", pathFromURI, uri_object))
       throw UALContextException("path should not be specified in the URI since user/database/version parameters are specified",LOG);
  }

   std::string shotFromURI;
   if(uri::queryParameter("shot", shotFromURI, uri_object))
      shot = std::stoi(shotFromURI);

  std::string runFromURI;
  if(uri::queryParameter("run", runFromURI, uri_object))
      run = std::stoi(runFromURI);

  if ( (!shotFromURI.empty() && runFromURI.empty()) || (shotFromURI.empty() && !runFromURI.empty())) {
    throw UALContextException("shot/run parameters, only one of these 2 parameters is specified in the URI, however both should be specified in the URI",LOG);
  } 

  if ( !shotFromURI.empty() && !runFromURI.empty()) {
    std::string refnameFromURI;
    if(uri::queryParameter("refname", refnameFromURI, uri_object))
       throw UALContextException("refname should not be specified in the URI since shot/run parameters are specified in the URI",LOG);
  }

  this->uid = ++SID;
}

std::string DataEntryContext::print() const 
{
  std::string s = ((Context)*this).print() +
    "shot \t\t\t = " + std::to_string(this->shot) + "\n" +
    "run \t\t\t = " + std::to_string(this->run) + "\n" +
    "user \t\t\t = \"" + this->user + "\"\n" +
    "tokamak \t\t = \"" + this->tokamak + "\"\n" +
    "version \t\t = \"" + this->version + "\"\n" +
    "backend_id \t\t = " + std::to_string(this->backend_id) + " (" + this->getBackendName() + ")\n" ;
  return s;
}


std::string DataEntryContext::fullPath() const
{
  std::string s = ((Context)*this).fullPath();
  return s;
}

int DataEntryContext::getType() const 
{
  return CTX_PULSE_TYPE;
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

std::string DataEntryContext::getURI() const
{
  return uri;
}

int DataEntryContext::getBackendID() const
{ 
  return backend_id; 
}

std::string DataEntryContext::getBackendName() const 
{ 
  return ualconst::backend_id_str.at(backend_id-BACKEND_ID_0); 
}

std::string DataEntryContext::getLegacyRootPath() {

    std::string filePath;

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

std::string DataEntryContext::getQueryString() const {
    auto uri_object = uri::parse_uri(uri);
    return uri_object.query_string;
}

bool DataEntryContext::getURIQueryParameter(const std::string &parameter, std::string &value) const {
    auto uri_object = uri::parse_uri(uri);
    auto got = uri_object.query.find(parameter);
    if (got != uri_object.query.end()) {
        value = got->second;
        return true;
    }
    else {
        return false;
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
    std::string opts("");
    if (options) {
        std::map<std::string, std::string> mapOptions;
        extractOptions(std::string(options), mapOptions);
        auto it = mapOptions.begin();
        while (it != mapOptions.end()) {
            const std::string &key = it->first;
            const std::string &value = it->second;
            opts += ";" + key + "=" + value;
            it++;
        }
    }
    desc << "imas:" << backend.c_str() << "?user=" << user << ";shot=" << shot << ";run=" << run << ";database=" << tokamak << ";version=" << version[0] << ";" << opts.c_str() << "\n";
    const std::string& tmp = desc.str();
    int size = tmp.length()+1;
    *uri = (char *)malloc(size);
    mempcpy(*uri, tmp.c_str(), tmp.length());
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

OperationContext::OperationContext(DataEntryContext ctx, std::string dataobject, int access)
  : DataEntryContext(ctx), dataobjectname(dataobject)
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
  this->uid = ++SID;
}

OperationContext::OperationContext(DataEntryContext ctx, std::string dataobject, int access, 
				   int range, double t, int interp)
  : DataEntryContext(ctx), dataobjectname(dataobject), time(t)
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
  std::string s = ((DataEntryContext)*this).print() +
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
  std::string s = ((DataEntryContext)*this).fullPath() + this->dataobjectname;
  return s;
}

int OperationContext::getType() const 
{
  return CTX_OPERATION_TYPE;
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




/// ArraystructContext ///

ArraystructContext::ArraystructContext(OperationContext ctx, std::string p, std::string tb)
  : OperationContext(ctx), path(p), timebase(tb)
{
  parent = NULL;
  index = 0;
  this->uid = ++SID;
}

ArraystructContext::ArraystructContext(OperationContext ctx, std::string p, std::string tb,
				       ArraystructContext *cont)
  : OperationContext(ctx), path(p), timebase(tb), parent(cont)
{
  index = 0;
  this->uid = ++SID;
}

ArraystructContext::ArraystructContext(OperationContext ctx, std::string p, std::string tb,
				       ArraystructContext *cont, int idx)
  : OperationContext(ctx), path(p), timebase(tb), parent(cont), index(idx)
{
  index = 0;
  this->uid = ++SID;
}

std::string ArraystructContext::print() const
{
  std::string s = ((OperationContext)*this).print() +
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
  std::string s = ((OperationContext)*this).fullPath() +
    "/" + ppath + this->path;
  return s;
}

int ArraystructContext::getType() const
{
  return CTX_ARRAYSTRUCT_TYPE;
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

ArraystructContext * ArraystructContext::getParent() 
{ 
  return parent; 
}

int ArraystructContext::getIndex() const
{ 
  return index; 
}

void ArraystructContext::nextIndex(int step) 
{ 
  this->index += step; 
}
