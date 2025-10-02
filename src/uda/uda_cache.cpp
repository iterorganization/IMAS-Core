#include "uda_cache.hpp"

#include <algorithm>
#include <complex>
#include <clientserver/udaTypes.h>

#include "al_const.h"

void imas::uda::add_node_to_cache(TreeReader* tree, NodeReader* node, CacheType& cache)
{
    const char* name = uda_capnp_read_name(node);

    size_t num_children = uda_capnp_num_children(node);
    if (num_children != 2) {
        throw imas::uda::CacheException("Invalid number of children on node: " + std::to_string(num_children));
    }

    auto shape_node = uda_capnp_read_child_n(tree, node, 0);
    auto data_node = uda_capnp_read_child_n(tree, node, 1);

    std::vector<size_t> size(1);
    uda_capnp_read_shape(shape_node, size.data());

    size_t count = std::accumulate(size.begin(), size.end(), 1, std::multiplies<size_t>());
    std::vector<int> shape(count);

    if (count != 0) {
        bool eos = uda_capnp_read_is_eos(shape_node);
        if (!eos) {
            throw imas::uda::CacheException("UDA backend does not currently handle streamed data");
        }

        size_t num_slices = uda_capnp_read_num_slices(shape_node);
        if (num_slices != 1) {
            throw imas::uda::CacheException("Incorrect number of slices for shape node");
        }

        size_t slice_size = uda_capnp_read_slice_size(shape_node, 0);
        if (slice_size / sizeof(int) != count) {
            throw imas::uda::CacheException("Incorrect amount of data found in shape node slices");
        }

        auto buffer = reinterpret_cast<char*>(shape.data());
        uda_capnp_read_data(shape_node, 0, buffer);
    }

    int type = uda_capnp_read_type(data_node);

    switch (type) {
        case CHAR_DATA:
        case UDA_TYPE_STRING:
            add_value_to_cache<char>(name, data_node, shape, cache);
            break;
        case INTEGER_DATA:
        case UDA_TYPE_INT:
            add_value_to_cache<int>(name, data_node, shape, cache);
            break;
        case DOUBLE_DATA:
        case UDA_TYPE_DOUBLE:
            add_value_to_cache<double>(name, data_node, shape, cache);
            break;
        case COMPLEX_DATA:
        case UDA_TYPE_DCOMPLEX:
            add_value_to_cache<std::complex<double>>(name, data_node, shape, cache);
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
            add_node_to_cache(tree, child, cache);
        }

        uda_capnp_free_tree_reader(tree);
    }
}
