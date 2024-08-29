#include "uda_backend.h"

#include <cstdlib>
#include <client/accAPI.h>
#include <client/udaClient.h>
#include <clientserver/udaTypes.h>

#include "semver.hpp"

#include "uda_utilities.hpp"
#include "uda_xml.hpp"
#include "uda_debug.hpp"

#include <string>

using namespace semver::literals;

namespace {

/**
 * Generate string path for the given ArraystructContext object.
 *
 * @param ctx the context to generate the path for
 * @param for_dim a flag to specify whether we should include the context index in the path
 * @return the generated path
 */
std::string array_path(ArraystructContext* ctx, bool for_dim = false)
{
    std::string path;
    if (for_dim) {
        path = ctx->getPath();
    } else {
        path = ctx->getPath() + "[" + std::to_string(ctx->getIndex()) + "]";
    }
    while (ctx->getParent() != nullptr) {
        ctx = ctx->getParent();
        path = ctx->getPath()
                .append("[")
                .append(std::to_string(ctx->getIndex()))
                .append("]/")
                .append(path);
    }
    return path;
}

/**
 * Generate string path for the given Context object.
 *
 * If the Context object is an ArraystructContext this forwards the request onto the function above, otherwise for
 * OperationContext objects it just returns the DataobjectName.
 *
 * @param ctx the context to generate the path for
 * @param for_dim a flag to specify whether we should include the context index in the path
 * @return the generated path
 */
std::string array_path(Context* ctx, bool for_dim = false)
{
    auto arr_ctx = dynamic_cast<ArraystructContext*>(ctx);
    if (arr_ctx != nullptr) {
        return arr_ctx->getOperationContext()->getDataobjectName() + "/" + array_path(arr_ctx, for_dim);
    }
    auto op_ctx = dynamic_cast<OperationContext*>(ctx);
    if (op_ctx != nullptr) {
        return op_ctx->getDataobjectName();
    }
    return "";
}

/**
 * Unpack the data returned from UDA as a NodeReader object.
 *
 * The NodeReader object allows deserialisation of the Capn Proto serialised data using the uda_capnp_read_data
 * function. This is used to read the data into a newly malloced array buffer returned via the `data` argument.
 *
 * @tparam T the type of the data being unpacked
 * @param node the `NodeReader` from which the data is being unpacked
 * @param shape the shape of the data being unpacked
 * @param data [OUT] a pointer to the buffer which is malloced and then populated with the unpacked data
 * @param dim [OUT] the rank of the data being returned
 * @param size [OUT] array of dimension sizes
 */
template <typename T>
void unpack_data(NodeReader* node,
                 const std::vector<int>& shape,
                 void** data,
                 int* dim,
                 int* size)
{
    size_t rank = uda_capnp_read_rank(node).value;
    std::vector<size_t> size_vec(rank);
    uda_capnp_read_shape(node, size_vec.data());

    size_t node_count = std::accumulate(size_vec.begin(), size_vec.end(), 1, std::multiplies<size_t>());
    size_t shape_count = std::accumulate(shape.begin(), shape.end(), 1, std::multiplies<size_t>());

    if (node_count != shape_count) {
        throw imas::uda::CacheException("Count of data does not match shape");
    }

    bool eos = uda_capnp_read_is_eos(node);
    if (!eos) {
        throw imas::uda::CacheException("UDA backend does not currently handle streamed data");
    }

    size_t num_slices = uda_capnp_read_num_slices(node);
    size_t buffer_size = node_count * sizeof(T);
    *data = malloc(buffer_size);
    auto buffer = reinterpret_cast<char*>(*data);
    size_t offset = 0;

    for (size_t i = 0; i < num_slices; ++i) {
        size_t slice_size = uda_capnp_read_slice_size(node, i);
        if ((offset + slice_size) > buffer_size) {
            throw imas::uda::CacheException("Too much data found in slices");
        }
        uda_capnp_read_data(node, i, buffer + offset);
        offset += slice_size;
    }

    if (offset != buffer_size) {
        throw imas::uda::CacheException("Sum of slice sizes not equal to provided data count");
    }

    *dim = static_cast<int>(shape.size());
    for (int i = 0; i < *dim; ++i) {
        size[i] = shape[i];
    }
}

/**
 * Unpack the data returned from UDA as a NodeReader object.
 *
 * This checks the name of the returned node against the passed path, and then uses the `unpack_data` function,
 * templated based on the type specified in the node.
 *
 * @param path the data path to check against the name of the node
 * @param node the `NodeReader` from which the data is being unpacked
 * @param shape the shape of the data being unpacked
 * @param data [OUT] a pointer to the buffer which is malloced and then populated with the unpacked data
 * @param dim [OUT] the rank of the data being returned
 * @param size [OUT] array of dimension sizes
 */
void unpack_node(const std::string& path,
                 TreeReader* tree,
                 NodeReader* node,
                 void** data,
                 int* datatype,
                 int* dim,
                 int* size)
{
    const char* name = uda_capnp_read_name(node);

    if (name != path) {
        throw ALBackendException("Invalid node returned: " + std::string(name));
    }

    size_t num_children = uda_capnp_num_children(node);
    if (num_children != 2) {
        throw ALBackendException("Invalid number of children on node: " + std::to_string(num_children));
    }

    auto shape_node = uda_capnp_read_child_n(tree, node, 0);
    auto data_node = uda_capnp_read_child_n(tree, node, 1);

    std::vector<size_t> shape_shape(1);
    uda_capnp_read_shape(shape_node, shape_shape.data());

    size_t count = std::accumulate(shape_shape.begin(), shape_shape.end(), 1, std::multiplies<size_t>());
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
            *datatype = CHAR_DATA;
            unpack_data<char>(data_node, shape, data, dim, size);
            break;
        case INTEGER_DATA:
        case UDA_TYPE_INT:
            *datatype = INTEGER_DATA;
            unpack_data<int>(data_node, shape, data, dim, size);
            break;
        case DOUBLE_DATA:
        case UDA_TYPE_DOUBLE:
            *datatype = DOUBLE_DATA;
            unpack_data<double>(data_node, shape, data, dim, size);
            break;
        case COMPLEX_DATA:
        case UDA_TYPE_COMPLEX:
            *datatype = COMPLEX_DATA;
            unpack_data<double _Complex>(data_node, shape, data, dim, size);
            break;
        default: throw ALBackendException("Unknown data type: " + std::to_string(type));
    }
}

std::string strip_occurrence(const std::string& ids)
{
    auto pos = ids.find('/');
    return ids.substr(0, pos);
}

std::string get_backend(uri::QueryDict& query) {
    auto maybe_mapping = query.get("mapping");
    auto maybe_backend = query.get("backend");

    return maybe_backend.value_or(maybe_mapping ? "uda" : "mdsplus");
}

} // anon namespace

std::pair<int, int> UDABackend::getVersion(DataEntryContext* ctx)
{
    std::pair<int, int> version;
    if (ctx == NULL) {
        version = {UDA_BACKEND_VERSION_MAJOR, UDA_BACKEND_VERSION_MINOR};
    } else {
        version = {0, 0}; // temporary placeholder
    }
    return version;
}

void UDABackend::process_options(uri::Uri uri)
{
    uri::OptionalValue maybe_cache_mode = uri.query.get("cache_mode");
    uri::OptionalValue maybe_verbose = uri.query.get("verbose");

    if (maybe_cache_mode) {
        std::string value = maybe_cache_mode.value();
        if (value == "none") {
            cache_mode_ = imas::uda::CacheMode::None;
        } else if (value == "ids") {
            cache_mode_ = imas::uda::CacheMode::IDS;
        } else if (value == "struct") {
            cache_mode_ = imas::uda::CacheMode::Struct;
        } else {
            throw ALException("invalid cache mode", LOG);
        }
    }

    if (maybe_verbose) {
        std::string value = maybe_verbose.value();
        if (value == "1" || value == "true") {
            verbose_ = true;
        } else {
            verbose_ = false;
        }
    }
}

void UDABackend::openPulse(DataEntryContext* ctx,
                           int mode)
{
    process_options(ctx->getURI());

    if (verbose_) {
        std::cout << "UDABackend openPulse\n";
    }
    open_mode_ = mode;

    auto maybe_plugin = ctx->getURI().query.get("plugin");
    if (maybe_plugin) {
        plugin_ = maybe_plugin.value();
    }

    std::string host = ctx->getURI().authority.host;
    int port = ctx->getURI().authority.port;

    if (!host.empty()) {
        uda::Client::setServerHostName(host);
    }
    if (port != 0) {
        uda::Client::setServerPort(port);
    }

    if (verbose_) {
        std::cout << "UDA server: " << uda::Client::serverHostName() << "\n";
        std::cout << "UDA port: " << uda::Client::serverPort() << "\n";
        std::cout << "UDA plugin: " << plugin_ << "\n";
    }

    auto maybe_args = ctx->getURI().query.get("init_args");

    std::string args;
    if (maybe_args) {
        args = maybe_args.value();
        std::replace(args.begin(), args.end(), ';', ',');
    }

    std::stringstream ss;
    ss << plugin_ << "::init(" << args << ")";

    std::string directive = ss.str();
    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }

    try {
        uda_client_.get(directive, "");
    } catch (const uda::UDAException& ex) {
        if (verbose_) {
            std::cout << "UDA error: " << ex.what() << "\n";
        }
    }

    auto query = ctx->getURI().query;

    std::string backend = get_backend(query);
    query.remove("backend");
    query.remove("cache_mode");
    query.remove("verbose");
    std::string dd_version = query.get("dd_version").value_or(dd_version_);
    query.set("dd_version", dd_version);
    std::string uri = "imas:" + backend + "?" + query.to_string();

    ss.str("");
    ss.clear();

    ss << plugin_
       << "::open("
       << "uri='" << uri << "'"
       << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::OpenMode>(mode) << "'"
       << ")";

    directive = ss.str();

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }

    try {
        uda_client_.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw ALException(ex.what(), LOG);
    }
}

void UDABackend::closePulse(DataEntryContext* ctx,
                            int mode)
{
    if (verbose_) {
        std::cout << "UDABackend closePulse\n";
    }

    cache_.clear();
    cache_mode_ = imas::uda::CacheMode::None;

    auto query = ctx->getURI().query;
    std::string backend = get_backend(query);
    query.remove("backend");
    query.remove("cache_mode");
    query.remove("verbose");
    std::string dd_version = query.get("dd_version").value_or(dd_version_);
    query.set("dd_version", dd_version);
    std::string uri = "imas:" + backend + "?" + query.to_string();

    std::stringstream ss;
    ss << plugin_
       << "::close("
       << "uri='" << uri << "'"
       << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::CloseMode>(mode) << "'"
       << ")";

    std::string directive = ss.str();

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }
    try {
        uda_client_.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw ALException(ex.what(), LOG);
    }
}

template <typename T>
T* read_data_from_cache(imas::uda::CacheData& cache_data, int rank)
{
    size_t count = std::accumulate(cache_data.shape.begin(), cache_data.shape.end(), 1, std::multiplies<size_t>());
    auto& vec = boost::get<std::vector<T>>(cache_data.values);
    if (rank == 0) {
        auto d = (T*)malloc(sizeof(T));
        *d = vec[0];
        return d;
    } else {
        auto d = (T*)malloc(sizeof(T) * count);
        memcpy(d, vec.data(), sizeof(T) * count);
        return d;
    }
}

int UDABackend::readData(Context* ctx,
                         std::string fieldname,
                         std::string timebasename,
                         void** data,
                         int* datatype,
                         int* dim,
                         int* size)
{
    auto path = array_path(ctx) + "/" + fieldname;

    if (verbose_) {
        std::cout << "UDABackend readData:" << path << "\n";
    }

    if (cache_.count(path)) {
        if (verbose_) {
            std::cout << "UDABackend readData: found in cache\n";
        }
        imas::uda::CacheData& cache_data = cache_.at(path);
        *dim = cache_data.shape.size();

        switch (*datatype) {
            case INTEGER_DATA:
                *data = read_data_from_cache<int>(cache_data, *dim);
                break;
            case DOUBLE_DATA:
                *data = read_data_from_cache<double>(cache_data, *dim);
                break;
            case CHAR_DATA:
                *data = read_data_from_cache<char>(cache_data, *dim);
                break;
        }
        std::copy(cache_data.shape.begin(), cache_data.shape.end(), size);
        // clear cache entry to save memory
        cache_.erase(path);
        return 1;
    } else if (fieldname == "ids_properties/homogeneous_time"
            || fieldname == "ids_properties/version_put/data_dictionary"
            || cache_mode_ == imas::uda::CacheMode::None) {
        try {
            auto arr_ctx = dynamic_cast<ArraystructContext*>(ctx);
            auto op_ctx = arr_ctx != nullptr ? arr_ctx->getOperationContext() : dynamic_cast<OperationContext*>(ctx);
//            auto entry_ctx = op_ctx->getDataEntryContext();

            auto query = ctx->getURI().query;
            std::string backend = get_backend(query);
            query.remove("backend");
            std::string dd_version = query.get("dd_version").value_or(dd_version_);
            query.set("dd_version", dd_version);
            std::string uri = "imas:" + backend + "?" + query.to_string();

            std::stringstream ss;
            ss << plugin_
               << "::get("
               << "uri='" << uri << "'"
               << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::OpenMode>(open_mode_) << "'"
               << ", dataObject='" << op_ctx->getDataobjectName() << "'"
               << ", access='" << imas::uda::convert_imas_to_uda<imas::uda::AccessMode>(op_ctx->getAccessmode()) << "'"
               << ", range='" << imas::uda::convert_imas_to_uda<imas::uda::RangeMode>(op_ctx->getRangemode()) << "'"
               << ", time=" << op_ctx->getTime()
               << ", interp='" << imas::uda::convert_imas_to_uda<imas::uda::InterpMode>(op_ctx->getInterpmode()) << "'"
               << ", path='" << path << "'"
               << ", datatype='" << imas::uda::convert_imas_to_uda<imas::uda::DataType>(*datatype) << "'"
               << ", rank=" << *dim
               << ", is_homogeneous=" << 0
               << ", dynamic_flags=" << 0
               << ", time_range_tmin=" << op_ctx->time_range.tmin
               << ", time_range_tmax=" << op_ctx->time_range.tmax
               << ", time_range_interp=" << op_ctx->time_range.interpolation_method
               << ", dtime=" << imas::uda::convert_dtime(op_ctx->time_range.dtime);
//               << ", is_homogeneous=" << is_homogeneous
//               << ", dynamic_flags=" << imas::uda::get_dynamic_flags(attributes, path);
            ss << ")";

            std::string directive = ss.str();

            if (verbose_) {
                std::cout << "UDABackend request: " << directive << "\n";
            }

            const uda::Result& result = uda_client_.get(directive, "");

            if (result.errorCode() == 0 && result.uda_type() == UDA_TYPE_CAPNP) {
                const char* bytes = result.raw_data();
                size_t num_bytes = result.size();
                auto tree = uda_capnp_deserialise(bytes, num_bytes);

                auto root = uda_capnp_read_root(tree);
                size_t num_children = uda_capnp_num_children(root);
                if (num_children == 0) {
                    return 0;
                }
                if (num_children > 1) {
                    throw ALBackendException(std::string("Too many tree elements returned"), LOG);
                }

                auto node = uda_capnp_read_child_n(tree, root, 0);
                unpack_node(path, tree, node, data, datatype, dim, size);

                uda_capnp_free_tree_reader(tree);
            }

            return 1;
        } catch (const uda::UDAException& ex) {
            return 0;
        }
    } else {
        return 0;
    }
}

bool UDABackend::get_homogeneous_flag(const std::string& ids, DataEntryContext* entry_ctx, OperationContext* op_ctx)
{
    if (verbose_) {
        std::cout << "UDABackend getting homogeneous_time\n";
    }

    std::string path = ids + "/ids_properties/homogeneous_time";

    auto query = entry_ctx->getURI().query;
    std::string backend = get_backend(query);
    query.remove("backend");
    std::string dd_version = query.get("dd_version").value_or(dd_version_);
    query.set("dd_version", dd_version);
    std::string uri = "imas:" + backend + "?" + query.to_string();

    std::stringstream ss;
    ss << plugin_
       << "::get("
       << "uri='" << uri << "'"
       << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::OpenMode>(open_mode_) << "'"
       << ", dataObject='" << ids << "'"
       << ", access='" << imas::uda::convert_imas_to_uda<imas::uda::AccessMode>(op_ctx->getAccessmode()) << "'"
       << ", range='" << imas::uda::convert_imas_to_uda<imas::uda::RangeMode>(op_ctx->getRangemode()) << "'"
       << ", time=" << op_ctx->getTime()
       << ", interp='" << imas::uda::convert_imas_to_uda<imas::uda::InterpMode>(op_ctx->getInterpmode()) << "'"
       << ", path='" << path << "'"
       << ", datatype='integer'"
       << ", rank=0"
       << ", is_homogeneous=0"
       << ", dynamic_flags=0"
       << ", time_range_tmin=" << op_ctx->time_range.tmin
       << ", time_range_tmax=" << op_ctx->time_range.tmax
       << ", time_range_interp=" << op_ctx->time_range.interpolation_method
       << ", dtime=" << imas::uda::convert_dtime(op_ctx->time_range.dtime);
    ss << ")";

    std::string directive = ss.str();

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }

    const uda::Result& result = uda_client_.get(directive, "");
    uda::Data* uda_data = result.data();

    imas::uda::add_data_to_cache(result, cache_);
    if (cache_.count(path)) {
        auto& cache_data = cache_.at(path);
        return boost::get<std::vector<int>>(cache_data.values).at(0);
    } else {
        throw ALBackendException(std::string("Invalid result for ids_properties/homogeneous_time: ") + uda_data->type().name(), LOG);
    }
}

void UDABackend::populate_cache(const std::string& ids, const std::string& path, DataEntryContext* entry_ctx, OperationContext* op_ctx)
{
    bool is_homogeneous = get_homogeneous_flag(ids, entry_ctx, op_ctx);

    auto ids_node = imas::uda::find_node(doc_->document_element(), strip_occurrence(ids), true);

    std::vector<std::string> ids_paths = imas::uda::generate_ids_paths(path, ids_node);

    std::vector<std::string> requests;
    imas::uda::AttributeMap attributes;

    auto node = imas::uda::find_node(doc_->document_element(), path, true);

    for (const auto& ids_path: ids_paths) {
        imas::uda::get_requests(requests, attributes, ids_path, node);
    }

    std::vector<std::string> uda_requests = {};

    for (const auto& request: requests) {
        auto attr = attributes.at(request);

        std::string data_type = imas::uda::convert_imas_to_uda<imas::uda::DataType>(attr.data_type);

        auto query = entry_ctx->getURI().query;
        std::string backend = get_backend(query);
        query.remove("backend");
        std::string dd_version = query.get("dd_version").value_or(dd_version_);
        query.set("dd_version", dd_version);
        std::string uri = "imas:" + backend + "?" + query.to_string();

        std::stringstream ss;
        ss << plugin_
           << "::get("
           << "uri='" << uri << "'"
           << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::OpenMode>(open_mode_) << "'"
           << ", dataObject='" << ids << "'"
           << ", access='" << imas::uda::convert_imas_to_uda<imas::uda::AccessMode>(op_ctx->getAccessmode()) << "'"
           << ", range='" << imas::uda::convert_imas_to_uda<imas::uda::RangeMode>(op_ctx->getRangemode()) << "'"
           << ", time=" << op_ctx->getTime()
           << ", interp='" << imas::uda::convert_imas_to_uda<imas::uda::InterpMode>(op_ctx->getInterpmode()) << "'"
           << ", path='" << request << "'"
           << ", datatype='" << imas::uda::convert_imas_to_uda<imas::uda::DataType>(attr.data_type) << "'"
           << ", rank=" << attr.rank
           << ", is_homogeneous=" << is_homogeneous
           << ", dynamic_flags=" << imas::uda::get_dynamic_flags(attributes, request)
           << ", time_range_tmin=" << op_ctx->time_range.tmin
           << ", time_range_tmax=" << op_ctx->time_range.tmax
           << ", time_range_interp=" << op_ctx->time_range.interpolation_method
           << ", dtime=" << imas::uda::convert_dtime(op_ctx->time_range.dtime);

        if (!attr.timebase.empty()) {
            ss << ", timebase='" << attr.timebase << "'";
        }

        auto maybe_pff_user = entry_ctx->getURI().query.get("ppf_user");
        if (maybe_pff_user) {
            ss << ", ppfUser='" << maybe_pff_user.value() << "'";
        }
        auto maybe_pff_sequence = entry_ctx->getURI().query.get("ppf_sequence");
        if (maybe_pff_sequence) {
            ss << ", ppfSequence='" << maybe_pff_sequence.value() << "'";
        }
        auto maybe_pff_dda = entry_ctx->getURI().query.get("ppf_dda");
        if (maybe_pff_dda) {
            ss << ", ppfDDA='" << maybe_pff_dda.value() << "'";
        }

        ss << ")";

        uda_requests.push_back(ss.str());
    }

    if (verbose_) {
        for (const auto& req: uda_requests) {
            std::cout << "UDABackend cache request: " << req << "\n";
        }
        std::cout << "UDABackend cache number of requests: " << uda_requests.size() << "\n";
    }

    int N = std::stoi(entry_ctx->getURI().query.get("batch_size").value_or("20"));
    auto query = entry_ctx->getURI().query;
    std::string backend = get_backend(query);
    try {
        size_t n = 0;
        while (n < uda_requests.size()) {
            size_t m = std::min(n + N, uda_requests.size());
            std::vector<std::string> reqs = std::vector<std::string>{ uda_requests.begin() + n, uda_requests.begin() + m };

            if (verbose_) {
                std::cout << "UDABackend cache get: " << n << " - " << m << std::endl;
            }
            uda::ResultList results = uda_client_.get_batch(reqs, "");
            for (auto handle: results.handles()) {
                auto& result = results.at(handle);
                imas::uda::add_data_to_cache(result, cache_);
            }

            ClientFlags flags = {};
            for (int i = 0; i < acc_getCurrentDataBlockIndex(&flags); ++i) {
                freeDataBlock(getIdamDataBlock(i));
            }
            acc_freeDataBlocks();

            n = m;
        }
    } catch (const uda::UDAException& ex) {
        throw ALException(ex.what(), LOG);
    }

    if (verbose_) {
        std::cout << cache_ << std::endl;
    }
}

void UDABackend::beginArraystructAction(ArraystructContext* ctx, int* size)
{
    if (verbose_) {
        std::cout << "UDABackend beginArraystructAction\n";
    }

    auto op_ctx = ctx->getOperationContext();
    auto entry_ctx = op_ctx->getDataEntryContext();

    auto ids = op_ctx->getDataobjectName();
    auto path = ids + "/" + array_path(ctx, true);

    if (cache_.count(path)) {
        if (verbose_) {
            std::cout << "UDABackend beginArraystructAction: found " << path << " in cache\n";
        }
        auto data = cache_.at(path);
        try {
            *size = boost::get<std::vector<int>>(data.values).at(0);
        } catch (boost::bad_get& ex) {
            throw ALException(ex.what(), LOG);
        } catch (std::out_of_range& ex) {
            throw ALException(ex.what(), LOG);
        }
        return;
    } else if (cache_mode_ == imas::uda::CacheMode::Struct) {
        if (verbose_) {
            std::cout << "UDABackend populating cache\n";
        }

        cache_.clear();

        populate_cache(ids, path, entry_ctx, op_ctx);

        if (cache_.count(path)) {
            auto& data = cache_.at(path);
            try {
                *size = boost::get<std::vector<int>>(data.values).at(0);
            } catch (boost::bad_get& ex) {
                *size = 0;
            } catch (std::out_of_range& ex) {
                *size = 0;
            }
        } else {
            *size = 0;
        }
    } else if (cache_mode_ == imas::uda::CacheMode::IDS) {
        *size = 0;
    } else {
        OperationContext* octx = ctx->getOperationContext();
        std::stringstream ss;
        ss << plugin_
           << "::beginArraystructAction("
           << ", size=" << *size
           << ", dataObject='" << octx->getDataobjectName() << "'"
           << ", access=" << octx->getAccessmode()
           << ", range=" << octx->getRangemode()
           << ", time=" << octx->getTime()
           << ", interp=" << octx->getInterpmode()
           << ", path='" << path << "'"
           << ", timebase='" << ctx->getTimebasePath() << "'"
           << ")";

        std::string directive = ss.str();

        std::cout << "UDABackend request: " << directive << "\n";
        const uda::Result& result = uda_client_.get(directive, "");
        uda::Data* uda_data = result.data();

        if (uda_data->type() == typeid(void)) {
            *size = 0;
            return;
        } else if (uda_data->type() != typeid(int)) {
            throw ALBackendException(
                    std::string("Invalid data type returned for beginArraystructAction: ") + uda_data->type().name(),
                    LOG);
        }
        *size = *reinterpret_cast<const int*>(uda_data->byte_data());
    }
}

void UDABackend::beginAction(OperationContext* op_ctx)
{
    if (verbose_) {
        std::cout << "UDABackend beginAction\n";
    }

    std::string ids = op_ctx->getDataobjectName();
    auto entry_ctx = op_ctx->getDataEntryContext();

    if (cache_mode_ == imas::uda::CacheMode::IDS) {
        std::string path = op_ctx->getDatapath();
        if (path != "ids_properties/homogeneous_time" && path != "ids_properties/version_put/data_dictionary") {
            if (verbose_) {
                std::cout << "UDABackend populating cache\n";
            }

            cache_.clear();

            if (path.empty()) {
                path = ids;
            } else {
                path = ids + "/" + path;
            }

            populate_cache(ids, path, entry_ctx, op_ctx);
        }
    } else {
        std::stringstream ss;
        ss << plugin_
           << "::beginAction("
           << ", dataObject='" << op_ctx->getDataobjectName() << "'"
           << ", access=" << op_ctx->getAccessmode()
           << ", range=" << op_ctx->getRangemode()
           << ", time=" << op_ctx->getTime()
           << ", interp=" << op_ctx->getInterpmode();

        auto maybe_pff_user = entry_ctx->getURI().query.get("ppf_user");
        if (maybe_pff_user) {
            ss << ", ppfUser='" << maybe_pff_user.value() << "'";
        }
        auto maybe_pff_sequence = entry_ctx->getURI().query.get("ppf_sequence");
        if (maybe_pff_sequence) {
            ss << ", ppfSequence='" << maybe_pff_sequence.value() << "'";
        }
        auto maybe_pff_dda = entry_ctx->getURI().query.get("ppf_dda");
        if (maybe_pff_dda) {
            ss << ", ppfDDA='" << maybe_pff_dda.value() << "'";
        }

        ss << ")";
        std::string directive = ss.str();

        if (verbose_) {
            std::cout << "UDABackend request: " << directive << "\n";
        }
        try {
            uda_client_.get(directive, "");
        } catch (const uda::UDAException& ex) {
            throw ALException(ex.what(), LOG);
        }
    }
}

void UDABackend::endAction(Context* ctx)
{
    if (verbose_) {
        std::cout << "UDABackend endAction\n";
    }

    if (ctx->getType() == CTX_PULSE_TYPE) {
        cache_.clear();
    }

//    std::stringstream ss;
//    ss << plugin_
//       << "::endAction("
//       << "type='" << imas::uda::convert_imas_to_uda<imas::uda::ContextType>(ctx->getType()) << "'"
//       << ")";
//
//    std::string directive = ss.str();
//
//    if (verbose_) {
//        std::cout << "UDABackend request: " << directive << "\n";
//    }
//    try {
//        uda_client_.get(directive, "");
//    } catch (const uda::UDAException& ex) {
//        throw ALException(ex.what(), LOG);
//    }
}

void
UDABackend::writeData(Context* ctx, std::string fieldname, std::string timebasename, void* data, int datatype, int dim,
                      int* size)
{
    if (verbose_) {
        std::cout << "UDABackend writeData\n";
    }

    std::stringstream ss;

    ss << plugin_
       << "::writeData("
       << ", field='" << fieldname << "'"
       << ", timebase='" << timebasename << "'"
       << ", datatype=" << datatype
       << ")";

    std::string directive = ss.str();

    std::vector<uda::Dim> dims;
    dims.reserve(dim);
    for (int i = 0; i < dim; ++i) {
        dims.emplace_back(i, size[i], "", "");
    }

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }

    int dummy[] = {0};
    uda::Array array{dummy, {}};

    try {
        switch (datatype) {
            case CHAR_DATA:
                array = uda::Array{reinterpret_cast<char*>(data), dims};
                break;
            case INTEGER_DATA:
                array = uda::Array{reinterpret_cast<int*>(data), dims};
                break;
            case DOUBLE_DATA:
                array = uda::Array{reinterpret_cast<double*>(data), dims};
                break;
            default:
                throw ALException("unknown datatype", LOG);
        }
        uda_client_.put(directive, array);
    } catch (const uda::UDAException& ex) {
        throw ALException(ex.what(), LOG);
    }
}

void UDABackend::deleteData(OperationContext* ctx, std::string path)
{
    if (verbose_) {
        std::cout << "UDABackend deleteData\n";
    }

    std::stringstream ss;

    ss << plugin_
       << "::deleteData("
       << ", path='" << path << "'"
       << ")";

    std::string directive = ss.str();

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }
    try {
        uda_client_.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw ALException(ex.what(), LOG);
    }
}


bool UDABackend::supportsTimeDataInterpolation()
{
    if (verbose_) {
        std::cout << "UDABackend supportsTimeDataInterpolation\n";
    }

    std::string directive = plugin_ + "::version()";

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }
    try {
        const uda::Result& result = uda_client_.get(directive, "");
        if (result.errorCode() == 0 && result.uda_type() == UDA_TYPE_STRING) {
            const uda::Data* uda_data = result.data();
            const uda::String* uda_string = dynamic_cast<const uda::String*>(uda_data);
            if (uda_string == nullptr) {
                throw ALException("Invalid string data returned", LOG);
            }
            // Return true if plugin is new enough that initDataInterpolationComponent is handled.
            semver::version version = semver::version::parse(uda_string->str());
            return version > "1.4.0"_v;
        } else {
            throw ALException("Invalid version returned from plugin", LOG);
        }
    } catch (const uda::UDAException& ex) {
        throw ALException(ex.what(), LOG);
    } catch (const semver::semver_exception& ex) {
        throw ALException(ex.what(), LOG);
    }
}

namespace {

std::vector<int> read_occurrences(NodeReader *node) {
    const char *name = uda_capnp_read_name(node);
    if (std::string(name) != "occurrences") {
        throw imas::uda::CacheException("Invalid node: " + std::string(name));
    }

    std::vector <size_t> size(1);
    uda_capnp_read_shape(node, size.data());

    size_t count = std::accumulate(size.begin(), size.end(), 1, std::multiplies<size_t>());
    std::vector<int> occurrences(count);

    if (count != 0) {
        bool eos = uda_capnp_read_is_eos(node);
        if (!eos) {
            throw imas::uda::CacheException("UDA backend does not currently handle streamed data");
        }

        size_t num_slices = uda_capnp_read_num_slices(node);
        if (num_slices != 1) {
            throw imas::uda::CacheException("Incorrect number of slices for occurrences node");
        }

        size_t slice_size = uda_capnp_read_slice_size(node, 0);
        if (slice_size / sizeof(int) != count) {
            throw imas::uda::CacheException("Incorrect amount of data found in occurrences node slices");
        }

        auto buffer = reinterpret_cast<char *>(occurrences.data());
        uda_capnp_read_data(node, 0, buffer);
    }

    return occurrences;
}

} // anon namespace

void UDABackend::get_occurrences(const char* ids_name, int** occurrences_list, int* size) {
    if (verbose_) {
        std::cout << "UDABackend get_occurrences\n";
    }

    std::stringstream ss;

    ss << plugin_
       << "::getOccurrences("
       << ", ids='" << ids_name << "'"
       << ")";

    std::string directive = ss.str();

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }
    try {
        const uda::Result& result = uda_client_.get(directive, "");

        if (result.errorCode() == 0 && result.uda_type() == UDA_TYPE_CAPNP) {
            const char* data = result.raw_data();
            size_t sz = result.size();
            auto tree = uda_capnp_deserialise(data, sz);
            auto root = uda_capnp_read_root(tree);

            auto occurrences = read_occurrences(root);
            *size = occurrences.size();
            *occurrences_list = reinterpret_cast<int*>(malloc(sizeof(int) * occurrences.size()));
            std::copy(occurrences.begin(), occurrences.end(), *occurrences_list);
        }
    } catch (const uda::UDAException& ex) {
        throw ALException(ex.what(), LOG);
    }
}

