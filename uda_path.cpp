#include "uda_path.hpp"

#include <stdexcept>

bool imas::is_integer(const std::string& string)
{
    if (string.empty()) {
        return false;
    }
    char* end = nullptr;
    std::strtol(string.c_str(), &end, 10);
    return end != nullptr && *end == '\0';
}

bool imas::is_integer(char chr)
{
    return chr >= '0' && chr <= '9';
}

bool imas::is_index(const std::string& string)
{
    if (string.back() != ']' && string.back() != ')') {
        return false;
    }

    auto pos = std::string::npos;
    if (string.back() == ']') {
        pos = string.find('[');
    } else {
        pos = string.find('(');
    }

    if (pos == std::string::npos || pos == 0) {
        return false;
    }

    std::string rem = std::string{ string.begin() + pos + 1, string.end() - 1 };
    if (rem.empty()) {
        return false;
    }

    const char* start = rem.c_str();
    const char* end = rem.end().base();
    char* next = const_cast<char*>(start);

    while (next != end) {
        char* prev = next;
        strtol(next, &next, 10);
        if (prev == next) {
            return false;
        }
        if (next != end) {
            if (*next != ':') {
                return false;
            }
            ++next;
        }
    }

    return true;
}

std::pair<std::string, imas::Range> imas::parse_index(const std::string& string)
{
    if (string.back() != ']' && string.back() != ')') {
        throw std::runtime_error{ "invalid string " + string };
    }

    auto pos = std::string::npos;
    if (string.back() == ']') {
        pos = string.find('[');
    } else {
        pos = string.find('(');
    }

    if (pos == std::string::npos || pos == 0) {
        throw std::runtime_error{ "invalid string " + string };
    }

    std::string name = std::string{ string.begin(), string.begin() + pos };
    std::string rem = std::string{ string.begin() + pos + 1, string.end() - 1 };

    imas::Range range = { 0, 0, 1 };

    const char* start = rem.c_str();
    const char* end = rem.end().base();
    char* next = const_cast<char*>(start);

    int count = 0;
    while (next != end) {
        char* prev = next;
        long num = strtol(next, &next, 10);
        if (prev == next) {
            throw std::runtime_error{ "invalid string " + string };
        }
        switch (count) {
            case 0: range.begin = num; range.end = num + 1; break;
            case 1: range.end = num + 1; break;
            case 2: range.stride = num; break;
            default:
                throw std::runtime_error{ "invalid string " + string };
        }
        ++count;
        if (next != end) {
            if (*next != ':') {
                throw std::runtime_error{ "invalid string " + string };
            }
            ++next;
        }
    }

    return { name, range };
}
