#include "uda_backend.h"

#include <sstream>

namespace {

const char* type_to_string(int datatype)
{
    switch (datatype) {
        case CHAR_DATA: return "char";
        case INTEGER_DATA: return "integer";
        case DOUBLE_DATA: return "double";
        case COMPLEX_DATA: return "complex";
        default:
            throw UALBackendException("Unknown datatype " + std::to_string(datatype), LOG);
    }
}

std::string array_path(ArraystructContext* ctx, bool for_dim=false)
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

void UDABackend::openPulse(PulseContext *ctx,
               int mode,
               std::string options) 
{
    if (verbose) {
        std::cout << "UDABackend openPulse\n";
    }

    std::stringstream ss;
    std::string directive;

    switch (mode) {
        case ualconst::open_pulse:
        case ualconst::force_open_pulse:
            ss << this->plugin << "::open(shot=" << ctx->getShot() << ", run=" << ctx->getRun() << ")";
            directive = ss.str();
            break;
        case ualconst::create_pulse:
        case ualconst::force_create_pulse:
            throw UALBackendException("Cannot use create mode with public data", LOG);
//            directive = format(
//                "%s::create(file='%s', shot=%d, run=%d, refShot=%d, refRun=%d, /CreateFromModel)",
//                this->plugin,
//                ctx->getBackendName().c_str(),
//                ctx->getShot(),
//                ctx->getRun(),
//                0,
//                0
//            );
            break;
        default:
            throw UALBackendException("Mode not yet supported", LOG);
    }

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }
    try {
        uda_client.get(directive, "");
    } catch (const uda::UDAException& ex) {
        throw UALException(ex.what(), LOG);
    }
}

void UDABackend::closePulse(PulseContext *ctx,
                int mode,
                std::string options) 
{
    if (verbose) {
        std::cout << "UDABackend closePulse\n";
    }

    std::stringstream ss;
    ss << this->plugin << "::close(shot=" << ctx->getShot() << ", run=" << ctx->getRun() << ")";

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

void UDABackend::readData(Context *ctx,
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
    auto opCtx = dynamic_cast<OperationContext*>(ctx);
    if (opCtx == nullptr) {
        throw UALNoDataException("no operation context", LOG);
    }

    try {
        auto arrCtx = dynamic_cast<ArraystructContext*>(ctx);
        std::string variable = fieldname;
        if (arrCtx != nullptr) {
            variable = array_path(arrCtx) + "/" + fieldname;
        }

        std::string group = opCtx->getDataobjectName();
        int occurrence = 0;

        std::size_t slash_pos = group.find('/');
        if (slash_pos != std::string::npos) {
            occurrence = (int)strtol(&group[slash_pos + 1], nullptr, 10);
            group.resize(slash_pos);
        }

<<<<<<< HEAD
        /*std::string directive = format(
            "%s::get(expName='%s', group='%s', occurrence=%d, type='%s', variable='%s', timebase='%s', shot=%d, run=%d, user='%s')",
            this->plugin,
            opCtx->getTokamak().c_str(),
            group.c_str(),
            occurrence,
            type_to_string(*datatype),
            variable.c_str(),
            timebasename.c_str(),
            opCtx->getShot(),
            opCtx->getRun(),
            opCtx->getUser().c_str()
        );*/
        char directive_buffer [300];
        	sprintf(directive_buffer, "%s::get(expName='%s', group='%s', occurrence=%d, type='%s', variable='%s', timebase='%s', shot=%d, run=%d, user='%s')", 
        	    this->plugin,
                opCtx->getTokamak().c_str(),
                group.c_str(),
                occurrence,
                type_to_string(*datatype),
                variable.c_str(),
                timebasename.c_str(),
                opCtx->getShot(),
                opCtx->getRun(),
                opCtx->getUser().c_str());
        	    std::string directive = std::string(directive_buffer);
        
=======
        std::stringstream ss;
        ss << this->plugin
           << "::get(expName='" << opCtx->getTokamak()
           << "', group='" << group
           << "', occurrence=" << occurrence
           << ", type='" << type_to_string(*datatype)
           << "', variable='" << variable
           << "', timebase='" << timebasename
           << "', shot=" << opCtx->getShot()
           << ", run=" << opCtx->getRun()
           << ", user='" << opCtx->getUser() << "')";

        std::string directive = ss.str();
>>>>>>> 5eb50fef8c4d60663189e238895c8581adf72f42
                                  
        std::cout << "UDA directive: " << directive << "\n";
        const uda::Result& result = uda_client.get(directive, "");
        uda::Data* uda_data = result.data();
        if (uda_data->type() == typeid(double)) {
            *datatype = DOUBLE_DATA;
        } else if (uda_data->type() == typeid(int)) {
            *datatype = INTEGER_DATA;
        } else {
            throw UALBackendException(std::string("Unknown data type returned: ") + uda_data->type().name(), LOG);
        }
        std::vector<size_t> shape = result.shape();
        *dim = static_cast<int>(shape.size());
        for (int i = 0; i < *dim; ++i) {
            size[i] = static_cast<int>(shape[i]);
        }
        *data = malloc(uda_data->byte_length());
        memcpy(*data, uda_data->byte_data(), uda_data->byte_length());
    } catch (const uda::UDAException& ex) {
        throw UALNoDataException(ex.what(), LOG);
    }
}

void UDABackend::beginArraystructAction(ArraystructContext* ctx, int* size)
{
    if (verbose) {
        std::cout << "UDABackend beginArraystructAction\n";
    }

    try {
        std::string path = array_path(ctx, true);

        std::stringstream ss;
        ss << this->plugin
           << "::getdim(expName='" << ctx->getTokamak()
           << "', ctx=" << ctx->getUid()
           << ", group='" << ctx->getDataobjectName()
           << "', path='" << path
           << "', shot=" << ctx->getShot()
           << ", run=" << ctx->getRun()
           << ", user='" << ctx->getUser() << "')";

        std::string directive = ss.str();

        std::cout << "UDA directive: " << directive << "\n";
        const uda::Result& result = uda_client.get(directive, "");
        uda::Data* uda_data = result.data();
        if (uda_data->type() != typeid(int)) {
            throw UALBackendException(std::string("Invalid data type returned for getdim: ") + uda_data->type().name(), LOG);
        }
        *size = *reinterpret_cast<const int*>(uda_data->byte_data());
    } catch (const uda::UDAException& ex) {
        throw UALNoDataException(ex.what(), LOG);
    }
}
