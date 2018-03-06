#ifndef UDA_BACKEND_H
#define UDA_BACKEND_H

#include <string>
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>
#include <mdsobjects.h>
#include <c++/UDA.hpp>

#include "ual_backend.h"
#include "ual_context.h"
#include "ual_defs.h"
#include "ual_const.h"
#include "dummy_backend.h"
#include "ual_defs.h"

#define NODENAME_MANGLING  //Use IMAS mangling

class UDABackend : public Backend {
private:
    bool verbose = false;
    uda::Client uda_client;

    std::string format(const char* fmt, ...);

public:

    explicit UDABackend(bool verb=false) : verbose(verb)
    {
        if (verbose) {
            std::cout << "UDABackend constructor\n";
            std::cout << "UDA Server: " << uda_client.serverHostName() << "\n";
            std::cout << "UDA Port: " << uda_client.serverPort() << "\n";
        }
    }

    ~UDABackend() override
    {
        if (verbose) {
            std::cout << "UDABackend destructor\n";
        }
    }

    void openPulse(PulseContext *ctx,
                   int mode,
                   std::string options) override
    {
        if (verbose) {
            std::cout << "UDABackend openPulse\n";
        }

        std::string directive;

        switch (mode) {
            case ualconst::open_pulse:
            case ualconst::force_open_pulse:
                directive = format("imas::open(file='%s', shot=%d, run=%d)",
                                   ctx->getBackendName(), ctx->getShot(), ctx->getRun());
                break;
            case ualconst::create_pulse:
            case ualconst::force_create_pulse:
                directive = format("imas::create(file='%s', shot=%d, run=%d, refShot=%d, refRun=%d, /CreateFromModel)",
                                   ctx->getBackendName(), ctx->getShot(), ctx->getRun(), 0, 0);
                break;
            default:
                throw UALBackendException("Mode not yet supported", LOG);
        }

        if (verbose) {
            std::cout << "UDA directive: " << directive << "\n";
        }
        uda_client.get(directive, "");
    }

    void closePulse(PulseContext *ctx,
                    int mode,
                    std::string options) override
    {
        if (verbose) {
            std::cout << "UDABackend closePulse\n";
        }

        std::string directive = format("imas::close(idx=%d, file='%s', shot=%d, run=%d)",
                                       ctx->getBackendID(), ctx->getBackendName(), ctx->getShot(), ctx->getRun());

        if (verbose) {
            std::cout << "UDA directive: " << directive << "\n";
        }
        uda_client.get(directive, "");
    }

    void beginAction(OperationContext *ctx) override
    {
        if (verbose) {
            std::cout << "UDABackend beginAction\n";
        }
        throw UALBackendException("Not implemented", LOG);
    }

    void endAction(Context *ctx) override
    {
        if (verbose) {
            std::cout << "UDABackend endAction\n";
        }
        throw UALBackendException("Not implemented", LOG);
    }

    void writeData(Context *ctx,
                   std::string fieldname,
                   std::string timebasename,
                   void* data,
                   int datatype,
                   int dim,
                   int* size) override
    {
        if (verbose) {
            std::cout << "UDABackend writeData\n";
        }
        throw UALBackendException("Not implemented", LOG);
    }

    void readData(Context *ctx,
                  std::string fieldname,
                  std::string timebasename,
                  void** data,
                  int* datatype,
                  int* dim,
                  int* size) override
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
                                           opCtx->getTokamak(), opCtx->getBackendID(), opCtx->getType(), fieldname, opCtx->getShot());
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

    void deleteData(OperationContext *ctx,
                    std::string path) override
    {
        if (verbose) {
            std::cout << "UDABackend deleteData\n";
        }
        throw UALBackendException("Not implemented", LOG);
    }

    void beginArraystructAction(ArraystructContext *ctx,
                                int *size) override
    {
        if (verbose) {
            std::cout << "UDABackend beginArraystructAction\n";
        }
        throw UALBackendException("Not implemented", LOG);
    }

};

#endif // UDA_BACKEND_H
