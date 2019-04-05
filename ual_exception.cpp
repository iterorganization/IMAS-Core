#include "ual_exception.h"



/// UALException ///

UALException::UALException(const char *m) 
  : std::runtime_error(m) 
{
  std::ostringstream o;
  o << "[UALException = " << m << "]";
  mesg = o.str();
}

UALException::UALException(const std::string &m) 
  : std::runtime_error(m) 
{
  std::ostringstream o;
  o << "[UALException = " << m << "]";
  mesg = o.str();
}

UALException::UALException(const char *m, const std::string &f, const int l)
  : std::runtime_error(m) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALException = " << m << "]";
    mesg = o.str();
  }
}

UALException::UALException(const std::string &m, const std::string &f, const int l)
  : std::runtime_error(m) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALException = " << m << "]";
    mesg = o.str();
  }
}

const char * UALException::what() const throw() { 
  return mesg.c_str();
}




/// UALLowlevelException ///

UALLowlevelException::UALLowlevelException(const char *m) 
  : UALException(m) 
{
  std::ostringstream o;
  o << "[UALLowlevelException = " << m << "]";
  mesg = o.str();
}

UALLowlevelException::UALLowlevelException(const std::string& m) 
  : UALException(m) 
{
  std::ostringstream o;
  o << "[UALLowlevelException = " << m << "]";
  mesg = o.str();
}

UALLowlevelException::UALLowlevelException(const char *m, const std::string &f, const int l)
  : UALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALLowlevelException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALLowlevelException = " << m << "]";
    mesg = o.str();
  }
}

UALLowlevelException::UALLowlevelException(const std::string &m, const std::string &f, const int l) 
  : UALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALLowlevelException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALLowlevelException = " << m << "]";
    mesg = o.str();
  }
}



/// UALBackendException ///

UALBackendException::UALBackendException(const char *m) 
  : UALException(m) 
{
  std::ostringstream o;
  o << "[UALBackendException = " << m << "]";
  mesg = o.str();
}

UALBackendException::UALBackendException(const std::string &m) 
  : UALException(m) 
{
  std::ostringstream o;
  o << "[UALBackendException = " << m << "]";
  mesg = o.str();
}

UALBackendException::UALBackendException(const char *m, const std::string &f, const int l)
  : UALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALBackendException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALBackendException = " << m << "]";
    mesg = o.str();
  }
}

UALBackendException::UALBackendException(const std::string &m, const std::string &f, const int l) 
  : UALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALBackendException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALBackendException = " << m << "]";
    mesg = o.str();
  }
}



/// UALContextException ///

UALContextException::UALContextException(const char *m) 
  : UALException(m) 
{
  std::ostringstream o;
  o << "[UALContextException = " << m << "]";
  mesg = o.str();
}

UALContextException::UALContextException(const std::string &m) 
  : UALException(m) 
{
  std::ostringstream o;
  o << "[UALContextException = " << m << "]";
  mesg = o.str();
}

UALContextException::UALContextException(const char *m, const std::string &f, const int l)
  : UALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALContextException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALContextException = " << m << "]";
    mesg = o.str();
  }
}

UALContextException::UALContextException(const std::string &m, const std::string &f, const int l) 
  : UALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[UALContextException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[UALContextException = " << m << "]";
    mesg = o.str();
  }
}
