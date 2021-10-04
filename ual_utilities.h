//-*-c++-*-

/**
   \file ual_utilities.h
   Contains definition of utilities functions
*/

#ifndef UAL_UTILITIES_H
#define UAL_UTILITIES_H 1

#include "ual_context.h"

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus

#include <vector>
#include <map>
#include <string>

// Check if regex is available (fully implemented since GCC 4.9)
// see https://stackoverflow.com/questions/12530406/is-gcc-4-8-or-earlier-buggy-about-regular-expressions
#include <regex>

#if __cplusplus >= 201103L &&                               \
    (!defined(__GLIBCXX__) || (__cplusplus >= 201402L)  ||  \
        (defined(_GLIBCXX_REGEX_DFS_QUANTIFIERS_LIMIT)  ||  \
        defined(_GLIBCXX_REGEX_STATE_LIMIT)             ||  \
            (defined(_GLIBCXX_RELEASE)                  &&  \
            _GLIBCXX_RELEASE > 4)))
#  define HAVE_WORKING_REGEX 1
#else
#  define HAVE_WORKING_REGEX 0
#endif


extern "C" {

/**
  Function uses to extract options and store them in map.
  @param[in] string containing options separated by spaces (ex: "-verbose -silent -readonly -ids=myids")
  @param[out] map containing extracted options and optionnal values
  @result number of options in map
*/
LIBRARY_API int extractOptions(const std::string& strOptions, std::map<std::string, std::string>& mapOptions);

/**
  Function uses to check in option is in map.
  @param[in] string containing option name
  @param[in] map containing options and optionnal values
  @param[out] optionnal value of the found option
  @result true if option found in map otherwise false
*/
LIBRARY_API bool isOptionExist(const std::string& strOption, const std::map<std::string, std::string>& mapOptions, std::string& strValue);

/**
  Function uses to compare UAL or DD version.
  The version format can be "x.y", "x.y.z", "x.y.z-www", "x.y.z_www", "x-y", "x-y-z" etc...
  @param[in] string containing version V1
  @param[in] string containing version V2
  @result 0 if versions are equal, -1 if V1 > V2 and 1 if V1 < V2
*/
LIBRARY_API int compareVersion(const std::string& strV1, const std::string& strV2);

}


#endif

#endif // UAL_UTILITIES_H