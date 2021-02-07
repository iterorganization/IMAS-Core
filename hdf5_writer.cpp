#include "hdf5_writer.h"

#include <assert.h>
#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"
#include "ual_defs.h"
#include "hdf5_hs_selection_reader.h"


HDF5Writer::HDF5Writer(std::string backend_version_)
:  backend_version(backend_version_), tensorized_paths(), opened_data_sets(), dataset_handlers(), selection_writers(), homogeneous_time(-1), current_arrctx_indices(), current_arrctx_shapes(), IDS_group_id(-1), init_slice_index(false), dynamic_aos_already_extended_by_slicing(), use_core_driver(false), slice_mode(GLOBAL_OP)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5Writer::~HDF5Writer()
{
}

hid_t HDF5Writer::IDS_core_file_id = -1;
hid_t HDF5Writer::core_tmp_group_id = -1;

void
 HDF5Writer::createPulse(PulseContext * ctx, int mode, std::string & options, std::string backend_version, hid_t * file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_paths_strategy, std::string & files_directory, std::string & relative_file_path)
{
    HDF5Utils hdf5_utils;
    std::string filePath = hdf5_utils.pulseFilePathFactory(ctx, files_paths_strategy, files_directory, relative_file_path);

    /* Save old error handler */
    H5E_auto2_t old_func;
    void *old_client_data;
    hid_t current_stack_id = H5Eget_current_stack();
    H5Eget_auto(current_stack_id, &old_func, &old_client_data);

    /* Turn off error handling */
    H5Eset_auto(H5E_DEFAULT, NULL, NULL);

    /* Probe. Likely to fail, but that's okay */
    //Opening master file
    *file_id = H5Fopen(filePath.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    /* Restore previous error handler */
    H5Eset_auto(current_stack_id, old_func, old_client_data);

    if (*file_id >= 0) {
        //std::cout << "Successfull read of the master file" << std::endl;
        struct opdata od;
        od.mode = false;
        od.files_directory = files_directory;
        od.relative_file_path = relative_file_path;
        od.count = 0;

        H5Literate(*file_id, H5_INDEX_NAME, H5_ITER_NATIVE, NULL, file_info, (void *) &od);

        for (int i = 0; i < od.count; i++) {
            std::string ids_name = std::string(od.link_names[i]);
            std::replace(ids_name.begin(), ids_name.end(), '/', '_');
            opened_IDS_files[ids_name] = -1;
        }
    }

    else {
        hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
        herr_t status = H5Pset_userblock(create_plist, 1024);
        if (status < 0) {
            throw UALBackendException("createPulse:unable to set a user block.", LOG);
        }
        //Creating master file
        *file_id = H5Fcreate(filePath.c_str(), H5F_ACC_TRUNC, create_plist, H5P_DEFAULT);

        H5Pclose(create_plist);

        //write backend version in the file
        if (*file_id < 0) {
            std::string message("Unable to create HDF5 file: ");
            message += filePath;
            throw UALBackendException(message, LOG);
        }

        hdf5_utils.writeHeader(ctx, *file_id, filePath, backend_version);
    }
    if (IDS_core_file_id == -1) {
        const std::string IDScorePulseFile = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, "core_file");
        create_file_in_memory("core_file", IDScorePulseFile, opened_IDS_files, false);
    }

}

void HDF5Writer::closePulse(PulseContext * ctx, int mode, std::string & options, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, int files_path_strategy, std::string & files_directory, std::string & relative_file_path)
{
    HDF5Utils hdf5_utils;
    close_datasets();

    //Closing first master file
    std::string filePath = hdf5_utils.pulseFilePathFactory(ctx, files_path_strategy, files_directory, relative_file_path);
    herr_t status = H5Fclose(file_id);

    if (status < 0) {
        char error_message[100];
        sprintf(error_message, "Unable to close HDF5 master file with handler: %d\n", (int) file_id);
        throw UALBackendException(error_message);
    }

    auto it = opened_IDS_files.begin();
    while (it != opened_IDS_files.end()) {
        const std::string & external_link_name = it->first;
        close_file_handler(external_link_name, opened_IDS_files);
        it++;
    }
}

void HDF5Writer::close_file_handler(std::string external_link_name, std::unordered_map < std::string, hid_t > &opened_IDS_files)
{
    std::replace(external_link_name.begin(), external_link_name.end(), '/', '_');
    hid_t pulse_file_id = opened_IDS_files[external_link_name];
    if (pulse_file_id != -1) {
		/*std::cout << "WRITER:close_file_handler :showing status for pulse file..." << std::endl;
			HDF5Utils hdf5_utils;
	        hdf5_utils.showStatus(pulse_file_id);*/
        herr_t status = H5Fclose(pulse_file_id);
        if (status < 0) {
            char error_message[100];
            sprintf(error_message, "Unable to close HDF5 file for IDS: %s\n", external_link_name.c_str());
            throw UALBackendException(error_message);
        }
        opened_IDS_files[external_link_name] = -1;
    }
}

void HDF5Writer::create_file_in_memory(std::string idsName, const std::string & coreFileName, std::unordered_map < std::string, hid_t > &opened_IDS_files, hbool_t flush)
{
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(H5Pset_fapl_core(fapl, 100000000, flush) >= 0);
    IDS_core_file_id = H5Fcreate(coreFileName.c_str(), H5F_ACC_EXCL, H5P_DEFAULT, fapl);
    assert(H5Pclose(fapl) >= 0);
    if (IDS_core_file_id < 0) {
        char error_message[200];
        sprintf(error_message, "unable to create (core) file: %s\n", coreFileName.c_str());
        throw UALBackendException(error_message, LOG);
    }
    core_tmp_group_id = H5Gcreate(IDS_core_file_id, "tmp", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    assert(core_tmp_group_id >= 0);
}


void HDF5Writer::deleteData(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    if (IDS_group_id == -1)
        return;

    close_datasets();

    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');

    //Deleting data from pulse file
    //bool removed = false; 
    if (H5Lexists(file_id, IDS_link_name.c_str(), H5P_DEFAULT) > 0) {
        if (H5Ldelete(file_id, IDS_link_name.c_str(), H5P_DEFAULT) >= 0) {
            //removed = true;
            H5Fflush(file_id, H5F_SCOPE_LOCAL);
            HDF5Utils hdf5_utils;
            std::string IDSpulseFile = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
            if (opened_IDS_files.find(IDS_link_name) != opened_IDS_files.end()) {
                hid_t IDS_file_id = opened_IDS_files[IDS_link_name];
                if (IDS_file_id != -1)
                    H5Ldelete(IDS_file_id, IDS_link_name.c_str(), H5P_DEFAULT);
            }
        }
    }

    H5Gclose(IDS_group_id);
    IDS_group_id = -1;

    assert(IDS_core_file_id >= 0);

    if (use_core_driver && core_tmp_group_id >= 0) {
        assert(H5Gclose(core_tmp_group_id) >= 0);
        assert(H5Ldelete(IDS_core_file_id, "tmp", H5P_DEFAULT) >= 0);
        core_tmp_group_id = H5Gcreate(IDS_core_file_id, "tmp", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        assert(core_tmp_group_id >= 0);
    }
}

void HDF5Writer::create_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');

    HDF5Utils hdf5_utils;
    std::string IDSpulseFile = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
    hid_t IDS_file_id = -1;

    if (opened_IDS_files.find(IDS_link_name) == opened_IDS_files.end()) {
        hid_t create_plist = H5Pcreate(H5P_FILE_CREATE);
        herr_t status = H5Pset_userblock(create_plist, 1024);
        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to set a user block on pulse file for IDS: %s.\n", ctx->getDataobjectName().c_str());
            throw UALBackendException(error_message, LOG);
        }
        //std::cout << "creating external file: " << IDSpulseFile.c_str() << std::endl;
        IDS_file_id = H5Fcreate(IDSpulseFile.c_str(), H5F_ACC_EXCL, create_plist, H5P_DEFAULT);
        assert(H5Pclose(create_plist) >= 0);

        if (IDS_file_id < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to create external file for IDS: %s.\n", ctx->getDataobjectName().c_str());
            throw UALBackendException(error_message, LOG);
        }
        opened_IDS_files[IDS_link_name] = IDS_file_id;

        hdf5_utils.writeHeader(ctx, IDS_file_id, IDSpulseFile, backend_version);

    } else {
        IDS_file_id = opened_IDS_files[IDS_link_name];
        if (IDS_file_id == -1) {
            IDS_file_id = H5Fopen(IDSpulseFile.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
            if (IDS_file_id < 0) {
                char error_message[200];
                sprintf(error_message, "unable to open external file for IDS: %s.\n", ctx->getDataobjectName().c_str());
                throw UALBackendException(error_message, LOG);
            }
            opened_IDS_files[IDS_link_name] = IDS_file_id;
        }
    }
    if (use_core_driver) {
        assert(IDS_core_file_id >= 0);
        if (core_tmp_group_id >= 0) {
            assert(H5Gclose(core_tmp_group_id) >= 0);
            H5Ldelete(IDS_core_file_id, "tmp", H5P_DEFAULT);
            core_tmp_group_id = H5Gcreate2(IDS_core_file_id, "tmp", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            assert(core_tmp_group_id >= 0);
        }
    }

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

void HDF5Writer::open_IDS_group(OperationContext * ctx, hid_t file_id, std::unordered_map < std::string, hid_t > &opened_IDS_files, std::string & files_directory, std::string & relative_file_path)
{
    //std::cout << "calling open_IDS_group..." << std::endl;
    if (IDS_group_id != -1) {
        H5Gclose(IDS_group_id);
        IDS_group_id = -1;
    }

    HDF5Utils hdf5_utils;
    std::string IDS_link_name = ctx->getDataobjectName();
    std::replace(IDS_link_name.begin(), IDS_link_name.end(), '/', '_');

    hid_t IDS_file_id = opened_IDS_files[IDS_link_name];
    bool makeCopy = false;
    //std::cout << "test1::calling open_IDS_group..." << std::endl;
    if (IDS_file_id == -1) {
        std::string IDS_pulse_file = hdf5_utils.getIDSPulseFilePath(files_directory, relative_file_path, IDS_link_name);
        IDS_file_id = H5Fopen(IDS_pulse_file.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
        if (IDS_file_id < 0) {
            char error_message[200];
            sprintf(error_message, "unable to open external file for IDS: %s.\n", ctx->getDataobjectName().c_str());
            throw UALBackendException(error_message, LOG);
        }
        opened_IDS_files[IDS_link_name] = IDS_file_id;
        if (use_core_driver) {
            assert(IDS_core_file_id >= 0);
            if (core_tmp_group_id >= 0) {
                assert(H5Gclose(core_tmp_group_id) >= 0);
                H5Ldelete(IDS_core_file_id, "tmp", H5P_DEFAULT);
            }
            makeCopy = true;
        }
    }

    hid_t loc_id = hdf5_utils.openHDF5Group(ctx->getDataobjectName().c_str(), file_id);
    //hid_t loc_id = hdf5_utils.openHDF5Group(ctx->getDataobjectName().c_str(), IDS_file_id);
    if (loc_id >= 0) {
        IDS_group_id = loc_id;

        if (use_core_driver && makeCopy && slice_mode == SLICE_OP) {
            //std::cout << "making a copy..." << std::endl;
            assert(H5Ocopy(IDS_file_id, IDS_link_name.c_str(), IDS_core_file_id, "/tmp", H5P_DEFAULT, H5P_DEFAULT) >= 0);
            core_tmp_group_id = H5Gopen2(IDS_core_file_id, "tmp", H5P_DEFAULT);
            assert(core_tmp_group_id >= 0);
        }
    } else {
        IDS_group_id = -1;
    }

}

void HDF5Writer::start_put_slice_operation()
{
    dynamic_aos_already_extended_by_slicing.clear();
    put_slice_count = 1;
}

void HDF5Writer::end_put_slice_operation()
{
    dynamic_aos_already_extended_by_slicing.clear();
    put_slice_count++;
}

void HDF5Writer::close_datasets()
{
    dataset_handlers.clear();
    opened_data_sets.clear();

    dynamic_aos_already_extended_by_slicing.clear();
    selection_writers.clear();
}

void HDF5Writer::close_group()
{
    if (IDS_group_id != -1) {
        H5Gclose(IDS_group_id);
        IDS_group_id = -1;
    }
}

void HDF5Writer::close_dataset(Context * ctx, HDF5DataSetHandler & fieldHandler, hid_t dataset_id, hid_t dataset_shape_id, std::string & tensorized_path)
{
    bool do_close = false;
    fieldHandler.IDS_core_file_id = IDS_core_file_id;
    fieldHandler.IDS_group_id = IDS_group_id;

    if (slice_mode == SLICE_OP) {
        return;
    } else {
        if (current_arrctx_indices.size() == 0) {
            if (tensorized_path.find("ids_properties&version_put") == std::string::npos)
                do_close = true;
        } else {
            /*for (size_t i = 0; i < current_arrctx_indices.size(); i++) {
                if (current_arrctx_indices[i] == current_arrctx_shapes[i] - 1) {
                    do_close = true;
                    continue;
                } else {
                    do_close = false;
                    break;
                }
            }*/
        }
    }
    if (do_close) {
        dataset_handlers.erase(dataset_id);
        opened_data_sets.erase(tensorized_path);
        selection_writers.erase(tensorized_path);
        if (dataset_shape_id != -1) {
            std::string shape_tensorized_path = tensorized_path + "_SHAPE";
            dataset_handlers.erase(dataset_shape_id);
            opened_data_sets.erase(shape_tensorized_path);
            selection_writers.erase(shape_tensorized_path);
        }
    }
}



void HDF5Writer::readTimedAOSShape(hid_t loc_id)
{

    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";

    if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {
        hid_t dataset_id = -1;
        dataset_id = H5Dopen2(loc_id, tensorized_path.c_str(), H5P_DEFAULT);

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
        current_arrctx_shapes.push_back(AOS_shapes[0]);
        free(AOS_shapes);
    } else {
        current_arrctx_shapes.push_back(0);
    }
    current_arrctx_indices.push_back(current_arrctx_shapes.size() - 1);
}

void HDF5Writer::beginWriteArraystructAction(ArraystructContext * ctx, int *size, hid_t loc_id, std::string & IDS_link_name)
{
    if (use_core_driver)
        loc_id = core_tmp_group_id;
    else
        loc_id = IDS_group_id;
    assert(loc_id >= 0);
    HDF5Utils hdf5_utils;
    hdf5_utils.setTensorizedPaths(ctx, tensorized_paths);

    if (slice_mode == SLICE_OP && ctx->getTimed()) {
        readTimedAOSShape(loc_id);
    } else {
        current_arrctx_indices.push_back(ctx->getIndex());
        current_arrctx_shapes.push_back(*size);
    }
    int timed_AOS_index = -1;
    hdf5_utils.getAOSIndices(ctx, current_arrctx_indices, &timed_AOS_index);    //getting current AOS indices
    createOrUpdateAOSShapesDataSet(ctx, loc_id, IDS_link_name);
}


void HDF5Writer::write_ND_Data(Context * ctx, hid_t loc_id, std::string & att_name, std::string & timebasename, int datatype, int dim, int *size, void *data)
{
    std::string & dataset_name = att_name;
    std::replace(dataset_name.begin(), dataset_name.end(), '/', '&');   // character '/' is not supported in datasets names
    std::replace(timebasename.begin(), timebasename.end(), '/', '&');

    if (use_core_driver)
        loc_id = core_tmp_group_id;
    else
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

    if (AOSRank == 0) {
        AOSRank = 1;            //no AOS, so we store data in a 1-rank data set
    }

    std::string tensorized_path = dataset_name;

    if (tensorized_paths.size() > 0)
        tensorized_path = tensorized_paths.back() + "&" + dataset_name;

    hid_t dataset_id = hdf5_utils.searchDataSetId(tensorized_path, opened_data_sets);

    int initial_size[H5S_MAX_RANK];
    for (int i = 0; i < dim; i++) {
        initial_size[i] = size[i];
    }

    //std::cout << "Writing data set: " << tensorized_path.c_str() << " ; timebase = " << timebasename.c_str() << std::endl;

    std::vector < int >AOSShapes;
    if (current_arrctx_shapes.size() != 0)
        AOSShapes.assign(current_arrctx_shapes.begin(), current_arrctx_shapes.end());
    else
        AOSShapes.push_back(1); //AOSRank = 1 in this case

    struct dataSetState ds_state;
    ds_state.mode = slice_mode;

    if (slice_mode != SLICE_OP) {
        //std::cout << "WRITER NOT IN SLICE MODE!!! " << std::endl;

        if (dataset_id < 0)     //in global mode (put), data set not yet created
        {
            ds_state.state = 0;
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());

            dataSetHandler->setNonSliceMode();
            dataSetHandler->createOrOpenTensorizedDataSet2(tensorized_path.c_str(), datatype, dim, size, loc_id, &dataset_id, AOSRank, AOSShapes.data(), 1, false, timed_AOS_index);

            dataset_handlers[dataset_id] = std::move(dataSetHandler);
            opened_data_sets[tensorized_path] = dataset_id;

        } else {
            ds_state.state = 1;
            HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];
            dataSetHandler.setNonSliceMode();
            dataSetHandler.extendTensorizedDataSet(datatype, dim, size, loc_id, dataset_id, AOSRank, AOSShapes.data());


        }
    } else {
        //std::cout << "WRITER IN SLICE MODE!!! " << std::endl;

        if (dataset_id < 0)     //in slice mode, data set does not exist or not yet opened
        {
            bool create_data_set = false;
            if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {
                ds_state.state = 1;
            } else {
                create_data_set = true;
                ds_state.state = 0;
            }

            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());

            dataSetHandler->setSliceMode(ctx, homogeneous_time);
            dataSetHandler->createOrOpenTensorizedDataSet2(tensorized_path.c_str(), datatype, dim, size, loc_id, &dataset_id, AOSRank, AOSShapes.data(), create_data_set, false, timed_AOS_index);

            dataSetHandler->setSliceIndex();

            dataSetHandler->updateTensorizedDataSet(ctx, tensorized_path, datatype, dim, size, loc_id, &dataset_id, AOSRank, AOSShapes.data(), current_arrctx_indices, dynamic_aos_already_extended_by_slicing, false);        //updating dataset_extended

            dataset_handlers[dataset_id] = std::move(dataSetHandler);
            opened_data_sets[tensorized_path] = dataset_id;
        } else {
            //Nothing to do
        }
    }


    char **p = nullptr;
    if (datatype == ualconst::char_data) {
        if (dim == 1) {
            p = (char **) malloc(sizeof(char *));
            p[0] = (char *) data;
            char *s = p[0];
            s[size[0]] = 0;
            data = p;
        } else {
            p = (char **) malloc(sizeof(char *) * initial_size[0]);
            char *c = (char *) data;
            for (int i = 0; i < initial_size[0]; i++) {
                p[i] = (char *) (c + i * initial_size[1]);
                if ((int) strlen(p[i]) > initial_size[1]) {
                    char *copy = (char *) malloc(initial_size[1] + 1);
                    strncpy(copy, p[i], initial_size[1]);
                    copy[initial_size[1]] = 0;
                    p[i] = copy;
                }
            }
            data = p;
        }
    }

    HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];

    herr_t status = -1;
    if (selection_writers.find(tensorized_path) != selection_writers.end()) {
        HDF5HsSelectionWriter & hsSelectionWriter = *selection_writers[tensorized_path];
        hsSelectionWriter.setHyperSlabs(dataset_id, current_arrctx_indices, slice_mode, dataSetHandler);
        status = H5Dwrite(dataset_id, dataSetHandler.dtype_id, hsSelectionWriter.memspace, dataSetHandler.getDataSpace(), H5P_DEFAULT, data);
    } else {
        std::unique_ptr < HDF5HsSelectionWriter > hsSelectionWriter(new HDF5HsSelectionWriter());
        hsSelectionWriter->setHyperSlabs(dataset_id, current_arrctx_indices, slice_mode, dataSetHandler);
        status = H5Dwrite(dataset_id, dataSetHandler.dtype_id, hsSelectionWriter->memspace, dataSetHandler.getDataSpace(), H5P_DEFAULT, data);
        selection_writers[tensorized_path] = std::move(hsSelectionWriter);
    }


    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to write HDF5 dataset: %s\n", tensorized_path.c_str());
        throw UALBackendException(error_message, LOG);
    }
    hid_t dataset_shape_id = -1;
    if ((datatype != ualconst::char_data && dim > 0)
        || (datatype == ualconst::char_data && dim == 2))
        dataset_shape_id = createOrUpdateShapesDataSet(ctx, loc_id, tensorized_path, dataSetHandler, timebasename, ds_state, timed_AOS_index);

    close_dataset(ctx, dataSetHandler, dataset_id, dataset_shape_id, tensorized_path);
}


hid_t HDF5Writer::createOrUpdateShapesDataSet(Context * ctx, hid_t loc_id, const std::string & field_tensorized_path, HDF5DataSetHandler & fieldHandler, std::string & timebasename, const struct dataSetState &ds_state, int timed_AOS_index)
{
    hid_t dataset_id = -1;
    int AOSRank = current_arrctx_indices.size();

    if (AOSRank == 0)
        return dataset_id;

    int rank = fieldHandler.getRank();
    if (rank == AOSRank)
        return dataset_id;

    const std::string & tensorized_path = field_tensorized_path + "_SHAPE";

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
            aos_indices[timed_AOS_index_] = timedShape - 1;
        }
    }

    herr_t status = -1;
    HDF5Utils hdf5_utils;

    bool create_data_set = false;

    dataset_id = hdf5_utils.searchDataSetId(tensorized_path, opened_data_sets);
    if (dataset_id < 0)         //dataset not yet visited by previous LL requests
    {
        if (slice_mode == SLICE_OP)
            create_data_set = false;
        else
            create_data_set = true;

        std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());
        dataSetHandler->IDS_core_file_id = IDS_core_file_id;
        dataSetHandler->IDS_group_id = IDS_group_id;

        if (ds_state.mode == SLICE_OP) {
            dataSetHandler->setSliceMode(ctx, homogeneous_time);
        } else {
            dataSetHandler->setNonSliceMode();
        }

        hsize_t dataspace_dims[H5S_MAX_RANK];
        fieldHandler.getDims(dataspace_dims);


        for (int i = 0; i < rank - AOSRank; i++) {
            shapes[i] = dataspace_dims[i + AOSRank];
        }
        dataSetHandler->createOrOpenTensorizedDataSet(tensorized_path.c_str(), ualconst::integer_data, dim, size, loc_id, &dataset_id, AOSRank, aos_shapes.data(), create_data_set, true, timed_AOS_index);

        if (ds_state.mode == SLICE_OP)
            dataSetHandler->setSliceIndex();

        std::unique_ptr < HDF5HsSelectionWriter > hsSelectionWriter(new HDF5HsSelectionWriter());

        hsSelectionWriter->setHyperSlabs(dataset_id, aos_indices, ds_state.mode, *dataSetHandler);
        status = H5Dwrite(dataset_id, dataSetHandler->dtype_id, hsSelectionWriter->memspace, dataSetHandler->getDataSpace(), H5P_DEFAULT, shapes);

        if (status < 0) {
            char error_message[200];
            sprintf(error_message, "Unable to write HDF5 dataset for SHAPE: %s\n", tensorized_path.c_str());
            throw UALBackendException(error_message, LOG);
        }

        selection_writers[tensorized_path] = std::move(hsSelectionWriter);
        dataset_handlers[dataset_id] = std::move(dataSetHandler);
        opened_data_sets[tensorized_path] = dataset_id;

    } else                      //dataset already used in previous LL request
    {
        HDF5DataSetHandler & dataSetHandler = *dataset_handlers[dataset_id];
        if (ds_state.mode == SLICE_OP) {
            dataSetHandler.setSliceMode(ctx, homogeneous_time);
        } else {
            dataSetHandler.setNonSliceMode();
        }

        hsize_t dataspace_dims_before_extension[H5S_MAX_RANK];
        fieldHandler.getDims(dataspace_dims_before_extension);

        for (int i = 0; i < rank - AOSRank; i++) {
            shapes[i] = dataspace_dims_before_extension[i + AOSRank];   //previous size before extension
        }

        if (ds_state.mode == SLICE_OP)
            dataSetHandler.setSliceIndex();

        dataSetHandler.updateTensorizedDataSet(ctx, tensorized_path, ualconst::integer_data, dim, size, loc_id, &dataset_id, AOSRank, aos_shapes.data(), aos_indices, dynamic_aos_already_extended_by_slicing, true); //updating AOS_SHAPE dataset

        dataSetHandler.setExtent();


        HDF5HsSelectionWriter & hsSelectionWriter = *selection_writers[tensorized_path];
        hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, ds_state.mode, dataSetHandler);

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

void HDF5Writer::createOrUpdateAOSShapesDataSet(Context * ctx, hid_t loc_id, std::string & IDS_link_name)
{
    HDF5Utils hdf5_utils;
    int AOSRank = current_arrctx_indices.size() - 1;

    std::vector < int >aos_indices(current_arrctx_indices.begin(), current_arrctx_indices.end() - 1);
    std::vector < int >aos_shapes(current_arrctx_shapes.begin(), current_arrctx_shapes.end() - 1);

    int dim = 1;                //AOS_SHAPE is a 1D dataset 
    int size[1] = { 1 };
    int shapes[1];              //buffer which will be written, previous size before extension

    std::string tensorized_path = tensorized_paths.back() + "&AOS_SHAPE";
    int timed_AOS_index = -1;
    hdf5_utils.isTimed(ctx, &timed_AOS_index);

    if (slice_mode == SLICE_OP && timed_AOS_index != -1 && timed_AOS_index < AOSRank) { //slicing is inside a dynamic AOS
        aos_shapes[timed_AOS_index] = put_slice_count + 1;
        aos_indices[timed_AOS_index] = put_slice_count;
    }

    if (slice_mode == SLICE_OP) {
        int AOS_shape_value = 0;
        if (timed_AOS_index == AOSRank)
            AOS_shape_value = put_slice_count + 1;

        if (AOS_shape_value != 0) {
            shapes[0] = AOS_shape_value;
        } else {
            shapes[0] = current_arrctx_shapes.back();
        }
    } else {
        shapes[0] = current_arrctx_shapes.back();
    }

    herr_t status = -1;
    hid_t dataset_id = -1;

    if (H5Lexists(loc_id, tensorized_path.c_str(), H5P_DEFAULT) > 0) {  //not yet used by a previous LL request

        dataset_id = hdf5_utils.searchDataSetId(tensorized_path, opened_data_sets);

        if (dataset_id < 0) {
            std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());
            dataSetHandler->IDS_core_file_id = IDS_core_file_id;
            dataSetHandler->IDS_group_id = IDS_group_id;

            if (slice_mode) {
                dataSetHandler->setSliceMode(ctx, 1);
            } else {
                dataSetHandler->setNonSliceMode();
            }

            dataSetHandler->createOrOpenTensorizedDataSet(tensorized_path.c_str(), ualconst::integer_data, dim, size, loc_id, &dataset_id, AOSRank, aos_shapes.data(), 0, true, timed_AOS_index);

            dataSetHandler->updateAOSShapesTensorizedDataSet(ctx, tensorized_path, ualconst::integer_data, dim, size, loc_id, &dataset_id, AOSRank, aos_shapes.data(), put_slice_count);        //updating AOS_SHAPE dataset


            HDF5HsSelectionWriter hsSelectionWriter;
            hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, GLOBAL_OP, *dataSetHandler);

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
            dataSetHandler.IDS_core_file_id = IDS_core_file_id;
            dataSetHandler.IDS_group_id = IDS_group_id;

            if (slice_mode) {
                dataSetHandler.setSliceMode(ctx, 1);
            } else {
                dataSetHandler.setNonSliceMode();
            }

            dataSetHandler.createOrOpenTensorizedDataSet(tensorized_path.c_str(), ualconst::integer_data, dim, size, loc_id, &dataset_id, AOSRank, aos_shapes.data(), 0, true, timed_AOS_index);

            dataSetHandler.updateAOSShapesTensorizedDataSet(ctx, tensorized_path, ualconst::integer_data, dim, size, loc_id, &dataset_id, AOSRank, aos_shapes.data(), put_slice_count); //updating AOS_SHAPE dataset

            HDF5HsSelectionWriter hsSelectionWriter;
            hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, GLOBAL_OP, dataSetHandler);

            status = H5Dwrite(dataset_id, dataSetHandler.dtype_id, hsSelectionWriter.memspace, dataSetHandler.getDataSpace(), H5P_DEFAULT, shapes);

            if (status < 0) {
                char error_message[200];
                sprintf(error_message, "Unable to write HDF5 dataset for AOS_SHAPE: %s\n", tensorized_path.c_str());
                throw UALBackendException(error_message, LOG);
            }

        }

    }

    else {                      //AOS_SHAPE doesn't exist yet, we create it

        std::unique_ptr < HDF5DataSetHandler > dataSetHandler(new HDF5DataSetHandler());
        dataSetHandler->IDS_core_file_id = IDS_core_file_id;
        dataSetHandler->IDS_group_id = IDS_group_id;

        if (slice_mode) {
            dataSetHandler->setSliceMode(ctx, 1);
        } else {
            dataSetHandler->setNonSliceMode();
        }

        hid_t dataset_id = -1;

        dataSetHandler->createOrOpenTensorizedDataSet(tensorized_path.c_str(), ualconst::integer_data, dim, size, loc_id, &dataset_id, AOSRank, aos_shapes.data(), 1, true, timed_AOS_index);

        HDF5HsSelectionWriter hsSelectionWriter;
        hsSelectionWriter.setHyperSlabs(dataset_id, aos_indices, GLOBAL_OP, *dataSetHandler);

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
