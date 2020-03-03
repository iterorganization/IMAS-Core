/*-*-c++-*-*/

#ifndef UAL_EXCEPTION_H
#define UAL_EXCEPTION_H 1

#include <exception>
#include <stdexcept>
#include <sstream>

#define LOG __FILE__,__LINE__
#define WHERE " ("<<__FILE__<<":"<<__LINE__<<")"

#ifdef DEBUG
#define VERBOSE true
#else
#define VERBOSE false 
#endif
 
#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

class LIBRARY_API UALException : public std::runtime_error
{
protected:
  std::string mesg;
  
public:
  UALException() : std::runtime_error("") {}
  virtual ~UALException() throw() {}

  UALException(const char *m); 
  UALException(const std::string &m);
  UALException(const char *m, const std::string &f, const int l);
  UALException(const std::string &m, const std::string &f, const int l);
  const char * what() const throw(); 
};


class LIBRARY_API UALLowlevelException : public UALException
{
public:
  UALLowlevelException() {}
  virtual ~UALLowlevelException() throw() {}

  UALLowlevelException(const char *m);
  UALLowlevelException(const std::string& m);
  UALLowlevelException(const char *m, const std::string &f, const int l);
  UALLowlevelException(const std::string &m, const std::string &f, const int l);
};


class LIBRARY_API UALBackendException : public UALException
{
public:
  UALBackendException() {}
  virtual ~UALBackendException() throw() {}

  UALBackendException(const char *m);
  UALBackendException(const std::string &m);
  UALBackendException(const char *m, const std::string &f, const int l);
  UALBackendException(const std::string &m, const std::string &f, const int l);
};


class LIBRARY_API UALContextException : public UALException
{
public:
  UALContextException() {}
  virtual ~UALContextException() throw() {}

  UALContextException(const char *m);
  UALContextException(const std::string &m);
  UALContextException(const char *m, const std::string &f, const int l);
  UALContextException(const std::string &m, const std::string &f, const int l);
};

#ifdef __cplusplus
}
#endif

#endif // UAL_EXCEPTION_H
