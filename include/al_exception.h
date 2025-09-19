/*-*-c++-*-*/

#ifndef AL_EXCEPTION_H
#define AL_EXCEPTION_H 1

#define LOG __FILE__,__LINE__
#define WHERE " ("<<__FILE__<<":"<<__LINE__<<")"

#ifdef DEBUG
#define VERBOSE true
#else
#define VERBOSE false 
#endif
 
#if defined(_WIN32)
#  define IMAS_CORE_LIBRARY_API __declspec(dllexport)
#else
#  define IMAS_CORE_LIBRARY_API
#endif

#ifdef __cplusplus

#include <sstream>
#include <exception>
#include <stdexcept>
#include <cstring>

#include "al_defs.h"

extern "C"
{

class IMAS_CORE_LIBRARY_API ALException : public std::runtime_error
{
protected:
  std::string mesg;
  
public:
  ALException() : std::runtime_error("") {}
  virtual ~ALException() throw() {}

  ALException(const char *m); 
  ALException(const std::string &m);
  ALException(const char *m, const std::string &f, const int l);
  ALException(const std::string &m, const std::string &f, const int l);
  const char * what() const throw(); 
  static void registerStatus(char *message, const char *func, const std::exception &e); 
};


class IMAS_CORE_LIBRARY_API ALLowlevelException : public ALException
{
public:
  ALLowlevelException() {}
  virtual ~ALLowlevelException() throw() {}

  ALLowlevelException(const char *m);
  ALLowlevelException(const std::string& m);
  ALLowlevelException(const char *m, const std::string &f, const int l);
  ALLowlevelException(const std::string &m, const std::string &f, const int l);
};


class IMAS_CORE_LIBRARY_API ALBackendException : public ALException
{
public:
  ALBackendException() {}
  virtual ~ALBackendException() throw() {}

  ALBackendException(const char *m);
  ALBackendException(const std::string &m);
  ALBackendException(const char *m, const std::string &f, const int l);
  ALBackendException(const std::string &m, const std::string &f, const int l);
};


class IMAS_CORE_LIBRARY_API ALContextException : public ALException
{
public:
  ALContextException() {}
  virtual ~ALContextException() throw() {}

  ALContextException(const char *m);
  ALContextException(const std::string &m);
  ALContextException(const char *m, const std::string &f, const int l);
  ALContextException(const std::string &m, const std::string &f, const int l);
};

class IMAS_CORE_LIBRARY_API ALPluginException : public ALException
{
public:
  ALPluginException() {}
  virtual ~ALPluginException() throw() {}

  ALPluginException(const char *m);
  ALPluginException(const std::string &m);
  ALPluginException(const char *m, const std::string &f, const int l);
  ALPluginException(const std::string &m, const std::string &f, const int l);
};

}
#endif

#endif // AL_EXCEPTION_H
