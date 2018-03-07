#include "uda_backend.h"

#include <cstdarg>

std::string UDABackend::format(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    size_t len = static_cast<size_t>(vsnprintf(NULL, 0, fmt, args));

    char* str = (char*)malloc(len + 1);

    vsnprintf(str, len, fmt, args);
    str[len] = '\0';

    std::string result(str);
    free(str);

    va_end(args);

    return result;
}

void UDABackend::openPulse(PulseContext *ctx,
               int mode,
               std::string options) 
{
    if (verbose) {
        std::cout << "UDABackend openPulse\n";
    }

    std::string directive;

    switch (mode) {
        case ualconst::open_pulse:
        case ualconst::force_open_pulse:
            directive = format("imas::open(file='%s', shot=%d, run=%d)",
                               ctx->getBackendName().c_str(), ctx->getShot(), ctx->getRun());
            break;
        case ualconst::create_pulse:
        case ualconst::force_create_pulse:
            directive = format("imas::create(file='%s', shot=%d, run=%d, refShot=%d, refRun=%d, /CreateFromModel)",
                               ctx->getBackendName().c_str(), ctx->getShot(), ctx->getRun(), 0, 0);
            break;
        default:
            throw UALBackendException("Mode not yet supported", LOG);
    }

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }
    uda_client.get(directive, "");
}

void UDABackend::closePulse(PulseContext *ctx,
                int mode,
                std::string options) 
{
    if (verbose) {
        std::cout << "UDABackend closePulse\n";
    }

    std::string directive = format("imas::close(idx=%d, file='%s', shot=%d, run=%d)",
                                   ctx->getBackendID(), ctx->getBackendName().c_str(), ctx->getShot(), ctx->getRun());

    if (verbose) {
        std::cout << "UDA directive: " << directive << "\n";
    }
    uda_client.get(directive, "");
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
    OperationContext* opCtx = dynamic_cast<OperationContext*>(ctx);
    if (opCtx == NULL) {
        throw UALNoDataException("no operation context", LOG);
    }

    try {
        std::string directive = format("imas::get(expName='%s', idx=%d, group='%s', variable='%s', shot=%d, %s)",
                                       opCtx->getTokamak().c_str(), opCtx->getBackendID(), opCtx->getType(),
                                       fieldname.c_str(), opCtx->getShot());
        const uda::Result& result = uda_client.get(directive, "");
        uda::Data* uda_data = result.data();
        if (uda_data->type() == typeid(double)) {
            *datatype = DOUBLE_DATA;
        } else if (uda_data->type() == typeid(int)) {
            *datatype = INTEGER_DATA;
        }
        std::vector<size_t> shape = result.shape();
        *dim = (int)shape.size();
    } catch (const uda::UDAException& ex) {
        throw UALNoDataException(ex.what(), LOG);
    }

    if (fieldname == "path/to/double") {
        **(double**)data = 123.45;
        *datatype = DOUBLE_DATA;
        *dim = 0;
        size[0] = 0; // size is optional with scalar
    } else if (fieldname == "path/to/int") {
        **(int**)data = 42;
        *datatype = INTEGER_DATA;
        *dim = 0;
        size[0] = 0; // size is optional with scalar
    } else {
        if (verbose) {
            std::cout << "UDABackend readData\n";
        }
        throw UALNoDataException("test recoverable exception", LOG);
    }
}
