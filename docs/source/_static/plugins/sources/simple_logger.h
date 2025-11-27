#ifndef simpleLogger_h__
#define simpleLogger_h__

#define BOOST_LOG_DYN_LINK 1 // necessary when linking the boost_log library dynamically

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>

// the logs are also written to LOGFILE
#define LOGFILE "logfile.log"

// just log messages with severity >= SEVERITY_THRESHOLD are written
#define SEVERITY_THRESHOLD logging::trivial::warning

// register a global logger
BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>)

// just a helper macro used by the macros below - don't use it in your code
#define LOGB(severity) BOOST_LOG_SEV(logger::get(),boost::log::trivial::severity)


#define LOG_LOCATION(severity)      \
  BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity)        \
    << boost::log::add_value("Function", __FUNCTION__)


// ===== log macros =====
#define LOG_TRACE   LOG_LOCATION(trace)
#define LOG_DEBUG   LOG_LOCATION(debug)
#define LOG_INFO    LOG_LOCATION(info)
#define LOG_WARNING LOG_LOCATION(warning)
#define LOG_ERROR   LOG_LOCATION(error)
#define LOG_FATAL   LOG_LOCATION(fatal)

#endif