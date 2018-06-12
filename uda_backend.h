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
    const char* plugin = "IMAS_MAPPING";
    uda::Client uda_client;

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
                   std::string options) override;

    void closePulse(PulseContext *ctx,
                    int mode,
                    std::string options) override;

    void beginAction(OperationContext *ctx) override
    {
        if (verbose) {
            std::cout << "UDABackend beginAction\n";
        }
        //throw UALBackendException("Not implemented", LOG);
    }

    void endAction(Context *ctx) override
    {
        if (verbose) {
            std::cout << "UDABackend endAction\n";
        }
        //throw UALBackendException("Not implemented", LOG);
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
                  int* size) override;

    void deleteData(OperationContext *ctx,
                    std::string path) override
    {
        if (verbose) {
            std::cout << "UDABackend deleteData\n";
        }
        //throw UALBackendException("Not implemented", LOG);
    }

    void beginArraystructAction(ArraystructContext* ctx, int* size) override;

};

#endif // UDA_BACKEND_H
