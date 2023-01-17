#include "uda_backend.h"

#include <cstdlib>
#include <client/accAPI.h>
#include <client/udaClient.h>

#include "uda_utilities.hpp"
#include "uda_xml.hpp"
#include "uda_debug.hpp"

namespace {

std::string array_path(ArraystructContext* ctx, bool for_dim = false)
{
    std::string path;
    if (for_dim) {
        path = ctx->getPath();
    } else {
        path = ctx->getPath() + "[" + std::to_string(ctx->getIndex() + 1) + "]";
    }
    while (ctx->getParent() != nullptr) {
        ctx = ctx->getParent();
        path = ctx->getPath()
                .append("[")
                .append(std::to_string(ctx->getIndex() + 1))
                .append("]/")
                .append(path);
    }
    return path;
}

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

}

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

void UDABackend::process_option(const std::string& option)
{
    if (option.empty()) {
        return;
    }

    std::vector<std::string> tokens;
    boost::split(tokens, option, boost::is_any_of("="), boost::token_compress_on);

    if (tokens.size() != 2) {
        throw UALException("invalid option (option must be name=value)", LOG);
    }

    std::string name = tokens[0];
    std::string value = tokens[1];

    boost::to_lower(name);
    boost::to_lower(value);

    if (name == "cache_mode") {
        if (value == "none") {
            cache_mode_ = imas::uda::CacheMode::None;
        } else if (value == "ids") {
            cache_mode_ = imas::uda::CacheMode::IDS;
        } else if (value == "struct") {
            cache_mode_ = imas::uda::CacheMode::Struct;
        } else {
            throw UALException("invalid cache mode", LOG);
        }
    } else {
        throw UALException("invalid option (option name not recognised)", LOG);
    }
}

void UDABackend::process_options(const std::string& options)
{
    std::vector<std::string> tokens;
    boost::split(tokens, options, boost::is_any_of(","), boost::token_compress_on);

    for (const auto& token : tokens) {
        process_option(token);
    }
}

void UDABackend::openPulse(DataEntryContext* ctx,
                           int mode)
{
    if (verbose_) {
        std::cout << "UDABackend openPulse\n";
    }
    open_mode_ = mode;

    process_options(ctx->getOptions());

    auto maybe_plugin = ctx->getURI().query.get("plugin");
    if (maybe_plugin) {
        plugin_ = maybe_plugin.value();
    }

    std::string host = ctx->getURI().authority.host;
    int port = ctx->getURI().authority.port;

    uda::Client::setServerHostName(host);
    uda::Client::setServerPort(port);

    if (verbose_) {
        std::cout << "UDA server: " << host << "\n";
        std::cout << "UDA port: " << port << "\n";
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

    std::string backend = ctx->getURI().query.get("backend").value_or("mdsplus");
    // TODO:
    // - remove host and port from URI
    // - set path to backend
    // - remove backend from query

    ss << plugin_
       << "::openPulse("
       << "uri=" << ctx->getURI().to_string()
       << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::OpenMode>(mode) << "'"
       << ")";

    directive = ss.str();

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }

    try {
        uda_client_.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
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

    std::stringstream ss;
    // TODO:
    // - remove host and port from URI
    // - set path to backend

    ss << plugin_
       << "::close("
       << "uri='" << ctx->getURI().to_string() << "'"
       << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::CloseMode>(mode) << "'"
       << ")";

    std::string directive = ss.str();

    if (verbose_) {
        std::cout << "UDABackend request: " << directive << "\n";
    }
    try {
        uda_client_.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
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
    if (verbose_) {
        std::cout << "UDABackend readData\n";
    }

    auto path = array_path(ctx) + "/" + fieldname;

    if (cache_.count(path)) {
        if (verbose_) {
            std::cout << "UDABackend readData: found " << path << " in cache\n";
        }
        imas::uda::CacheData& cache_data = cache_.at(path);
        *dim = cache_data.shape.size();
        switch (*datatype) {
            case INTEGER_DATA: {
                auto& vec = boost::get<std::vector<int>>(cache_data.values);
                if (*dim == 0) {
                    int* d = (int*)malloc(sizeof(int));
                    *d = vec[0];
                    *data = d;
                } else {
                    *data = vec.data();
                }
                break;
            }
            case DOUBLE_DATA: {
                auto& vec = boost::get<std::vector<double>>(cache_data.values);
                if (*dim == 0) {
                    double* d = (double*)malloc(sizeof(double));
                    *d = vec[0];
                    *data = d;
                } else {
                    *data = vec.data();
                }
                break;
            }
            case CHAR_DATA: {
                auto& vec = boost::get<std::vector<char>>(cache_data.values);
                if (*dim == 0) {
                    char* d = (char*)malloc(sizeof(char));
                    *d = vec[0];
                    *data = d;
                } else {
                    *data = vec.data();
                }
                break;
            }
        }
        std::copy(cache_data.shape.begin(), cache_data.shape.end(), size);
        return 1;
    } else if (cache_mode_ == imas::uda::CacheMode::None) {
        try {
            auto arr_ctx = dynamic_cast<ArraystructContext*>(ctx);
            auto op_ctx = arr_ctx != nullptr ? arr_ctx->getOperationContext() : dynamic_cast<OperationContext*>(ctx);
            auto entry_ctx = op_ctx->getDataEntryContext();

            std::string backend = ctx->getURI().query.get("backend").value_or("mdsplus");

            std::stringstream ss;

            ss << plugin_
               << "::get(backend='" << backend << "'"
               << ", uri='" << entry_ctx->getURI().to_string() << "'"
               << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::OpenMode>(open_mode_) << "'"
               << ", dataObject='" << op_ctx->getDataobjectName() << "'"
               << ", access='" << imas::uda::convert_imas_to_uda<imas::uda::AccessMode>(op_ctx->getAccessmode()) << "'"
               << ", range='" << imas::uda::convert_imas_to_uda<imas::uda::RangeMode>(op_ctx->getRangemode()) << "'"
               << ", time=" << op_ctx->getTime()
               << ", interp='" << imas::uda::convert_imas_to_uda<imas::uda::InterpMode>(op_ctx->getInterpmode()) << "'"
               << ", path='" << fieldname
               << ", datatype='" << imas::uda::convert_imas_to_uda<imas::uda::DataType>(*datatype) << "'"
               << ", rank=" << *dim;
//               << ", is_homogeneous=" << is_homogeneous
//               << ", dynamic_flags=" << imas::uda::get_dynamic_flags(attributes, path);

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

            const uda::Result& result = uda_client_.get(directive, "");
            uda::Data* uda_data = result.data();
            if (uda_data->type() == typeid(double)) {
                *datatype = DOUBLE_DATA;
                *data = malloc(uda_data->byte_length());
                memcpy(*data, uda_data->byte_data(), uda_data->byte_length());
            } else if (uda_data->type() == typeid(int)) {
                *datatype = INTEGER_DATA;
                *data = malloc(uda_data->byte_length());
                memcpy(*data, uda_data->byte_data(), uda_data->byte_length());
            } else if (uda_data->type() == typeid(float)) {
                *datatype = DOUBLE_DATA;
                auto fdata = reinterpret_cast<const float*>(uda_data->byte_data());
                if (uda_data->size() == 0) {
                    *data = malloc(sizeof(double));
                    auto ddata = reinterpret_cast<double*>(*data);
                    *ddata = *fdata;
                } else {
                    *data = malloc(uda_data->size() * sizeof(double));
                    auto ddata = reinterpret_cast<double*>(*data);
                    for (int i = 0; i < (int)uda_data->size(); ++i) {
                        ddata[i] = fdata[i];
                    }
                }
            } else if (uda_data->type() == typeid(char*)) {
                *datatype = CHAR_DATA;
                *data = malloc(uda_data->byte_length() + 1);
                memcpy(*data, uda_data->byte_data(), uda_data->byte_length());
                char n = '\0';
                memcpy((char*)*data + uda_data->byte_length(), &n, sizeof(char));
            } else if (uda_data->type() == typeid(void)) {
                return 0;
            } else {
                throw UALBackendException(std::string("Unknown data type returned: ") + uda_data->type().name(), LOG);
            }

            std::vector<size_t> shape = result.shape();
            *dim = static_cast<int>(shape.size());
            for (int i = 0; i < *dim; ++i) {
                size[i] = static_cast<int>(shape[i]);
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

    std::string backend = entry_ctx->getURI().query.get("backend").value_or("mdsplus");
    std::string path = ids + "/ids_properties/homogeneous_time";

    std::stringstream ss;
    ss << plugin_
       << "::get(backend='"<< backend << "'"
       << ", uri='" << entry_ctx->getURI().to_string() << "'"
       << ", mode='" << imas::uda::convert_imas_to_uda<imas::uda::OpenMode>(open_mode_) << "'"
       << ", dataObject='" << ids << "'"
       << ", access='" << imas::uda::convert_imas_to_uda<imas::uda::AccessMode>(op_ctx->getAccessmode()) << "'"
       << ", range='" << imas::uda::convert_imas_to_uda<imas::uda::RangeMode>(op_ctx->getRangemode()) << "'"
       << ", time=-999"
       << ", interp='" << imas::uda::convert_imas_to_uda<imas::uda::InterpMode>(op_ctx->getInterpmode()) << "'"
       << ", path='" << path << "'"
       << ", datatype='integer'"
       << ", rank=0"
       << ", is_homogeneous=0"
       << ", dynamic_flags=0)";

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
        throw UALBackendException(std::string("Invalid result for ids_properties/homogeneous_time: ") + uda_data->type().name(), LOG);
    }
}

void UDABackend::populate_cache(const std::string& ids, const std::string& path, DataEntryContext* entry_ctx, OperationContext* op_ctx)
{
    bool is_homogeneous = get_homogeneous_flag(ids, entry_ctx, op_ctx);

    auto nodes = doc_->child("IDSs");

    std::vector<std::string> size_requests;
    std::vector<std::string> ids_paths = imas::uda::generate_ids_paths(path, nodes, size_requests);

    std::vector<std::string> requests;
    imas::uda::AttributeMap attributes;

    for (const auto& ids_path: ids_paths) {
        imas::uda::get_requests(requests, attributes, ids_path, nodes);
    }

    auto node = imas::uda::find_node(doc_->document_element(), ids, true);
    imas::uda::get_attributes(attributes, ids, node);

    std::vector<std::string> uda_requests = {};

    for (const auto& request: requests) {
        auto attr = attributes.at(request);

        std::stringstream ss;

        std::string data_type = imas::uda::convert_imas_to_uda<imas::uda::DataType>(attr.data_type);
        std::string backend = entry_ctx->getURI().query.get("backend").value_or("mdsplus");

        ss << plugin_
           << "::get(backend='"<< backend << "'"
           << ", uri='" << entry_ctx->getURI().to_string() << "'"
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
           << ", dynamic_flags=" << imas::uda::get_dynamic_flags(attributes, request);

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
    }

    std::cout << "UDABackend cache number of requests: " << uda_requests.size() << "\n";

    constexpr size_t N = 20;
    std::string backend = entry_ctx->getURI().query.get("backend").value_or("mdsplus");
    try {
        size_t n = 0;
        while (n < uda_requests.size()) {
            size_t m = std::min(n + N, uda_requests.size());
            std::vector<std::string> reqs = std::vector<std::string>{ uda_requests.begin() + n, uda_requests.begin() + m };

            std::cout << "UDABackend cache get: " << n << " - " << m << std::endl;
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
        throw UALException(ex.what(), LOG);
    }

    std::cout << cache_ << std::endl;
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
            throw UALException(ex.what(), LOG);
        } catch (std::out_of_range& ex) {
            throw UALException(ex.what(), LOG);
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
            throw UALBackendException(
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

    if (cache_.count(ids)) {
        if (verbose_) {
            std::cout << "UDABackend found value in cache\n";
        }
        return;
    } else if (cache_mode_ == imas::uda::CacheMode::IDS) {
        if (verbose_) {
            std::cout << "UDABackend populating cache\n";
        }

        cache_.clear();
        std::string path = op_ctx->getDatapath();
        if (path.empty()) {
            path = ids;
        } else {
            path = ids + "/" + path;
        }

        populate_cache(ids, path, entry_ctx, op_ctx);
        cache_[ids] = { {}, {} };
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
            throw UALException(ex.what(), LOG);
        }
    }
}

void UDABackend::endAction(Context* ctx)
{
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
//        throw UALException(ex.what(), LOG);
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
                throw UALException("unknown datatype", LOG);
        }
        uda_client_.put(directive, array);
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
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
        throw UALException(ex.what(), LOG);
    }
}

