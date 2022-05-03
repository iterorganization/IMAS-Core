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



/// PulseContext ///

PulseContext::PulseContext(int id, int s, int r, std::string u, std::string t, 
			   std::string v) : backend_id(id), shot(s), run(r)
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
  std::string s = "backend_id \t\t = " + 
    std::to_string(this->backend_id) + " (" + 
    this->getBackendName() + ")\n" + 
    "shot \t\t\t = " + std::to_string(this->shot) + "\n" +
    "run \t\t\t = " + std::to_string(this->run) + "\n" +
    "user \t\t\t = \"" + this->user + "\"\n" +
    "tokamak \t\t = \"" + this->tokamak + "\"\n" +
    "version \t\t = \"" + this->version + "\"\n";
  return s;
}

std::string PulseContext::fullPath() const
{
  std::string s = "";
  return s;
}

int PulseContext::getType() const 
{
  return CTX_PULSE_TYPE;
}

int PulseContext::getBackendID() const
{ 
  return backend_id; 
}

std::string PulseContext::getBackendName() const 
{ 
  return ualconst::backend_id_str.at(backend_id-BACKEND_ID_0); 
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




/// OperationContext ///

OperationContext::OperationContext(PulseContext* ctx, std::string dataobject, int access)
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
  this->uid = ++SID;
}

OperationContext::OperationContext(PulseContext* ctx, std::string dataobject, int access, 
				   int range, double t, int interp)
  : pctx(ctx), dataobjectname(dataobject), time(t)
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

PulseContext* OperationContext::getPulseContext() const
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
