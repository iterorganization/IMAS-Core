#include "hdf5_events_handler.h"

#include "hdf5_utils.h"

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
		writer.create_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, access_mode);
		writer.slice_mode = GLOBAL_OP;
	} else if (ctx->getAccessmode() == WRITE_OP && ctx->getRangemode() == SLICE_OP) {
		std::string IDS_link_name = ctx->getDataobjectName();
		std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
		HDF5Utils hdf5_utils;
		std::string IDS_pulse_file = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
		bool call_put_required = false;
		
		if (hdf5_utils.pulseFileExists(IDS_pulse_file)) {
			writer.slice_mode = SLICE_OP;
			hid_t loc_id = -1;
			writer.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, &loc_id);
			if (loc_id == -1) {
				call_put_required = true;
			} 
			else {
				int homogeneous_time = -1;
				writer.read_homogeneous_time(&homogeneous_time, loc_id);
				call_put_required = (homogeneous_time == -1);
			}
		}
		else {
			call_put_required = true;
		}
		if (call_put_required) {
			writer.slice_mode = GLOBAL_OP;
			writer.create_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, access_mode);	
		}
		
	} else if (ctx->getAccessmode() == READ_OP) {
		reader.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path);
		reader.slice_mode = ctx->getRangemode();
	}
}

void HDF5EventsHandler::endAction(Context * ctx, hid_t file_id, HDF5Writer & writer, HDF5Reader & reader, std::unordered_map < std::string, hid_t > &opened_IDS_files)
{
	if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
		OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);
		if (opCtx->getAccessmode() == WRITE_OP) {
			writer.endAction(ctx);
		} else if (opCtx->getAccessmode() == READ_OP) {
			reader.endAction(ctx);
		}
	} else if (ctx->getType() == CTX_OPERATION_TYPE) {
		OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);

		if (opCtx->getAccessmode() == WRITE_OP) {
            if (opCtx->getRangemode() == GLOBAL_OP)
			    writer.write_buffers();
			H5Fflush(file_id, H5F_SCOPE_LOCAL);
			writer.close_datasets();
			writer.close_group(opCtx);
			writer.close_file_handler(opCtx->getDataobjectName(), opened_IDS_files);
		} 
        else if (opCtx->getAccessmode() == READ_OP) {
			reader.close_datasets();
            reader.close_group(opCtx);
			reader.close_file_handler(opCtx->getDataobjectName(), opened_IDS_files);
		}
	}
}
