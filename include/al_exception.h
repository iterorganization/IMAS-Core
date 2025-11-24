/*-*-c++-*-*/

/**
 * @file al_exception.h
 * @brief Defines custom exception classes for the AL library.
 */
#ifndef AL_EXCEPTION_H
#define AL_EXCEPTION_H 1

/**
 * @def LOG
 * @brief Macro to log the current file and line number.
 */
#define LOG __FILE__,__LINE__

/**
 * @def WHERE
 * @brief Macro to format the current file and line number as a string.
 */
#define WHERE " ("<<__FILE__<<":"<<__LINE__<<")"

/**
 * @def VERBOSE
 * @brief Macro to enable verbose logging in debug mode.
 */
#ifdef DEBUG
#define VERBOSE true
#else
#define VERBOSE false 
#endif

/**
 * @def LIBRARY_API
 * @brief Macro to handle symbol export for different platforms.
 */
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

/**
 * @class ALException
 * @brief Base class for all exceptions in the AL library.
 */
class IMAS_CORE_LIBRARY_API ALException : public std::runtime_error
{
protected:
  std::string mesg; ///< Exception message

public:
  /**
   * @brief Default constructor.
   */
  ALException() : std::runtime_error("") {}

  /**
   * @brief Destructor.
   */
  virtual ~ALException() throw() {}

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALException(const char *m);

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALException(const std::string &m);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALException(const char *m, const std::string &f, const int l);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALException(const std::string &m, const std::string &f, const int l);

  /**
   * @brief Get the exception message.
   * @return Exception message.
   */
  const char * what() const throw();

  /**
   * @brief Register the status of an exception.
   * @param message Status message.
   * @param func Function name.
   * @param e Exception object.
   */
  static void registerStatus(char *message, const char *func, const std::exception &e);
};


/**
 * @class ALLowlevelException
 * @brief Exception class for low-level errors.
 */
class IMAS_CORE_LIBRARY_API ALLowlevelException : public ALException
{
public:
  /**
   * @brief Default constructor.
   */
  ALLowlevelException() {}

  /**
   * @brief Destructor.
   */
  virtual ~ALLowlevelException() throw() {}

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALLowlevelException(const char *m);

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALLowlevelException(const std::string& m);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALLowlevelException(const char *m, const std::string &f, const int l);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALLowlevelException(const std::string &m, const std::string &f, const int l);
};


/**
 * @class ALBackendException
 * @brief Exception class for backend errors.
 */
class IMAS_CORE_LIBRARY_API ALBackendException : public ALException
{
public:
  /**
   * @brief Default constructor.
   */
  ALBackendException() {}

  /**
   * @brief Destructor.
   */
  virtual ~ALBackendException() throw() {}

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALBackendException(const char *m);

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALBackendException(const std::string &m);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALBackendException(const char *m, const std::string &f, const int l);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALBackendException(const std::string &m, const std::string &f, const int l);
};


/**
 * @class ALContextException
 * @brief Exception class for context errors.
 */
class IMAS_CORE_LIBRARY_API ALContextException : public ALException
{
public:
  /**
   * @brief Default constructor.
   */
  ALContextException() {}

  /**
   * @brief Destructor.
   */
  virtual ~ALContextException() throw() {}

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALContextException(const char *m);

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALContextException(const std::string &m);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALContextException(const char *m, const std::string &f, const int l);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALContextException(const std::string &m, const std::string &f, const int l);
};


/**
 * @class ALPluginException
 * @brief Exception class for plugin errors.
 */
class IMAS_CORE_LIBRARY_API ALPluginException : public ALException
{
public:
  /**
   * @brief Default constructor.
   */
  ALPluginException() {}

  /**
   * @brief Destructor.
   */
  virtual ~ALPluginException() throw() {}

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALPluginException(const char *m);

  /**
   * @brief Constructor with a message.
   * @param m Exception message.
   */
  ALPluginException(const std::string &m);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALPluginException(const char *m, const std::string &f, const int l);

  /**
   * @brief Constructor with a message, file, and line number.
   * @param m Exception message.
   * @param f File name.
   * @param l Line number.
   */
  ALPluginException(const std::string &m, const std::string &f, const int l);
};

}
#endif

#endif // AL_EXCEPTION_H
