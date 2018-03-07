/*-*-c++-*-*/

#include <exception>
#include <sstream>

#define LOG __FILE__,__LINE__
#define WHERE " ("<<__FILE__<<":"<<__LINE__<<")"

#ifdef DEBUG
#define VERBOSE true
#else
#define VERBOSE false 
#endif
 


class UALException : public std::runtime_error
{
protected:
  std::string mesg;
  
public:
  UALException() : std::runtime_error("") {}
  virtual ~UALException() throw() {}

  UALException(const char *m) : std::runtime_error(m) {
    std::ostringstream o;
    o << "[UALException = " << m << "]";
    mesg = o.str();
  }
  UALException(const std::string &m) : std::runtime_error(m) {
    std::ostringstream o;
    o << "[UALException = " << m << "]";
    mesg = o.str();
  }

  UALException(const char *m, const std::string &f, const int l): std::runtime_error(m) {
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
  UALException(const std::string &m, const std::string &f, const int l): std::runtime_error(m) {
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

  const char * what() const throw() { 
    return mesg.c_str();
  }
};


class UALLowlevelException : public UALException
{
public:
  UALLowlevelException() {}
  virtual ~UALLowlevelException() throw() {}

  UALLowlevelException(const char *m) : UALException(m) {
    std::ostringstream o;
    o << "[UALLowlevelException = " << m << "]";
    mesg = o.str();
  }
  UALLowlevelException(const std::string& m) : UALException(m) {
    std::ostringstream o;
    o << "[UALLowlevelException = " << m << "]";
    mesg = o.str();
  }

  UALLowlevelException(const char *m, const std::string &f, const int l) : 
    UALException(m,f,l) {
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
  UALLowlevelException(const std::string &m, const std::string &f, const int l) : 
    UALException(m,f,l) {
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
};


class UALBackendException : public UALException
{
public:
  UALBackendException() {}
  virtual ~UALBackendException() throw() {}

  UALBackendException(const char *m) : UALException(m) {
    std::ostringstream o;
    o << "[UALBackendException = " << m << "]";
    mesg = o.str();
  }
  UALBackendException(const std::string &m) : UALException(m) {
    std::ostringstream o;
    o << "[UALBackendException = " << m << "]";
    mesg = o.str();
  }

  UALBackendException(const char *m, const std::string &f, const int l) : 
    UALException(m,f,l) {
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
  UALBackendException(const std::string &m, const std::string &f, const int l) : 
    UALException(m,f,l) {
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
};


class UALNoDataException : public UALBackendException
{
public:
  UALNoDataException() {}
  virtual ~UALNoDataException() throw() {}

  UALNoDataException(const char *m) : UALBackendException(m) {
    std::ostringstream o;
    o << "[UALNoDataException = " << m << "]";
    mesg = o.str();
  }
  UALNoDataException(const std::string &m) : UALBackendException(m) {
    std::ostringstream o;
    o << "[UALNoDataException = " << m << "]";
    mesg = o.str();
  }

  UALNoDataException(const char *m, const std::string &f, const int l) : 
    UALBackendException(m,f,l) {
    std::ostringstream o;
    if (VERBOSE) {
      o << "[UALNoDataException (" << f << ":" << l << ") = " << m << "]";
      mesg = o.str();
    }
    else {
      o << "[UALNoDataException = " << m << "]";
      mesg = o.str();
    }
  }
  UALNoDataException(const std::string &m, const std::string &f, const int l) : 
    UALBackendException(m,f,l) {
    std::ostringstream o;
    if (VERBOSE) {
      o << "[UALNoDataException (" << f << ":" << l << ") = " << m << "]";
      mesg = o.str();
    }
    else {
      o << "[UALNoDataException = " << m << "]";
      mesg = o.str();
    }
  }
};


class UALContextException : public UALException
{
public:
  UALContextException() {}
  virtual ~UALContextException() throw() {}

  UALContextException(const char *m) : UALException(m) {
    std::ostringstream o;
    o << "[UALContextException = " << m << "]";
    mesg = o.str();
  }
  UALContextException(const std::string &m) : UALException(m) {
    std::ostringstream o;
    o << "[UALContextException = " << m << "]";
    mesg = o.str();
  }

  UALContextException(const char *m, const std::string &f, const int l) : 
    UALException(m,f,l) {
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
  UALContextException(const std::string &m, const std::string &f, const int l) : 
    UALException(m,f,l) {
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
};

