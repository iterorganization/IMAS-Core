#include "hdf5_reader.h"

#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "hdf5_dataset_handler.h"
#include <math.h>
#include <stdlib.h>
#include <limits>

#define MAX_LENGTH 200

HDF5Reader::HDF5Reader(std::string backend_version_)
:  backend_version(backend_version_), opened_data_sets(), opened_shapes_data_sets(), aos_opened_shapes_data_sets(), existing_data_sets(), 
tensorized_paths_per_context(), tensorized_paths_per_op_context(), arrctx_shapes_per_context(), homogeneous_time(-1), IDS_group_id(), slice_mode(GLOBAL_OP)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    INTERPOLATION_WARNING = (std::getenv("IMAS_AL_DISABLE_INTERPOLATION_WARNING") == nullptr);
}

HDF5Reader::~HDF5Reader()
{
}

void HDF5Reader::closePulse(DataEntryContext * ctx, int mode, hid_t *file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path)
{
    close_datasets();
    HDF5Utils hdf5_utils;

    auto it = opened_IDS_files.begin();
    while (it != opened_IDS_files.end()) {
        const std::string & external_link_name = it->first;
        hid_t &pulse_file_id = it->second;
        hdf5_utils.closeIDSFile(pulse_file_id, external_link_name); //closing the IDS file
        it++;
    }
    hdf5_utils.closeMasterFile(file_id);
}

void HDF5Reader::close_datasets()
{
    auto it_ds = opened_data_sets.begin ();
    while (it_ds != opened_data_sets.end ())
    {
      HDF5DataSetHandler &dh = *(it_ds->second);
      dh.close();
      it_ds++;
    }
    opened_data_sets.clear();

	auto it_sds = opened_shapes_data_sets.begin ();
	while (it_sds != opened_shapes_data_sets.end ())
    {
      HDF5DataSetHandler &dh = *(it_sds->second);
      dh.close();
      it_sds++;
    }
	opened_shapes_data_sets.clear();

	auto it_aosds = aos_opened_shapes_data_sets.begin ();
	while (it_aosds != aos_opened_shapes_data_sets.end ())
    {
      HDF5DataSetHandler &dh = *(it_aosds->second);
      dh.close();
      it_aosds++;
    }
	aos_opened_shapes_data_sets.clear();
    existing_data_sets.clear();
}

void HDF5Reader::close_group(OperationContext *ctx)
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


void HDF5Reader::beginReadArraystructAction(ArraystructContext * ctx, int *size)
{
    HDF5Utils hdf5_utils;
    OperationContext *opctx = ctx->getOperationContext();
    hid_t gid = -1;
    auto got_gid = IDS_group_id.find(opctx);
    if (got_gid != IDS_group_id.end())
        gid = got_gid->second;

    if (gid == -1) {   //IDS does not exist in the file
        *size = 0;
        return;
    }
    //std::cout << "beginReadArraystructAction called for: " << ctx->getPath().c_str() << std::endl;
    std::string tensorized_path;
    std::vector < std::string > tensorized_paths;
    
    auto got = tensorized_paths_per_context.find(ctx);
    if (got != tensorized_paths_per_context.end()) {
      auto &tensorized_paths = got->second;
      tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";
    }
    else {
       if (ctx->getParent() != NULL)
	     tensorized_paths = tensorized_paths_per_context[ctx->getParent()];
       hdf5_utils.setTensorizedPaths(ctx, tensorized_paths);
       tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";
    }
    
    if (got == tensorized_paths_per_context.end()) {
			//printf("updating tensorized_paths_per_context\n");
			tensorized_paths_per_context[ctx] = tensorized_paths;
		    tensorized_paths_per_op_context[opctx] = tensorized_paths_per_context[ctx];
	}
    
    if (existing_data_sets.find(tensorized_path) != existing_data_sets.end())   //optimization
    {
        if (existing_data_sets[tensorized_path] == 0) {
            *size = 0;
            return;
        }
    } else {
        if (H5Lexists(gid, tensorized_path.c_str(), H5P_DEFAULT) <= 0) {
            existing_data_sets[tensorized_path] = 0;
            *size = 0;
            return;
        }
        else {
            existing_data_sets[tensorized_path] = 1;
        }
    }

    int timed_AOS_index = -1;
    std::vector < int > current_arrctx_indices;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices
    int *shapes = nullptr;
    bool isTimed = (timed_AOS_index != -1);
    int slice_index = -1;
    OperationContext *opCtx = ctx->getOperationContext();
    slice_mode = opCtx->getRangemode();

    if (slice_mode == SLICE_OP && isTimed) {
			double linear_interpolation_factor = 0;
			int slice_sup = -1;
            bool ignore_linear_interpolation = true;
            std::string time_dataset_name = getTimeVectorDataSetName(timed_AOS_index, tensorized_paths);
            std::unique_ptr < HDF5DataSetHandler > time_data_set = std::move(getTimeVectorDataSet(opCtx, gid, time_dataset_name)); //get time_data_set from the opened_data_sets map if it exists or create it
            if (!time_data_set)
                throw ALBackendException("HDF5Backend: unexpected NULL time_data_set in HDF5Reader::beginReadArraystructAction()", LOG);
            slice_index = getSliceIndex(opCtx, time_data_set, "",&slice_sup, &linear_interpolation_factor, timed_AOS_index, current_arrctx_indices, &ignore_linear_interpolation);
            opened_data_sets[time_dataset_name] = std::move(time_data_set); //move unique_ptr to the std::map
    }
   
    if (readAOSPersistentShapes(ctx, gid, tensorized_path, timed_AOS_index, slice_index, (void **) &shapes, current_arrctx_indices) == 0) {
	   *size = 0;
       return;
    }
    *size = shapes[0];
    //std::cout << "size: " << *size << std::endl;
    free(shapes);

    auto got_arrctx_shapes = arrctx_shapes_per_context.find(ctx);
    if (got_arrctx_shapes != arrctx_shapes_per_context.end()) {
      auto &arrctx_shapes = (*got_arrctx_shapes).second;
      arrctx_shapes.push_back(*size);
    }
    else {
      std::vector<int> arrctx_shapes;
      arrctx_shapes.push_back(*size);
      arrctx_shapes_per_context[ctx] = arrctx_shapes;
    }

    if (slice_mode == SLICE_OP && ctx->getTimed() && *size > 0) {
        *size = 1;
    }
   
}

std::string HDF5Reader::getTimeVectorDataSetName(OperationContext * opCtx, std::string timebasename, int timed_AOS_index) {

    std::string dataset_name;

     if (homogeneous_time == 1) {
            dataset_name = "time";
            return dataset_name;
       }
    
    std::string tensorized_path = timebasename;
    //printf("timebasename = %s\n", timebasename.c_str());

    if (timed_AOS_index != -1) { //that means we are inside an AOS
        if (timebasename.substr(0,1) == "&") {
            dataset_name = timebasename.replace(0,1,"");
        } 
        else{
            auto &tensorized_paths = tensorized_paths_per_op_context[opCtx];
            dataset_name = tensorized_paths[timed_AOS_index] + "&time";
        } 
    } 
    else {
        if (timebasename.substr(0,1) == "&"){
            dataset_name = timebasename.replace(0,1,"");
        } else {
            auto got = tensorized_paths_per_op_context.find(opCtx);
            if (got != tensorized_paths_per_op_context.end()) {
                auto &tensorized_paths = got->second;
                dataset_name = tensorized_paths.back() + "&" + timebasename;
            }
            else {
                dataset_name = timebasename;
            }
            
        }
    }
    //printf("getTimeVectorDataSetName::result=%s\n", dataset_name.c_str());
    return dataset_name;
}

std::string HDF5Reader::getTimeVectorDataSetName(int timed_AOS_index, std::vector < std::string > &tensorized_paths) {

    std::string dataset_name;
     if (homogeneous_time == 1) {
            dataset_name = "time";
            return dataset_name;
    }
    if (timed_AOS_index == -1)
        throw ALBackendException("HDF5Backend: unexpected timed_AOS_index (-1) value in HDF5Reader::getTimeVectorDataSetName()", LOG);
    dataset_name = tensorized_paths[timed_AOS_index] + "&time";
    return dataset_name;
}

std::unique_ptr < HDF5DataSetHandler > HDF5Reader::getTimeVectorDataSet(OperationContext *opCtx, hid_t gid, const std::string & dataset_name) {

    hid_t dataset_id = -1;
    std::unique_ptr < HDF5DataSetHandler > data_set;
    auto got = opened_data_sets.find(dataset_name);
    if (got != opened_data_sets.end()) {
        data_set = std::move(got->second);
        opened_data_sets.erase(got);
        dataset_id = data_set->dataset_id;
    } else {
        std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler(false, opCtx->getDataEntryContext()->getURI()));
        dataSetHandler-> open(dataset_name.c_str(), gid, &dataset_id, 1, nullptr, alconst::double_data, false, true, opCtx->getDataEntryContext()->getURI());
        dataset_id = dataSetHandler->dataset_id;
        data_set = std::move(dataSetHandler);
    }

    if (dataset_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to open dataset: %s for getting time vector.\n", dataset_name.c_str());
        throw ALBackendException(error_message, LOG);
    }
    
    return data_set;
}

int HDF5Reader::getSliceIndex(OperationContext * opCtx, std::unique_ptr < HDF5DataSetHandler > &data_set, const std::string & timebasename, int *slice_sup, 
double *linear_interpolation_factor, int timed_AOS_index, const std::vector < int > &current_arrctx_indices, bool *ignore_linear_interpolation)
{
    double time = opCtx->getTime();
    int interp = opCtx->getInterpmode();

    int dataset_rank = data_set->getRank();
    size_t timeVectorLength = data_set->getMaxShape(dataset_rank - 1);

    double *slices_times = nullptr;
    bool homogeneous_time_basis = homogeneous_time == 1 || timebasename.substr(0,1) == "&" || data_set->getName().compare("time") == 0;

    if (homogeneous_time_basis) {
        slices_times = (double *) malloc(timeVectorLength * sizeof(double));
        herr_t status = H5Dread(data_set->dataset_id, H5T_NATIVE_DOUBLE, H5P_DEFAULT, H5P_DEFAULT,
                                H5P_DEFAULT, (void *) slices_times);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read the homogeneous time basis:%s\n", data_set->getName().c_str());
            throw ALBackendException(error_message, LOG);
        }
    } else {
        // std::cout << "GETTING THE INHOMOGENEOUS TIME BASIS: " << std::endl;
        int dim = -1;
        HDF5HsSelectionReader hsSelectionReader(dataset_rank, data_set->dataset_id, data_set->getDataSpace(), data_set->getLargestDims(), alconst::double_data, current_arrctx_indices.size(), &dim);
        hsSelectionReader.allocateInhomogeneousTimeDataSet((void **) &slices_times, timed_AOS_index);
        hsSelectionReader.setHyperSlabsGlobalOp(current_arrctx_indices, timed_AOS_index, timed_AOS_index!=-1);
        herr_t status = H5Dread(data_set->dataset_id, H5Dget_type(data_set->dataset_id),
                                hsSelectionReader.memspace,
                                hsSelectionReader.dataspace, H5P_DEFAULT,
                                slices_times);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read the inhomogeneous time basis: %s\n", data_set->getName().c_str());
            throw ALBackendException(error_message, LOG);
        }
    }

    *slice_sup = timeVectorLength - 1;
    int slice_inf = 0;
    if (*slice_sup > 0)
        slice_inf = *slice_sup - 1;
    int closest;
    for (size_t i = 0; i < timeVectorLength; i++) {
        //std::cout << "--> slices_times[" << i << "]=" << slices_times[i] << std::endl;
        if (slices_times[i] >= time) {
            *slice_sup = i;
			if (interp != CLOSEST_INTERP) {
				if (*slice_sup == 0 && timeVectorLength > 1)
					*slice_sup = 1;
			}
				if (*slice_sup > 0)
					slice_inf = *slice_sup - 1;
				break;
			
        }
    }
    //std::cout << "--> slice_inf = " << slice_inf << std::endl;
    //std::cout << "--> slice_sup = " << *slice_sup << std::endl;
    switch (interp) {

    case CLOSEST_INTERP:

        if (fabs(time - slices_times[*slice_sup]) <= (fabs(time - slices_times[slice_inf]))) {
            closest = *slice_sup;
        } else {
            closest = slice_inf;
        }
        free(slices_times);
        return closest;
        break;
    case PREVIOUS_INTERP:
        closest = slice_inf;
        if (*slice_sup > 0 && ( (fabs(time - slices_times[*slice_sup]) < std::numeric_limits<double>::epsilon()) 
                              || (time > slices_times[*slice_sup])) )
            closest = *slice_sup;
        free(slices_times);
        return closest;
        break;
    case LINEAR_INTERP:
        if (*slice_sup == slice_inf) {
            *ignore_linear_interpolation = true;
            free(slices_times);
            return slice_inf;
        } else if (time > slices_times[*slice_sup]) {
            *ignore_linear_interpolation = true;
            free(slices_times);
            return *slice_sup;
        } else if (time < slices_times[slice_inf]) {
            *ignore_linear_interpolation = true;
            free(slices_times);
            return slice_inf;
        } else {
            *ignore_linear_interpolation = false;
        }
        *linear_interpolation_factor = (time - slices_times[slice_inf]) / (slices_times[*slice_sup] - slices_times[slice_inf]);
        //std::cout << "linear_interpolation_factor: " << *linear_interpolation_factor << std::endl;
        free(slices_times);
        return slice_inf;
        break;

    default:
        throw ALBackendException("Interpolation mode not yet supported", LOG);
    }
}

int HDF5Reader::read_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, void **data, int *dim, int *size)
{
    OperationContext *opctx = nullptr;
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        opctx = (static_cast<ArraystructContext*> (ctx))->getOperationContext();
    }
    else {
        opctx = static_cast<OperationContext*> (ctx);
    }
    hid_t gid = -1;
    auto got_gid = IDS_group_id.find(static_cast<OperationContext*> (opctx));
    if (got_gid != IDS_group_id.end())
        gid = got_gid->second;

    if (gid == -1) //IDS does not exist in the file
        return 0;

    std::string & dataset_name = att_name;
    std::replace(dataset_name.begin(), dataset_name.end(), '/', '&');   // character '/' is not supported in datasets names
    std::replace(timebasename.begin(), timebasename.end(), '/', '&');

    hid_t dataset_id = -1;
    std::string tensorized_path = dataset_name;
    
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
	    auto &tensorized_paths = tensorized_paths_per_context[static_cast<ArraystructContext*> (ctx)];
            tensorized_path = tensorized_paths.back() + "&" + dataset_name;
     }

    if (existing_data_sets.find(tensorized_path) != existing_data_sets.end())   //optimization
    {
        if (existing_data_sets[tensorized_path] == 0)
            return 0;
    } 
    else 
    { 
        if (H5Lexists(gid, tensorized_path.c_str(), H5P_DEFAULT) <= 0) {
            existing_data_sets[tensorized_path] = 0;
            return 0;
           }
        else {
            existing_data_sets[tensorized_path] = 1;
        }
    }

    //std::cout << "Reading data set: " << tensorized_path.c_str() << std::endl;
    HDF5Utils hdf5_utils;
    int timed_AOS_index = -1;
    std::vector < int > current_arrctx_indices;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices

    bool is_dynamic = timebasename.compare("") != 0 ? true : false;

    bool ignore_linear_interpolation = true;
    int slice_sup = -1;         //used for linear interpolation
    double linear_interpolation_factor = 0;

    bool isTimed = (timed_AOS_index != -1);
    int slice_index = -1;

    if (ctx->getType() == CTX_OPERATION_TYPE || ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {

        //OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);
        bool search_slice_index = is_dynamic || isTimed;
        if (opctx->getRangemode() == SLICE_OP && search_slice_index) {
            std::string time_dataset_name = getTimeVectorDataSetName(opctx, timebasename, timed_AOS_index);
            std::unique_ptr < HDF5DataSetHandler > time_data_set = std::move(getTimeVectorDataSet(opctx, gid, time_dataset_name)); //get time_data_set from the opened_data_sets map if it exists or create it
            if (!time_data_set)
                throw ALBackendException("HDF5Backend: unexpected NULL time_data_set in HDF5Reader::read_ND_Data()", LOG);
            slice_mode = opctx->getRangemode();
            slice_index = getSliceIndex(opctx, time_data_set, timebasename, &slice_sup, &linear_interpolation_factor, timed_AOS_index, current_arrctx_indices, &ignore_linear_interpolation);
            opened_data_sets[time_dataset_name] = std::move(time_data_set); //move unique_ptr to the std::map
        }
    }

    auto got = opened_data_sets.find(tensorized_path);
    std::unique_ptr < HDF5DataSetHandler > data_set;

    if (got != opened_data_sets.end()) {
        data_set = std::move(got->second);
        opened_data_sets.erase(got);
        if (!data_set)
            throw ALBackendException("HDF5Backend: unexpected NULL data_set in HDF5Reader::read_ND_Data()", LOG);
        dataset_id = data_set->dataset_id;
    }

    if (dataset_id < 0) {
        DataEntryContext *dec = getDataEntryContext(ctx);
        std::unique_ptr < HDF5DataSetHandler > new_data_set(new HDF5DataSetHandler(false, dec->getURI()));
        new_data_set-> open(tensorized_path.c_str(), gid, &dataset_id, *dim, size, datatype, false, true, dec->getURI());
        dataset_id = new_data_set->dataset_id;
        data_set = std::move(new_data_set);
    }

    herr_t status = -1;
    hid_t dataset_id_shapes = -1;
    bool isOpenedShapesDataSet;

    if ((data_set->selection_reader).get()) {

        HDF5HsSelectionReader & hsSelectionReader = *(data_set->selection_reader);

        if (!hsSelectionReader.isRequestInExtent(current_arrctx_indices)) {
            return exit_request(data_set, 0);
        }

        bool zero_shape = false;
        isOpenedShapesDataSet = true;
        int shapesDataSetExists = getPersistentShapes(ctx,
                                                      gid,
                                                      tensorized_path,
                                                      datatype,
                                                      slice_mode,
                                                      is_dynamic,
                                                      isTimed,
                                                      slice_index,
                                                      hsSelectionReader.getDim(),
                                                      size,
						      timed_AOS_index,
                                                      &zero_shape,
                                                      &dataset_id_shapes,
                                                      isOpenedShapesDataSet,
						      current_arrctx_indices
 						    );


        if (zero_shape) {
            return exit_request(data_set, 0);
        }

        if (shapesDataSetExists == 1)
            hsSelectionReader.setSize(size, hsSelectionReader.getDim());

        data_set->readData(current_arrctx_indices, datatype, *dim, slice_mode, is_dynamic, isTimed, timed_AOS_index, slice_index, data);
        data_set->selection_reader->getSize(size, slice_mode, is_dynamic);

    } else {
        std::unique_ptr < HDF5HsSelectionReader > hsSelectionReader(new HDF5HsSelectionReader(data_set->getRank(), dataset_id, 
            data_set->getDataSpace(), data_set->getLargestDims(), datatype, current_arrctx_indices.size(), dim));

        if (!hsSelectionReader->isRequestInExtent(current_arrctx_indices)) {
            return exit_request(data_set, 0);
        }

        bool zero_shape = false;
        isOpenedShapesDataSet = false;
        int shapesDataSetExists = getPersistentShapes(ctx,
                                                      gid,
                                                      tensorized_path,
                                                      datatype,
                                                      slice_mode,
                                                      is_dynamic,
                                                      isTimed,
                                                      slice_index,
                                                      *dim,
                                                      size,
						      timed_AOS_index,
                                                      &zero_shape,
                                                      &dataset_id_shapes,
                                                      isOpenedShapesDataSet,
						      current_arrctx_indices
 						    );

        if (zero_shape) {
            return exit_request(data_set, 0);
        }

        if (shapesDataSetExists == 1)
            hsSelectionReader->setSize(size, *dim);

        data_set->selection_reader = std::move(hsSelectionReader);

        data_set->readData(current_arrctx_indices, datatype, *dim, slice_mode, is_dynamic, isTimed, timed_AOS_index, slice_index, data);
        data_set->selection_reader->getSize(size, slice_mode, is_dynamic);

    }

    if (tensorized_path.compare("ids_properties&homogeneous_time") == 0) {
        int *d = (int *) *data;
        homogeneous_time = *d;
    }
    
    char **p = nullptr;
    if (datatype == alconst::char_data) {
        p = (char **) data;
        if (*dim == 1) {
            if (*p == nullptr) {
                char ch = '\0';
                *data = strdup(&ch);
                size[0] = 1;
            } else {
                std::string t(p[0]);
                size[0] = t.length();
                char* c = p[0];
                c[size[0]] = 0;
            }
        } else {
            if (*p == nullptr) {
                return exit_request(data_set, 0);
            }
        }
    }

    if (!ignore_linear_interpolation) {

        HDF5HsSelectionReader & hsSelectionReader = *(data_set->selection_reader);

         //Taking the next slide to make the linear interpolation
        void *next_slice_data = nullptr;

        if (isTimed) { //checking if neighbour nodes have the same shapes

            int first_slice_shape[H5S_MAX_RANK];
            int second_slice_shape[H5S_MAX_RANK];

            for (int i = 0; i < hsSelectionReader.getDim(); i++)
                    first_slice_shape[i] = size[i];

            bool zero_shape = false;
            getPersistentShapes(ctx,
                                gid,
                                tensorized_path,
                                datatype,
                                slice_mode,
                                is_dynamic,
                                isTimed,
                                slice_sup,
                                hsSelectionReader.getDim(),
                                size,
                                timed_AOS_index,
                                &zero_shape,
                                &dataset_id_shapes,
                                isOpenedShapesDataSet,
				current_arrctx_indices
 			      );

            if (datatype != alconst::char_data) {
                for (int i = 0; i < hsSelectionReader.getDim(); i++)
                    second_slice_shape[i] = size[i];

                if (hdf5_utils.compareShapes(first_slice_shape, second_slice_shape, hsSelectionReader.getDim()) != 0) {
		    if (this->INTERPOLATION_WARNING)
		        printf("WARNING: Linear interpolation not possible for node '%s' because its size isn't constant at time indices %d and %d.\n", tensorized_path.c_str(), slice_index, slice_sup);
		    return exit_request(data_set, 0);
                }
            }
        }

        hsSelectionReader.setHyperSlabs(slice_mode, is_dynamic, isTimed, slice_sup, timed_AOS_index, current_arrctx_indices);
        int buffer = hsSelectionReader.allocateBuffer(&next_slice_data, slice_mode, is_dynamic, isTimed, slice_sup);
	if (datatype != alconst::char_data)
           status = H5Dread(dataset_id, H5Dget_type(dataset_id), hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, next_slice_data);
	else
	   status = H5Dread(dataset_id, H5Dget_type(dataset_id), hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, (char**) &next_slice_data);
    
    hsSelectionReader.getSize(size, slice_mode, is_dynamic);

	if (status < 0) {
	    if (this->INTERPOLATION_WARNING)
	      printf("WARNING: Linear interpolation unable to read data from neighbouring node: %s at time index %d\n", tensorized_path.c_str(), slice_sup);
	    return exit_request(data_set, 0);
        }
        size_t N =  buffer / hsSelectionReader.dtype_size;

        //Making linear interpolation
        switch (datatype) {

        case alconst::integer_data:
            {
                int *data_int = (int *) *data;
                int *next_slice_data_int = (int *) next_slice_data;
                for (size_t i = 0; i < N; i++)
                    data_int[i] = (int) round(data_int[i] + (next_slice_data_int[i] - data_int[i]) * linear_interpolation_factor);
                free(next_slice_data);
                break;
            }
        case alconst::double_data:
            {
                double *data_double = (double *) *data;
                double *next_slice_data_double = (double *) next_slice_data;
                for (size_t i = 0; i < N; i++) 
                    data_double[i] = data_double[i] + (next_slice_data_double[i] - data_double[i]) * linear_interpolation_factor;
                free(next_slice_data);
                break;
            }
        case alconst::char_data:
            {
		char* data_str = (char*) *data;
		char* next_data_str = (char*) next_slice_data;
		if (strcmp(data_str, next_data_str) != 0)
		   strcpy(data_str, ""); //returning an empty string since values from neighboring slices are different
	        free(next_slice_data);
                break;
            }
        }
    }

	return exit_request(data_set, 1);

}

int HDF5Reader::exit_request(std::unique_ptr < HDF5DataSetHandler > &data_set, int exit_status) {
	if (data_set.get())
		opened_data_sets[data_set->getName().c_str()] = std::move(data_set);
	return exit_status;
}

int HDF5Reader::getPersistentShapes(Context * ctx, hid_t gid, const std::string & tensorized_path, int datatype, int slice_mode, 
				    bool is_dynamic, bool isTimed, int slice_index, int dim, int *size, int timed_AOS_index, 
				    bool * zero_shape, hid_t * dataset_id_shapes, bool isOpenedShapesDataSet, const std::vector < int > &current_arrctx_indices)
{

    int AOSRank = current_arrctx_indices.size();
    if (AOSRank == 0)
        return 0;

    int shapesDataSetExists = 0;

    if ((datatype != alconst::char_data && dim > 0)
        || (datatype == alconst::char_data && dim == 2)) {
        int *shapes = nullptr;
        shapesDataSetExists = readPersistentShapes(ctx, gid, tensorized_path, (void **) &shapes, slice_mode, is_dynamic, isTimed, 
slice_index, timed_AOS_index, zero_shape, dataset_id_shapes, isOpenedShapesDataSet, current_arrctx_indices);

        if (*zero_shape) {
            free(shapes);
            return shapesDataSetExists;
        }

        if (shapesDataSetExists == 1) {

            if (datatype == alconst::char_data && dim == 2) {
                size[0] = shapes[0];
            } else {
                size[0] = 1;    //for scalars
                for (int i = 0; i < dim; i++)
                    size[i] = shapes[dim - i - 1];
            }
        }
        free(shapes);
    }
    return shapesDataSetExists;
}

int
HDF5Reader::readPersistentShapes(Context * ctx,
                  hid_t gid,
				  const std::string & field_tensorized_path,
				  void **shapes, int slice_mode,
				  bool is_dynamic, bool isTimed,
				  int slice_index, int timed_AOS_index, bool *zero_shape, hid_t *dataset_id_shapes, bool isOpenedShapesDataSet, const std::vector < int > &current_arrctx_indices)
{
    if (slice_mode == SLICE_OP) {
		return readPersistentShapes_GetSlice(ctx, gid, field_tensorized_path, shapes, slice_mode, is_dynamic, isTimed, slice_index, timed_AOS_index, zero_shape, dataset_id_shapes, isOpenedShapesDataSet, current_arrctx_indices);
    }
    else {
		return readPersistentShapes_Get(ctx, gid, field_tensorized_path, shapes, slice_mode, is_dynamic, isTimed, timed_AOS_index, slice_index, zero_shape, dataset_id_shapes, isOpenedShapesDataSet, current_arrctx_indices);
    }
}

int HDF5Reader::readPersistentShapes_Get(Context * ctx, hid_t gid, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, 
					 bool isTimed, int timed_AOS_index, int slice_index, bool * zero_shape, hid_t * dataset_id_shapes, bool isOpenedShapesDataSet,
					 const std::vector < int > &current_arrctx_indices 
					)
{
    const std::string & tensorized_path = field_tensorized_path + "_SHAPE";
    std::unique_ptr < HDF5DataSetHandler > data_set;

    if (isOpenedShapesDataSet) {
        data_set = std::move(opened_shapes_data_sets[tensorized_path]);
        opened_shapes_data_sets.erase(tensorized_path);
    } else {
        DataEntryContext *dec = getDataEntryContext(ctx);
        std::unique_ptr < HDF5DataSetHandler > new_data_set(new HDF5DataSetHandler(false, dec->getURI()));
        hid_t dataset_id = -1;
        new_data_set-> open(tensorized_path.c_str(), gid, &dataset_id, 1, nullptr, alconst::integer_data, true, true, dec->getURI());
        if (!(new_data_set->dataset_id >=0))
            throw ALBackendException("HDF5Backend: unexpected dataset_id value in HDF5Reader::readPersistentShapes_Get()", LOG);
        *dataset_id_shapes = dataset_id;
        int dim = -1;
        auto hsSelectionReader = std::unique_ptr < HDF5HsSelectionReader >(new HDF5HsSelectionReader(new_data_set->getRank(), new_data_set->dataset_id, 
            new_data_set->getDataSpace(), new_data_set->getLargestDims(), alconst::integer_data, current_arrctx_indices.size(), &dim));
        new_data_set->selection_reader = std::move(hsSelectionReader);
        data_set = std::move(new_data_set);
    }

    int dim = 1; 
    data_set->readData(current_arrctx_indices, alconst::integer_data, dim, slice_mode, is_dynamic, isTimed, timed_AOS_index, slice_index, shapes);
    opened_shapes_data_sets[tensorized_path] = std::move(data_set);
    return 1;
}


int
HDF5Reader::readPersistentShapes_GetSlice(Context * ctx,
                  hid_t gid,
				  const std::string & field_tensorized_path,
				  void **shapes, int slice_mode,
				  bool is_dynamic, bool isTimed,
				  int slice_index, int timed_AOS_index, bool *zero_shape, hid_t *dataset_id_shapes, bool isOpenedShapesDataSet,
				  const std::vector < int > &current_arrctx_indices
 					)
{
  const std::string & tensorized_path = field_tensorized_path + "_SHAPE";

  std::vector < int >aos_indices(current_arrctx_indices.begin(), current_arrctx_indices.end());
  hid_t dataset_id = -1;
  std::unique_ptr < HDF5DataSetHandler > data_set;
  if (isOpenedShapesDataSet)
    {
        data_set = std::move(opened_shapes_data_sets[tensorized_path]);
        opened_shapes_data_sets.erase(tensorized_path);
        dataset_id = data_set->dataset_id;
    }
  else
    {
        DataEntryContext *dec = getDataEntryContext(ctx);
        std::unique_ptr < HDF5DataSetHandler > new_data_set(new HDF5DataSetHandler(false, dec->getURI()));
        new_data_set-> open(tensorized_path.c_str(), gid, &dataset_id, 1, nullptr, alconst::integer_data, true, true, dec->getURI());
        int dim = -1;
        auto hsSelectionReader = std::unique_ptr < HDF5HsSelectionReader >(new HDF5HsSelectionReader(new_data_set->getRank(), dataset_id, 
            new_data_set->getDataSpace(), new_data_set->getLargestDims(), alconst::integer_data, aos_indices.size(), &dim));
        new_data_set->selection_reader = std::move(hsSelectionReader);
        dataset_id = new_data_set->dataset_id;
        data_set = std::move(new_data_set);
    }

    if (timed_AOS_index != -1) {
        aos_indices[timed_AOS_index] = slice_index;
    }

	*dataset_id_shapes = dataset_id;
    
	HDF5HsSelectionReader &hsSelectionReader = *(data_set->selection_reader);
    hsSelectionReader.setHyperSlabs(GLOBAL_OP, false, isTimed, -1, timed_AOS_index, aos_indices);
    hsSelectionReader.allocateBuffer(shapes, GLOBAL_OP, false, isTimed, -1);
	herr_t status = H5Dread(dataset_id, H5Dget_type(dataset_id), hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, *shapes);
   
	if (status < 0)
    {
      char error_message[200];
      sprintf (error_message,
	       "Unable to read SHAPE: %s\n", tensorized_path.c_str ());
      throw ALBackendException (error_message, LOG);
    }

  int *shapes_int = (int *) *shapes;
  if (shapes_int[0] == 0)
    {				//if this condition is satisfied, it means that the dataset has 0-shapes for the current AOSs indices
      *zero_shape = true;
      opened_shapes_data_sets[tensorized_path] = std::move(data_set);
      return 1;
    }

  if (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1)
    {
      shapes_int[0] = 1;	//we set the latest element of the shapes vector to 1 when slicing
    }

  opened_shapes_data_sets[tensorized_path] = std::move(data_set);
  return 1;
}

int HDF5Reader::readAOSPersistentShapes(Context * ctx, hid_t gid, const std::string & tensorized_path, int timed_AOS_index, int slice_index, void **shapes, const std::vector < int > &current_arrctx_indices)
{
    //std::cout << "Reading AOS SHAPE datasets: " <<  tensorized_path.c_str() << std::endl;
    std::vector < int >aos_indices(current_arrctx_indices.begin(), current_arrctx_indices.end() -1);

    if (slice_mode == SLICE_OP && timed_AOS_index != -1 && timed_AOS_index < (int) aos_indices.size()) {
        aos_indices[timed_AOS_index] = slice_index;
    }
    auto got = aos_opened_shapes_data_sets.find(tensorized_path);
   
    std::unique_ptr < HDF5DataSetHandler > data_set;
    bool isOpenedShapesDataSet = (got != aos_opened_shapes_data_sets.end());
    if (isOpenedShapesDataSet) {
        data_set = std::move(got->second);
        aos_opened_shapes_data_sets.erase(tensorized_path);
    } else {
        DataEntryContext *dec = getDataEntryContext(ctx);
        std::unique_ptr < HDF5DataSetHandler > new_data_set(new HDF5DataSetHandler(false, dec->getURI()));
        hid_t dataset_id = -1;
        new_data_set-> open(tensorized_path.c_str(), gid, &dataset_id, 1, nullptr, alconst::integer_data, true, true, dec->getURI());
        dataset_id = new_data_set->dataset_id;
        int dim = -1;
        auto hsSelectionReader = std::unique_ptr < HDF5HsSelectionReader >(new HDF5HsSelectionReader(new_data_set->getRank(), dataset_id, 
            new_data_set->getDataSpace(), new_data_set->getLargestDims(), alconst::integer_data, aos_indices.size(), &dim));
        new_data_set->selection_reader = std::move(hsSelectionReader);
        data_set = std::move(new_data_set);
    }
	
    int dim = 1;
    if (!data_set->isRequestInExtent(aos_indices))
       return 0;
    data_set->readData(aos_indices, alconst::integer_data, dim, slice_mode, false, timed_AOS_index != -1, timed_AOS_index, slice_index, shapes);
    aos_opened_shapes_data_sets[tensorized_path] = std::move(data_set);
    return 1;
}

void HDF5Reader::open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    HDF5Utils hdf5_utils;

    hid_t gid = -1;
    auto got = IDS_group_id.find(ctx);
    if (got != IDS_group_id.end())
        gid = got->second;
    if (gid == -1) {
        hdf5_utils.open_IDS_group(ctx, file_id, opened_IDS_files, files_directory, relative_file_path, &gid);
    }
    if (gid >= 0)
        IDS_group_id[ctx] = gid;
}

void HDF5Reader::close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files)
{
    std::replace(external_link_name.begin(), external_link_name.end(), '/', '_');
    if (opened_IDS_files.find(external_link_name) != opened_IDS_files.end()) {
        hid_t pulse_file_id = opened_IDS_files[external_link_name];
        if (pulse_file_id != -1) {
            HDF5Utils hdf5_utils;
		        /*std::cout << "READER:close_file_handler :showing status for pulse file..." << std::endl;
			    HDF5Utils hdf5_utils;
	            hdf5_utils.showStatus(pulse_file_id);*/
            hdf5_utils.closeIDSFile(pulse_file_id, external_link_name);
            opened_IDS_files[external_link_name] = -1;
        }
    }
}

void HDF5Reader::endAction(Context * ctx)
{   
  if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
    ArraystructContext *arrctxt = static_cast<ArraystructContext*> (ctx);
    //printf("calling endAction for path=%s\n", arrctxt->getPath().c_str());
    auto arrctx_shapes_got = arrctx_shapes_per_context.find(arrctxt);
    if (arrctx_shapes_got != arrctx_shapes_per_context.end())
      arrctx_shapes_per_context.erase(arrctx_shapes_got);
    auto got = tensorized_paths_per_context.find(arrctxt);
    if (got != tensorized_paths_per_context.end())
      tensorized_paths_per_context.erase(got);
    auto got2 = tensorized_paths_per_op_context.find(arrctxt->getOperationContext());
    if (got2 != tensorized_paths_per_op_context.end()) {
        if (arrctxt->getParent() == NULL) {
            tensorized_paths_per_op_context.erase(got2);
		}
        else {
            auto &tensorized_paths = got2->second;
			if (tensorized_paths.size() > 0) {
				tensorized_paths.pop_back();
				if (tensorized_paths.size() == 0) {
					tensorized_paths_per_op_context.erase(got2);
				}
			}
        }
    }
   }
   else {
        //OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);
        //tensorized_paths_per_op_context.erase(opCtx);
    }
}

DataEntryContext* HDF5Reader::getDataEntryContext(Context * ctx) {
   OperationContext *opctx = nullptr;
    if (ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {
        opctx = (static_cast<ArraystructContext*> (ctx))->getOperationContext();
    }
    else {
        opctx = static_cast<OperationContext*> (ctx);
    }
    return opctx->getDataEntryContext();
}

void HDF5Reader::setSliceMode(int slice_mode) {
	this->slice_mode = slice_mode;
}

void HDF5Reader::get_occurrences(const char* ids_name, int** occurrences_list, int* size, hid_t master_file_id) {

    if (master_file_id <= 0) {
        char error_message[200];
        sprintf (error_message, "Error calling get_occurrences() for %s, HDF5 master file not opened.\n", ids_name);
        throw ALBackendException (error_message, LOG);
    }
    *occurrences_list = nullptr;
    *size = 0;
    std::vector<std::string> od;
    H5L_iterate1_t file_info = (H5L_iterate1_t) &HDF5Reader::iterate_callback;
    herr_t status = H5Literate (master_file_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, file_info, (void*) &od);
    if (status != 0)
        throw ALBackendException("HDF5Backend: H5Literate has failed in HDF5Reader::get_occurrences()", LOG);
    std::string ids_name_str(ids_name);
    std::vector<int> occurrences;

    for (size_t i = 0; i < od.size(); i++) {
        
        std::string found_occurrence_name = od[i];
        char ch = '_';
        
        // Searching index of last occurrence '_'
        auto it = std::find(found_occurrence_name.rbegin(), found_occurrence_name.rend(), ch); //using inverse iterator

        size_t lastOccurrenceIndex = -1;

        if (it != found_occurrence_name.rend())
            lastOccurrenceIndex = std::distance(found_occurrence_name.begin(), (it + 1).base());

        int occ = 0;
        if (lastOccurrenceIndex > 0) {

            // Extract substring of the last occurrence
            std::string subStr = found_occurrence_name.substr(lastOccurrenceIndex + 1);
            std::string ids_name_from_hdf5_metadata = found_occurrence_name.substr(0, lastOccurrenceIndex);

            // Check if the substring is an integer (occurrence)
            try {
                occ = std::stoi(subStr);

                if (ids_name_from_hdf5_metadata == ids_name_str) 
                    occurrences.push_back(occ);
                
            } catch (const std::invalid_argument&) { //the '_' is a part of the ids_name, it' not the separator of the occurrence (integer) from the IDS name
                //No integer found, so maybe it's occurrence 0; we check if ids_name matches
                if (found_occurrence_name == ids_name_str)
                    occurrences.push_back(occ);

            }
        }
        else { //No '_' found at all, maybe it's the occurrence 0, so we check only the ids_name matches
            if (found_occurrence_name == ids_name_str)
                occurrences.push_back(occ);
        }

    }
    *size = occurrences.size();
    std::sort (occurrences.begin(), occurrences.end());
    *occurrences_list = (int*) malloc(sizeof(int)*occurrences.size()); //allocated results on the heap
    int* p = *occurrences_list;
    for (size_t i = 0; i < occurrences.size(); i++) 
        p[i] = occurrences[i];
}


herr_t HDF5Reader::iterate_callback (hid_t loc_id, const char *name, const H5L_info_t *info, void *callback_data)
{
    std::vector<std::string>* op = reinterpret_cast<std::vector<std::string>*>(callback_data);
    op->push_back(name);
    return 0;
}
