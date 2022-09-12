#include "uda_cache.hpp"

#include <algorithm>

#include "ual_const.h"
#include "uda_exceptions.hpp"

void imas::uda::add_node_to_cache(::uda::TreeNode& node, CacheType& cache)
{
    auto data = node.atomicVector("value");

    if (data.type() == typeid(unsigned char)) {
        auto type = node.atomicScalar("datatype");
        auto itype = type.as<int>();

        auto rank = node.atomicScalar("rank");
        auto irank = rank.as<int>();

        auto dims = node.atomicVector("dims");
        auto idims = dims.as<int>();
        idims.resize(irank);

        auto name = node.atomicScalar("name");
        std::string sname = name.as<char*>();

        auto vec = data.as<unsigned char>();
        switch (itype) {
            case CHAR_DATA:
                add_value_to_cache<char>(sname, vec, idims, cache);
                break;
            case INTEGER_DATA:
                add_value_to_cache<int>(sname, vec, idims, cache);
                break;
            case DOUBLE_DATA:
                add_value_to_cache<double>(sname, vec, idims, cache);
                break;
            case COMPLEX_DATA:
                add_value_to_cache<double _Complex>(sname, vec, idims, cache);
                break;
            case 0:
                // no data - ignore
                break;
            default: throw imas::uda::CacheException("Unknown data type: " + std::to_string(itype));
        }
    }
}

void imas::uda::add_data_to_cache(const ::uda::Result& result, CacheType& cache)
{
    if (result.errorCode() == 0 && result.isTree()) {
        auto tree = result.tree();
        auto data_node = tree.child(0);

        for (int n = 0; n < data_node.numChildren(); ++n) {
            auto child = data_node.child(n);
            add_node_to_cache(child, cache);
        }
    }
}