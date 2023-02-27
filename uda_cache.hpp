#pragma once

#ifndef IMAS_UDA_CACHE_HPP
#define IMAS_UDA_CACHE_HPP

#include <boost/variant.hpp>
#include <map>
#include <string>
#include <vector>
#include <numeric>
#include <UDA.hpp>
#include <serialisation/capnp_serialisation.h>

namespace imas {
namespace uda {

using VariantVector = boost::variant<std::vector<int>, std::vector<double>, std::vector<char>, std::vector<double _Complex>>;

struct CacheData {
    std::vector<size_t> shape;
    VariantVector values;
};

using CacheType = std::map<std::string, imas::uda::CacheData>;

template <typename T>
void add_value_to_cache(const std::string& name, NodeReader* node, const std::vector<size_t>& shape, imas::uda::CacheType& cache)
{
    size_t count = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

    std::vector<T> data(count);
    auto buffer = reinterpret_cast<char*>(data.data());

    uda_capnp_read_data(node, buffer);
    cache[name] = CacheData{ shape, data };
}

void add_node_to_cache(NodeReader* node, CacheType& cache);
void add_data_to_cache(const ::uda::Result& result, CacheType& cache);

}
}
#endif // IMAS_UDA_CACHE_HPP
