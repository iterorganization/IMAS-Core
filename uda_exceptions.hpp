#pragma once

#ifndef IMAS_UDA_EXCEPTIONS_HPP
#define IMAS_UDA_EXCEPTIONS_HPP

#include <string>

#include "ual_lowlevel.h"
#include "ual_const.h"
#include "pugixml.hpp"

namespace imas {
namespace uda {

class LowlevelException : public std::runtime_error
{
public:
    LowlevelException(int code, const std::string& message) : std::runtime_error(message), code_(code)
    {}

    int code() const
    { return code_; }

private:
    int code_;
};

class CacheException : public std::runtime_error
{
public:
    explicit CacheException(const std::string& message) : std::runtime_error(message)
    {}
};

}
} // namespace imas

#endif // IMAS_UDA_EXCEPTIONS_HPP