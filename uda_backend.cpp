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

std::pair<int, int> UDABackend::getVersion(PulseContext* ctx)
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
    } else if (boost::starts_with(name, "uda_")) {
        env_options_[name] = value;
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

void UDABackend::load_env_options()
{
    constexpr const char* option_names[] = {"UDA_PPF_USER", "UDA_PPF_SEQUENCE", "UDA_PPF_DDA"};

    for (auto name : option_names) {
        char* value = getenv(name);
        if (value != nullptr) {
            std::string lower_name = boost::to_lower_copy(std::string{name});
            env_options_[lower_name] = value;
        }
    }
}

void UDABackend::openPulse(PulseContext* ctx,
                           int mode,
                           std::string options)
{
    if (verbose_) {
        std::cout << "UDABackend openPulse\n";
    }

    load_env_options();
    process_options(options);

    std::vector<std::string> init_args = {};
    std::string mapped_plugin = machine_mapping_.plugin(ctx->getTokamak());
    if (!mapped_plugin.empty()) {
        plugin_ = mapped_plugin;
        init_args = machine_mapping_.args(ctx->getTokamak());
    }

    std::stringstream ss;
    ss << plugin_ << "::init(";

    std::string delim;
    for (const auto& arg: init_args) {
        ss << delim << arg;
        delim = ", ";
    }

    ss << ")";

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

    std::string backend = env_options_.count("uda_backend") ? env_options_.at("uda_backend") : "mdsplus";

    ss = std::stringstream();
    ss << plugin_
       << "::open(backend='"<< backend << "'"
       << ", shot=" << ctx->getShot()
       << ", run=" << ctx->getRun()
       << ", user='" << ctx->getUser() << "'"
       << ", tokamak='" << ctx->getTokamak() << "'"
       << ", version='" << ctx->getVersion() << "'"
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

void UDABackend::closePulse(PulseContext* ctx,
                            int mode,
                            std::string options)
{
    if (verbose_) {
        std::cout << "UDABackend closePulse\n";
    }

    cache_.clear();
    cache_mode_ = imas::uda::CacheMode::None;

    std::stringstream ss;

    ss << plugin_
       << "::close("
       << "shot=" << ctx->getShot()
       << ", run=" << ctx->getRun()
       << ", user='" << ctx->getUser() << "'"
       << ", tokamak='" << ctx->getTokamak() << "'"
       << ", version='" << ctx->getVersion() << "'"
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
    idamFreeAll();
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
        switch (*datatype) {
            case INTEGER_DATA: {
                auto& vec = boost::get<std::vector<int>>(cache_data.values);
                *data = vec.data();
                break;
            }
            case DOUBLE_DATA: {
                auto& vec = boost::get<std::vector<double>>(cache_data.values);
                *data = vec.data();
                break;
            }
            case CHAR_DATA: {
                auto& vec = boost::get<std::vector<char>>(cache_data.values);
                *data = vec.data();
                break;
            }
        }
        *dim = cache_data.shape.size();
        std::copy(cache_data.shape.begin(), cache_data.shape.end(), size);
        return 1;
    } else if (cache_mode_ == imas::uda::CacheMode::None) {
        try {
            auto arr_ctx = dynamic_cast<ArraystructContext*>(ctx);
            auto op_ctx = arr_ctx != nullptr ? arr_ctx->getOperationContext() : dynamic_cast<OperationContext*>(ctx);
            auto pulse_ctx = op_ctx->getPulseContext();

            std::stringstream ss;

            ss << plugin_
               << "::get(backend_id='mdsplus'"
               << ", shot=" << pulse_ctx->getShot()
               << ", run=" << pulse_ctx->getRun()
               << ", user='" << pulse_ctx->getUser() << "'"
               << ", tokamak='" << pulse_ctx->getTokamak() << "'"
               << ", version='" << pulse_ctx->getVersion() << "'"
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

            if (env_options_.count("uda_ppf_user")) {
                ss << ", ppfUser='" << env_options_.at("uda_ppf_user") << "'";
            }
            if (env_options_.count("uda_ppf_sequence")) {
                ss << ", ppfSequence='" << env_options_.at("uda_ppf_sequence") << "'";
            }
            if (env_options_.count("uda_ppf_dda")) {
                ss << ", ppfDDA='" << env_options_.at("uda_ppf_dda") << "'";
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

void UDABackend::populate_cache(const std::string& ids, const std::string& path, PulseContext* pulse_ctx, OperationContext* op_ctx)
{
    bool is_homogeneous = true; // TODO: get this from UDA

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

        ss << plugin_
           << "::get(backend='mdsplus'"
           << ", shot=" << pulse_ctx->getShot()
           << ", run=" << pulse_ctx->getRun()
           << ", user='" << pulse_ctx->getUser() << "'"
           << ", tokamak='" << pulse_ctx->getTokamak() << "'"
           << ", version='" << pulse_ctx->getVersion() << "'"
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

        if (env_options_.count("uda_ppf_user")) {
            ss << ", ppfUser='" << env_options_.at("uda_ppf_user") << "'";
        }
        if (env_options_.count("uda_ppf_sequence")) {
            ss << ", ppfSequence='" << env_options_.at("uda_ppf_sequence") << "'";
        }
        if (env_options_.count("uda_ppf_dda")) {
            ss << ", ppfDDA='" << env_options_.at("uda_ppf_dda") << "'";
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

    constexpr size_t N = 50;
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

            for (int i = 0; i < acc_getCurrentDataBlockIndex(); ++i) {
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
    auto pulse_ctx = op_ctx->getPulseContext();

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
        populate_cache(ids, path, pulse_ctx, op_ctx);

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
    auto pulse_ctx = op_ctx->getPulseContext();

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
        populate_cache(ids, ids, pulse_ctx, op_ctx);
    } else {
        std::stringstream ss;
        ss << plugin_
           << "::beginAction("
           << ", dataObject='" << op_ctx->getDataobjectName() << "'"
           << ", access=" << op_ctx->getAccessmode()
           << ", range=" << op_ctx->getRangemode()
           << ", time=" << op_ctx->getTime()
           << ", interp=" << op_ctx->getInterpmode();

        if (env_options_.count("uda_ppf_user")) {
            ss << ", ppfUser='" << env_options_.at("uda_ppf_user") << "'";
        }
        if (env_options_.count("uda_ppf_sequence")) {
            ss << ", ppfSequence='" << env_options_.at("uda_ppf_sequence") << "'";
        }
        if (env_options_.count("uda_ppf_dda")) {
            ss << ", ppfDDA='" << env_options_.at("uda_ppf_dda") << "'";
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

