#pragma once

// No gRPC or protobuf headers here.
// All gRPC types are hidden behind GRPCBackendImpl (defined in grpc_backend.cpp).
// This keeps the generated build-dir headers away from anything that includes
// this file (e.g. al_backend.cpp), which avoids include-path and
// dependency-ordering headaches.

#include "al_backend.h"
#include "al_exception.h"

#include <memory>
#include <string>

struct GRPCBackendImpl;  // defined in grpc_backend.cpp

class GRPCBackend : public Backend {
public:
    explicit GRPCBackend(const uri::Uri& uri);

    // Destructor must be defined in grpc_backend.cpp where GRPCBackendImpl
    // is a complete type (required by unique_ptr).
    ~GRPCBackend() override;

    GRPCBackend(const GRPCBackend&)            = delete;
    GRPCBackend& operator=(const GRPCBackend&) = delete;

    std::pair<int,int> getVersion(DataEntryContext* ctx) override;

    void openPulse(DataEntryContext* ctx, int mode) override;
    void closePulse(DataEntryContext* ctx, int mode) override;

    void beginAction(OperationContext* ctx) override;
    void endAction(Context* ctx) override;

    // I haven't tried this, only did a readData (pure virtual needed)
    void writeData(Context* ctx, std::string fieldname, std::string timebasename, void* data,
                   int datatype, int dim, int* size) override;

    int readData(Context* ctx, std::string fieldname, std::string timebasename, void** data,
                 int* datatype, int* dim, int* size) override;

    // I haven't tried this, only did a readData (pure virtual, needed)
    void deleteData(OperationContext* ctx, std::string path) override;

    void beginArraystructAction(ArraystructContext* ctx, int* size) override;

    void get_occurrences(Context* ctx, const char* ids_name, int** occurrences_list, int* size) override;

    void list_filled_paths(Context* ctx, const char* dataobjectname, char*** path_list, int* size) override;

    // Not yet implemented or tested - to do later after POC
    bool supportsTimeDataInterpolation() override { return false; }
    void initDataInterpolationComponent() override {}
    bool supportsTimeRangeOperation()     override { return false; }

private:
    std::unique_ptr<GRPCBackendImpl> impl_;
};
