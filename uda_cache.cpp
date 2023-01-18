#include "uda_cache.hpp"

#include <algorithm>
#include <clientserver/udaTypes.h>

#include "ual_const.h"
#include "uda_exceptions.hpp"

void imas::uda::add_node_to_cache(NodeReader* node, CacheType& cache)
{
    const char* name = uda_capnp_read_name(node);
    int type = uda_capnp_read_type(node);
    size_t rank = uda_capnp_read_rank(node).value;

    std::vector<size_t> shape(rank);
    uda_capnp_read_shape(node, shape.data());

    size_t count = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());
    std::vector<char> bytes(count);

    switch (type) {
        case CHAR_DATA:
        case UDA_TYPE_STRING:
            add_value_to_cache<char>(name, node, shape, cache);
            break;
        case INTEGER_DATA:
        case UDA_TYPE_INT:
            add_value_to_cache<int>(name, node, shape, cache);
            break;
        case DOUBLE_DATA:
        case UDA_TYPE_DOUBLE:
            add_value_to_cache<double>(name, node, shape, cache);
            break;
        case COMPLEX_DATA:
        case UDA_TYPE_COMPLEX:
            add_value_to_cache<double _Complex>(name, node, shape, cache);
            break;
        case 0:
            // no data - ignore
            break;
        default: throw imas::uda::CacheException("Unknown data type: " + std::to_string(type));
    }
}

void imas::uda::add_data_to_cache(const ::uda::Result& result, CacheType& cache)
{
    if (result.errorCode() == 0 && result.uda_type() == UDA_TYPE_CAPNP) {
        const char* data = result.raw_data();
        size_t size = result.size();
        auto tree = uda_capnp_deserialise(data, size);

        auto root = uda_capnp_read_root(tree);
        size_t num_children = uda_capnp_num_children(root);
        for (size_t n = 0; n < num_children; ++n) {
            auto child = uda_capnp_read_child_n(tree, root, n);
            add_node_to_cache(child, cache);
        }
    }
}