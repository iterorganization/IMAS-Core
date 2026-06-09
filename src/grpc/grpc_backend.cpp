#include "grpc_backend.h"

// Generated headers only needed here, not in grpc_backend.h.
#include "imas.grpc.pb.h"
#include "imas.pb.h"

#include "al_const.h"
#include "al_defs.h"
#include "al_exception.h"

#include <grpcpp/grpcpp.h>

#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>

// Impl — all gRPC types live here, invisible to anyone including the header

struct GRPCBackendImpl
{
    std::shared_ptr<grpc::Channel>           channel;
    std::unique_ptr<imas_remote::ImasRemote::Stub> stub;

    // Returned by OpenPulse; passed in every subsequent RPC.
    std::string session_id;

    // Maps local Context::getUid() → server-side operation_id / arraystruct_id.
    std::unordered_map<unsigned long int, std::string> ctx_ids;
};

GRPCBackend::GRPCBackend(const uri::Uri& uri) : impl_(std::make_unique<GRPCBackendImpl>())
{
    const std::string host = uri.authority.host;
    const long        port = uri.authority.port;

    if (host.empty()) {
        throw ALBackendException("GRPCBackend: no host in URI", LOG);
    }
    if (port <= 0) {
        throw ALBackendException("GRPCBackend: no port in URI", LOG);
    }

    const std::string target = host + ":" + std::to_string(port);

    // change this madness
    const bool use_tls = uri.query.get("tls").value_or("false") == "true";

    std::shared_ptr<grpc::ChannelCredentials> creds = use_tls ? grpc::SslCredentials(grpc::SslCredentialsOptions())
                                                              : grpc::InsecureChannelCredentials();

    grpc::ChannelArguments args;
    args.SetCompressionAlgorithm(GRPC_COMPRESS_GZIP);

    impl_->channel = grpc::CreateCustomChannel(target, creds, args);
    impl_->stub    = imas_remote::ImasRemote::NewStub(impl_->channel);
}

// Must be in the .cpp so unique_ptr can see the complete GRPCBackendImpl type.
GRPCBackend::~GRPCBackend() = default;

static void checkRpcStatus(const grpc::Status& status, const std::string& op)
{
    if (!status.ok()) {
        throw ALBackendException("GRPCBackend::" + op + " RPC failed: " + status.error_message(), LOG);
    }
}

static const std::string& lookupContextId(const std::unordered_map<unsigned long int, std::string>& ctx_ids,
                                          Context* ctx) {
    auto it = ctx_ids.find(ctx->getUid());
    if (it == ctx_ids.end()) {
        throw ALBackendException("GRPCBackend: no server context for local uid "
                                 + std::to_string(ctx->getUid())
                                 + " — was beginAction/beginArraystructAction called?", LOG);
    }
    return it->second;
}

static size_t byteCount(int datatype, int dim, const int* sizes)
{
    size_t elem_bytes = 0;
    switch (datatype) {
        case CHAR_DATA: 
            elem_bytes = sizeof(char);
            break;
        case INTEGER_DATA:
            elem_bytes = sizeof(int); 
            break;
        case DOUBLE_DATA:
            elem_bytes = sizeof(double);
            break;
        case COMPLEX_DATA:
            elem_bytes = 2*sizeof(double);
            break;
        default:
            throw ALBackendException("GRPCBackend::byteCount: unknown datatype " + std::to_string(datatype), LOG);
    }

    if (dim == 0) {
        return elem_bytes;
    }

    size_t n = 1;
    for (int i = 0; i < dim; ++i) {
        n *= static_cast<size_t>(sizes[i]);
    }
    return n * elem_bytes;
}

std::pair<int,int> GRPCBackend::getVersion(DataEntryContext* ctx)
{
    imas_remote::GetVersionRequest req;
    if (ctx != nullptr) {
        req.set_session_id(impl_->session_id);
    }

    imas_remote::GetVersionResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->GetVersion(&grpc_ctx, req, &resp), "getVersion");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::getVersion: " + resp.error_message(), LOG);
    }

    return {resp.major(), resp.minor()};
}

void GRPCBackend::openPulse(DataEntryContext* ctx, int mode)
{
    imas_remote::OpenPulseRequest req;
    req.set_uri(ctx->getURI().to_string());
    req.set_mode(mode);

    imas_remote::OpenPulseResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->OpenPulse(&grpc_ctx, req, &resp), "openPulse");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::openPulse: " + resp.error_message(), LOG);
    }

    impl_->session_id = resp.session_id();
}

void GRPCBackend::closePulse(DataEntryContext* /*ctx*/, int mode)
{
    imas_remote::ClosePulseRequest req;
    req.set_session_id(impl_->session_id);
    req.set_mode(mode);

    imas_remote::StatusResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->ClosePulse(&grpc_ctx, req, &resp), "closePulse");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::closePulse: " + resp.error_message(), LOG);
    }

    impl_->session_id.clear();
    impl_->ctx_ids.clear();
}

void GRPCBackend::beginAction(OperationContext* ctx)
{
    imas_remote::BeginActionRequest req;
    req.set_session_id(impl_->session_id);
    req.set_dataobject_name(ctx->getDataobjectName());
    req.set_datapath(ctx->getDatapath());
    req.set_access_mode(ctx->getAccessmode());
    req.set_range_mode(ctx->getRangemode());

    if (ctx->getRangemode() == SLICE_OP) {
        req.set_time(ctx->getTime());
        req.set_interp_mode(ctx->getInterpmode());
    } else if (ctx->getRangemode() == TIMERANGE_OP) {
        double tmin, tmax;
        std::vector<double> dtime;
        int interp;
        ctx->getTimeRange(&tmin, &tmax, dtime, &interp);
        req.set_tmin(tmin);
        req.set_tmax(tmax);
        req.set_interp_mode(interp);
        for (double t : dtime) req.add_dtime(t);
    }

    imas_remote::BeginActionResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->BeginAction(&grpc_ctx, req, &resp), "beginAction");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::beginAction: " + resp.error_message(), LOG);
    }

    impl_->ctx_ids[ctx->getUid()] = resp.operation_id();
}

void GRPCBackend::endAction(Context* ctx)
{
    auto it = impl_->ctx_ids.find(ctx->getUid());
    if (it == impl_->ctx_ids.end()) {
        return;  // pulse/session contexts are not tracked in ctx_ids — nothing to do
    }

    imas_remote::EndActionRequest req;
    req.set_session_id(impl_->session_id);
    req.set_context_id(it->second);

    imas_remote::StatusResponse resp;
    grpc::ClientContext grpc_ctx;
    grpc::Status status = impl_->stub->EndAction(&grpc_ctx, req, &resp);

    impl_->ctx_ids.erase(ctx->getUid());  // always clean up, even on error

    checkRpcStatus(status, "endAction");
    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::endAction: " + resp.error_message(), LOG);
    }
}

void GRPCBackend::writeData(Context* ctx, std::string fieldname, std::string timebasename, void* data,
                            int datatype, int dim, int* size)
{
    throw ALBackendException("GRPCBackend::writeData: ADAM TO IMPLEMENT", LOG);
}

// void GRPCBackend::writeData(Context*    ctx,
//                              std::string fieldname,
//                              std::string timebasename,
//                              void*       data,
//                              int         datatype,
//                              int         dim,
//                              int*        size)
// {
//     if (datatype == CHAR_DATA && dim > 1)
//         throw ALBackendException(
//             "GRPCBackend::writeData: CHAR_DATA dim>1 (arrays of strings) not yet supported", LOG);
//
//     imas_remote::WriteDataRequest req;
//     req.set_session_id(impl_->session_id);
//     req.set_context_id(lookupContextId(impl_->ctx_ids, ctx));
//     req.set_field_name(std::move(fieldname));
//     req.set_timebase_name(std::move(timebasename));
//     req.set_data_type(datatype);
//     req.set_dim(dim);
//     for (int i = 0; i < dim; ++i) req.add_sizes(size[i]);
//
//     // dim=0: null-terminated scalar string — include the terminator in the payload.
//     // dim=1: char array of known length (sizes[0] bytes) — no terminator needed,
//     //        the receiver uses sizes[0] to know how many bytes to read back.
//     size_t n_bytes = (datatype == CHAR_DATA && dim == 0)
//         ? std::strlen(static_cast<const char*>(data)) + 1
//         : byteCount(datatype, dim, size);
//     req.set_data(static_cast<const char*>(data), n_bytes);
//
//     imas_remote::StatusResponse resp;
//     grpc::ClientContext grpc_ctx;
//     checkRpcStatus(impl_->stub->WriteData(&grpc_ctx, req, &resp), "writeData");
//
//     if (!resp.ok())
//         throw ALBackendException("GRPCBackend::writeData: " + resp.error_message(), LOG);
// }

int GRPCBackend::readData(Context* ctx, std::string fieldname, std::string timebasename, void** data,
                          int* datatype, int* dim, int* size)
{
    imas_remote::ReadDataRequest req;
    req.set_session_id(impl_->session_id);
    req.set_context_id(lookupContextId(impl_->ctx_ids, ctx));
    req.set_field_name(std::move(fieldname));
    req.set_timebase_name(std::move(timebasename));
    req.set_data_type(*datatype);
    req.set_dim(*dim);

    auto arr_ctx = dynamic_cast<ArraystructContext*>(ctx);
    if (arr_ctx != nullptr) {
        req.set_array_index(arr_ctx->getIndex());
    }

    imas_remote::ReadDataResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->ReadData(&grpc_ctx, req, &resp), "readData");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::readData: " + resp.error_message(), LOG);
    }

    if (!resp.found()) {
        return 0;
    }

    // not keen on this
    *datatype = resp.data_type();
    *dim      = resp.dim();
    for (int i = 0; i < resp.dim(); ++i) {
        size[i] = resp.sizes(i);
    }

    const std::string& payload = resp.data();
    *data = std::malloc(payload.size());
    if (!*data) {
        throw ALBackendException("GRPCBackend::readData: malloc failed", LOG);
    }
    std::memcpy(*data, payload.data(), payload.size());

    return 1;
}

void GRPCBackend::deleteData(OperationContext* ctx, std::string path)
{
    throw ALBackendException("GRPCBackend::deleteData: ADAM TO IMPLEMENT", LOG);
}

// void GRPCBackend::deleteData(OperationContext* ctx, std::string path)
// {
//     imas_remote::DeleteDataRequest req;
//     req.set_session_id(impl_->session_id);
//     req.set_operation_id(lookupContextId(impl_->ctx_ids, ctx));
//     req.set_path(std::move(path));
//
//     imas_remote::StatusResponse resp;
//     grpc::ClientContext grpc_ctx;
//     checkRpcStatus(impl_->stub->DeleteData(&grpc_ctx, req, &resp), "deleteData");
//
//     if (!resp.ok())
//         throw ALBackendException("GRPCBackend::deleteData: " + resp.error_message(), LOG);
// }

void GRPCBackend::beginArraystructAction(ArraystructContext* ctx, int* size)
{
    imas_remote::BeginArraystructActionRequest req;
    req.set_session_id(impl_->session_id);
    req.set_path(ctx->getPath());
    req.set_timebase(ctx->getTimebasePath());
    req.set_index(ctx->getIndex());
    req.set_requested_size(*size);

    ArraystructContext* parent_as = ctx->getParent();
    if (parent_as != nullptr) {
        req.set_is_nested(true);
        req.set_parent_id(lookupContextId(impl_->ctx_ids, parent_as));
    } else {
        req.set_is_nested(false);
        req.set_parent_id(lookupContextId(impl_->ctx_ids, ctx->getOperationContext()));
    }

    imas_remote::BeginArraystructActionResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->BeginArraystructAction(&grpc_ctx, req, &resp), "beginArraystructAction");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::beginArraystructAction: " + resp.error_message(), LOG);
    }

    impl_->ctx_ids[ctx->getUid()] = resp.arraystruct_id();
    *size = resp.size();
}

void GRPCBackend::get_occurrences(Context* /*ctx*/, const char* ids_name, int** occurrences_list, int* size)
{
    imas_remote::GetOccurrencesRequest req;
    req.set_session_id(impl_->session_id);
    req.set_ids_name(ids_name);

    imas_remote::GetOccurrencesResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->GetOccurrences(&grpc_ctx, req, &resp), "get_occurrences");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::get_occurrences: " + resp.error_message(), LOG);
    }

    *size = resp.occurrences_size();
    *occurrences_list = static_cast<int*>(std::malloc(static_cast<size_t>(*size) * sizeof(int)));
    if (!*occurrences_list) {
        throw ALBackendException("GRPCBackend::get_occurrences: malloc failed", LOG);
    }

    for (int i = 0; i < *size; ++i) {
        (*occurrences_list)[i] = resp.occurrences(i);
    }
}

void GRPCBackend::list_filled_paths(Context* /*ctx*/, const char* dataobjectname, char*** path_list, int* size)
{
    imas_remote::ListFilledPathsRequest req;
    req.set_session_id(impl_->session_id);
    req.set_dataobject_name(dataobjectname);

    imas_remote::ListFilledPathsResponse resp;
    grpc::ClientContext grpc_ctx;
    checkRpcStatus(impl_->stub->ListFilledPaths(&grpc_ctx, req, &resp), "list_filled_paths");

    if (!resp.ok()) {
        throw ALBackendException("GRPCBackend::list_filled_paths: " + resp.error_message(), LOG);
    }

    *size = resp.paths_size();
    *path_list = static_cast<char**>(std::malloc(static_cast<size_t>(*size) * sizeof(char*)));
    if (!*path_list) {
        throw ALBackendException("GRPCBackend::list_filled_paths: malloc failed", LOG);
    }

    for (int i = 0; i < *size; ++i) {
        const std::string& p = resp.paths(i);
        (*path_list)[i] = static_cast<char*>(std::malloc(p.size() + 1));
        if (!(*path_list)[i]) {
            throw ALBackendException("GRPCBackend::list_filled_paths: malloc failed", LOG);
        }
        std::memcpy((*path_list)[i], p.c_str(), p.size() + 1);
    }
}
