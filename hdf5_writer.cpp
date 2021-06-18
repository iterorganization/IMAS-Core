#include "hdf5_writer.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "ual_defs.h"
#include "hdf5_hs_selection_reader.h"
#include "ual_backend.h"
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

HDF5Writer::HDF5Writer(std::string backend_version_)
:  backend_version(backend_version_), tensorized_paths(), opened_data_sets(), dataset_handlers(), selection_writers(), current_arrctx_indices(), current_arrctx_shapes(), homogeneous_time(-1), IDS_group_id(-1), init_slice_index(false), dynamic_AOS_slices_extension(0), slice_mode(GLOBAL_OP)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Writer::~HDF5Writer()
{
}

bool HDF5Writer::compression_enabled = true;

void HDF5Writer::closePulse(PulseContext * ctx, int mode, std::string & options, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path)
{
    close_datasets();
    HDF5Utils hdf5_utils;
    auto it = opened_IDS_files.begin();
    while (it != opened_IDS_files.end()) {
        const std::string & external_link_name = it->first;
        close_file_handler(external_link_name, opened_IDS_files);
        it++;
    }
    hdf5_utils.closeMasterFile(file_id);
}

void HDF5Writer::close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files)
{
    std::replace(external_link_name.begin(), external_link_name.end(), '/', '_');
    hid_t pulse_file_id = -1;
    if (opened_IDS_files.find(external_link_name) != opened_IDS_files.end()) {
        pulse_file_id = opened_IDS_files[external_link_name];
    }
    if (pulse_file_id != -1) {
		/*std::cout << "WRITER:close_file_handler :showing status for pulse file..." << std::endl;
			HDF5Utils hdf5_utils;
	        hdf5_utils.showStatus(pulse_file_id);*/
        herr_t status = H5Fclose(pulse_file_id);
        if (status < 0) {
            char error_message[100];
            sprintf(error_message, "Unable to close HDF5 file for IDS: %s\n", external_link_name.c_str());
            throw UALBackendException(error_message, LOG);
        }
        opened_IDS_files[external_link_name] = -1;
    }
}

void HDF5Writer::deleteData(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    assert(file_id != -1); //the master file is assumed to be opened
      
    if (IDS_group_id == -1)
        return;

    close_datasets();

    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    HDF5Utils hdf5_utils;
    //Deleting IDS link from master file
    if (H5Lexists(file_id, IDS_link_name.c_str(), H5P_DEFAULT) > 0) { //the IDS is referenced in the master file
        if (opened_IDS_files.find(IDS_link_name) != opened_IDS_files.end()) {
            hid_t IDS_file_id = opened_IDS_files[IDS_link_name];
            if (IDS_file_id != -1) {
                assert(H5Fclose(IDS_file_id) >=0);
            }
            opened_IDS_files[IDS_link_name] = -1;
        }
        std::string IDSpulseFile = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
        hdf5_utils.deleteIDSFile(IDSpulseFile);
        if (H5Ldelete(file_id, IDS_link_name.c_str(), H5P_DEFAULT) < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to remove HDF5 link %s from master file.\n", IDS_link_name.c_str());
            throw UALBackendException(error_message, LOG);
        }
    }

    H5Gclose(IDS_group_id);
    IDS_group_id = -1;
}

void HDF5Writer::open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, 
hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    HDF5Utils hdf5_utils;
    hdf5_utils.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, &IDS_group_id);
}

void HDF5Writer::create_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, int access_mode)
{
    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    
    HDF5Utils hdf5_utils;
    std::string IDSpulseFile = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
    hid_t IDS_file_id = -1;

    if (opened_IDS_files.find(IDS_link_name) == opened_IDS_files.end()) {
            if (!exists(IDSpulseFile.c_str())) {
                hdf5_utils.createIDSFile(ctx, IDSpulseFile, backend_version, &IDS_file_id);
            }
            else {
                hdf5_utils.openIDSFile(ctx, IDSpulseFile, &IDS_file_id);
            }
        
        opened_IDS_files[IDS_link_name] = IDS_file_id;

    } else {
        IDS_file_id = opened_IDS_files[IDS_link_name];
        if (IDS_file_id == -1) { //file not opened
            if (!exists(IDSpulseFile.c_str())) {
                hdf5_utils.createIDSFile(ctx, IDSpulseFile, backend_version, &IDS_file_id);
            }
            else {
                hdf5_utils.openIDSFile(ctx, IDSpulseFile, &IDS_file_id);
            }
            
            opened_IDS_files[IDS_link_name] = IDS_file_id;
        }
    }
    assert(IDS_file_id >= 0);

    if (H5Lexists(file_id, IDS_link_name.c_str(), H5P_DEFAULT) == 0) {
        std::string relative_IDSpulseFile = hdf5_utils.getIDSPulseFilePath(".", relative_file_path, IDS_link_name);
        herr_t status = H5Lcreate_external(relative_IDSpulseFile.c_str(), IDS_link_name.c_str(),
                                           file_id, IDS_link_name.c_str(), H5P_DEFAULT,
                                           H5P_DEFAULT);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "unable to create external link for IDS: %s.\n", ctx->getDataobjectName().c_str());
            throw UALBackendException(error_message, LOG);
        }
    }

    if (IDS_group_id != -1) {
        H5Gclose(IDS_group_id);
    }

    IDS_group_id = hdf5_utils.createOrOpenHDF5Group(ctx->getDataobjectName().c_str(), IDS_file_id);
    assert(IDS_group_id >= 0);
}


void HDF5Writer::start_put_slice_operation()
{
    put_slice_count = 1;
}

void HDF5Writer::end_put_slice_operation()
{
    put_slice_count++;
}

void HDF5Writer::close_datasets()
{
    dataset_handlers.clear();
    auto it_ds = opened_data_sets.begin ();
    while (it_ds != opened_data_sets.end ())
    {
      hid_t dset_id = it_ds->second;
      assert(H5Dclose (dset_id) >=0);
      it_ds++;
    }
    opened_data_sets.clear();
    selection_writers.clear();
}

void HDF5Writer::close_group()
{
    if (IDS_group_id != -1) {
        H5Gclose(IDS_group_id);
        IDS_group_id = -1;
    }
}

int HDF5Writer::readTimedAOSShape(hid_t loc_id)
{
    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";
	int shape = 0;

    if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {
        hid_t dataset_id = -1;
		hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
		size_t rdcc_nbytes = 500*1024*1024;
        size_t rdcc_nslots = 521;
        H5Pset_chunk_cache(dapl, rdcc_nslots, rdcc_nbytes, H5D_CHUNK_CACHE_W0_DEFAULT);
        dataset_id = H5Dopen2(IDS_group_id, tensorized_path.c_str(), dapl);
        H5Pclose(dapl);

        int dim = -1;
        int *AOS_shapes = nullptr;
        HDF5HsSelectionReader hsSelectionReader(dataset_id, ualconst::integer_data, current_arrctx_indices.size(), &dim);
        hsSelectionReader.allocateGlobalOpBuffer((void **) &AOS_shapes);
        hsSelectionReader.setHyperSlabsGlobalOp(current_arrctx_indices);
        herr_t status = H5Dread(dataset_id, hsSelectionReader.dtype_id,
                                hsSelectionReader.memspace,
                                hsSelectionReader.dataspace, H5P_DEFAULT,
                                AOS_shapes);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read timed AOS_SHAPE: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }
        assert(H5Dclose(dataset_id) >=0);
		shape = AOS_shapes[0];
        free(AOS_shapes);
    }
	return shape;
}

void HDF5Writer::beginWriteArraystructAction(ArraystructContext * ctx, int *size, hid_t loc_id, std::string & IDS_link_name)
{
    loc_id = IDS_group_id;
    assert(loc_id >= 0);
    HDF5Utils hdf5_utils;
    hdf5_utils.setTensorizedPaths(ctx, tensorized_paths);
	int AOS_timed_shape = 0;
    if (slice_mode == SLICE_OP && ctx->getTimed()) {
        AOS_timed_shape = readTimedAOSShape(loc_id);
		dynamic_AOS_slices_extension = *size;
		current_arrctx_indices.push_back(ctx->getIndex());
		current_arrctx_shapes.push_back(AOS_timed_shape);
    } else {
        current_arrctx_indices.push_back(ctx->getIndex());
        current_arrctx_shapes.push_back(*size);
    }
    int timed_AOS_index = -1;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices
    createOrUpdateAOSShapesDataSet(ctx, loc_id, IDS_link_name, AOS_timed_shape);
}


void HDF5Writer::write_ND_Data(Context * ctx, hid_t loc_id, std::string & att_name, std::string & timebasename, int datatype, int dim, int *size, void *data)
{
    std::string & dataset_name = att_name;
    std::replace(dataset_name.begin(), dataset_name.end(), '/', '&');   // character '/' is not supported in datasets names
    std::replace(timebasename.begin(), timebasename.end(), '/', '&');

    loc_id = IDS_group_id;

    assert(loc_id >= 0);

    if (dataset_name == "ids_properties&homogeneous_time") {
        int *v = (int *) data;
        homogeneous_time = v[0];
    }


    HDF5Utils hdf5_utils;
    int timed_AOS_index = -1;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices    

    int AOSRank = current_arrctx_indices.size();
    std::string tensorized_path = dataset_name;

    if (tensorized_paths.size() > 0)
        tensorized_path = tensorized_paths.back() + "&" + dataset_name;

    hid_t dataset_id = hdf5_utils.searchDataSetId(tensorized_path, opened_data_sets);

    int initial_size[H5S_MAX_RANK];
    for (int i = 0; i < dim; i++) {
        initial_size[i] = size[i];
    }

    std::vector < int >AOSShapes;
	AOSShapes.assign(current_arrctx_shapes.begin(), current_arrctx_shapes.end());
	bool shapes_dataset = false;

    if (slice_mode != SLICE_OP) {

		//std::cout << "WRITER NOT IN SLICE MODE!!! " << std::endl;
        if (dataset_id < 0)     //in global mode (put), data set not yet created
        {
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());
            dataSetHandler->setNonSliceMode();
            dataSetHandler->create(tensorized_path.c_str(), &dataset_id, datatype, loc_id, dim, size, AOSRank, AOSShapes.data(), false, compression_enabled);
            dataset_handlers[dataset_id] = std::move(dataSetHandler);
            opened_data_sets[tensorized_path] = dataset_id;
        } else {
            HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];
            dataSetHandler.setNonSliceMode();
			dataSetHandler.setCurrentShapesAndExtend(size, AOSShapes.data());
        }
    } else {
        //std::cout << "WRITER IN SLICE MODE!!! " << std::endl;

        if (dataset_id < 0)     //in slice mode, data set does not exist or not yet opened
        {
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());
            dataSetHandler->setSliceMode(ctx);
			dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, datatype, shapes_dataset);
            dataSetHandler->storeInitialDims(); //store the dims into initial_dims at beginning of the put_slice
			dataSetHandler->extendDataSpaceForTimeSlices(size, AOSShapes.data(), dynamic_AOS_slices_extension);
			dataSetHandler->setTimeAxisOffset(current_arrctx_indices, dynamic_AOS_slices_extension);
            dataset_handlers[dataset_id] = std::move(dataSetHandler);
            opened_data_sets[tensorized_path] = dataset_id;
        } else {
			HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];
            dataSetHandler.setSliceMode(ctx);
            dataSetHandler.updateTimeAxisOffset(current_arrctx_indices);
			dataSetHandler.setCurrentShapesAndExtend(size, AOSShapes.data());
        }
    }

    char **p = nullptr;
    if (datatype == ualconst::char_data) {
        if (dim == 1) {
            p = (char **) malloc(sizeof(char *));
            p[0] = (char *) data;
            char *s = p[0];
            s[initial_size[0]] = 0;
            data = p;
        } else {
            p = (char **) malloc(sizeof(char *) * initial_size[0]);
            char *c = (char *) data;
            for (int i = 0; i < initial_size[0]; i++) {
				char *s = (char *) (c + i * initial_size[1]);
				char *copy = (char *) malloc(initial_size[1] + 1);
				strncpy(copy, s, initial_size[1]);
				copy[initial_size[1]] = 0;
				p[i] = copy;
            }
            data = p;
        }
    }
	
    HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];

    herr_t status = -1;
    if (selection_writers.find(tensorized_path) != selection_writers.end()) {
        HDF5HsSelectionWriter & hsSelectionWriter = *selection_writers[tensorized_path];
        hsSelectionWriter.setHyperSlabs(dataset_id, current_arrctx_indices, slice_mode, dataSetHandler, dynamic_AOS_slices_extension);
        status = H5Dwrite(dataset_id, dataSetHandler.dtype_id, hsSelectionWriter.memspace, dataSetHandler.getDataSpace(), H5P_DEFAULT, data);
    } else {
        std::unique_ptr < HDF5HsSelectionWriter > hsSelectionWriter(new HDF5HsSelectionWriter());
        hsSelectionWriter->setHyperSlabs(dataset_id, current_arrctx_indices, slice_mode, dataSetHandler, dynamic_AOS_slices_extension);
        status = H5Dwrite(dataset_id, dataSetHandler.dtype_id, hsSelectionWriter->memspace, dataSetHandler.getDataSpace(), H5P_DEFAULT, data);
        selection_writers[tensorized_path] = std::move(hsSelectionWriter);
    }


    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write HDF5 dataset: %s\n", tensorized_path.c_str());
        throw UALBackendException(error_message, LOG);
    }

    if ((datatype != ualconst::char_data && dim > 0)
        || (datatype == ualconst::char_data && dim == 2))
        createOrUpdateShapesDataSet(ctx, loc_id, tensorized_path, dataSetHandler, timebasename, timed_AOS_index);
}


hid_t HDF5Writer::createOrUpdateShapesDataSet(Context * ctx, hid_t loc_id, const std::string & field_tensorized_path, HDF5DataSetHandler & fieldHandler, 
std::string & timebasename, int timed_AOS_index)
{
    hid_t dataset_id = -1;
    int AOSRank = current_arrctx_indices.size();
	int rank = fieldHandler.getRank();

    if (AOSRank == 0 || (rank == AOSRank) )
        return dataset_id;

    const std::string & tensorized_path = field_tensorized_path + "_SHAPE";

	//std::cout << "Writing data set: " << tensorized_path << std::endl;

    int dim = 1;                //SHAPE is a 1D dataset
    int size[1] = { rank - AOSRank };   //length of the shapes vector
    int *shapes = (int *) malloc(sizeof(int) * size[0]);

    std::vector < int >aos_indices(current_arrctx_indices.begin(), current_arrctx_indices.end());
    std::vector < int >aos_shapes(current_arrctx_shapes.begin(), current_arrctx_shapes.end());

    if (slice_mode == SLICE_OP) {
        int timed_AOS_index_ = -1;
        int timedShape = fieldHandler.getTimedShape(&timed_AOS_index_);
        if (timedShape != -1) {
            aos_shapes[timed_AOS_index_] = timedShape;
        }
    }

    herr_t status = -1;
    HDF5Utils hdf5_utils;

    bool shapes_dataset = true;

    dataset_id = hdf5_utils.searchDataSetId(tensorized_path, opened_data_sets);
    if (dataset_id < 0)         //dataset not yet created in GLOBAL_OP or not yet opened in SLICE_OP
    {

        std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());

		hsize_t* dataspace_dims = fieldHandler.getDims();

		for (int i = 0; i < rank - AOSRank; i++) {
            if (slice_mode == SLICE_OP && i == 0)
                shapes[i] = dataspace_dims[i + AOSRank] + fieldHandler.getTimeWriteOffset();
            else
			    shapes[i] = dataspace_dims[i + AOSRank];
		}

        if (slice_mode == SLICE_OP) {
            dataSetHandler->setSliceMode(ctx);
			dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, ualconst::integer_data, shapes_dataset);
            dataSetHandler->storeInitialDims();
			dataSetHandler->extendDataSpaceForTimeSlices(size, aos_shapes.data(), dynamic_AOS_slices_extension);
			dataSetHandler->setTimeAxisOffset(current_arrctx_indices, dynamic_AOS_slices_extension);
			
        } else {
            dataSetHandler->setNonSliceMode();
			dataSetHandler->create(tensorized_path.c_str(), &dataset_id, ualconst::integer_data, loc_id, dim, size, AOSRank, aos_shapes.data(), shapes_dataset, compression_enabled);
        }

        std::unique_ptr < HDF5HsSelectionWriter > hsSelectionWriter(new HDF5HsSelectionWriter());
        hsSelectionWriter->setHyperSlabs(dataset_id, aos_indices, slice_mode, *dataSetHandler, dynamic_AOS_slices_extension);
        status = H5Dwrite(dataset_id, dataSetHandler->dtype_id, hsSelectionWriter->memspace, dataSetHandler->getDataSpace(), H5P_DEFAULT, shapes);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to write HDF5 dataset for SHAPE: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }

        selection_writers[tensorized_path] = std::move(hsSelectionWriter);
        dataset_handlers[dataset_id] = std::move(dataSetHandler);
        opened_data_sets[tensorized_path] = dataset_id;

    } else  //dataset already used in previous LL request
    {
        HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];
        if (slice_mode == SLICE_OP) {
            dataSetHandler.setSliceMode(ctx);
            dataSetHandler.updateTimeAxisOffset(current_arrctx_indices);
            dataSetHandler.setCurrentShapesAndExtend(size, aos_shapes.data());
        } else {
            dataSetHandler.setNonSliceMode();
			dataSetHandler.setCurrentShapesAndExtend(size, aos_shapes.data());
        }
	
        hsize_t* dataspace_dims = fieldHandler.getDims();

		for (int i = 0; i < rank - AOSRank; i++) {
			 if (slice_mode == SLICE_OP && i == 0)
                shapes[i] = dataspace_dims[i + AOSRank] + fieldHandler.getTimeWriteOffset();
            else
                shapes[i] = dataspace_dims[i + AOSRank];
		}

        HDF5HsSelectionWriter & hsSelectionWriter = *selection_writers[tensorized_path];
        hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, slice_mode, dataSetHandler, dynamic_AOS_slices_extension);

        status = H5Dwrite(dataset_id, dataSetHandler.dtype_id, hsSelectionWriter.memspace, dataSetHandler.getDataSpace(), H5P_DEFAULT, shapes);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to write HDF5 dataset for SHAPE: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }
    }

    free(shapes);
    return dataset_id;
}

void HDF5Writer::createOrUpdateAOSShapesDataSet(ArraystructContext * ctx, hid_t loc_id, std::string & IDS_link_name, int timedAOS_shape)
{
    int dim = 1;                //AOS_SHAPE is a 1D dataset 
    int size[1] = { 1 };
    int shapes[1];              //buffer which will be written, previous size before extension

    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";

	//std::cout << "Writing data set for AOS shape: " << tensorized_path.c_str() << std::endl;

    int timed_AOS_index = -1;
	HDF5Utils hdf5_utils;
    hdf5_utils.isTimed(ctx, &timed_AOS_index);

    herr_t status = -1;
    hid_t dataset_id = -1;
	bool shapes_dataset = true;

    shapes[0] = current_arrctx_shapes.back();

    std::vector < int >aos_indices(current_arrctx_indices.begin(), current_arrctx_indices.end() - 1);
    std::vector < int >aos_shapes(current_arrctx_shapes.begin(), current_arrctx_shapes.end() - 1);

    if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {  //not yet used by a previous LL request

        dataset_id = hdf5_utils.searchDataSetId(tensorized_path, opened_data_sets);

        if (dataset_id < 0) { //this code is called once at the beginning of a put_slice

            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());
            
			if (slice_mode == SLICE_OP) {
            	dataSetHandler->setSliceMode(ctx);
                if (ctx->getTimed()) {
                    dataSetHandler->setTimedAOSShape(timedAOS_shape);
                    shapes[0] = timedAOS_shape + dynamic_AOS_slices_extension;
                }
				dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, ualconst::integer_data, shapes_dataset); //dataset extension occurs
				dataSetHandler->extendDataSpaceForTimeSlicesForAOSDataSet(size, aos_shapes.data(), dynamic_AOS_slices_extension);
				dataSetHandler->setTimeAxisOffsetForAOSDataSet();
			} else {
				dataSetHandler->setNonSliceMode();
				dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, ualconst::integer_data, shapes_dataset);
				dataSetHandler->setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			}
			
            HDF5HsSelectionWriter hsSelectionWriter;
            hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, slice_mode, *dataSetHandler, dynamic_AOS_slices_extension);
            status = H5Dwrite(dataset_id, dataSetHandler->dtype_id, hsSelectionWriter.memspace, dataSetHandler->getDataSpace(), H5P_DEFAULT, shapes);

            if (status < 0) {
                char error_message[200];
                sprintf(error_message, "Unable to write HDF5 dataset for AOS_SHAPE: %s\n", tensorized_path.c_str());
                throw UALBackendException(error_message, LOG);
            }

            dataset_handlers[dataset_id] = std::move(dataSetHandler);
            opened_data_sets[tensorized_path] = dataset_id;
        }
        else {
			HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];

			if (slice_mode == SLICE_OP) {
            	dataSetHandler.setSliceMode(ctx);
                if (ctx->getTimed()) {
                    shapes[0] = dataSetHandler.getTimedAOSShape() + dynamic_AOS_slices_extension;
                }
				dataSetHandler.setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			} else {
				dataSetHandler.setNonSliceMode();
				dataSetHandler.setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			}

            HDF5HsSelectionWriter hsSelectionWriter;
            hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, slice_mode, dataSetHandler, dynamic_AOS_slices_extension);
            status = H5Dwrite(dataset_id, dataSetHandler.dtype_id, hsSelectionWriter.memspace, dataSetHandler.getDataSpace(), H5P_DEFAULT, shapes);
            

            if (status < 0) {
                char error_message[200];
                sprintf(error_message, "Unable to write HDF5 dataset for AOS_SHAPE: %s\n", tensorized_path.c_str());
                throw UALBackendException(error_message, LOG);
            }
        }
    }
    else {//AOS_SHAPE doesn't exist yet, we create it, this code is used only on a put() operation, an error is raised otherwise

		std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());
         
		if (slice_mode == SLICE_OP) {
			char error_message[200];
            sprintf(error_message, "Inconsistent state detected in slice mode. The AOS shapes dataset: %s is not present in the HDF5 file.\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
		} else {
			dataSetHandler->setNonSliceMode();
		}

        hid_t dataset_id = -1;
		dataSetHandler->create(tensorized_path.c_str(), &dataset_id, ualconst::integer_data, loc_id, dim, size, aos_shapes.size(), aos_shapes.data(), shapes_dataset, compression_enabled);
        HDF5HsSelectionWriter hsSelectionWriter;
        hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, slice_mode, *dataSetHandler, dynamic_AOS_slices_extension);
        status = H5Dwrite(dataset_id, dataSetHandler->dtype_id, hsSelectionWriter.memspace, dataSetHandler->getDataSpace(), H5P_DEFAULT, shapes);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to write HDF5 dataset for AOS_SHAPE: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }
        dataset_handlers[dataset_id] = std::move(dataSetHandler);
        opened_data_sets[tensorized_path] = dataset_id;
    }
}

void HDF5Writer::pop_back_stacks()
{
    if (current_arrctx_indices.size() > 0)
        current_arrctx_indices.pop_back();
    if (current_arrctx_shapes.size() > 0)
        current_arrctx_shapes.pop_back();
    if (tensorized_paths.size() > 0)
        tensorized_paths.pop_back();
}

void HDF5Writer::clear_stacks()
{
    current_arrctx_indices.clear();
    current_arrctx_shapes.clear();
    tensorized_paths.clear();
}

