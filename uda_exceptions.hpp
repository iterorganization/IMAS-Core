#pragma once

#ifndef IMAS_UDA_EXCEPTIONS_HPP
#define IMAS_UDA_EXCEPTIONS_HPP

/**
 * Exceptions specific to the UDA backend.
 */

#include <string>

#include "ual_lowlevel.h"
#include "ual_const.h"
#include "pugixml.hpp"

namespace imas {
namespace uda {

/**
 * Exception thrown by the UDA caching functions.
 */
class CacheException : public std::runtime_error
{
public:
    explicit CacheException(const std::string& message) : std::runtime_error(message)
    {}
};

}
} // namespace imas

#endif // IMAS_UDA_EXCEPTIONS_HPP