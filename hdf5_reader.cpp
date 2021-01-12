#include "hdf5_reader.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "hdf5_dataset_handler.h"
#include <math.h>
#include <stdlib.h>


HDF5Reader::HDF5Reader(std::string backend_version_)
:  backend_version(backend_version_), tensorized_paths(), opened_data_sets(), opened_shapes_data_sets(), shapes_data(), shapes_selection_readers(), non_existing_data_sets(), selection_readers(), homogeneous_time(-1), ignore_linear_interpolation(true), current_arrctx_indices(), current_arrctx_shapes(), IDS_group_id(-1), slice_mode(GLOBAL_OP)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Reader::~HDF5Reader()
{
}

void
 HDF5Reader::openPulse(PulseContext * ctx, int mode, std::string & options, std::string & backend_version, hid_t * file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_paths_strategy, std::string & files_directory, std::string & relative_file_path)
{
    HDF5Utils hdf5_utils;
    std::string filePath = hdf5_utils.pulseFilePathFactory(ctx, files_paths_strategy, files_directory, relative_file_path);
    //std::cout << "Opening HDF5 file at path: " << filePath << std::endl;
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_alignment(fapl, 0, 16);
    H5Pclose(fapl);

    /* Save old error handler */
    H5E_auto2_t old_func;
    void *old_client_data;
    hid_t current_stack_id = H5Eget_current_stack();
    H5Eget_auto(current_stack_id, &old_func, &old_client_data);

    /* Turn off error handling */
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* Probe. Likely to fail, but that's okay */
    *file_id = H5Fopen(filePath.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    /* Restore previous error handler */
    H5Eset_auto(current_stack_id, old_func, old_client_data);


    if (*file_id < 0) {
        std::string message("Unable to open HDF5 file: ");
        message += filePath;
        throw UALBackendException(message, LOG);
    }
    //std::cout << "Successfull read of the master file" << std::endl;
    struct opdata od;
    od.mode = true;
    od.files_directory = files_directory;
    od.relative_file_path = relative_file_path;
    od.count = 0;
    H5Literate(*file_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, file_info, (void *) &od);
    for (int i = 0; i < od.count; i++) {
        std::string ids_name = std::string(od.link_names[i]);
        free(od.link_names[i]);
        std::replace(ids_name.begin(), ids_name.end(), '/', '_');
        //opened_IDS_files[ids_name] = od.file_ids[i];
        opened_IDS_files[ids_name] = -1;
    }

    const char *backend_version_attribute_name = "HDF5_BACKEND_VERSION";

    //read version from file
    if (H5Aexists(*file_id, backend_version_attribute_name) > 0) {
        hid_t att_id = H5Aopen(*file_id, backend_version_attribute_name, H5P_DEFAULT);
        if (att_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open attribute: %s\n", backend_version_attribute_name);
            throw UALBackendException(error_message, LOG);
        }
        hid_t dtype_id = H5Tcreate(H5T_STRING, 10);
        herr_t tset = H5Tset_cset(dtype_id, H5T_CSET_UTF8);
        if (tset < 0) {
            char error_message[100];
            sprintf(error_message, "Unable to set characters to UTF8 for: %s\n", backend_version_attribute_name);
            throw UALBackendException(error_message);
        }

        char version[10];
        herr_t status = H5Aread(att_id, dtype_id, version);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read attribute: %s\n", backend_version_attribute_name);
            throw UALBackendException(error_message, LOG);
        }
        backend_version = std::string(version);
        H5Tclose(dtype_id);
        H5Aclose(att_id);
    } else {
        char error_message[200];
        sprintf(error_message, "Not a IMAS HDF5 pulse file. Unable to find attribute: %s\n", backend_version_attribute_name);
        throw UALBackendException(error_message, LOG);
    }
}

void HDF5Reader::closePulse(PulseContext * ctx, int mode, std::string & options, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path)
{
    close_datasets();
    herr_t status = H5Fclose(file_id);
    HDF5Utils hdf5_utils;
    std::string filePath = hdf5_utils.pulseFilePathFactory(ctx, files_path_strategy, files_directory, relative_file_path);

    if (status < 0) {
        char error_message[100];
        sprintf(error_message, "Unable to close HDF5 file with handler: %d\n", (int) file_id);
        throw UALBackendException(error_message);
    }
    auto it = opened_IDS_files.begin();
    while (it != opened_IDS_files.end()) {
        const std::string & external_link_name = it->first;
        hid_t pulse_file_id = opened_IDS_files[external_link_name];

        if (pulse_file_id != -1) {
            status = H5Fclose(pulse_file_id);

            if (status < 0) {
                char error_message[100];
                sprintf(error_message, "Unable to close HDF5 file for IDS: %s\n", external_link_name.c_str());
                throw UALBackendException(error_message);
            }
        }
        it++;
    }
}


void HDF5Reader::close_datasets()
{
    opened_data_sets.clear();
    auto it = datasets_data.begin();
    while (it != datasets_data.end()) {
        free(it->second);
        it++;
    }
    datasets_data.clear();
    non_existing_data_sets.clear();
    selection_readers.clear();
    shapes_selection_readers.clear();
    auto it2 = shapes_data.begin();
    while (it2 != shapes_data.end()) {
        free(it2->second);
        it2++;
    }
    shapes_data.clear();
    opened_shapes_data_sets.clear();
}

void HDF5Reader::close_dataset(hid_t dataset_id, std::string & tensorized_path, hid_t dataset_id_shapes)
{
    if (slice_mode == SLICE_OP)
        return;

    bool do_close = false;
    for (size_t i = 0; i < current_arrctx_indices.size(); i++) {
        if (current_arrctx_indices[i] == current_arrctx_shapes[i] - 1) {
            do_close = true;
            continue;
        } else {
            do_close = false;
            break;
        }
    }
    if (do_close) {
        if (shapes_data.find(dataset_id_shapes) != shapes_data.end()) {
            free(shapes_data[dataset_id_shapes]);
            shapes_data.erase(dataset_id_shapes);
            shapes_selection_readers.erase(dataset_id_shapes);
        }
        H5Dclose(dataset_id);
        opened_data_sets.erase(tensorized_path);
        selection_readers.erase(tensorized_path);
        free(datasets_data[dataset_id]);
        datasets_data.erase(dataset_id);
    }
}

void HDF5Reader::close_group()
{
    if (IDS_group_id != -1) {
        H5Gclose(IDS_group_id);
        IDS_group_id = -1;
    }
}


void HDF5Reader::beginReadArraystructAction(ArraystructContext * ctx, int *size)
{
    if (IDS_group_id == -1) {   //IDS does not exist in the file
        *size = 0;
        return;
    }

    HDF5Utils hdf5_utils;
    hdf5_utils.setTensorizedPaths(ctx, tensorized_paths);

    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";

    if (non_existing_data_sets.find(tensorized_path) != non_existing_data_sets.end())   //optimization
    {
        *size = 0;
        tensorized_paths.pop_back();
        return;
    } else {
        if (H5Lexists(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT) == 0) {
            non_existing_data_sets[tensorized_path] = 1;
            *size = 0;
            tensorized_paths.pop_back();
            return;
        }
    }

    current_arrctx_indices.push_back(ctx->getIndex());
    int timed_AOS_index = -1;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices

    int *shapes = nullptr;
    readAOSPersistentShapes(ctx, tensorized_path, (void **) &shapes);
    *size = shapes[0];
    free(shapes);
    current_arrctx_shapes.push_back(*size);

    if (slice_mode == SLICE_OP && ctx->getTimed() && *size > 0) {
        *size = 1;
    }

    else if (*size == 0) {
        current_arrctx_indices.pop_back();
        tensorized_paths.pop_back();
    }
}

int HDF5Reader::getSliceIndex(OperationContext * opCtx, std::string & att_name, std::string & timebasename, int *slice_sup, double *linear_interpolation_factor, int timed_AOS_index)
{

    HDF5Utils hdf5_utils;
    double time = opCtx->getTime();
    int interp = opCtx->getInterpmode();
    std::string dataset_name;

    if (homogeneous_time == 1) {
        dataset_name = "time";
    } else {
        std::string tensorized_path = timebasename;
        if (tensorized_paths.size() > 0)
            tensorized_path = tensorized_paths.back() + "&" + timebasename;

        if (timed_AOS_index != -1) {
            dataset_name = "time";
        } else {
            if (timebasename.compare("&time") == 0 || timebasename.compare("time") == 0) {
                dataset_name = "time";
            } else {
                dataset_name = tensorized_path;
            }
        }
    }

    hid_t dataset_id = -1;
    if (opened_data_sets.find(dataset_name) != opened_data_sets.end()) {
        dataset_id = opened_data_sets[dataset_name];
    } else {
        dataset_id = H5Dopen2(IDS_group_id, dataset_name.c_str(), H5P_DEFAULT);
        opened_data_sets[dataset_name] = dataset_id;
    }

    if (dataset_id < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to open dataset: %s for getting time vector.\n", dataset_name.c_str());
        throw UALBackendException(error_message, LOG);
    }

    hid_t dataspace = H5Dget_space(dataset_id);
    hsize_t dataspace_dims[H5S_MAX_RANK];
    int dataset_rank = H5Sget_simple_extent_dims(dataspace, dataspace_dims, NULL);
    H5Sclose(dataspace);
    if (dataset_rank < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to call H5Sget_simple_extent_dims on dataset:%s\n", dataset_name.c_str());
        throw UALBackendException(error_message, LOG);
    }

    double *slices_times = nullptr;


    if (homogeneous_time == 1 || dataset_name.compare("time") == 0) {
        slices_times = (double *) malloc(dataspace_dims[dataset_rank - 1] * sizeof(H5T_NATIVE_DOUBLE));
        herr_t status = H5Dread(dataset_id, H5T_NATIVE_DOUBLE, H5P_DEFAULT, H5P_DEFAULT,
                                H5P_DEFAULT, (void *) slices_times);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read the homogeneous time basis:%s\n", dataset_name.c_str());
            throw UALBackendException(error_message, LOG);
        }
    } else {

        // std::cout << "GETTING THE INHOMOGENEOUS TIME BASIS: " << std::endl;
        HDF5Utils hdf5_utils;
        int timed_AOS_index;
        hdf5_utils.getAOSIndices(opCtx, current_arrctx_indices, &timed_AOS_index);
        int dim = -1;

        HDF5HsSelectionReader hsSelectionReader(dataset_id, ualconst::double_data, current_arrctx_indices.size(), &dim);
        hsSelectionReader.allocateGlobalOpBuffer((void **) &slices_times);
        hsSelectionReader.setHyperSlabsGlobalOp(current_arrctx_indices);
        herr_t status = H5Dread(dataset_id, hsSelectionReader.dtype_id,
                                hsSelectionReader.memspace,
                                hsSelectionReader.dataspace, H5P_DEFAULT,
                                slices_times);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read the inhomogeneous time basis: %s\n", dataset_name.c_str());
            throw UALBackendException(error_message, LOG);
        }
    }

    //std::cout << "--> time = " << time << std::endl;

    *slice_sup = dataspace_dims[dataset_rank - 1] - 1;
    int slice_inf = 0;
    if (*slice_sup > 0)
        slice_inf = *slice_sup - 1;
    int closest;
    int n = (int) dataspace_dims[dataset_rank - 1];
    for (int i = 0; i < n; i++) {
        //std::cout << "--> slices_times[" << i << "]=" << slices_times[i] << std::endl;
        if (slices_times[i] >= time) {
            *slice_sup = i;
            if (*slice_sup == 0 && n > 1)
                *slice_sup = 1;
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
        free(slices_times);
        return slice_inf;
        break;
    case LINEAR_INTERP:
        if (*slice_sup == slice_inf) {
            ignore_linear_interpolation = true;
            free(slices_times);
            return slice_inf;
        } else {
            ignore_linear_interpolation = false;
        }
        *linear_interpolation_factor = (time - slices_times[slice_inf]) / (slices_times[*slice_sup] - slices_times[slice_inf]);
        //std::cout << "linear_interpolation_factor: " << *linear_interpolation_factor << std::endl;
        free(slices_times);
        return slice_inf;
        break;

    default:
        throw UALBackendException("Interpolation mode not yet supported", LOG);
    }
}

int HDF5Reader::read_ND_Data(Context * ctx, std::string & att_name, std::string & timebasename, int datatype, void **data, int *dim, int *size)
{

    if (IDS_group_id == -1) {   //IDS does not exist in the file
        return 0;
    }

    std::string & dataset_name = att_name;
    std::replace(dataset_name.begin(), dataset_name.end(), '/', '&');   // character '/' is not supported in datasets names
    std::replace(timebasename.begin(), timebasename.end(), '/', '&');

    hid_t dataset_id = -1;
    std::string tensorized_path = dataset_name;

    if (tensorized_paths.size() > 0)
        tensorized_path = tensorized_paths.back() + "&" + dataset_name;

    if (non_existing_data_sets.find(tensorized_path) != non_existing_data_sets.end())   //optimization
    {
        return 0;
    } else if (opened_data_sets.find(tensorized_path) != opened_data_sets.end()) {
        dataset_id = opened_data_sets[tensorized_path];
    } else if (H5Lexists(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT) == 0) {
        non_existing_data_sets[tensorized_path] = 1;
        return 0;
    }
    //std::cout << "Reading data set: " << tensorized_path.c_str() << std::endl;
    HDF5Utils hdf5_utils;
    int timed_AOS_index = -1;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices

    bool is_dynamic = timebasename.compare("") != 0 ? true : false;

    ignore_linear_interpolation = true;
    int slice_sup = -1;         //used for linear interpolation
    double linear_interpolation_factor = 0;

    bool isTimed = (timed_AOS_index != -1);
    int slice_index = -1;

    if (ctx->getType() == CTX_OPERATION_TYPE || ctx->getType() == CTX_ARRAYSTRUCT_TYPE) {

        OperationContext *opCtx = dynamic_cast < OperationContext * >(ctx);
        if (opCtx->getRangemode() == SLICE_OP) {
            slice_mode = opCtx->getRangemode();
            bool search_slice_index = (slice_mode == SLICE_OP) && (is_dynamic || isTimed);
            if (search_slice_index) {
                slice_index = getSliceIndex(opCtx, dataset_name, timebasename, &slice_sup, &linear_interpolation_factor, timed_AOS_index);
            }
        }
    }

    if (dataset_id < 0) {
        size_t M = 6 * 1024 * 1024;
        hid_t dapl = H5Pcreate(H5P_DATASET_ACCESS);
        /*H5Pset_chunk_cache (dapl, H5D_CHUNK_CACHE_NSLOTS_DEFAULT, M,
           H5D_CHUNK_CACHE_W0_DEFAULT); */
        H5Pset_chunk_cache(dapl, 10000, M * 200, H5D_CHUNK_CACHE_W0_DEFAULT);
        dataset_id = H5Dopen2(IDS_group_id, tensorized_path.c_str(), dapl);
        H5Pclose(dapl);
        if (dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open dataset: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }
        opened_data_sets[tensorized_path] = dataset_id;

    }

    herr_t status = -1;
    hid_t dataset_id_shapes = -1;
    bool brutal_approach = false;

    if (selection_readers.find(tensorized_path) != selection_readers.end()) {
        HDF5HsSelectionReader & hsSelectionReader = *selection_readers[tensorized_path];

        if (!hsSelectionReader.isRequestInExtent(current_arrctx_indices)) {
            return 0;
        }

        bool zero_shape = false;
        int shapesDataSetExists = getPersistentShapes(ctx,
                                                      tensorized_path,
                                                      datatype,
                                                      slice_mode,
                                                      is_dynamic,
                                                      isTimed,
                                                      slice_index,
                                                      hsSelectionReader.getDim(),
                                                      size,
                                                      &zero_shape,
                                                      &dataset_id_shapes);


        if (zero_shape)
            return 0;

        if (shapesDataSetExists == 1) {
            hsSelectionReader.setSize(size, hsSelectionReader.getDim());
        }
        hsSelectionReader.getSize(size, slice_mode, is_dynamic);
        if (brutal_approach) {
            status = readDataSet(ctx, tensorized_path, data, datatype, dataset_id, slice_mode, is_dynamic, isTimed, slice_index, timed_AOS_index, hsSelectionReader);
        }
        else {
            hsSelectionReader.setHyperSlabs(slice_mode, is_dynamic, isTimed, slice_index, timed_AOS_index, current_arrctx_indices);
            hsSelectionReader.getSize(size, slice_mode, is_dynamic);
            hsSelectionReader.allocateBuffer(data, slice_mode, is_dynamic, isTimed, slice_index);
    
            if (datatype != ualconst::char_data) {
                status = H5Dread(dataset_id, hsSelectionReader.dtype_id, hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, *data);
            } else {
                status = H5Dread(dataset_id, hsSelectionReader.dtype_id, hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, (char **) data);
            }
        }

    } else {
        std::unique_ptr < HDF5HsSelectionReader > hsSelectionReader(new HDF5HsSelectionReader(dataset_id, datatype, current_arrctx_indices.size(), dim));

        if (!hsSelectionReader->isRequestInExtent(current_arrctx_indices)) {
            return 0;
        }

        bool zero_shape = false;
        int shapesDataSetExists = getPersistentShapes(ctx,
                                                      tensorized_path,
                                                      datatype,
                                                      slice_mode,
                                                      is_dynamic,
                                                      isTimed,
                                                      slice_index,
                                                      *dim,
                                                      size,
                                                      &zero_shape,
                                                      &dataset_id_shapes);

        if (zero_shape)
            return 0;

        if (shapesDataSetExists == 1) {
            hsSelectionReader->setSize(size, *dim);
        }
        hsSelectionReader->getSize(size, slice_mode, is_dynamic);
        if (brutal_approach) {
            status = readDataSet(ctx, tensorized_path, data, datatype, dataset_id, slice_mode, is_dynamic, isTimed, slice_index, timed_AOS_index, *hsSelectionReader);
        }
        else {
            hsSelectionReader->setHyperSlabs(slice_mode, is_dynamic, isTimed, slice_index, timed_AOS_index, current_arrctx_indices);
            hsSelectionReader->getSize(size, slice_mode, is_dynamic);
            hsSelectionReader->allocateBuffer(data, slice_mode, is_dynamic, isTimed, slice_index);
    
            if (datatype != ualconst::char_data) {
                status = H5Dread(dataset_id, hsSelectionReader->dtype_id, hsSelectionReader->memspace, hsSelectionReader->dataspace, H5P_DEFAULT, *data);
            } else {
                status = H5Dread(dataset_id, hsSelectionReader->dtype_id, hsSelectionReader->memspace, hsSelectionReader->dataspace, H5P_DEFAULT, (char **) data);
            }
        }
        selection_readers[tensorized_path] = std::move(hsSelectionReader);
    }

    if (tensorized_path.compare("ids_properties&homogeneous_time") == 0) {
        int *d = (int *) *data;
        homogeneous_time = *d;
    }


    char **p = nullptr;
    if (datatype == ualconst::char_data) {

        if (*dim == 1) {
            p = (char **) data;
            if (*p == nullptr) {
                char ch = '\0';
                *data = strdup(&ch);
                size[0] = 1;
            } else {
                char *copy = (char *) malloc(strlen(p[0]) + 1);
                strncpy(copy, p[0], strlen(p[0]) + 1);
                free(*data);
                *data = copy;
                size[0] = strlen(p[0]);
            }
        } else {
            p = (char **) data;
            if (*p == nullptr) {
                return 0;
            } else {
                size_t maxlength = 0;
                for (int i = 0; i < size[0]; i++) {
                    if (strlen(p[i]) > maxlength)
                        maxlength = strlen(p[i]);
                }
                char *buffer = (char *) malloc(size[0] * (maxlength + 1));
                char ch = '\0';
                memset(buffer, 0, size[0] * (maxlength + 1));
                for (int i = 0; i < size[0]; i++) {
                    strcpy(buffer + i * maxlength, p[i]);
                    for (int j = 0; j < (int) (maxlength - strlen(p[i])); j++) {
                        strncat(buffer + i * maxlength, &ch, 1);
                    }
                }
                free(*data);
                *data = buffer;
                size[1] = maxlength;
            }
        }
    }

    if (!ignore_linear_interpolation) {

        //Taking the next slide for linear interpolation
        void *next_slice_data = nullptr;
        HDF5HsSelectionReader & hsSelectionReader = *selection_readers[tensorized_path];
        hsSelectionReader.setHyperSlabs(slice_mode, is_dynamic, isTimed, slice_sup, timed_AOS_index, current_arrctx_indices);
        hsSelectionReader.getSize(size, slice_mode, is_dynamic);
        int buffer = hsSelectionReader.allocateBuffer(&next_slice_data, slice_mode, is_dynamic, isTimed, slice_sup);
        status = H5Dread(dataset_id, hsSelectionReader.dtype_id, hsSelectionReader.memspace, hsSelectionReader.dataspace, H5P_DEFAULT, next_slice_data);
        size_t N =  buffer / hsSelectionReader.dtype_size;

        //Making linear interpolation
        switch (datatype) {

        case ualconst::integer_data:
            {
                int *data_int = (int *) *data;
                int *next_slice_data_int = (int *) next_slice_data;
                for (size_t i = 0; i < N; i++)
                    data_int[i] = (int) round(data_int[i] + (next_slice_data_int[i] - data_int[i]) * linear_interpolation_factor);

                break;
            }
        case ualconst::double_data:
            {
                double *data_double = (double *) *data;
                double *next_slice_data_double = (double *) next_slice_data;
                for (size_t i = 0; i < N; i++) 
                    data_double[i] = data_double[i] + (next_slice_data_double[i] - data_double[i]) * linear_interpolation_factor;
                break;
            }
        case ualconst::char_data:
            {
                char error_message[200];
                sprintf(error_message, "Linear interpolation of char data not supported, dataset: %s\n", tensorized_path.c_str());
                throw UALBackendException(error_message, LOG);
                break;
            }
        }
    }

    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to read dataset: %s\n", tensorized_path.c_str());
        throw UALBackendException(error_message, LOG);
    }

    close_dataset(dataset_id, tensorized_path, dataset_id_shapes);      //close the dataset only if AOSs iterations are finished
    return 1;

}

int HDF5Reader::getPersistentShapes(Context * ctx, const std::string & tensorized_path, int datatype, int slice_mode, bool is_dynamic, bool isTimed, int slice_index, int dim, int *size, bool * zero_shape, hid_t * dataset_id_shapes)
{

    int AOSRank = current_arrctx_indices.size();
    if (AOSRank == 0)
        return 0;

    int shapesDataSetExists = 0;

    if ((datatype != ualconst::char_data && dim > 0)
        || (datatype == ualconst::char_data && dim == 2)) {
        int *shapes = nullptr;
        shapesDataSetExists = readPersistentShapes(ctx, tensorized_path, (void **) &shapes, slice_mode, is_dynamic, isTimed, slice_index, zero_shape, dataset_id_shapes);

        if (*zero_shape)
            return shapesDataSetExists;

        if (shapesDataSetExists == 1) {

            if (datatype == ualconst::char_data && dim == 2) {
                size[0] = shapes[0];
            } else {
                size[0] = 1;    //for scalars
                for (int i = 0; i < dim; i++)
                    size[i] = shapes[i];
            }
        }
        free(shapes);
    }
    return shapesDataSetExists;

}

herr_t HDF5Reader::readDataSet(Context * ctx, const std::string & tensorized_path, void **data, int datatype, hid_t dataset_id, int slice_mode, bool is_dynamic, bool isTimed, int slice_index, int timed_AOS_index, HDF5HsSelectionReader &hsSelectionReader)
{
    herr_t status = -1;
    if (datasets_data.find(dataset_id) == datasets_data.end()) {
        void *dataset_buffer = nullptr;
        hsSelectionReader.allocateFullBuffer(&dataset_buffer);
         if (datatype != ualconst::char_data) {
            status = H5Dread(dataset_id, hsSelectionReader.dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, dataset_buffer);
        } else {
            status = H5Dread(dataset_id, hsSelectionReader.dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, (char **) &dataset_buffer);
        }
        if (status < 0) {
            return status;
        }
        datasets_data[dataset_id] = dataset_buffer;
    }

    void *src_buffer = datasets_data[dataset_id];
    hsSelectionReader.setHyperSlabs(slice_mode, is_dynamic, isTimed, slice_index, timed_AOS_index, current_arrctx_indices);
    int dst_buf_size = hsSelectionReader.allocateBuffer(data, slice_mode, is_dynamic, isTimed, slice_index);
    if (datatype != ualconst::char_data) {
        assert(H5Dgather(hsSelectionReader.dataspace, src_buffer, hsSelectionReader.dtype_id, (size_t) dst_buf_size, *data, nullptr, nullptr) >=0);
    }
    else {
        assert(H5Dgather(hsSelectionReader.dataspace, src_buffer, hsSelectionReader.dtype_id, (size_t) dst_buf_size, data, nullptr, nullptr) >=0);
    }
    status = 0;
    return status;
}


int HDF5Reader::readPersistentShapes(Context * ctx, const std::string & field_tensorized_path, void **shapes, int slice_mode, bool is_dynamic, bool isTimed, int slice_index, bool * zero_shape, hid_t * dataset_id_shapes)
{
    const std::string & tensorized_path = field_tensorized_path + "_SHAPE";

    hid_t dataset_id = -1;
    if (opened_shapes_data_sets.find(tensorized_path) != opened_shapes_data_sets.end()) {
        dataset_id = opened_shapes_data_sets[tensorized_path];
    } else {
        dataset_id = H5Dopen2(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT);

        opened_shapes_data_sets[tensorized_path] = dataset_id;

        if (dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open dataset SHAPE: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }

        int *shapes_buffer;
        int dim = -1;
        std::unique_ptr < HDF5HsSelectionReader > hsSelectionReader(new HDF5HsSelectionReader(dataset_id, ualconst::integer_data, current_arrctx_indices.size(), &dim));

        hsSelectionReader->allocateFullBuffer((void **) &shapes_buffer);
        herr_t status = H5Dread(dataset_id, hsSelectionReader->dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, shapes_buffer);

        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read dataset: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }

        shapes_selection_readers[dataset_id] = std::move(hsSelectionReader);
        shapes_data[dataset_id] = shapes_buffer;
        H5Dclose(dataset_id);   //buffer is read only one, so we close the shapes dataset. Its handler (used by shapes_data) is still available from the opened_shapes_data_sets. 
    }

    *dataset_id_shapes = dataset_id;

    int *buffer = shapes_data[dataset_id];

    HDF5HsSelectionReader & hsSelectionReader = *shapes_selection_readers[dataset_id];

    std::vector < int >index;
    hsSelectionReader.getDataIndex(current_arrctx_indices, index);
    int n = hsSelectionReader.getShape(hsSelectionReader.getRank() - 1);        //n is the length of the vector of shapes
    *shapes = (int *) malloc(sizeof(int) * n);
    int *shapes_int = (int *) *shapes;

    for (size_t i = 0; i < (size_t) n; i++) {
        shapes_int[i] = buffer[index[i]];
    }


    if (shapes_int[0] == 0) {   //if this condition is satisfied, it means that the dataset has 0-shapes for the current AOSs indices
        *zero_shape = true;
        return 1;
    }

    if (slice_mode == SLICE_OP && is_dynamic && !isTimed && slice_index != -1) {
        shapes_int[n - 1] = 1;  //we set the latest element of the shapes vector to 1 when slicing
    }

    return 1;
}

int HDF5Reader::readAOSPersistentShapes(Context * ctx, const std::string & tensorized_path, void **shapes)
{
    hid_t dataset_id = -1;
    if (opened_shapes_data_sets.find(tensorized_path) != opened_shapes_data_sets.end()) {
        dataset_id = opened_shapes_data_sets[tensorized_path];
    } else {
        dataset_id = H5Dopen2(IDS_group_id, tensorized_path.c_str(), H5P_DEFAULT);

        opened_shapes_data_sets[tensorized_path] = dataset_id;

        if (dataset_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to open dataset AOS_SHAPE: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }

        int *shapes_buffer;
        int dim = -1;
        std::unique_ptr < HDF5HsSelectionReader > hsSelectionReader(new HDF5HsSelectionReader(dataset_id, ualconst::integer_data, current_arrctx_indices.size(), &dim));

        hsSelectionReader->allocateFullBuffer((void **) &shapes_buffer);
        herr_t status = H5Dread(dataset_id, hsSelectionReader->dtype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, shapes_buffer);

        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to read dataset: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }

        shapes_selection_readers[dataset_id] = std::move(hsSelectionReader);
        shapes_data[dataset_id] = shapes_buffer;
        H5Dclose(dataset_id);   //buffer is read only one, so we close the shapes dataset. Its handler (used by shapes_data) is still available from the opened_shapes_data_sets. 
    }

    int *buffer = shapes_data[dataset_id];

    HDF5HsSelectionReader & hsSelectionReader = *shapes_selection_readers[dataset_id];

    std::vector < int >index;
    hsSelectionReader.getDataIndex(current_arrctx_indices, index);
    int n = hsSelectionReader.getShape(hsSelectionReader.getRank() - 1);        //n is the length of the vector of shapes
    *shapes = (int *) malloc(sizeof(int) * n);
    int *shapes_int = (int *) *shapes;
    shapes_int[0] = buffer[index[0]];
    return 1;
}



void HDF5Reader::open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    if (IDS_group_id != -1)
        H5Gclose(IDS_group_id);

    HDF5Utils hdf5_utils;
    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');
    hid_t IDS_file_id = opened_IDS_files[IDS_link_name];
    if (IDS_file_id == -1) {
        std::string IDS_pulse_file = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
        IDS_file_id = H5Fopen(IDS_pulse_file.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
        if (IDS_file_id < 0) {
            char error_message[200];
            sprintf(error_message, "unable to open external file for IDS: %s.\n", ctx->getDataobjectName().c_str());
            throw UALBackendException(error_message, LOG);
        }
        opened_IDS_files[IDS_link_name] = IDS_file_id;
    }

    hid_t loc_id = hdf5_utils.openHDF5Group(ctx->getDataobjectName().c_str(), file_id);

    if (loc_id >= 0) {
        IDS_group_id = loc_id;
    } else {
        IDS_group_id = -1;
    }
}

void HDF5Reader::close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files)
{
    std::replace(external_link_name.begin(), external_link_name.end(), '/', '_');
    hid_t pulse_file_id = opened_IDS_files[external_link_name];
    if (pulse_file_id != -1) {
        H5Fclose(pulse_file_id);
        opened_IDS_files[external_link_name] = -1;
    }
}

void HDF5Reader::pop_back_stacks()
{
    if (current_arrctx_indices.size() > 0)
        current_arrctx_indices.pop_back();
    if (current_arrctx_shapes.size() > 0)
        current_arrctx_shapes.pop_back();
    if (tensorized_paths.size() > 0) {
        tensorized_paths.pop_back();
    }


}

void HDF5Reader::clear_stacks()
{
    current_arrctx_indices.clear();
    current_arrctx_shapes.clear();
    tensorized_paths.clear();
}
