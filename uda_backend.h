#ifndef UDA_BACKEND_H
#define UDA_BACKEND_H 1

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <mdsobjects.h>
#include <c++/UDA.hpp>

#include "dummy_backend.h"
#include "ual_backend.h"
#include "ual_const.h"
#include "ual_context.h"
#include "ual_defs.h"

#define NODENAME_MANGLING  //Use IMAS mangling

#if defined(_WIN32)
#  define LIBRARY_API __declspec(dllexport)
#else
#  define LIBRARY_API
#endif

#ifdef __cplusplus
extern "C" {
#endif


class LIBRARY_API UDABackend : public Backend
{
private:
    bool verbose = false;
    std::string plugin = "IMAS_MAPPING";
    uda::Client uda_client;
    int ctx_id = -1;

public:

    explicit UDABackend(bool verb=false) : verbose(verb)
    {
        const char* env = getenv("UDA_PLUGIN");
        if (env != nullptr) {
            plugin = env;
        }

        if (verbose) {
            std::cout << "UDABackend constructor\n";
            std::cout << "UDA Server: " << uda_client.serverHostName() << "\n";
            std::cout << "UDA Port: " << uda_client.serverPort() << "\n";
            std::cout << "UDA Plugin: " << plugin << "\n";
        }
    }

    ~UDABackend() override
    {
        if (verbose) {
            std::cout << "UDABackend destructor\n";
        }
    }

    void openPulse(PulseContext *ctx, int mode, std::string options) override;

    void closePulse(PulseContext *ctx, int mode, std::string options) override;

    void beginAction(OperationContext *ctx) override;

    void endAction(Context *ctx) override;

    void writeData(Context *ctx,
                   std::string fieldname,
                   std::string timebasename,
                   void* data,
                   int datatype,
                   int dim,
                   int* size) override;

    int readData(Context *ctx,
                  std::string fieldname,
                  std::string timebasename,
                  void** data,
                  int* datatype,
                  int* dim,
                  int* size) override;

    void deleteData(OperationContext *ctx,
                    std::string path) override;

    void beginArraystructAction(ArraystructContext* ctx, int* size) override;

};

#ifdef __cplusplus
}
#endif

#endif // UDA_BACKEND_H

