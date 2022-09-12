#ifndef IMAS_UDA_PREFETCH_UDA_PATH_HPP
#define IMAS_UDA_PREFETCH_UDA_PATH_HPP

#include <string>

namespace imas {

struct Range {
    long begin;
    long end;
    long stride;
};

bool is_integer(const std::string& string);
bool is_integer(char chr);
bool is_index(const std::string& string);
std::pair<std::string, imas::Range> parse_index(const std::string& string);

}

#endif // IMAS_UDA_PREFETCH_UDA_PATH_HPP
