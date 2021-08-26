#include "ual_context.h"


std::atomic<unsigned long int> Context::SID(0);


std::ostream& operator<< (std::ostream& o, Context const& ctx)
{
  return o << ctx.print();
}

std::ostream& operator<<(std::ostream& o, PulseContext const& ctx)
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

Context::Context(int id)
{ 
  try
  {
    ualconst::backend_id_str.at(id-BACKEND_ID_0);
  }
  catch (const std::out_of_range& e) 
  {
    throw UALContextException("Wrong backend identifier "+std::to_string(id),LOG);
  }
  backend_id = id;
}

Context::Context(const Context& ctx) 
{
  backend_id = ctx.backend_id;
}

std::string Context::print() const 
{
  std::string s = "context_uid \t\t = " + 
    std::to_string(this->uid) + "\n" + 
    "backend_id \t\t = " + 
    std::to_string(this->backend_id) + " (" + 
    this->getBackendName() + ")\n";
  return s;
}

std::string Context::fullPath() const
{
  std::string s = "";
  return s;
}

int Context::getBackendID() const
{ 
  return backend_id; 
}

std::string Context::getBackendName() const 
{ 
  return ualconst::backend_id_str.at(backend_id-BACKEND_ID_0); 
}

unsigned long int Context::getUid() const 
{
  return uid;
}

int Context::getType() const 
{
  return CTX_TYPE;
}


/// PulseContext ///

PulseContext::PulseContext(std::string uri_) : Context(getBackendID(uri_)), uri(uri_)
{
  std::string userFromURI = getQueryParameter("user");
  if (!userFromURI.empty())
    user = userFromURI;
  else {
    char *usr = std::getenv("USER"); 
    if (usr!=NULL)
        user = usr;
  }

  std::string databaseFromURI = getQueryParameter("database");
  if (!databaseFromURI.empty())
    tokamak = databaseFromURI;
  else {
    char *tok = std::getenv("TOKAMAKNAME");
    if (tok!=NULL)
        tokamak = tok;
  }

  std::string versionFromURI = getQueryParameter("version");
  if (!versionFromURI.empty())
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
     if (!getQueryParameter("path").empty())
       throw UALContextException("path should be not specified in the URI since user/database/version parameters are specified",LOG);
  }

  std::string shotFromURI = getQueryParameter("shot");
  if (!shotFromURI.empty())
      shot = std::stoi(shotFromURI);

  std::string runFromURI = getQueryParameter("run");
  if (!runFromURI.empty())
      run = std::stoi(runFromURI);

  if ( (!shotFromURI.empty() && runFromURI.empty()) || (shotFromURI.empty() && !runFromURI.empty())) {
    throw UALContextException("shot/run parameters, only one of these 2 parameters is specified in the URI, however both should be specified in the URI",LOG);
  } 

  if ( !shotFromURI.empty() && !runFromURI.empty()) {
    if (!getQueryParameter("refname").empty())
       throw UALContextException("refname should be not specified in the URI since shot/run parameters are specified in the URI",LOG);
  }

  this->uid = ++SID;
}

PulseContext::PulseContext(int id, int s, int r, std::string u, std::string t, 
			   std::string v) : Context(id), shot(s), run(r)
{
  char *usr = std::getenv("USER"); 
  if (u=="")
    {
      if (usr!=NULL)
	user = usr;
      else
	throw UALContextException("Undefined env variable USER",LOG);
    }
  else 
    user = u;

  char *tok = std::getenv("TOKAMAKNAME"); 
  if (t=="") 
    {
      if (tok!=NULL)
	tokamak = tok;
      else
	throw UALContextException("Undefined env variable TOKAMAKNAME",LOG);
    }
  else
    tokamak = t;

  char *ver = std::getenv("DATAVERSION"); 
  if (v=="") 
    {
      if (ver!=NULL) 
	{
	  version = ver;
	}
      else
	throw UALContextException("Undefined env variable DATAVERSION",LOG);
    }
  else
    {
      size_t pos = v.find('.');
      if (pos == std::string::npos)
	version = v;
      else
	version = v.substr(0,pos);
    }
  this->uid = ++SID;
}

std::string PulseContext::print() const 
{
  std::string s = ((Context)*this).print() +
    "shot \t\t\t = " + std::to_string(this->shot) + "\n" +
    "run \t\t\t = " + std::to_string(this->run) + "\n" +
    "user \t\t\t = \"" + this->user + "\"\n" +
    "tokamak \t\t = \"" + this->tokamak + "\"\n" +
    "version \t\t = \"" + this->version + "\"\n";
  return s;
}

std::string PulseContext::fullPath() const
{
  std::string s = ((Context)*this).fullPath();
  return s;
}

int PulseContext::getType() const 
{
  return CTX_PULSE_TYPE;
}

int PulseContext::getBackendID() const
{
  return getBackendID(uri);
}

int PulseContext::getShot() const
{ 
  return shot; 
}

int PulseContext::getRun() const 
{ 
  return run; 
}

std::string PulseContext::getUser() const
{ 
  return user; 
}

std::string PulseContext::getTokamak() const
{ 
  return tokamak; 
}

std::string PulseContext::getVersion() const
{ 
  return version; 
}


std::string PulseContext::getQuery() const
{ 
  std::string query;
  if (!uri.empty()) {
    std::size_t pos = uri.find("?");
    if (pos != std::string::npos)
        query = uri.substr(pos + 1, std::string::npos);
  }
  return query; 
}

std::string PulseContext::getQueryParameter(const std::string &parameter) const
{ 
  std::string parameter_value;
  std::string query = getQuery();
  std::size_t pos = query.find(parameter + "=");
  if (pos != std::string::npos) {
    query = query.substr(pos + parameter.length() + 1, std::string::npos);
    std::size_t sep_pos = query.find(";");
    std::size_t sharp_pos = query.find("#");
    if (sep_pos != std::string::npos) {
        parameter_value = query.substr(0, sep_pos);
    }
    else if (sharp_pos != std::string::npos) {
        parameter_value = query.substr(0, sharp_pos);
    }
    else {
        parameter_value = query;
    }
  }
  return parameter_value; 
}

std::string PulseContext::getURI() const
{
  return uri;
}

int PulseContext::getBackendID(const std::string &uri) const {
    std::string query = getQuery();
    std::string authority_and_path = uri;
    if (!query.empty()) {
        std::size_t pos = uri.find("?");
        authority_and_path = uri.substr(0, pos);
    }
    std::size_t pos = authority_and_path.find("imas:");
    if (pos == std::string::npos)
        throw UALContextException("Missing URI scheme",LOG);

    pos = authority_and_path.find("imas:mdsplus");
    if (pos != std::string::npos)
        return MDSPLUS_BACKEND;
    pos = authority_and_path.find("imas:hdf5");
    if (pos != std::string::npos)
        return HDF5_BACKEND;
    pos = authority_and_path.find("imas:ascii");
    if (pos != std::string::npos)
        return ASCII_BACKEND;
    pos = authority_and_path.find("imas:memory");
    if (pos != std::string::npos)
        return MEMORY_BACKEND;
    pos = authority_and_path.find("imas:localhost");
    if (pos != std::string::npos)
        return UDA_BACKEND;
    pos = authority_and_path.find("imas://");
    if (pos != std::string::npos)
        return UDA_BACKEND;

    throw UALContextException("Unspecified backend in URI",LOG);
}


/// OperationContext ///

OperationContext::OperationContext(PulseContext ctx, std::string dataobject, int access)
  : PulseContext(ctx), dataobjectname(dataobject)
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

OperationContext::OperationContext(PulseContext ctx, std::string dataobject, int access, 
				   int range, double t, int interp)
  : PulseContext(ctx), dataobjectname(dataobject), time(t)
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
  std::string s = ((PulseContext)*this).print() +
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
  std::string s = ((PulseContext)*this).fullPath() + this->dataobjectname;
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
