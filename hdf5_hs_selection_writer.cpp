#include "hdf5_hs_selection_writer.h"

#include <string.h>
#include <algorithm>
#include "hdf5_utils.h"

HDF5HsSelectionWriter::HDF5HsSelectionWriter():dataset_rank(-1), memspace(-1)
{
    //H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
}

HDF5HsSelectionWriter::~HDF5HsSelectionWriter()
{
	if (memspace != H5S_ALL)
    	H5Sclose(memspace);
}

void
 HDF5HsSelectionWriter::setHyperSlabs(hid_t dataset_id, std::vector < int >&current_arrctx_indices, int slice_mode, HDF5DataSetHandler & dataSetHandler, int dynamic_AOS_slices_extension)
{
    bool isTimed;
    int timed_AOS_index;

    dataSetHandler.getAttributes(&isTimed, &timed_AOS_index);
    int AOSRank = current_arrctx_indices.size();

    hid_t dataspace = dataSetHandler.getDataSpace();
    dataset_rank = dataSetHandler.getRank();
    hsize_t *dataspace_dims = dataSetHandler.getDims();

	if (dataset_rank == 0) {
		memspace = H5S_ALL;
		return;
	}

    //Create hyperslabs
    //creating selection in the dataspace
    for (int i = 0; i < AOSRank; i++) {

        if (current_arrctx_indices.size() == 0) {       //data is not located in an AOS
            offset[i] = 0;
        } else {
            if (slice_mode == SLICE_OP && timed_AOS_index == i) {
				offset[i] = dataspace_dims[i] - dynamic_AOS_slices_extension + current_arrctx_indices[timed_AOS_index];
            } else {
                offset[i] = current_arrctx_indices[i];  //slicing is outside a dynamic AOS
            }
        }
        count[i] = 1;
    }

    for (int i = 0; i < dataset_rank - AOSRank; i++) {
        if (slice_mode == SLICE_OP && !isTimed && (i == dataset_rank - AOSRank - 1)) {
			offset[i + AOSRank] = dataspace_dims[i + AOSRank] - dataSetHandler.getSlicesExtension(); 
			count[i + AOSRank] = dataSetHandler.getSlicesExtension();
        } else {
            offset[i + AOSRank] = 0;
            count[i + AOSRank] = dataspace_dims[i + AOSRank];
        }
    }


    /*std::cout << "-------->dataspace dimensions in setHyperSlabs for AOSs" << std::endl;
       for (int i = 0; i < AOSRank; i++) {
       std::cout << "dataspace_dims[" << i << "] = " <<  dataspace_dims[i] << std::endl;
       std::cout << "offset[" << i << "] = " <<  offset[i] << std::endl;
       std::cout << "count[" << i << "] = " <<  count[i] << std::endl;
       }
       std::cout << "-------->end of dataspace dimensions in setHyperSlabs for AOSs" << std::endl;

       std::cout << "-------->dataspace dimensions in setHyperSlabs for the array" << std::endl;
       for (int i = 0; i <  dataset_rank - AOSRank; i++) {
       std::cout << "dataspace_dims[" << i + AOSRank << "] = " <<  dataspace_dims[i + AOSRank] << std::endl;
       std::cout << "offset[" << i + AOSRank << "] = " <<  offset[i + AOSRank] << std::endl;
       std::cout << "count[" << i + AOSRank << "] = " <<  count[i + AOSRank] << std::endl;
       }
       std::cout << "---------dataspace dimensions in setHyperSlabs" << std::endl; */



    herr_t status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset, NULL, count,
                                        NULL);

    /*int n = H5Sget_select_npoints( dataspace );
       std::cout << "n(dataspace) = " << n << std::endl; */

    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create dataspace HDF5 hyperslab for dataset: %d\n", (int) dataset_id);
        throw UALBackendException(error_message, LOG);
    }
    //creating selection in the memory dataspace
    for (int i = 0; i < AOSRank; i++) {
        dims[i] = 1;
        offset_out[i] = 0;
        count_out[i] = 1;
    }

    bool slicing = (slice_mode == SLICE_OP && !isTimed);   //appending slice

    for (int i = 0; i < dataset_rank - AOSRank; i++) {
        dims[i + AOSRank] = (hsize_t) dataspace_dims[i + AOSRank];
        if (slicing && (i == dataset_rank - AOSRank - 1)) {
            offset_out[i + AOSRank] = 0;        // when sliced and not timed, offset_out = 0, count_out = 1
			count_out[i + AOSRank] = dataSetHandler.getSlicesExtension();
            dims[i + AOSRank] = dataSetHandler.getSlicesExtension();
        } else {
            offset_out[i + AOSRank] = 0;
            count_out[i + AOSRank] = dims[i + AOSRank];
        }
    }

    /*std::cout << "-------->memory dimensions" << std::endl;
       for (int i = 0; i < AOSRank; i++) {
       std::cout << "dims[" << i << "] = " <<  dims[i] << std::endl;
       std::cout << "offset[" << i << "] = " <<  offset[i] << std::endl;
       std::cout << "count[" << i << "] = " <<  count[i] << std::endl;
       }

       for (int i = 0; i <  dataset_rank - AOSRank; i++) {
       std::cout << "dims[" << i + AOSRank << "] = " <<  dims[i + AOSRank] << std::endl;
       std::cout << "offset[" << i + AOSRank << "] = " <<  offset[i + AOSRank] << std::endl;
       std::cout << "count[" << i + AOSRank << "] = " <<  count[i + AOSRank] << std::endl;
       }
       std::cout << "---------memory dimensions" << std::endl; */

    bool msHasChanged = memSpaceHasChanged(dims);
    if (memspace == -1 || msHasChanged) {
        if (memspace != -1)
            H5Sclose(memspace);
        memspace = H5Screate_simple(dataset_rank, dims, NULL);
        memcpy(memspace_dims_copy, dims, H5S_MAX_RANK * sizeof(hsize_t));
    }

    status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL, count_out, NULL);
    /*int n2 = H5Sget_select_npoints( memspace );
       std::cout << "n2(dataspace) = " << n2 << std::endl; */

    if (status < 0) {
        char error_message[200];
        sprintf(error_message, "Unable to create memory HDF5 hyperslab for dataset: %d\n", (int) dataset_id);
        throw UALBackendException(error_message, LOG);
    }
}

bool HDF5HsSelectionWriter::memSpaceHasChanged(hsize_t * dims)
{
    for (int i = 0; i < dataset_rank; i++) {
        if (dims[i] != memspace_dims_copy[i]) {
            return true;
        }
    }
    return false;
}
