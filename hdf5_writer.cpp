#include "hdf5_writer.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "ual_defs.h"
#include "hdf5_hs_selection_reader.h"
#include "hdf5_hs_selection_writer.h"
#include "ual_backend.h"
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

HDF5Writer::HDF5Writer(std::string backend_version_)
:  backend_version(backend_version_), tensorized_paths(), opened_data_sets(), existing_data_sets(), current_arrctx_indices(), current_arrctx_shapes(), homogeneous_time(-1), IDS_group_id(-1), init_slice_index(false), dynamic_AOS_slices_extension(0), slice_mode(GLOBAL_OP)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Writer::~HDF5Writer()
{
}

bool HDF5Writer::compression_enabled = true;
bool HDF5Writer::useBuffering = true;

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
    HDF5Utils hdf5_utils;
    hid_t pulse_file_id = -1;
    auto got = opened_IDS_files.find(external_link_name);
    if (got != opened_IDS_files.end()) {
        pulse_file_id = got->second;
        if (pulse_file_id != -1) {
		    /*std::cout << "WRITER:close_file_handler :showing status for pulse file..." << std::endl;
			    HDF5Utils hdf5_utils;
	            hdf5_utils.showStatus(pulse_file_id);*/
            hdf5_utils.closeIDSFile(pulse_file_id, external_link_name);
            opened_IDS_files[external_link_name] = -1;
        }
    }
}

void HDF5Writer::deleteData(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    assert(file_id != -1); //the master file is assumed to be opened
    if (IDS_group_id < 0)
        return;
    close_datasets();
    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    HDF5Utils hdf5_utils;
    //Deleting IDS link from master file
    if (H5Lexists(file_id, IDS_link_name.c_str(), H5P_DEFAULT) > 0) { //the IDS is referenced in the master file
        auto got = opened_IDS_files.find(IDS_link_name);
        hid_t IDS_file_id = -1;
        std::string IDSpulseFile = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
        if (got != opened_IDS_files.end()) {
            IDS_file_id = got->second;
            if (IDS_file_id < 0) {
                if (exists(IDSpulseFile.c_str())) {
                    hdf5_utils.openIDSFile(ctx, IDSpulseFile, &IDS_file_id, false);
                }
            }
            else {
                hdf5_utils.closeIDSFile(IDS_file_id, IDS_link_name);
                hdf5_utils.deleteIDSFile(IDSpulseFile);
            }
            opened_IDS_files[IDS_link_name] = -1;
        }
        else {
            if (exists(IDSpulseFile.c_str())) 
                hdf5_utils.deleteIDSFile(IDSpulseFile);
        }
    }
    H5Gclose(IDS_group_id);
    IDS_group_id = -1;
}

void HDF5Writer::read_homogeneous_time(int* homogenenous_time) {
	if (IDS_group_id == -1) {
		*homogenenous_time = -1;
		return;
	}
	const char* dataset_name = "ids_properties&homogeneous_time";
	int datatype = ualconst::integer_data;
	hid_t dataset_id = H5Dopen2(IDS_group_id, dataset_name, H5P_DEFAULT);
	herr_t status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, homogenenous_time);
	if (status < 0)
	   *homogenenous_time = -1;
	if (dataset_id >=0)
	   H5Dclose(dataset_id);
}

void HDF5Writer::open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, 
hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, hid_t *loc_id)
{
    HDF5Utils hdf5_utils;
    hdf5_utils.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, &IDS_group_id);
    *loc_id = IDS_group_id;
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
                hdf5_utils.openIDSFile(ctx, IDSpulseFile, &IDS_file_id, false);
            }
        
        opened_IDS_files[IDS_link_name] = IDS_file_id;

    } else {
        IDS_file_id = opened_IDS_files[IDS_link_name];
        if (IDS_file_id == -1) { //file not opened
            if (!exists(IDSpulseFile.c_str())) {
                hdf5_utils.createIDSFile(ctx, IDSpulseFile, backend_version, &IDS_file_id);
            }
            else {
                hdf5_utils.openIDSFile(ctx, IDSpulseFile, &IDS_file_id, false);
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

    if (IDS_group_id >= 0) {
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
    HDF5Utils hdf5_utils;
    auto it_ds = opened_data_sets.begin ();
    while (it_ds != opened_data_sets.end ())
    {
      HDF5DataSetHandler &dh = *(it_ds->second);
      dh.close();
      it_ds++;
    }
    opened_data_sets.clear();
    existing_data_sets.clear();
}

void HDF5Writer::close_group()
{
    if (IDS_group_id >= 0) {
        H5Gclose(IDS_group_id);
        IDS_group_id = -1;
    }
}

int HDF5Writer::readTimedAOSShape(hid_t loc_id)
{
    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";
    if (existing_data_sets.find(tensorized_path) == existing_data_sets.end())   //optimization
    {
        if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {
            existing_data_sets[tensorized_path] = 1;
            return readTimedAOSShape(loc_id, tensorized_path);
        }
        else {
            existing_data_sets[tensorized_path] = 0;
            return 0;
        }
    }
    else {
        if (existing_data_sets[tensorized_path] == 1) {
            return readTimedAOSShape(loc_id, tensorized_path);
        }
        else {
            return 0;
        }
    }
    return readTimedAOSShape(loc_id, tensorized_path);
}

int HDF5Writer::readTimedAOSShape(hid_t loc_id, std::string &tensorized_path)
{
    int shape = 0;
    hid_t dataset_id = -1;
    std::unique_ptr < HDF5DataSetHandler > new_data_set(new HDF5DataSetHandler(false));
    new_data_set-> open(tensorized_path.c_str(), IDS_group_id, &dataset_id, 1, nullptr, ualconst::integer_data, true, true, false);
    dataset_id = new_data_set->dataset_id;
    int dim = -1;
    int *AOS_shapes = nullptr;
    HDF5HsSelectionReader hsSelectionReader(new_data_set->getRank(), dataset_id, 
            new_data_set->getDataSpace(), new_data_set->getLargestDims(), ualconst::integer_data, current_arrctx_indices.size(), &dim);
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
    return shape;
}

void HDF5Writer::beginWriteArraystructAction(ArraystructContext * ctx, int *size)
{
    assert(IDS_group_id >= 0);
    HDF5Utils hdf5_utils;
    hdf5_utils.setTensorizedPaths(ctx, tensorized_paths);
	int AOS_timed_shape = 0;
    if (slice_mode == SLICE_OP && ctx->getTimed()) {
        AOS_timed_shape = readTimedAOSShape(IDS_group_id);
		dynamic_AOS_slices_extension = *size;
		current_arrctx_indices.push_back(ctx->getIndex());
		current_arrctx_shapes.push_back(AOS_timed_shape);
    } else {
        current_arrctx_indices.push_back(ctx->getIndex());
        current_arrctx_shapes.push_back(*size);
    }
    int timed_AOS_index = -1;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices
    createOrUpdateAOSShapesDataSet(ctx, IDS_group_id, AOS_timed_shape);
}


void HDF5Writer::write_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, int dim, int *size, void *data)
{
    std::string & dataset_name = att_name;
    std::replace(dataset_name.begin(), dataset_name.end(), '/', '&');   // character '/' is not supported in datasets names
    std::replace(timebasename.begin(), timebasename.end(), '/', '&');

    assert(IDS_group_id >= 0);

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

    hid_t dataset_id = -1;
    bool dataSetAlreadyOpened = false;
    auto got = opened_data_sets.find(tensorized_path);
    if (got != opened_data_sets.end()) {
        dataSetAlreadyOpened = true;
        const HDF5DataSetHandler &dh = *(got->second);
        dataset_id =  dh.dataset_id;
    }

    int initial_size[H5S_MAX_RANK];
    for (int i = 0; i < dim; i++)
        initial_size[i] = size[i];

    std::vector < int >AOSShapes;
	AOSShapes.assign(current_arrctx_shapes.begin(), current_arrctx_shapes.end());
	bool shapes_dataset = false;

    //std::cout << "Writing data set: " << tensorized_path.c_str() << std::endl;

    std::unique_ptr < HDF5DataSetHandler > data_set;

    if (slice_mode != SLICE_OP) {

		//std::cout << "WRITER NOT IN SLICE MODE!!! " << std::endl;
        if (dataset_id < 0)     //in global mode (put), data set not yet created
        {
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true));
            dataSetHandler->setNonSliceMode();
            bool create_chunk_cache = true;
            dataSetHandler->create(tensorized_path.c_str(), &dataset_id, datatype, IDS_group_id, dim, size, AOSRank, AOSShapes.data(), false, create_chunk_cache, compression_enabled, useBuffering);
            data_set = std::move(dataSetHandler);
        } else {
            data_set = std::move(got->second);
            opened_data_sets.erase(got);
            data_set->setNonSliceMode();
			data_set->setCurrentShapesAndExtend(size, AOSShapes.data());
        }
    } else {
        //std::cout << "WRITER IN SLICE MODE!!! " << std::endl;

        if (dataset_id < 0)     //in slice mode, data set does not exist or not yet opened
        {
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true));
            dataSetHandler->setSliceMode(ctx);
            bool create_chunk_cache = true;
			dataSetHandler->open(tensorized_path.c_str(), IDS_group_id, &dataset_id, dim, size, datatype, shapes_dataset, create_chunk_cache, useBuffering);
            dataSetHandler->storeInitialDims(); //store the dims into initial_dims at beginning of the put_slice
			dataSetHandler->extendDataSpaceForTimeSlices(size, AOSShapes.data(), dynamic_AOS_slices_extension);
			dataSetHandler->setTimeAxisOffset(current_arrctx_indices, dynamic_AOS_slices_extension);
            data_set = std::move(dataSetHandler);
        } else {
            data_set = std::move(got->second);
            opened_data_sets.erase(got);
            data_set->setSliceMode(ctx);
            data_set->updateTimeAxisOffset(current_arrctx_indices);
			data_set->setCurrentShapesAndExtend(size, AOSShapes.data());
        }
    }

    char **p = nullptr;
    int number_of_copies = 0;
    if (datatype == ualconst::char_data) {
        if (dim == 1) {
            p = (char **) malloc(sizeof(char *));
            p[0] = (char *) data;
            char *s = p[0];
            char* char_copy = (char *) malloc(initial_size[0] + 1);
            strncpy(char_copy, s, initial_size[0]);
            char_copy[initial_size[0]] = 0;
            p[0] = char_copy;
            data = p;
            number_of_copies++;
        } else {
            p = (char **) malloc(sizeof(char *) * initial_size[0]);
            char *c = (char *) data;
            for (int i = 0; i < initial_size[0]; i++) {
                char *s = (char *) (c + i * initial_size[1]);
                char *copy = (char *) malloc(initial_size[1] + 1);
                strncpy(copy, s, initial_size[1]);
                copy[initial_size[1]] = 0;
                p[i] = copy;
                number_of_copies++;
            }
            data = p;
        }
    }

    if (useBuffering) {
        data_set->appendToBuffer(current_arrctx_indices, dataSetAlreadyOpened, datatype, dim, slice_mode, dynamic_AOS_slices_extension, p, data);
    }
    else {
        data_set->writeUsingHyperslabs(current_arrctx_indices, slice_mode, dynamic_AOS_slices_extension, data);
        }

    if ((datatype != ualconst::char_data && dim > 0)
        || (datatype == ualconst::char_data && dim == 2))
        createOrUpdateShapesDataSet(ctx, IDS_group_id, tensorized_path, *data_set, timebasename, timed_AOS_index);
    
    if (p != nullptr) {
        for (int i = 0; i < number_of_copies; i++) {
            if (p[i])
                free(p[i]);
        }
        free(p);
    }
    data_set->requests_shapes.push_back(data_set->getDimsAsVector());
    opened_data_sets[tensorized_path] = std::move(data_set);
}

void HDF5Writer::write_buffers() {

    if (!useBuffering) return;

    auto it_ds = opened_data_sets.begin ();

    while (it_ds != opened_data_sets.end ())
    {
      HDF5DataSetHandler &data_set = *(it_ds->second);
      data_set.write_buffers();
      it_ds++;
    }
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

    HDF5Utils hdf5_utils;

    bool shapes_dataset = true;
    bool dataSetAlreadyOpened = false;
    auto got = opened_data_sets.find(tensorized_path);
    if (got != opened_data_sets.end()) {
        dataSetAlreadyOpened = true;
        const HDF5DataSetHandler &dh = *(got->second);
        dataset_id =  dh.dataset_id;
    }
    
    std::unique_ptr < HDF5DataSetHandler > data_set;

    if (dataset_id < 0)         //dataset not yet created in GLOBAL_OP or not yet opened in SLICE_OP
    {
        std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true));
        data_set = std::move(dataSetHandler);

		const hsize_t* dataspace_dims = fieldHandler.getDims();

		for (int i = 0; i < rank - AOSRank; i++) {
            if (slice_mode == SLICE_OP && i == 0)
                shapes[i] = dataspace_dims[i + AOSRank] + fieldHandler.getTimeWriteOffset();
            else
			    shapes[i] = dataspace_dims[i + AOSRank];
		}

        if (slice_mode == SLICE_OP) {
            data_set->setSliceMode(ctx);
            bool create_chunk_cache = true;
			data_set->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, ualconst::integer_data, shapes_dataset, create_chunk_cache, useBuffering);
            data_set->storeInitialDims();
			data_set->extendDataSpaceForTimeSlices(size, aos_shapes.data(), dynamic_AOS_slices_extension);
			data_set->setTimeAxisOffset(current_arrctx_indices, dynamic_AOS_slices_extension);
			
        } else {
            data_set->setNonSliceMode();
            bool create_chunk_cache = true;
			data_set->create(tensorized_path.c_str(), &dataset_id, ualconst::integer_data, loc_id, dim, size, AOSRank, aos_shapes.data(), shapes_dataset, create_chunk_cache, compression_enabled, useBuffering);
        }

        if (useBuffering && slice_mode != SLICE_OP) {
            data_set->appendToBuffer(current_arrctx_indices, dataSetAlreadyOpened, ualconst::integer_data, 1, slice_mode, dynamic_AOS_slices_extension, nullptr, shapes);
        }
        else {
             data_set->writeUsingHyperslabs(current_arrctx_indices, slice_mode, dynamic_AOS_slices_extension, shapes);
        }

    } else  //dataset already used in previous LL request
    {
        data_set = std::move(got->second);
        opened_data_sets.erase(got);
        if (slice_mode == SLICE_OP) {
            data_set->setSliceMode(ctx);
            data_set->updateTimeAxisOffset(current_arrctx_indices);
            data_set->setCurrentShapesAndExtend(size, aos_shapes.data());
        } else {
            data_set->setNonSliceMode();
			data_set->setCurrentShapesAndExtend(size, aos_shapes.data());
        }
	
        const hsize_t* dataspace_dims = fieldHandler.getDims();

		for (int i = 0; i < rank - AOSRank; i++) {
			 if (slice_mode == SLICE_OP && i == 0)
                shapes[i] = dataspace_dims[i + AOSRank] + fieldHandler.getTimeWriteOffset();
            else
                shapes[i] = dataspace_dims[i + AOSRank];
		}

        if (useBuffering && slice_mode != SLICE_OP) {
            data_set->appendToBuffer(current_arrctx_indices, dataSetAlreadyOpened, ualconst::integer_data, 1, slice_mode, dynamic_AOS_slices_extension, nullptr, shapes);
        }
        else {
            data_set->writeUsingHyperslabs(aos_indices, slice_mode, dynamic_AOS_slices_extension, shapes);
        }
    }
    data_set->requests_shapes.push_back(data_set->getDimsAsVector());
    opened_data_sets[tensorized_path] = std::move(data_set);
    free(shapes);
    return dataset_id;
}

void HDF5Writer::createOrUpdateAOSShapesDataSet(ArraystructContext * ctx, hid_t loc_id, int timedAOS_shape)
{

    std::unique_ptr < HDF5DataSetHandler > data_set;

    int dim = 1;                //AOS_SHAPE is a 1D dataset 
    int size[1] = { 1 };
    int shapes[1];              //buffer which will be written, previous size before extension
    shapes[0] = current_arrctx_shapes.back();

    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";

	//std::cout << "Writing data set for AOS shape: " << tensorized_path.c_str() << std::endl;

    int timed_AOS_index = -1;
	HDF5Utils hdf5_utils;
    hdf5_utils.isTimed(ctx, &timed_AOS_index);

    hid_t dataset_id = -1;
	bool shapes_dataset = true;

    std::vector < int >aos_indices(current_arrctx_indices.begin(), current_arrctx_indices.end() - 1);
    std::vector < int >aos_shapes(current_arrctx_shapes.begin(), current_arrctx_shapes.end() - 1);

    bool dataSetAlreadyOpened = false;

    if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {  //not yet used by a previous LL request

        auto got = opened_data_sets.find(tensorized_path);
            if (got != opened_data_sets.end()) {
            dataSetAlreadyOpened = true;
            const HDF5DataSetHandler &dh = *(got->second);
            dataset_id =  dh.dataset_id;
        }

        if (dataset_id < 0) { //this code is called once at the beginning of a put_slice

            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true));
            bool create_chunk_cache = true;
			if (slice_mode == SLICE_OP) {
            	dataSetHandler->setSliceMode(ctx);
                if (ctx->getTimed()) {
                    dataSetHandler->setTimedAOSShape(timedAOS_shape);
                    shapes[0] = timedAOS_shape + dynamic_AOS_slices_extension;
                }
				dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, ualconst::integer_data, shapes_dataset, create_chunk_cache, useBuffering); //dataset extension occurs
				dataSetHandler->extendDataSpaceForTimeSlicesForAOSDataSet(size, aos_shapes.data(), dynamic_AOS_slices_extension);
				dataSetHandler->setTimeAxisOffsetForAOSDataSet();
			} else {
				dataSetHandler->setNonSliceMode();
				dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, ualconst::integer_data, shapes_dataset, create_chunk_cache, useBuffering);
				dataSetHandler->setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			}
			
            if (useBuffering && slice_mode != SLICE_OP) {
                dataSetHandler->appendToBuffer(aos_indices, dataSetAlreadyOpened, ualconst::integer_data, 1, slice_mode, dynamic_AOS_slices_extension, nullptr, shapes);
            }
            else {
                dataSetHandler->writeUsingHyperslabs(aos_indices, slice_mode, dynamic_AOS_slices_extension, shapes);
            }
            data_set = std::move(dataSetHandler);
        }
        else {
            data_set = std::move(got->second);
            opened_data_sets.erase(got);
			if (slice_mode == SLICE_OP) {
            	data_set->setSliceMode(ctx);
                if (ctx->getTimed()) {
                    shapes[0] = data_set->getTimedAOSShape() + dynamic_AOS_slices_extension;
                }
				data_set->setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			} else {
				data_set->setNonSliceMode();
				data_set->setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			}

            if (useBuffering && slice_mode != SLICE_OP) {
                data_set->appendToBuffer(aos_indices, dataSetAlreadyOpened, ualconst::integer_data, 1, slice_mode, dynamic_AOS_slices_extension, nullptr, shapes);
                
            }
            else {
                data_set->writeUsingHyperslabs(aos_indices, slice_mode, dynamic_AOS_slices_extension, shapes);
            }
        }
    }
    else {//AOS_SHAPE doesn't exist yet, we create it, this code is used only on a put() operation, an error is raised otherwise

		std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true));

		if (slice_mode == SLICE_OP) {
			char error_message[200];
            sprintf(error_message, "Inconsistent state detected in slice mode. The AOS shapes dataset: %s is not present in the HDF5 file.\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
		} else {
			dataSetHandler->setNonSliceMode();
		}

        hid_t dataset_id = -1;
        bool create_chunk_cache = true;
		dataSetHandler->create(tensorized_path.c_str(), &dataset_id, ualconst::integer_data, loc_id, dim, size, aos_shapes.size(), aos_shapes.data(), shapes_dataset, create_chunk_cache, compression_enabled, useBuffering);
        if (useBuffering && slice_mode != SLICE_OP) {
                dataSetHandler->appendToBuffer(aos_indices, dataSetAlreadyOpened, ualconst::integer_data, 1, slice_mode, dynamic_AOS_slices_extension, nullptr, shapes);
            }
        else {
            dataSetHandler->writeUsingHyperslabs(aos_indices, slice_mode, dynamic_AOS_slices_extension, shapes);
        }
        data_set = std::move(dataSetHandler);
    }
    data_set->requests_shapes.push_back(data_set->getDimsAsVector());
    opened_data_sets[tensorized_path] = std::move(data_set);
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

