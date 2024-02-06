#include "al_exception.h"

#if defined(_MSC_VER)
#  define mempcpy memcpy
#endif

/// ALException ///

ALException::ALException(const char *m) 
  : std::runtime_error(m) 
{
  std::ostringstream o;
  o << "[ALException = " << m << "]";
  mesg = o.str();
}

ALException::ALException(const std::string &m) 
  : std::runtime_error(m) 
{
  std::ostringstream o;
  o << "[ALException = " << m << "]";
  mesg = o.str();
}

ALException::ALException(const char *m, const std::string &f, const int l)
  : std::runtime_error(m) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALException = " << m << "]";
    mesg = o.str();
  }
}

ALException::ALException(const std::string &m, const std::string &f, const int l)
  : std::runtime_error(m) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALException = " << m << "]";
    mesg = o.str();
  }
}

const char * ALException::what() const throw() { 
  return mesg.c_str();
}

void ALException::registerStatus(char *message, const char *func, const std::exception &e) { 
  memset(message, ' ', MAX_ERR_MSG_LEN);
  sprintf(message, "%s: %s", func, e.what());
}



/// ALLowlevelException ///

ALLowlevelException::ALLowlevelException(const char *m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALLowlevelException = " << m << "]";
  mesg = o.str();
}

ALLowlevelException::ALLowlevelException(const std::string& m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALLowlevelException = " << m << "]";
  mesg = o.str();
}

ALLowlevelException::ALLowlevelException(const char *m, const std::string &f, const int l)
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALLowlevelException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALLowlevelException = " << m << "]";
    mesg = o.str();
  }
}

ALLowlevelException::ALLowlevelException(const std::string &m, const std::string &f, const int l) 
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALLowlevelException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALLowlevelException = " << m << "]";
    mesg = o.str();
  }
}



/// ALBackendException ///

ALBackendException::ALBackendException(const char *m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALBackendException = " << m << "]";
  mesg = o.str();
}

ALBackendException::ALBackendException(const std::string &m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALBackendException = " << m << "]";
  mesg = o.str();
}

ALBackendException::ALBackendException(const char *m, const std::string &f, const int l)
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALBackendException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALBackendException = " << m << "]";
    mesg = o.str();
  }
}

ALBackendException::ALBackendException(const std::string &m, const std::string &f, const int l) 
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALBackendException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALBackendException = " << m << "]";
    mesg = o.str();
  }
}



/// ALContextException ///

ALContextException::ALContextException(const char *m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALContextException = " << m << "]";
  mesg = o.str();
}

ALContextException::ALContextException(const std::string &m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALContextException = " << m << "]";
  mesg = o.str();
}

ALContextException::ALContextException(const char *m, const std::string &f, const int l)
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALContextException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALContextException = " << m << "]";
    mesg = o.str();
  }
}

ALContextException::ALContextException(const std::string &m, const std::string &f, const int l) 
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALContextException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALContextException = " << m << "]";
    mesg = o.str();
  }
}

/// ALPluginException ///

ALPluginException::ALPluginException(const char *m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALPluginException = " << m << "]";
  mesg = o.str();
}

ALPluginException::ALPluginException(const std::string &m) 
  : ALException(m) 
{
  std::ostringstream o;
  o << "[ALPluginException = " << m << "]";
  mesg = o.str();
}

ALPluginException::ALPluginException(const char *m, const std::string &f, const int l)
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALPluginException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALPluginException = " << m << "]";
    mesg = o.str();
  }
}

ALPluginException::ALPluginException(const std::string &m, const std::string &f, const int l) 
  : ALException(m,f,l) 
{
  std::ostringstream o;
  if (VERBOSE) {
    o << "[ALPluginException (" << f << ":" << l << ") = " << m << "]";
    mesg = o.str();
  }
  else {
    o << "[ALPluginException = " << m << "]";
    mesg = o.str();
  }
}
