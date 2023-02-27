#pragma once

#ifndef IMAS_UDA_DEBUG_HPP
#define IMAS_UDA_DEBUG_HPP

#include <ostream>
#include <deque>
#include <vector>
#include <map>
#include <unordered_map>
#include <boost/variant.hpp>

#include "uda_utilities.hpp"
#include "uda_xml.hpp"
#include "uda_cache.hpp"

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

inline std::ostream& operator<<(std::ostream& out, const std::vector<char>& vec)
{
    std::string s = { vec.data(), vec.size() };
    out << s;
    return out;
}

namespace imas {
namespace uda {

class stream_visitor : public boost::static_visitor<>
{
public:
    explicit stream_visitor(std::ostream& out) : out_{out} {};
    void operator()(const std::vector<int>& vec) const { out_ << vec; }
    void operator()(const std::vector<double>& vec) const { out_ << vec; }
    void operator()(const std::vector<char>& vec) const { out_ << vec; }
    void operator()(const std::vector<double _Complex>& vec) const { out_ << vec; }
private:
    std::ostream& out_;
};

}
}

inline std::ostream& operator<<(std::ostream& out, const imas::uda::VariantVector& vec)
{
    boost::apply_visitor(imas::uda::stream_visitor(out), vec);
    return out;
}

inline std::ostream& operator<<(std::ostream& out, const imas::uda::CacheData& data)
{
    out << "{ shape=" << data.shape << ", values=" << data.values << " }";
    return out;
}

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

inline std::ostream& operator<<(std::ostream& out, const imas::uda::Attribute& attr) {
    out << "{ type = " << attr.type << ", "
        << "dtype = " << attr.dtype << ", "
        << "data_type = " << attr.data_type << ", "
        << "rank = " << attr.rank << " }";

    return out;
}

#endif // IMAS_UDA_DEBUG_HPP
