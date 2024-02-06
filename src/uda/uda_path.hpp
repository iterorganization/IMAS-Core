#pragma once

#ifndef IMAS_UDA_PATH_HPP
#define IMAS_UDA_PATH_HPP

/**
 * Utility functions for working with IMAS paths.
 */

#include <string>

namespace imas {

struct Range {
    long begin;
    long end;
    long stride;
};

/**
 * Tests if the given string is a valid integer.
 */
bool is_integer(const std::string& string);

/**
 * Tests if the given char is a valid integer.
 */
bool is_integer(char chr);

/**
 * Tests if the given string contains a valid UDA path index.
 *
 * A valid path index is a name followed by either a number or numbers separated by colons, inside either square or
 * round brackets.
 *
 * Examples:
 *  - foo[2]
 *  - foo(0:1)
 */
bool is_index(const std::string& string);

/**
 * Extract the index range (begin, end, stride) and the non-index name from the given string.
 *
 * @return a pair containing (the non-index name, index range)
 */
std::pair<std::string, imas::Range> parse_index(const std::string& string);

} // namespace imas

#endif // IMAS_UDA_PATH_HPP
