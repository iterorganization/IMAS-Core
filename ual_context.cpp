#include "ual_context.h"

#include <unordered_map>

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

unsigned long int Context::getUid() const 
{
  return uid;
}

/// DataEntryContext ///

DataEntryContext::DataEntryContext(std::string uri_) : uri(uri::parse_uri(uri_))
{
  if (uri.error != uri::Error::None) {
    throw UALContextException("Unable to parse the URI: " + uri_, LOG);
  }

  setBackendID(uri.path, uri.authority.host);

  auto maybe_path = uri.query.get("path");
  if (!maybe_path) {
    uri = buildURIFromLegacy();
  }

  /* too soon to decide conflicting query options? */
  /*if (!pathFromURI.empty() && (!userFromURI.empty() && !databaseFromURI.empty() && !versionFromURI.empty())) {
      throw UALContextException("path should not be specified in the URI since user/database/version parameters are specified",LOG);
      }*/

  this->uid = ++SID;
}

std::string DataEntryContext::print() const 
{
  std::string s = "uri \t\t\t = " + this->uri.to_string() + "\n";
  return s;
}

int DataEntryContext::getType() const 
{
  return CTX_PULSE_TYPE;
}

int DataEntryContext::getBackendID() const
{ 
  return this->backend_id;
}

std::string DataEntryContext::getBackendName() const 
{ 
  return std::string(const2str(this->backend_id));
}

std::string DataEntryContext::getOptions() const
{ 
  return this->uri.query.get("options").value_or("");
}

uri::Uri DataEntryContext::getURI() const
{
  return this->uri;
}

uri::Uri DataEntryContext::buildURIFromLegacy() {
  std::string filePath;

  auto maybe_user = uri.query.get("user");
  if (!maybe_user) {
    throw UALContextException("'user' is not specified in URI but it is required when path is not specified (legacy mode)", LOG);
  }

  auto maybe_database = uri.query.get("database");
  if (!maybe_database) {
    throw UALContextException("'database' is not specified in URI but it is required when path is not specified (legacy mode)", LOG);
  }

  auto maybe_version = uri.query.get("version");
  if (!maybe_version) {
    throw UALContextException("'version' is not specified in URI but it is required when path is not specified (legacy mode)", LOG);
  }

  auto maybe_shot = uri.query.get("shot");
  if (!maybe_shot) {
    throw UALContextException("'shot' is not specified in URI but it is required when path is not specified (legacy mode)", LOG);
  }

  auto maybe_run = uri.query.get("run");
  if (!maybe_run) {
    throw UALContextException("'run' is not specified in URI but it is required when path is not specified (legacy mode)", LOG);
  }

  // using legacy to build standard paths
  std::string user = maybe_user.value();
  if (user == "public") {
    char *home = getenv("IMAS_HOME");
    if (home == NULL) {
      throw UALBackendException("when user is 'public', IMAS_HOME environment variable should be set.", LOG);
    }
    filePath += home;
    filePath += "/shared/imasdb/";
    filePath += maybe_database.value();
    filePath += "/";
    filePath += maybe_version.value();
  } else if (user.rfind('/', 0) == 0) {
    filePath += user;
    filePath += "/";
    filePath += maybe_database.value();
    filePath += "/";
    filePath += maybe_version.value();
  } else {
#ifdef WIN32
    char szHomeDir[256];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szHomeDir))) {
      filePath += szHomeDir;
#else
    struct passwd *pw = getpwnam(user.c_str());
    if (pw != NULL) {
      filePath += pw->pw_dir;
#endif
    } else {
      throw  UALBackendException("Can't find or access " + user + " user's data",LOG);
    }
    filePath += "/public/imasdb/";
    filePath += maybe_database.value();
    filePath += "/";
    filePath += maybe_version.value();
  }
  filePath += "/";
  filePath += maybe_shot.value();
  filePath += "/";
  filePath += maybe_run.value();

  uri::QueryDict query = {};
  query.insert("path", filePath);
  return { uri.scheme, uri.authority, uri.path, query, "" };
}

void DataEntryContext::setBackendID(const std::string &path, const std::string &host) {
    if (path == "mdsplus") {
        backend_id = MDSPLUS_BACKEND;
    } else if (path =="hdf5") {
        backend_id = HDF5_BACKEND;
    } else if (path =="ascii") {
        backend_id = ASCII_BACKEND;
    } else if (path =="memory") {
        backend_id = MEMORY_BACKEND;
    } else if (path == "uda" || !host.empty()) {
        backend_id = UDA_BACKEND;
    } else {
        throw UALContextException("Unable to identify a backend from the URI",LOG);
    }
}

void DataEntryContext::addOptions(const std::string &options_) {
    uri.query.insert("options", options_);
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
    desc << "imas:" << backend.c_str() << "?user=" << user << ";shot=" << shot << ";run=" << run << ";database=" << tokamak << ";version=" << version[0];
    if (strcmp(options,"")!=0)
      desc << ";options=" << options;
    const std::string& tmp = desc.str();
    int size = tmp.length()+1;
    *uri = (char *)malloc(size);
    memcpy(*uri, tmp.c_str(), tmp.length());
    (*uri)[tmp.length()] = '\0';
}

std::string DataEntryContext::getURIBackend(int backend_id)
{
    if (backend_id == MDSPLUS_BACKEND) {
        return "mdsplus";
    }
    else if (backend_id == HDF5_BACKEND) {
        return "hdf5";
    }
    else if (backend_id == ASCII_BACKEND) {
        return "ascii";
    }
    else if (backend_id == MEMORY_BACKEND) {
        return "memory";
    }
    else if (backend_id == UDA_BACKEND) {
        return "uda";
    }
    else {
        throw UALContextException("getURIBackend, converting backend ID to backend URI string not yet implemented",LOG);
    }
}


/// OperationContext ///

OperationContext::OperationContext(DataEntryContext* ctx, std::string dataobject, int access)
  : pctx(ctx), dataobjectname(dataobject)
{
  rangemode = ualconst::global_op;
  time = ualconst::undefined_time;
  interpmode = ualconst::undefined_interp;

  try {
    ualconst::op_access_list.at(access-OP_ACCESS_0);
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
  : pctx(ctx), dataobjectname(dataobject), time(t)
{
  try {
    ualconst::op_range_list.at(range-OP_RANGE_0);
  } 
  catch (const std::out_of_range& e) {
    throw UALContextException("Wrong range mode "+std::to_string(range),LOG);
  }
  rangemode = range;

  try {
    ualconst::op_access_list.at(access-OP_ACCESS_0);
  } 
  catch (const std::out_of_range& e) {
    throw UALContextException("Wrong access mode "+std::to_string(access),LOG);
  }
  accessmode = access;

  try {
    ualconst::op_interp_list.at(interp-OP_INTERP_0);
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
    " (" + std::string(const2str(this->accessmode)) + ")\n" +
    "rangemode \t\t = " + std::to_string(this->rangemode) +
    " (" + std::string(const2str(this->rangemode)) + ")\n" +
    "time \t\t\t = " + std::to_string(this->time) + "\n" +
    "interpmode \t\t = " + std::to_string(this->interpmode) +
    " (" + std::string(const2str(this->interpmode)) + ")\n";
  return s;
}

int OperationContext::getType() const 
{
  return CTX_OPERATION_TYPE;
}

int OperationContext::getBackendID() const
{ 
  return getDataEntryContext()->getBackendID(); 
}

uri::Uri OperationContext::getURI() const
{
  return getDataEntryContext()->getURI();
}

std::string OperationContext::getBackendName() const
{ 
  return std::string(const2str(getDataEntryContext()->getBackendID()));
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

DataEntryContext* OperationContext::getDataEntryContext() const
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

int ArraystructContext::getType() const
{
  return CTX_ARRAYSTRUCT_TYPE;
}

int ArraystructContext::getBackendID() const
{ 
  return getOperationContext()->getBackendID(); 
}

uri::Uri ArraystructContext::getURI() const
{
  return getOperationContext()->getURI();
}

std::string ArraystructContext::getBackendName() const
{ 
  return std::string(const2str(getOperationContext()->getBackendID()));
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

DataEntryContext* ArraystructContext::getDataEntryContext() const
{
  return opctx->getDataEntryContext();
}

void ArraystructContext::nextIndex(int step)
{ 
  this->index += step; 
}
