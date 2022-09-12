#pragma once

#ifndef IMAS_UDA_PREFETCH_UDA_CACHE_HPP
#define IMAS_UDA_PREFETCH_UDA_CACHE_HPP

#include <boost/variant.hpp>
#include <map>
#include <string>
#include <vector>
#include <numeric>
#include <UDA.hpp>

namespace imas {
namespace uda {

using VariantVector = boost::variant<std::vector<int>, std::vector<double>, std::vector<char>, std::vector<double _Complex>>;

struct CacheData {
    std::vector<int> shape;
    VariantVector values;
};

using CacheType = std::map<std::string, imas::uda::CacheData>;

template <typename T>
void add_value_to_cache(const std::string& name, const std::vector<unsigned char>& bytes, const std::vector<int>& shape, imas::uda::CacheType& cache)
{
    size_t count = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

    std::vector<T> d = {};
    auto buffer = reinterpret_cast<const T *>(bytes.data());
    d.resize(count);
    std::copy(buffer, buffer + count, d.begin());
    cache[name] = CacheData{ shape, d };
}

void add_node_to_cache(::uda::TreeNode& node, CacheType& cache);
void add_data_to_cache(const ::uda::Result& result, CacheType& cache);

}
}
#endif // IMAS_UDA_PREFETCH_UDA_CACHE_HPP
