#include "hdf5_events_handler.h"

HDF5EventsHandler::HDF5EventsHandler()
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5EventsHandler::~HDF5EventsHandler()
{
}

void
 HDF5EventsHandler::beginAction(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, HDF5Writer & writer, HDF5Reader & reader, std::string & files_directory, std::string & relative_file_path, int access_mode)
{
    if (ctx->getAccessmode() == WRITE_OP && ctx->getRangemode() == GLOBAL_OP) {
        writer.clear_stacks();
        writer.create_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, access_mode);
        writer.slice_mode = GLOBAL_OP;
    } else if (ctx->getAccessmode() == WRITE_OP && ctx->getRangemode() == SLICE_OP) {
        writer.clear_stacks();
        writer.slice_mode = SLICE_OP;
        writer.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path);
    } else if (ctx->getAccessmode() == READ_OP) {
        reader.clear_stacks();
        reader.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path);
        reader.slice_mode = ctx->getRangemode();
    }
}

void HDF5EventsHandler::endAction(Context * ctx, hid_t file_id, HDF5Writer & writer, HDF5Reader & reader, std::unordered_map < std::string, hid_t > &opened_IDS_files)
{
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);
        if (opCtx->getAccessmode() == WRITE_OP) {
            writer.pop_back_stacks();
        } else if (opCtx->getAccessmode() == READ_OP) {
            reader.pop_back_stacks();
        }
    } else if (ctx->getType() == CTX_OPERATION_TYPE) {
        OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);
        if (opCtx->getRangemode() == GLOBAL_OP && opCtx->getAccessmode() == WRITE_OP) {
            writer.write_buffers();
            H5Fflush(file_id, H5F_SCOPE_LOCAL);
            writer.close_datasets();
            writer.close_group();
            writer.close_file_handler(opCtx->getDataobjectName(), opened_IDS_files);
            writer.start_put_slice_operation();
            reader.close_datasets();
        } else if (opCtx->getRangemode() == SLICE_OP && opCtx->getAccessmode() == WRITE_OP) {
            H5Fflush(file_id, H5F_SCOPE_LOCAL);
            writer.end_put_slice_operation();
            writer.close_datasets();
            writer.close_group();
            writer.close_file_handler(opCtx->getDataobjectName(), opened_IDS_files);
            reader.close_datasets();
        } else if (opCtx->getAccessmode() == READ_OP) {
            reader.close_datasets();
            reader.close_file_handler(opCtx->getDataobjectName(), opened_IDS_files);
        }
    }
}
