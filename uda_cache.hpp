#pragma once

#ifndef IMAS_UDA_CACHE_HPP
#define IMAS_UDA_CACHE_HPP

/**
 * Functions for storing and retrieving UDA data from a RAM-cache.
 */

#include <boost/variant.hpp>
#include <map>
#include <string>
#include <vector>
#include <numeric>
#include <UDA.hpp>
#include <serialisation/capnp_serialisation.h>

#include "uda_exceptions.hpp"

namespace imas {
namespace uda {

/**
 * Variant type used for storing different vector types in the cache.
 */
using VariantVector = boost::variant<std::vector<int>, std::vector<double>, std::vector<char>, std::vector<double _Complex>>;

/**
 * Structure for holding a cache entry.
 */
struct CacheData {
    std::vector<int> shape;
    VariantVector values;
};

/**
 * Type alias for RAM-cache.
 */
using CacheType = std::map<std::string, imas::uda::CacheData>;

/**
 * Read the byte data out of the NodeReader into a std::vector<T> and store in the cache alongside the given shape, with
 * the cache key given by the specified name.
 *
 * @tparam T the type of the std::vector<T> which is used to store a copy of the data read out of the NodeReader buffer
 * @param name the cache key to store the cache entry against
 * @param node the NodeReader containing the data to cache
 * @param shape the shape of the data
 * @param cache the cache to store the data in
 */
template <typename T>
void add_value_to_cache(const std::string& name, NodeReader* node, const std::vector<int>& shape, imas::uda::CacheType& cache)
{
    size_t rank = uda_capnp_read_rank(node).value;
    std::vector<size_t> size(rank);
    uda_capnp_read_shape(node, size.data());

    size_t node_count = std::accumulate(size.begin(), size.end(), 1, std::multiplies<size_t>());
    size_t shape_count = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

    if (node_count != shape_count) {
        throw imas::uda::CacheException("Count of data does not match shape");
    }

    std::vector<T> data(node_count);
    auto buffer = reinterpret_cast<char*>(data.data());

    uda_capnp_read_data(node, buffer);
    cache[name] = CacheData{ shape, data };
}

/**
 * Use the provided NodeReader to access the deserialised Capn Proto data node and store it in the provided cache.
 *
 * @param tree the TreeReader wrapper the Capn Proto deserialised data tree
 * @param node the NodeReader wrapping the Capn Proto node to read
 * @param cache the cache to store the data in
 */
void add_node_to_cache(TreeReader* tree, NodeReader* node, CacheType& cache);

/**
 * Extract the data from the uda::Result object and store it in the provided cache.
 *
 * @param result the uda::Result object containing the data to cache - this must contain Capn Proto serialised data.
 * @param cache the cache to store the data in
 */
void add_data_to_cache(const ::uda::Result& result, CacheType& cache);

}
}
#endif // IMAS_UDA_CACHE_HPP
