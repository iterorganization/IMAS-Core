#include "hdf5_writer.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "al_defs.h"
#include "hdf5_hs_selection_reader.h"
#include "hdf5_hs_selection_writer.h"
#include "al_backend.h"
#include <boost/filesystem.hpp>


using namespace boost::filesystem;

HDF5Writer::HDF5Writer(std::string backend_version_)
:  backend_version(backend_version_), opened_data_sets(), existing_data_sets(), tensorized_paths_per_context(), arrctx_shapes_per_context(), 
dynamic_AOS_slices_extension(), homogeneous_time(-1), IDS_group_id(), slice_mode(GLOBAL_OP)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Writer::~HDF5Writer()
{
}

bool HDF5Writer::compression_enabled = true;
bool HDF5Writer::useBuffering = true;
size_t HDF5Writer::read_chunk_cache_size = READ_CHUNK_CACHE_SIZE;
size_t HDF5Writer::write_chunk_cache_size = WRITE_CHUNK_CACHE_SIZE;

void HDF5Writer::closePulse(DataEntryContext * ctx, int mode, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path)
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
    auto got = opened_IDS_files.find(external_link_name);
    if (got != opened_IDS_files.end()) {
        pulse_file_id = got->second;
        if (pulse_file_id != -1) {
            HDF5Utils hdf5_utils;
		    /*std::cout << "WRITER:close_file_handler :showing status for pulse file ..." << std::endl;    
	        hdf5_utils.showStatus(pulse_file_id);*/
            hdf5_utils.closeIDSFile(pulse_file_id, external_link_name);
            opened_IDS_files[external_link_name] = -1;
        }
    }
}

void HDF5Writer::deleteData(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    assert(file_id != -1); //the master file is assumed to be opened

    hid_t gid = -1;
    auto got = IDS_group_id.find(ctx);
    if (got != IDS_group_id.end())
        gid = got->second;

    if (gid == -1)
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
    close_group(ctx);
}

void HDF5Writer::read_homogeneous_time(int* homogenenous_time, hid_t gid) {

	if (gid == -1) {
		*homogenenous_time = -1;
		return;
	}
	const char* dataset_name = "ids_properties&homogeneous_time";
	hid_t dataset_id = H5Dopen2(gid, dataset_name, H5P_DEFAULT);
	herr_t status = H5Dread(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, homogenenous_time);
	if (status < 0)
	   *homogenenous_time = -1;
	if (dataset_id >=0)
	   H5Dclose(dataset_id);
}

void HDF5Writer::open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, 
hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, hid_t *gid)
{
    HDF5Utils hdf5_utils;
    hdf5_utils.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, gid);
    if (*gid >= 0)
        IDS_group_id[ctx] = *gid;
}

void HDF5Writer::create_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path, int access_mode)
{
    HDF5Utils hdf5_utils;

    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    
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
            throw ALBackendException(error_message, LOG);
        }
    }
    close_group(ctx);
    hid_t loc_id = hdf5_utils.createOrOpenHDF5Group(ctx->getDataobjectName().c_str(), IDS_file_id);
    assert(loc_id >= 0);
    IDS_group_id[ctx] = loc_id;
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

void HDF5Writer::close_group(OperationContext *ctx)
{
    hid_t gid = -1;
    auto got = IDS_group_id.find(ctx);
    if (got != IDS_group_id.end()) {
        gid = got->second;
        IDS_group_id.erase(ctx);
    }
    
    if (gid >= 0) {
        herr_t status = H5Gclose(gid);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to close HDF5 group: %ld\n", gid);
            throw ALBackendException(error_message, LOG);
        }
    }
        
}

int HDF5Writer::readTimedAOSShape(Context * ctx, hid_t loc_id, const std::vector < int > &current_arrctx_indices)
{
    if (ctx->getType() != CTX_ARRAYSTRUCT_TYPE) return 0;
    ArraystructContext *dynamic_ctx = getDynamicAOS(ctx);
    if (!dynamic_ctx) return 0;

    auto &tensorized_paths = tensorized_paths_per_context[dynamic_ctx];
    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";
    if (existing_data_sets.find(tensorized_path) == existing_data_sets.end())   //optimization
    {
        if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {
            existing_data_sets[tensorized_path] = 1;
            return readTimedAOSShape(loc_id, tensorized_path, current_arrctx_indices, dynamic_ctx->getDataEntryContext()->getURI());
        }
        else {
            existing_data_sets[tensorized_path] = 0;
            return 0;
        }
    }
    else {
        if (existing_data_sets[tensorized_path] == 1) {
            return readTimedAOSShape(loc_id, tensorized_path, current_arrctx_indices, dynamic_ctx->getDataEntryContext()->getURI());
        }
        else {
            return 0;
        }
    }
    return readTimedAOSShape(loc_id, tensorized_path, current_arrctx_indices, dynamic_ctx->getDataEntryContext()->getURI());
}

int HDF5Writer::readTimedAOSShape(hid_t loc_id, std::string &tensorized_path, const std::vector < int > &current_arrctx_indices, uri::Uri uri)
{
    int shape = 0;
    hid_t dataset_id = -1;
    std::unique_ptr < HDF5DataSetHandler > new_data_set(new HDF5DataSetHandler(false, uri));
    new_data_set-> open(tensorized_path.c_str(), loc_id, &dataset_id, 1, nullptr, alconst::integer_data, true, true, uri);
    dataset_id = new_data_set->dataset_id;
    int dim = -1;
    int *AOS_shapes = nullptr;
    HDF5HsSelectionReader hsSelectionReader(new_data_set->getRank(), dataset_id, 
            new_data_set->getDataSpace(), new_data_set->getLargestDims(), alconst::integer_data, current_arrctx_indices.size(), &dim);
    hsSelectionReader.allocateGlobalOpBuffer((void **) &AOS_shapes);
    hsSelectionReader.setHyperSlabsGlobalOp(current_arrctx_indices);
    herr_t status = H5Dread(dataset_id, hsSelectionReader.dtype_id,
                            hsSelectionReader.memspace,
                            hsSelectionReader.dataspace, H5P_DEFAULT,
                            AOS_shapes);
    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to read timed AOS_SHAPE: %s\n", tensorized_path.c_str());
        throw ALBackendException(error_message, LOG);
    }
    H5Dclose(dataset_id);
    shape = AOS_shapes[0];
    free(AOS_shapes);
    return shape;
}

void HDF5Writer::beginWriteArraystructAction(ArraystructContext * ctx, int *size)
{
    HDF5Utils hdf5_utils;
    OperationContext *opctx = ctx->getOperationContext();
    hid_t gid = -1;
    auto got_gid = IDS_group_id.find(opctx);
    if (got_gid != IDS_group_id.end())
        gid = got_gid->second;

    assert(gid >= 0);
    //std::cout << "Preparing AOS: " << ctx->getPath().c_str() << std::endl;
    auto got = tensorized_paths_per_context.find(ctx);
    if (got == tensorized_paths_per_context.end()) {
       std::vector < std::string > tensorized_paths;
       if (ctx->getParent() != NULL)
	 tensorized_paths = tensorized_paths_per_context[ctx->getParent()];
       hdf5_utils.setTensorizedPaths(ctx, tensorized_paths);
       tensorized_paths_per_context[ctx] = tensorized_paths;
    }
    
    auto got_arrctx_shapes = arrctx_shapes_per_context.find(ctx);
    if (got_arrctx_shapes == arrctx_shapes_per_context.end()) {
      std::vector<int> arrctx_shapes;
      if (ctx->getParent() != NULL)
        arrctx_shapes = arrctx_shapes_per_context[ctx->getParent()];
      arrctx_shapes.push_back(*size);
      std::pair<ArraystructContext*, std::vector<int>> p(ctx, arrctx_shapes);
      std::pair<std::unordered_map < ArraystructContext *,  std::vector<int>>::iterator, bool> res;
      res = arrctx_shapes_per_context.insert(p);
      got_arrctx_shapes = res.first;
    }
    
    int timed_AOS_index = -1;
    std::vector < int > current_arrctx_indices;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices
    
    int AOS_timed_shape = 0;
    std::vector<int> &arrctx_shapes = (*got_arrctx_shapes).second;
    if (slice_mode == SLICE_OP && ctx->getTimed()) {
        AOS_timed_shape = readTimedAOSShape(ctx, gid, current_arrctx_indices);
	    dynamic_AOS_slices_extension[ctx] = *size;
        arrctx_shapes.back() = AOS_timed_shape;
        //printf("AOS_timed_shape=%d, size=%d, for path:%s\n", AOS_timed_shape, *size, ctx->getPath().c_str());
    } else {
        arrctx_shapes.back() = *size;
        //printf("size=%d, for path:%s\n", *size, ctx->getPath().c_str());
    }
    
    createOrUpdateAOSShapesDataSet(ctx, gid, AOS_timed_shape, current_arrctx_indices, arrctx_shapes);
}

ArraystructContext* HDF5Writer::getDynamicAOS(Context * ctx) {
    ArraystructContext * timed_ctx = dynamic_cast<ArraystructContext*> (ctx);
    while(timed_ctx != NULL) {
        if (timed_ctx->getTimed()) return timed_ctx;
        timed_ctx = timed_ctx->getParent();
    }
    return NULL;
}


void HDF5Writer::write_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, int dim, int *size, void *data)
{
    std::string & dataset_name = att_name;
    std::replace(dataset_name.begin(), dataset_name.end(), '/', '&');   // character '/' is not supported in datasets names
    std::replace(timebasename.begin(), timebasename.end(), '/', '&');

    OperationContext *opctx = nullptr;
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        opctx = (static_cast<ArraystructContext*> (ctx))->getOperationContext();
    }
    else {
        opctx = static_cast<OperationContext*> (ctx);
    }
    DataEntryContext *dec = opctx->getDataEntryContext();
    hid_t gid = -1;
    auto got_gid = IDS_group_id.find(opctx);
    if (got_gid != IDS_group_id.end())
        gid = got_gid->second;

    assert(gid >= 0);

    if (dataset_name == "ids_properties&homogeneous_time") {
        int *v = (int *) data;
        homogeneous_time = v[0];
    }

    HDF5Utils hdf5_utils;
    int timed_AOS_index = -1;
    std::vector < int > current_arrctx_indices;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices    

    int AOSRank = current_arrctx_indices.size();
    std::string tensorized_path = dataset_name;

    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
      auto &tensorized_paths = tensorized_paths_per_context[static_cast<ArraystructContext*> (ctx)];
      tensorized_path = tensorized_paths.back() + "&" + dataset_name;
    }

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

    std::vector<int> arrctx_shapes;
    
    auto got_arrctx_shapes = arrctx_shapes_per_context.find(static_cast<ArraystructContext*> (ctx));
    
    if (got_arrctx_shapes != arrctx_shapes_per_context.end()) 
      arrctx_shapes = (*got_arrctx_shapes).second;
    
    bool shapes_dataset = false;

    //std::cout << "-->Writing data set: " << tensorized_path.c_str() << std::endl;

    std::unique_ptr < HDF5DataSetHandler > data_set;
    
    int time_vector_length = 0;
    if (timed_AOS_index == -1 && dim > 0)
       time_vector_length = size[dim - 1];
    int slices_extension = getDynamic_slices_extension(ctx, timed_AOS_index, time_vector_length);

    if (slice_mode != SLICE_OP) {

		//std::cout << "WRITER NOT IN SLICE MODE!!! " << std::endl;
        if (dataset_id < 0)     //in global mode (put), data set not yet created
        {
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true, dec->getURI()));
            dataSetHandler->setNonSliceMode();
            bool create_chunk_cache = true;
            dataSetHandler->create(tensorized_path.c_str(), &dataset_id, datatype, gid, dim, size, AOSRank, arrctx_shapes.data(), false, create_chunk_cache);
            data_set = std::move(dataSetHandler);
        } else {
            data_set = std::move(got->second);
            opened_data_sets.erase(got);
            data_set->setNonSliceMode();
	        data_set->setCurrentShapesAndExtend(size, arrctx_shapes.data());
        }
    } else {
        //std::cout << "WRITER IN SLICE MODE!!! " << std::endl;

        if (dataset_id < 0)     //in slice mode, data set does not exist or not yet opened
        {
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true, dec->getURI()));
            dataSetHandler->setSliceMode(ctx);
            bool create_chunk_cache = true;

            if (H5Lexists(gid, tensorized_path.c_str(), H5P_DEFAULT) == 0) {
                int timedAOSShape = readTimedAOSShape(ctx, gid, current_arrctx_indices);
                if (timedAOSShape != 0)
                    arrctx_shapes[timed_AOS_index] = timedAOSShape - slices_extension;
                dataSetHandler->create(tensorized_path.c_str(), &dataset_id, datatype, gid, dim, size, AOSRank, arrctx_shapes.data(), shapes_dataset, create_chunk_cache);
            }
            else {
	            dataSetHandler->open(tensorized_path.c_str(), gid, &dataset_id, dim, size, datatype, shapes_dataset, create_chunk_cache, dec->getURI(), AOSRank, arrctx_shapes.data());
            }
            dataSetHandler->storeInitialDims(); //store the dims into initial_dims at beginning of the put_slice
	        dataSetHandler->extendDataSpaceForTimeSlices(size, arrctx_shapes.data(), slices_extension);
	        dataSetHandler->setTimeAxisOffset(current_arrctx_indices, slices_extension);
            data_set = std::move(dataSetHandler);
        } else {
            data_set = std::move(got->second);
            opened_data_sets.erase(got);
            data_set->setSliceMode(ctx);
            data_set->updateTimeAxisOffset(current_arrctx_indices);
	        data_set->setCurrentShapesAndExtend(size, arrctx_shapes.data());
        }
    }

    char **p = nullptr;
    int number_of_copies = 0;
    if (datatype == alconst::char_data) {
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
    
    
    if (data_set->useBuffering) {
        
        data_set->appendToBuffer(current_arrctx_indices, dataSetAlreadyOpened, datatype, dim, slice_mode, slices_extension, p, data);
    }
    else {
        data_set->writeUsingHyperslabs(current_arrctx_indices, slice_mode, slices_extension, data);
        }

    if ((datatype != alconst::char_data && dim > 0) || (datatype == alconst::char_data && dim == 2)) {
        createOrUpdateShapesDataSet(ctx, gid, tensorized_path, *data_set, timebasename, timed_AOS_index, current_arrctx_indices, arrctx_shapes);
    }
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

    auto it_ds = opened_data_sets.begin ();

    while (it_ds != opened_data_sets.end ())
    {
      HDF5DataSetHandler &data_set = *(it_ds->second);
      if (data_set.useBuffering) 
		  data_set.write_buffers();
      it_ds++;
    }
}


hid_t HDF5Writer::createOrUpdateShapesDataSet(Context * ctx, hid_t loc_id, const std::string & field_tensorized_path, HDF5DataSetHandler & fieldHandler, 
std::string & timebasename, int timed_AOS_index, const std::vector < int > &current_arrctx_indices, const std::vector < int > &arrctx_shapes)
{
    hid_t dataset_id = -1;
    int AOSRank = current_arrctx_indices.size();
	int rank = fieldHandler.getRank();

    if (AOSRank == 0 || (rank == AOSRank) )
        return dataset_id;

    const std::string & tensorized_path = field_tensorized_path + "_SHAPE";

	//std::cout << "Writing shapes data set: " << tensorized_path << std::endl;

    int dim = 1;                //SHAPE is a 1D dataset
    int size[1] = { rank - AOSRank };   //length of the shapes vector
    int *shapes = (int *) malloc(sizeof(int) * size[0]);

    std::vector < int >aos_indices(current_arrctx_indices.begin(), current_arrctx_indices.end());
    std::vector < int >aos_shapes(arrctx_shapes.begin(), arrctx_shapes.end());

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
    
    int slices_extension;
    std::unique_ptr < HDF5DataSetHandler > data_set;

    OperationContext *opctx = nullptr;
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        opctx = (static_cast<ArraystructContext*> (ctx))->getOperationContext();
    }
    else {
        opctx = static_cast<OperationContext*> (ctx);
    }
    DataEntryContext *dec = opctx->getDataEntryContext();

    if (dataset_id < 0)         //dataset not yet created in GLOBAL_OP or not yet opened in SLICE_OP
    {
        std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true, dec->getURI()));
        data_set = std::move(dataSetHandler);

		const hsize_t* dataspace_dims = fieldHandler.getDims();

		for (int i = 0; i < rank - AOSRank; i++) {
            if (slice_mode == SLICE_OP && i == 0)
                shapes[i] = dataspace_dims[i + AOSRank] + fieldHandler.getTimeWriteOffset();
            else
			    shapes[i] = dataspace_dims[i + AOSRank];
		}
		
		slices_extension = getDynamic_slices_extension(ctx, timed_AOS_index, shapes[0]);

        if (slice_mode == SLICE_OP) {
            data_set->setSliceMode(ctx);
            bool create_chunk_cache = true;
            bool extendDataSet = H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) != 0;
            
	        data_set->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, alconst::integer_data, 
            shapes_dataset, create_chunk_cache, dec->getURI(), AOSRank, aos_shapes.data());
            data_set->storeInitialDims();

            if (extendDataSet) {
	            data_set->extendDataSpaceForTimeSlices(size, aos_shapes.data(), slices_extension);
	            data_set->setTimeAxisOffset(current_arrctx_indices, slices_extension);
            }
			
        } else {
            data_set->setNonSliceMode();
            bool create_chunk_cache = true;
			data_set->create(tensorized_path.c_str(), &dataset_id, alconst::integer_data, loc_id, dim, size, AOSRank, aos_shapes.data(), shapes_dataset, create_chunk_cache);
        }

        if (data_set->useBuffering && slice_mode != SLICE_OP) {
            data_set->appendToBuffer(current_arrctx_indices, dataSetAlreadyOpened, alconst::integer_data, 1, slice_mode, slices_extension, nullptr, shapes);
        }
        else {
             data_set->writeUsingHyperslabs(current_arrctx_indices, slice_mode, slices_extension, shapes);
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
		
		slices_extension = getDynamic_slices_extension(ctx, timed_AOS_index, shapes[0]);

        if (data_set->useBuffering && slice_mode != SLICE_OP) {
            data_set->appendToBuffer(current_arrctx_indices, dataSetAlreadyOpened, alconst::integer_data, 1, slice_mode, slices_extension, nullptr, shapes);
        }
        else {
            data_set->writeUsingHyperslabs(aos_indices, slice_mode, slices_extension, shapes);
        }
    }
    data_set->requests_shapes.push_back(data_set->getDimsAsVector());
    opened_data_sets[tensorized_path] = std::move(data_set);
    free(shapes);
    return dataset_id;
}

void HDF5Writer::createOrUpdateAOSShapesDataSet(ArraystructContext * ctx, hid_t loc_id, int timedAOS_shape, const std::vector < int > &arrctx_indices, const std::vector < int > &arrctx_shapes)
{

    std::unique_ptr < HDF5DataSetHandler > data_set;

    int dim = 1;                //AOS_SHAPE is a 1D dataset 
    int size[1] = { 1 };
    int shapes[1];              //buffer which will be written, previous size before extension
    shapes[0] = arrctx_shapes.back();
    
    auto &tensorized_paths = tensorized_paths_per_context[ctx];
    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";

	//std::cout << "Writing data set for AOS shape: " << tensorized_path.c_str() << std::endl;

    int timed_AOS_index = -1;
	HDF5Utils hdf5_utils;
    hdf5_utils.isTimed(ctx, &timed_AOS_index);
    
    int slices_extension = getDynamic_AOS_slices_extension(ctx);

    hid_t dataset_id = -1;
	bool shapes_dataset = true;

    std::vector < int >aos_indices(arrctx_indices.begin(), arrctx_indices.end() - 1);
    std::vector < int >aos_shapes(arrctx_shapes.begin(), arrctx_shapes.end() - 1);

    bool dataSetAlreadyOpened = false;

    if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {  //not yet used by a previous LL request

        auto got = opened_data_sets.find(tensorized_path);
            if (got != opened_data_sets.end()) {
            dataSetAlreadyOpened = true;
            const HDF5DataSetHandler &dh = *(got->second);
            dataset_id =  dh.dataset_id;
        }

        if (dataset_id < 0) { //this code is called once at the beginning of a put_slice

            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true, ctx->getDataEntryContext()->getURI()));
            bool create_chunk_cache = true;
			if (slice_mode == SLICE_OP) {
            	dataSetHandler->setSliceMode(ctx);
                if (ctx->getTimed()) {
                    dataSetHandler->setTimedAOSShape(timedAOS_shape);
                    shapes[0] = timedAOS_shape + slices_extension;
                }
				dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, alconst::integer_data, shapes_dataset, create_chunk_cache, ctx->getDataEntryContext()->getURI()); //dataset extension occurs
				dataSetHandler->extendDataSpaceForTimeSlicesForAOSDataSet(size, aos_shapes.data(), slices_extension);
				dataSetHandler->setTimeAxisOffsetForAOSDataSet();
			} else {
				dataSetHandler->setNonSliceMode();
				dataSetHandler->open(tensorized_path.c_str(), loc_id, &dataset_id, dim, size, alconst::integer_data, shapes_dataset, create_chunk_cache, ctx->getDataEntryContext()->getURI());
				dataSetHandler->setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			}
			
            if (dataSetHandler->useBuffering && slice_mode != SLICE_OP) {
                dataSetHandler->appendToBuffer(aos_indices, dataSetAlreadyOpened, alconst::integer_data, 1, slice_mode, slices_extension, nullptr, shapes);
            }
            else {
                dataSetHandler->writeUsingHyperslabs(aos_indices, slice_mode, slices_extension, shapes);
            }
            data_set = std::move(dataSetHandler);
        }
        else {
            data_set = std::move(got->second);
            opened_data_sets.erase(got);
			if (slice_mode == SLICE_OP) {
            	data_set->setSliceMode(ctx);
                if (ctx->getTimed()) {
                    shapes[0] = data_set->getTimedAOSShape() + slices_extension;
                }
				data_set->setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			} else {
				data_set->setNonSliceMode();
				data_set->setCurrentShapesAndExtendForAOSDataSet(size, aos_shapes.data());
			}

            if (data_set->useBuffering && slice_mode != SLICE_OP) {
                data_set->appendToBuffer(aos_indices, dataSetAlreadyOpened, alconst::integer_data, 1, slice_mode, slices_extension, nullptr, shapes);
                
            }
            else {
                data_set->writeUsingHyperslabs(aos_indices, slice_mode, slices_extension, shapes);
            }
        }
    }
    else {//AOS_SHAPE doesn't exist yet, we create it, this code is used only on a put() operation, an error is raised otherwise

		std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(true, ctx->getDataEntryContext()->getURI()));
        //printf("No AOS_SHAPE dataset for %s\n", tensorized_path.c_str());
		if (slice_mode == SLICE_OP) {
            dataSetHandler->setSliceMode(ctx);
            timedAOS_shape = readTimedAOSShape(ctx, loc_id, arrctx_indices);
            //printf("modified timedAOS_shape=%d\n", timedAOS_shape);
            aos_shapes[timed_AOS_index] = timedAOS_shape - slices_extension;
            /*for (size_t i = 0; i < aos_shapes.size(); i++) {
                printf("tensorized_path=%s, aos_shapes[%d]=%d\n", tensorized_path.c_str(), i, aos_shapes[i]);
            }*/

		} else {
			dataSetHandler->setNonSliceMode();
		}

        hid_t dataset_id = -1;
        bool create_chunk_cache = true;
		dataSetHandler->create(tensorized_path.c_str(), &dataset_id, alconst::integer_data, loc_id, dim, size, aos_shapes.size(), aos_shapes.data(), shapes_dataset, create_chunk_cache);
        if (slice_mode == SLICE_OP) {
            	
                if (ctx->getTimed()) {
                    dataSetHandler->setTimedAOSShape(timedAOS_shape);
                    shapes[0] = timedAOS_shape + slices_extension;
                }
				dataSetHandler->extendDataSpaceForTimeSlicesForAOSDataSet(size, aos_shapes.data(), slices_extension);
				dataSetHandler->setTimeAxisOffsetForAOSDataSet();
		}
        
        if (dataSetHandler->useBuffering && slice_mode != SLICE_OP) {
                dataSetHandler->appendToBuffer(aos_indices, dataSetAlreadyOpened, alconst::integer_data, 1, slice_mode, slices_extension, nullptr, shapes);
            }
        else {
            dataSetHandler->writeUsingHyperslabs(aos_indices, slice_mode, slices_extension, shapes);
        }
        data_set = std::move(dataSetHandler);
    }
    data_set->requests_shapes.push_back(data_set->getDimsAsVector());
    opened_data_sets[tensorized_path] = std::move(data_set);
}

int HDF5Writer::getDynamic_slices_extension(Context *ctx, int timed_AOS_index, int time_vector_length) {
  if (timed_AOS_index != -1) {
	  return getDynamic_AOS_slices_extension(ctx);
  }
  else {
	  return time_vector_length;
  }
}

int HDF5Writer::getDynamic_AOS_slices_extension(Context *ctx) {
   auto got = dynamic_AOS_slices_extension.find(static_cast<ArraystructContext*> (ctx));
   if (got != dynamic_AOS_slices_extension.end())
	return (*got).second;
   return 1;
}

void HDF5Writer::endAction(Context * ctx)
{
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
      auto arrctx_shapes_got = arrctx_shapes_per_context.find(static_cast<ArraystructContext*> (ctx));
      if (arrctx_shapes_got != arrctx_shapes_per_context.end())
        arrctx_shapes_per_context.erase(arrctx_shapes_got);
      auto got = tensorized_paths_per_context.find(static_cast<ArraystructContext*> (ctx));
      if (got != tensorized_paths_per_context.end())
        tensorized_paths_per_context.erase(got);
      auto got_dynamic_AOS_slices_extension = dynamic_AOS_slices_extension.find(static_cast<ArraystructContext*> (ctx));
      if (got_dynamic_AOS_slices_extension != dynamic_AOS_slices_extension.end())
        dynamic_AOS_slices_extension.erase(got_dynamic_AOS_slices_extension);
    }
}

void HDF5Writer::setSliceMode(int slice_mode) {
	this->slice_mode = slice_mode;
}
