#include "uda_backend.h"

#include <cstdlib>

namespace {

std::string array_path(ArraystructContext* ctx, bool for_dim = false)
{
    std::string path;
    if (for_dim) {
        path = ctx->getPath();
    } else {
        path = ctx->getPath() + "/" + std::to_string(ctx->getIndex() + 1);
    }
    while (ctx->getParent() != nullptr) {
        ctx = ctx->getParent();
        path = ctx->getPath()
                .append("/")
                .append(std::to_string(ctx->getIndex() + 1))
                .append("/")
                .append(path);
    }
    return path;
}

}

std::pair<int,int> UDABackend::getVersion(PulseContext *ctx)
{
  std::pair<int,int> version;
  if(ctx==NULL)
    version = {UDA_BACKEND_VERSION_MAJOR, UDA_BACKEND_VERSION_MINOR};
  else
    {
      version = {0,0}; // temporary placeholder
    }
  return version;
}

void UDABackend::openPulse(PulseContext* ctx,
                           int mode,
                           std::string options)
{
    if (verbose) {
        std::cout << "UDABackend openPulse\n";
    }

    std::string mapped_plugin = machine_mapping.plugin(ctx->getTokamak());
    if (!mapped_plugin.empty()) {
        this->plugin = mapped_plugin;
        std::stringstream ss;

        ss << plugin << "::init(";

        auto args = machine_mapping.args(ctx->getTokamak());
        std::string delim;
        for (const auto& arg : args) {
            ss << delim << arg;
            delim = ", ";
        }

        ss << ")";

        std::string directive = ss.str();
        if (verbose) {
            std::cout << "UDA directive: " << directive << "\n";
        }

        try {
            uda_client.get(directive, "");
        } catch (const uda::UDAException& ex) {
            if (verbose) {
                std::cout << "UDA error: " << ex.what() << "\n";
            }
        }
    }

    std::stringstream ss;

    ss << this->plugin
       << "::openPulse("
       << "backend_id=" << ualconst::mdsplus_backend
       << ", shot=" << ctx->getShot()
       << ", run=" << ctx->getRun()
       << ", user=" << ctx->getUser()
       << ", tokamak=" << ctx->getTokamak()
       << ", version=" << ctx->getVersion()
       << ", mode=" << mode
       << ", options='" << options << "'"
       << ")";

    std::string directive = ss.str();

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }

    try {
        const uda::Result& result = uda_client.get(directive, "");
        uda::Data* data = result.data();
        if (data->type() != typeid(int)) {
            throw UALBackendException(std::string("Unknown data type returned: ") + data->type().name(), LOG);
        }
        auto scalar = dynamic_cast<uda::Scalar*>(data);
        if (scalar == nullptr) {
            throw UALBackendException("UDA openPulse did not return scalar data");
        }
        ctx_id = scalar->as<int>();
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
    }
}

void UDABackend::closePulse(PulseContext* ctx,
                            int mode,
                            std::string options)
{
    if (verbose) {
        std::cout << "UDABackend closePulse\n";
    }

    if (ctx_id == -1) {
        throw UALException("invalid ctx_id", LOG);
    }

    std::stringstream ss;

    ss << this->plugin
       << "::closePulse("
       << "ctxId=" << ctx_id
       << ", mode=" << mode
       << ", options='" << options << "'"
       << ")";

    std::string directive = ss.str();

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }
    try {
        uda_client.get(directive, "");
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
    if (verbose) {
        std::cout << "UDABackend readData\n";
    }

    if (ctx_id == -1) {
        throw UALException("invalid ctx_id", LOG);
    }

    try {
        auto arrCtx = dynamic_cast<ArraystructContext*>(ctx);

        std::stringstream ss;
        ss << this->plugin
           << "::readData("
           << "ctxId=" << ctx_id
           << ", field='" << fieldname << "'"
           << ", timebase='" << timebasename << "'"
           << ", datatype=" << *datatype;
        if (arrCtx != nullptr) {
            ss << ", index=" << arrCtx->getIndex() + 1;
        }
        ss << ")";

        std::string directive = ss.str();

        if (verbose) {
            std::cout << "UDA directive: " << directive << "\n";
        }

        const uda::Result& result = uda_client.get(directive, "");
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
    } catch (const uda::UDAException& ex) {
        return 0;
    }
    return 1;
}

void UDABackend::beginArraystructAction(ArraystructContext* ctx, int* size)
{
    if (verbose) {
        std::cout << "UDABackend beginArraystructAction\n";
    }

    if (ctx_id == -1) {
        throw UALException("invalid ctx_id", LOG);
    }

    try {
        std::string path = array_path(ctx, true);
	OperationContext* octx = ctx->getOperationContext();
        std::stringstream ss;
        ss << this->plugin
           << "::beginArraystructAction("
           << "ctxId=" << ctx_id
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

        std::cout << "UDA directive: " << directive << "\n";
        const uda::Result& result = uda_client.get(directive, "");
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
    } catch (const uda::UDAException& ex) {
        *size = 0;
        return;
    }
}

void UDABackend::beginAction(OperationContext* ctx)
{
    if (verbose) {
        std::cout << "UDABackend beginAction\n";
    }

    if (ctx_id == -1) {
        throw UALException("invalid ctx_id", LOG);
    }

    std::stringstream ss;

    ss << this->plugin
       << "::beginAction("
       << "ctxId=" << ctx_id
       << ", dataObject='" << ctx->getDataobjectName() << "'"
       << ", access=" << ctx->getAccessmode()
       << ", range=" << ctx->getRangemode()
       << ", time=" << ctx->getTime()
       << ", interp=" << ctx->getInterpmode();

    char* ppf_user = getenv("UDA_PPF_USER");
    if (ppf_user != nullptr) {
        ss << ", ppfUser='" << ppf_user << "'";
    }
    char* ppf_sequence = getenv("UDA_PPF_SEQUENCE");
    if (ppf_sequence != nullptr) {
        ss << ", ppfSequence=" << ppf_sequence;
    }
    char* ppf_dda = getenv("UDA_PPF_DDA");
    if (ppf_dda != nullptr) {
        ss << ", ppfDDA='" << ppf_dda << "'";
    }
    
    ss << ")";
    std::string directive = ss.str();

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }
    try {
        uda_client.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
    }
}

void UDABackend::endAction(Context* ctx)
{
    if (verbose) {
        std::cout << "UDABackend endAction\n";
    }

    if (ctx_id == -1) {
        throw UALException("invalid ctx_id", LOG);
    }

    std::stringstream ss;

    ss << this->plugin
       << "::endAction("
       << "ctxId=" << ctx_id
       << ", type=" << ctx->getType()
       << ")";

    std::string directive = ss.str();

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }
    try {
        uda_client.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
    }
}

void
UDABackend::writeData(Context* ctx, std::string fieldname, std::string timebasename, void* data, int datatype, int dim,
                      int* size)
{
    if (verbose) {
        std::cout << "UDABackend writeData\n";
    }

    if (ctx_id == -1) {
        throw UALException("invalid ctx_id", LOG);
    }

    std::stringstream ss;

    ss << this->plugin
       << "::writeData("
       << "ctxId=" << ctx_id
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

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }

    int dummy[] = { 0 };
    uda::Array array{ dummy, {}};

    try {
        switch (datatype) {
            case CHAR_DATA:
                array = uda::Array{ reinterpret_cast<char*>(data), dims };
                break;
            case INTEGER_DATA:
                array = uda::Array{ reinterpret_cast<int*>(data), dims };
                break;
            case DOUBLE_DATA:
                array = uda::Array{ reinterpret_cast<double*>(data), dims };
                break;
            default:
                throw UALException("unknown datatype", LOG);
        }
        uda_client.put(directive, array);
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
    }
}

void UDABackend::deleteData(OperationContext* ctx, std::string path)
{
    if (verbose) {
        std::cout << "UDABackend deleteData\n";
    }

    if (ctx_id == -1) {
        throw UALException("invalid ctx_id", LOG);
    }

    std::stringstream ss;

    ss << this->plugin
       << "::deleteData("
       << "ctxId=" << ctx_id
       << ", path='" << path << "'"
       << ")";

    std::string directive = ss.str();

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }
    try {
        uda_client.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
    }
}

