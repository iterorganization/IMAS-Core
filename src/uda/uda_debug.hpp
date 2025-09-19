#pragma once

#ifndef IMAS_UDA_DEBUG_HPP
#define IMAS_UDA_DEBUG_HPP

/**
 * Functions for streaming UDA data to iostream for debug purposes.
 */

#include <ostream>
#include <deque>
#include <vector>
#include <map>
#include <unordered_map>
#include <complex>
#include <boost/variant.hpp>

#include "uda_utilities.hpp"
#include "uda_xml.hpp"
#include "uda_cache.hpp"

/**
 * Stream a std::deque<T> to std::ostream as a comma separated list.
 */
template <typename T>
std::ostream& operator<<(std::ostream& out, const std::deque<T>& vec)
{
    const char* delim = "";
    out << "[";
    for (const T& item : vec) {
        out << delim << item;
        delim = ", ";
    }
    out << "]";
    return out;
}

/**
 * Stream a std::vector<T> to std::ostream as a comma separated list.
 */
template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& vec)
{
    if (vec.size() == 1) {
        out << vec[0];
    } else {
        const char* delim = "";
        out << "[";
        int n = 0;
        for (const T& item : vec) {
            if (n == 10) {
                out << delim << "...";
                break;
            } else {
                out << delim << item;
            }
            delim = ", ";
            ++n;
        }
        out << "]";
    }
    return out;
}

/**
 * Stream a std::vector<char> to std::ostream, treating it as a standard string.
 */
inline std::ostream& operator<<(std::ostream& out, const std::vector<char>& vec)
{
    std::string s = { vec.data(), vec.size() };
    out << s;
    return out;
}

namespace imas {
namespace uda {

/**
 * Class derived from boost::static_visitor to allow streaming of imas::uda::VariantVector to std::ostream.
 */
class stream_visitor : public boost::static_visitor<>
{
public:
    explicit stream_visitor(std::ostream& out) : out_{out} {};
    void operator()(const std::vector<int>& vec) const { out_ << vec; }
    void operator()(const std::vector<double>& vec) const { out_ << vec; }
    void operator()(const std::vector<char>& vec) const { out_ << vec; }
    void operator()(const std::vector<std::complex<double>>& vec) const { out_ << vec; }
private:
    std::ostream& out_;
};

}
}

/**
 * Stream a imas::uda::VariantVector to std::ostream by applying the stream_visitor class.
 */
inline std::ostream& operator<<(std::ostream& out, const imas::uda::VariantVector& vec)
{
    boost::apply_visitor(imas::uda::stream_visitor(out), vec);
    return out;
}

/**
 * Stream a imas::uda::Cachdata object to std::ostream.
 */
inline std::ostream& operator<<(std::ostream& out, const imas::uda::CacheData& data)
{
    out << "{ shape=" << data.shape << ", values=" << data.values << " }";
    return out;
}

/**
 * Stream a std::map<T, U> to std::ostream as a newline separated list of "key: value" pairs.
 *
 * T and U will need to be streamable to std::ostream.
 */
template <typename T, typename U>
std::ostream& operator<<(std::ostream& out, const std::map<T, U>& map)
{
    const char* delim = "\n  ";
    out << "{";
    for (const auto& item : map) {
        out << delim << item.first << ": " << item.second;
    }
    out << "\n}";
    return out;
}

/**
 * Stream a std::unordered_map<T, U> to std::ostream as a newline separated list of "key: value" pairs.
 *
 * T and U will need to be streamable to std::ostream.
 */
template <typename T, typename U>
std::ostream& operator<<(std::ostream& out, const std::unordered_map<T, U>& map)
{
    const char* delim = "\n  ";
    out << "{";
    for (const auto& item : map) {
        out << delim << item.first << ": " << item.second;
    }
    out << "\n}";
    return out;
}

/**
 * Stream a imas::uda::Attribute object to std::ostream.
 */
inline std::ostream& operator<<(std::ostream& out, const imas::uda::Attribute& attr) {
    out << "{ type = " << attr.type << ", "
        << "dtype = " << attr.dtype << ", "
        << "data_type = " << attr.data_type << ", "
        << "rank = " << attr.rank << " }";

    return out;
}

#endif // IMAS_UDA_DEBUG_HPP
